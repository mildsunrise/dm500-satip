//pallas/drv/os/pm-alloc.c  version 0.1a
/*----------------------------------------------------------------------------+
|
|       This source code has been made available to you by IBM on an AS-IS
|       basis.  Anyone receiving this source is licensed under IBM
|       copyrights to use it in any way he or she deems fit, including
|       copying it, modifying it, compiling it, and redistributing it either
|       with or without modifications.  No license under IBM patents or
|       patent applications is to be implied by the copyright license.
|
|       Any user of this software should understand that IBM cannot provide
|       technical support for this software and will not be responsible for
|       any consequences resulting from the use of this software.
|
|       Any person who transfers this source code or any derivative work
|       must include the IBM copyright notice, this paragraph, and the
|       preceding two paragraphs in the transferred software.
|
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
//
//Comment: 
//  Physical memory block allocation routines  
//Revision Log:   
//  Sept/03/2001                         Created by YYD
//  Sept/05/2001                         Version 0.1 passed test    (YYD)
//  Oct/17/2001                          Add justified alloc        (YYD)
//  May/27/2002                          Rewrite to make it a generic 
//                                       physical heap manager (YYD)

// Compile time options :
//  Allocation strategies
//    #define __PM_ALLOC_OPTIMIZED_UTILIZATION__  // try to find best match block
//    #define __PM_ALLOC_DEBUG_CHECK_OVERRUN__       // we may check if there are memory overrun in many cases
//  Syncronization locks
//    Default is using critical sections if nothing is chosen
//    #define __PM_ALLOC_ATOM__    // if pm-alloc can be promised as atomic, else see below
//    #define __PM_ALLOC_SYNC_APP__    // if this is defined, we use only mutex to avoid re-entrance
//  Test
//    #define __PM_ALLOC_TEST__    // for testing only, should not be defined during normal use

#ifdef __PM_ALLOC_TEST__

#ifndef __PM_ALLOC_ATOM__
    #define __PM_ALLOC_ATOM__   // test is controled
#endif

#ifndef __PM_ALLOC_DEBUG
    #define  __PM_ALLOC_DEBUG 1
#endif

#define  __PM_ALLOC_DEBUG_CHECK_OVERRUN__
#define  __PM_ALLOC_OPTIMIZED_UTILIZATION__

#include <stdio.h>     // for test, std c header
#include <string.h>

// fake system functions
#define KERN_CRIT ""
#define KERN_INFO ""
#define KERN_NOTICE ""
#define printk(fmt, args...)    fprintf(stdout, fmt, ##args); fflush(stdout)
#define request_region(a, b, c)  (b)
#define release_region(a,b)
#define ioremap(a,b)    (a)
#define iounmap(a)

#ifndef __PM_ALLOC_ATOM__    // if pm-alloc can be promised as atomic
    // fake sync for test locks
    static int crit;
    #define PML_INIT()
    #define PML_DEINIT()
    static void PML_ENTER()
    {
      fprintf(stderr, "----------- Enter\n"); fflush(stderr);
      if(crit) {fprintf(stderr, "******** Mismatched enter/leave sync\n"); fflush(stderr);}
      crit=1;
    }

    static void PML_LEAVE()
    {
      if(crit) crit=0;
      else { fprintf(stderr, "********  Mismatched enter/leave sync\n"); fflush(stderr); }
      fprintf(stderr, "----------- Leave\n"); fflush(stderr);
    }
#endif

#else  // __PM_ALLOC_TEST__
// Begin Linux Specific Header
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/stddef.h>
#include <linux/ioport.h>
#include <linux/string.h>    
#include <asm/io.h>
// End Linux Specific Header
#endif

#include "os/pm-alloc.h"
#include "os/helper-pool.h"
#include "os/os-generic.h"

#include "pm-alloc-local.h"

#ifdef __PM_ALLOC_ATOM__    // if pm-alloc can be promised as atomic
    #define PML_INIT()
    #define PML_ENTER()
    #define PML_LEAVE()
    #define PML_DEINIT()
#elif !defined(__PM_ALLOC_TEST__)    // for test, we defined the fake ones
    #include "os/os-sync.h"
    #ifdef __PM_ALLOC_SYNC_APP__    // if this is defined, we use only mutex to avoid re-entrance
        #define PML_INIT()   (pPMRoot->sync = (UINT32)os_create_mutex())
        #define PML_ENTER()  os_get_mutex((MUTEX_T)pPMRoot->sync)
        #define PML_LEAVE()  os_release_mutex((MUTEX_T)pPMRoot->sync)
        #define PML_DEINIT() os_delete_mutex((MUTEX_T)pPMRoot->sync)
    #else    // we need interrupt level protection
        #define PML_INIT()
        #define PML_ENTER()  (pPMRoot->sync = os_enter_critical_section())
        #define PML_LEAVE()  os_leave_critical_section(pPMRoot->sync)
        #define PML_DEINIT()
    #endif
#endif


//#define __PM_ALLOC_DEBUG
#define  __PM_ALLOC_DEBUG_CHECK_OVERRUN__
#define  __PM_ALLOC_OPTIMIZED_UTILIZATION__


#ifdef __PM_ALLOC_DEBUG
#define __DRV_DEBUG
#endif
#include "os/drv_debug.h"


// defined the root allocation data structure

#ifdef __PM_ALLOC_DEBUG
    PM_ALLOC_FREE_NODE_T *  __PM_GET_BLOCK_POINTER_DEBUG(PM_ALLOC_ROOT_T *pPMRoot, INT n)
    {
        if (n >= (INT)pPMRoot->uTotalUnits || n < 0)
        {
            PFATAL(" Try to use an invalid block pointer !\n");
            return NULL;
        }
        return ((PM_ALLOC_FREE_NODE_T *)pPMRoot->pLogicalAddress + (n));
    }
    #define __PM_GET_BLOCK_POINTER(n)  __PM_GET_BLOCK_POINTER_DEBUG(pPMRoot, n)
#else // use macro for speed
    #define __PM_GET_BLOCK_POINTER(n)    ((PM_ALLOC_FREE_NODE_T *)pPMRoot->pLogicalAddress + (n))
#endif

#ifdef __PM_ALLOC_DEBUG_CHECK_OVERRUN__    // we may check if there are memory overrun in many cases

static void __init_overrun_check(PM_ALLOC_ROOT_T *pPMRoot, UINT block)
{
    UINT i;
    PM_ALLOC_FREE_NODE_T *pNode;
    BYTE *ptr;

    pNode = __PM_GET_BLOCK_POINTER(block);
    // ok, fill pattern
    ptr = pNode->bDummy1;
    for(i=0; i<__PM_ALLOC_DUMMY1_SIZE; i++) ptr[i] = (BYTE)i;
    ptr = pNode->bDummy2;
    for(i=0; i<__PM_ALLOC_DUMMY2_SIZE; i++) ptr[i] = (BYTE)i;
    return;
}
static void __check_overrun(PM_ALLOC_ROOT_T *pPMRoot, UINT block)
{
    UINT i;
    PM_ALLOC_FREE_NODE_T *pNode;
    BYTE *ptr;
    pNode = __PM_GET_BLOCK_POINTER(block);
    // ok, check filled pattern
    ptr = pNode->bDummy1;
    for(i=0; i<__PM_ALLOC_DUMMY1_SIZE; i++) 
        if (ptr[i] != (BYTE)i)
        {
            PDEBUGE(" **** Buffer overrun detected !\n"
                   "  Before logical_addr= 0x%8.8x, physical_addr= 0x%8.8x\n",
                   (UINT)pNode, pPMRoot->uPhysicalAddress + block*__PM_ALLOC_UNIT); 
            for(; i<__PM_ALLOC_DUMMY1_SIZE; i++) ptr[i] = (BYTE)i; // reset it
            break;
        }
    ptr = pNode->bDummy2;
    for(i=0; i<__PM_ALLOC_DUMMY2_SIZE; i++) 
        if (ptr[i] != (BYTE)i)
        {
            PDEBUGE(" **** Buffer overrun detected !\n"
                   "  After logical_addr= 0x%8.8x, physical_addr= 0x%8.8x\n",
                   (UINT)pNode, pPMRoot->uPhysicalAddress + block*__PM_ALLOC_UNIT); 
            for(; i<__PM_ALLOC_DUMMY2_SIZE; i++) ptr[i] = (BYTE)i; // reset it
            break;
        }
    return;
}

#define INIT_OVERRUN_DET(root, block)        __init_overrun_check(root, block)
#define OVERRUN_DET(root, block)             __check_overrun(root, block)

#else    // don't check for overrun

#define INIT_OVERRUN_DET(root, block)
#define OVERRUN_DET(root, block)

#endif


static void __insert_new_free_block(PM_ALLOC_ROOT_T *pPMRoot, INT npNewBlock, UINT uNewBlockUnits, INT npNext, INT npPrev, int check_merge)
{
    PM_ALLOC_FREE_NODE_T *pNewBlock = __PM_GET_BLOCK_POINTER(npNewBlock);

    pNewBlock->npAddr = npNewBlock;
    pNewBlock->uUnits = uNewBlockUnits;
    pNewBlock->npNext = npNext;
    pNewBlock->npPrev = npPrev;
    if (npNext >= 0) // we need to adjust next block's prev pointer
    {
        PM_ALLOC_FREE_NODE_T *pNextBlock = __PM_GET_BLOCK_POINTER(npNext);
        if(npNewBlock + (INT)uNewBlockUnits > npNext) // error !!!
        {
            PFATAL("Find heap curruption (overlap) !\n");
            // any way, continue
        }
        pNextBlock->npPrev = npNewBlock;
    }

    if (npPrev >= 0) // we need to adjust prev block's next pointer
    {
        PM_ALLOC_FREE_NODE_T *pPrevBlock = __PM_GET_BLOCK_POINTER(npPrev);
        if(npPrev + (INT)pPrevBlock->uUnits > npNewBlock) // error !!!
        {
            PFATAL("Find heap curruption (overlap) !\n");
            // any way, continue
        }
        pPrevBlock->npNext = npNewBlock;
    }
    // then the last one is to init overrun detection by option
    INIT_OVERRUN_DET(pPMRoot, npNewBlock);

    // for the merge case
    if (check_merge)
    {
        if (npNext >= 0 && npNewBlock + (INT)uNewBlockUnits == npNext)  // first to try next
        {
            // Ok, merge
            PM_ALLOC_FREE_NODE_T *pNextBlock = __PM_GET_BLOCK_POINTER(npNext);
            uNewBlockUnits += pNextBlock->uUnits;
            npNext = pNextBlock->npNext;
            // simply eat it
            pNewBlock->npNext = npNext;
            pNewBlock->uUnits = uNewBlockUnits;
            if(npNext >= 0)  // adjust next->prev link
            {
                PM_ALLOC_FREE_NODE_T *pNextBlock = __PM_GET_BLOCK_POINTER(npNext);
                pNextBlock->npPrev = npNewBlock;
            }
        }
        if(npPrev >= 0)
        {
            PM_ALLOC_FREE_NODE_T *pPrevBlock = __PM_GET_BLOCK_POINTER(npPrev);
            if(npPrev + (INT)pPrevBlock->uUnits == npNewBlock) // ok merge
            {
                pPrevBlock->uUnits += uNewBlockUnits;
                pPrevBlock->npNext = npNext;
                if(npNext >= 0)  // adjust next->prev link
                {
                    PM_ALLOC_FREE_NODE_T *pNextBlock = __PM_GET_BLOCK_POINTER(npNext);
                    pNextBlock->npPrev = npPrev;
                }
            }
        }
    }
    return;
}




MEM_HANDLE_T pm_alloc_physical_justify(void *pRoot, UINT uNumBytes, UINT uJustify)
{
    UINT uReqUnits;
    UINT uJustifyUnits;
    UINT uJustifyAdjust;
    INT  npBestFitBlock;
    UINT uBestFitUnits;
    MEM_HANDLE_T hRtn = NULL;

    PM_ALLOC_ROOT_T *pPMRoot = (PM_ALLOC_ROOT_T *) pRoot;

    if(!pPMRoot || pPMRoot->init_magic != __PM_INIT_MAGIC) 
    {
        PFATAL("Tried to use an invalid heap root pointer !\n");
        return NULL;
    } 
    
    if (0 == uNumBytes)   return NULL;  // nothing to alloc
    if (0 == pPMRoot->uTotalUnits)// uninitialized
    {
        PFATAL("Tried to allocate before heap is initialized !\n");
        return NULL;
    }

    // check for justification condition
    if(uJustify > __PM_ALLOC_UNIT)
    {
        UINT n=0, j=0, b=1;
        while(b)
        {
            if(uJustify & b) { n = b; j++; }
            b <<= 1;
        }
        if(j > 1 ) 
        {
            if(n<<1) 
            {
                PDEBUG("Justification condition modified from 0x%8.8x to 0x%8.8x\n", uJustify, n<<1);
                uJustify = n<<1;
            }
            else
            {
                PDEBUG("Justification condition %8.8x can not be satisfied!\n", uJustify);
                return NULL;
            }
        }
    }
    else
    {
        uJustify = 0; // since it's less than our basic justification bound
    }

    uJustifyUnits = uJustify / __PM_ALLOC_UNIT; // and now the justification in units

    if(uJustifyUnits)   // also add the base adjust
        uJustifyAdjust = (pPMRoot->uPhysicalAddress/__PM_ALLOC_UNIT) %  uJustifyUnits;
    else
        uJustifyAdjust = 0;

    // calculate the required units
    uReqUnits = (uNumBytes + __PM_ALLOC_UNIT - 1) / __PM_ALLOC_UNIT;


    PDEBUG(" Try to allocate %d bytes\n", uNumBytes);
    //########################################################################
    PML_ENTER();

    if (os_get_pool_status(&pPMRoot->handlePool) <= 0) goto __pm_alloc_just_out; // out of handle

    if (uReqUnits > pPMRoot->uFreeUnits) {
        printk("pm_alloc_physical_justify: not enough free memory available\n");
        goto __pm_alloc_just_out;    // out of memory
    }
    if (pPMRoot->npFreeList < 0) // something wrong with the free list, this should never happen
    {
        // there ares still
        PFATAL("Allocation heap corrupted !\n");
        goto __pm_alloc_just_out;
    }

    PDEBUG(" Lookup heap\n");
    // search for the best fit
    npBestFitBlock = -1; uBestFitUnits = 0;
    {
        INT npCurr;
        PM_ALLOC_FREE_NODE_T *pCurrBlock;

        // the first free
        npCurr = pPMRoot->npFreeList;
        while(npCurr >= 0) 
        {
            pCurrBlock = __PM_GET_BLOCK_POINTER(npCurr);
            if (pCurrBlock->uUnits >= uReqUnits)  // this one possiblly fits
            {

                if(uJustifyUnits) // check if justification is required
                {
                    // the number of units skipped for justification requirements
                    UINT uAdjust = uJustifyUnits - ((UINT)pCurrBlock->npAddr+uJustifyAdjust)%uJustifyUnits;
                    if(pCurrBlock->uUnits < uReqUnits + uAdjust) // justify condition not fit
                    {
                        if(pCurrBlock->npNext < 0) break;
                        if(npCurr >= pCurrBlock->npNext)
                        {
                            PFATAL("Allocation heap corrupted !\n");
                            goto __pm_alloc_just_out;
                        }
                        npCurr = pCurrBlock->npNext;    // look for next
                        continue;
                    }
                }
                // ok justification is fine
#ifdef __PM_ALLOC_OPTIMIZED_UTILIZATION__  // try to find best match block
                if (npBestFitBlock < 0 || uBestFitUnits > pCurrBlock->uUnits)
                {
                    npBestFitBlock = npCurr;
                    uBestFitUnits = pCurrBlock->uUnits;
                    if (uBestFitUnits == uReqUnits) // yeah, here we go
                        break;
                }
#else    // just use the first fit
                npBestFitBlock = npCurr;
                uBestFitUnits = pCurrBlock->uUnits;
                break;
#endif
            }
            if(pCurrBlock->npNext < 0) break;   // end of heap
            if(npCurr >= pCurrBlock->npNext)    // check for heap corrupt
            {
                PFATAL("Allocation heap corrupted !\n");
                goto __pm_alloc_just_out;
            }
            npCurr = pCurrBlock->npNext;    // look for next
        }
    }
    if (npBestFitBlock < 0) // :-(, Are you too greedy? I could not find such a big block due to mem fragmentation
    {
        goto __pm_alloc_just_out;
    }

    PDEBUG(" Get it %d\n", npBestFitBlock);

    // :-), yeah, we get it!  Goto allocation
    {
        PM_ALLOC_FREE_NODE_T *pBestBlock;

        pBestBlock = __PM_GET_BLOCK_POINTER(npBestFitBlock);
        
        if (uBestFitUnits > uReqUnits) // split is needed
        {
            if(uJustifyUnits && ((UINT)npBestFitBlock + uJustifyAdjust)%uJustifyUnits)
            {
                // the big trouble, we may need to split it into three
                INT npHeadBlock = npBestFitBlock;
                UINT uHeadBlockUnits = uJustifyUnits - ((UINT)npBestFitBlock + uJustifyAdjust)%uJustifyUnits;
                INT npTailBlock = npBestFitBlock + (INT)(uReqUnits + uHeadBlockUnits);  // get the tailing block's address
                UINT uTailBlockUnits = uBestFitUnits - uReqUnits - uHeadBlockUnits;     // get the tailing units
                
                PDEBUG(" Split from %d to %d [%d] %d\n", uBestFitUnits, uHeadBlockUnits, uReqUnits, uTailBlockUnits);
                
                // this will actually replace the origianl node
                __insert_new_free_block(pPMRoot, npHeadBlock, uHeadBlockUnits, pBestBlock->npNext, pBestBlock->npPrev, 0);
                if(uTailBlockUnits) // ok tailing block
                {
                    __insert_new_free_block(pPMRoot, npTailBlock, uTailBlockUnits, pBestBlock->npNext, npHeadBlock, 0);
                }
                // and then the best fit block is ajusted to satisfy justification condition
                npBestFitBlock = npHeadBlock + (INT)uHeadBlockUnits;
            }
            else    // no justification or justification is satisfied at the beginning
            {
                INT npNewBlock = npBestFitBlock + (INT)uReqUnits;  // get the remainning block's address
                UINT uNewBlockUnits = uBestFitUnits - uReqUnits;  // get the remainning units
                
                PDEBUG(" Split from %d to [%d] %d\n", uBestFitUnits, uReqUnits, uNewBlockUnits);
                
                // this will actually replace the origianl node
                __insert_new_free_block(pPMRoot, npNewBlock, uNewBlockUnits, pBestBlock->npNext, pBestBlock->npPrev, 0);
                // check if it is the head one
                if (npBestFitBlock == pPMRoot->npFreeList)
                {
                    pPMRoot->npFreeList = npNewBlock;
                }
            }
        }
        else  // no split is needed
        {
            // and certainly justification should be satisfied here
            // we need to delete it from the heap link
            if (pBestBlock->npNext >= 0)
            {
                PM_ALLOC_FREE_NODE_T *pNextBlock = __PM_GET_BLOCK_POINTER(pBestBlock->npNext);
                pNextBlock->npPrev = pBestBlock->npPrev;
            }
            
            if (pBestBlock->npPrev >= 0)
            {
                PM_ALLOC_FREE_NODE_T *pPrevBlock = __PM_GET_BLOCK_POINTER(pBestBlock->npPrev);
                pPrevBlock->npNext = pBestBlock->npNext;
            }
            else  // it should be the head one
            {
                // check if it is the head one
                if (npBestFitBlock == pPMRoot->npFreeList)
                {
                    pPMRoot->npFreeList = pBestBlock->npNext;
                }
                else // heap mistake
                {
                    PFATAL("Heap corrupted, broken link!\n");
                }
            }
        }

        PDEBUG(" Get a handle \n");

        // the last step, get a free handle
        hRtn = os_get_from_pool(&pPMRoot->handlePool);
        // and fill it with proper values
        hRtn->pLogical = (void *)((BYTE *)pPMRoot->pLogicalAddress + npBestFitBlock*__PM_ALLOC_UNIT);
        hRtn->uPhysical = pPMRoot->uPhysicalAddress + npBestFitBlock*__PM_ALLOC_UNIT;
        hRtn->uSize = uReqUnits;
        pPMRoot->uFreeUnits -= uReqUnits;  // decrease the free size
        // done, baby!
    }

__pm_alloc_just_out:
    PML_LEAVE();
    //########################################################################
    return hRtn;
}

MEM_HANDLE_T pm_alloc_physical (void *pRoot, UINT uNumBytes)
{
    return pm_alloc_physical_justify(pRoot, uNumBytes, 0);
}

void pm_free_physical (void *pRoot, MEM_HANDLE_T hPhysicalMem)
{
    PM_ALLOC_ROOT_T *pPMRoot = (PM_ALLOC_ROOT_T *) pRoot;

    if(!pPMRoot || pPMRoot->init_magic != __PM_INIT_MAGIC) 
    {
        PFATAL("Tried to use an invalid heap root pointer !\n");
        return;
    } 

    if (NULL == hPhysicalMem) return;
    if (0 == pPMRoot->uTotalUnits)// uninitialized
    {
        PFATAL("Tried to free after heap is destroyed !\n");
        return;  
    }

    // check if it is my baby, just protect me from mistakes
    if (os_validate_pool_element(&pPMRoot->handlePool, hPhysicalMem) < 0 ||
        hPhysicalMem->uPhysical < pPMRoot->uPhysicalAddress || 
        hPhysicalMem->uPhysical >= pPMRoot->uPhysicalAddress + pPMRoot->uTotalUnits*__PM_ALLOC_UNIT ||
        0 == hPhysicalMem->uSize )
    {
        PFATAL("Tried to free a wrong / unallocated handle 0x%8.8x !\n", (UINT)hPhysicalMem);
        return;
    }

    PDEBUG(" Freeing 0x%8.8x size 0x%8.8x\n", hPhysicalMem->uPhysical, hPhysicalMem->uSize);

    //########################################################################
    PML_ENTER();
    {
        INT npAfterMe = -1;  // haven't find yet

        INT npFree = (INT)(hPhysicalMem->uPhysical - pPMRoot->uPhysicalAddress)/__PM_ALLOC_UNIT;
        UINT uFreeUnits = hPhysicalMem->uSize;

        // now put it back into handle pool
        hPhysicalMem->uSize = 0;  // unmark it
        // should we check if it is in pool already, this is fatal!
        os_put_back_to_pool(&pPMRoot->handlePool, hPhysicalMem);

        PDEBUG(" Look up it's position\n");

        // look for it's position in blocks
        {
            INT npCurr = pPMRoot->npFreeList;
            while(npCurr >= 0 && npCurr < npFree)
            {
                PM_ALLOC_FREE_NODE_T *pCurrBlock = __PM_GET_BLOCK_POINTER(npCurr);
                if(pCurrBlock->npNext > npFree || pCurrBlock->npNext < 0)
                {
                    npAfterMe = npCurr;
                    break;
                }
                if(pCurrBlock->npNext < 0) break;   // end of heap
                if(npCurr >= pCurrBlock->npNext)    // check for heap corrupt
                {
                    PFATAL("Allocation heap corrupted !\n");
                    goto __pm_free_out;
                }
                npCurr = pCurrBlock->npNext;
            }
            if(npCurr == npFree) // oops, you passed in an unallocated block
            {
                PM_ALLOC_FREE_NODE_T *pCurrB = __PM_GET_BLOCK_POINTER(npCurr);
                if(uFreeUnits != pCurrB->uUnits)
                {
                    PFATAL("Heap chain are currupted !\n");
                }
                else
                {
                    PFATAL("Tried to free an unallocated"
                           " block at physical 0x%8.8x !\n", 
                          (UINT)( pPMRoot->uPhysicalAddress + __PM_ALLOC_UNIT*npFree));
                }
                goto __pm_free_out;
            }
            if(npAfterMe >= 0) // Yes, after me
            {
                PM_ALLOC_FREE_NODE_T *pAfterMeBlock = __PM_GET_BLOCK_POINTER(npAfterMe);
                __insert_new_free_block(pPMRoot, npFree, uFreeUnits, pAfterMeBlock->npNext, npAfterMe, 1);
            }
            else // no, before everyone
            {
                __insert_new_free_block(pPMRoot, npFree, uFreeUnits, pPMRoot->npFreeList, -1, 1);
                pPMRoot->npFreeList = npFree;
            }
            // ok done
        }
        pPMRoot->uFreeUnits += uFreeUnits;  // increase the free size
    }
__pm_free_out:
    PML_LEAVE();
    //########################################################################
    return;
}

ULONG pm_get_actual_physical_size(MEM_HANDLE_T hPhysicalMem)
{
    if (NULL == hPhysicalMem) return 0;
    return (ULONG)hPhysicalMem->uSize*__PM_ALLOC_UNIT;
}


UINT pm_get_physical_base(void *pRoot)
{
    PM_ALLOC_ROOT_T *pPMRoot = (PM_ALLOC_ROOT_T *) pRoot;

    if(!pPMRoot || pPMRoot->init_magic != __PM_INIT_MAGIC) 
    {
        PFATAL("Tried to use an invalid heap root pointer !\n");
        return 0;
    } 

    return pPMRoot->uPhysicalAddress;
}

void *pm_get_logical_base(void *pRoot)
{
    PM_ALLOC_ROOT_T *pPMRoot = (PM_ALLOC_ROOT_T *) pRoot;

    if(!pPMRoot || pPMRoot->init_magic != __PM_INIT_MAGIC) 
    {
        PFATAL("Tried to use an invalid heap root pointer !\n");
        return NULL;
    } 
    return pPMRoot->pLogicalAddress;
}

UINT pm_get_physical_total_size(void *pRoot)
{
    PM_ALLOC_ROOT_T *pPMRoot = (PM_ALLOC_ROOT_T *) pRoot;

    if(!pPMRoot || pPMRoot->init_magic != __PM_INIT_MAGIC) 
    {
        PFATAL("Tried to use an invalid heap root pointer !\n");
        return 0;
    } 
    return pPMRoot->uTotalUnits*__PM_ALLOC_UNIT;
}


// Only used during initialize and deinitialize
void * __pm_alloc_physical_init(UINT uPhysicalAddr, void *pLogicalAddr, UINT uSize, UINT uMaxHandles)
{
    PM_ALLOC_ROOT_T *pPMRoot;

    PDEBUG(" Physical mem at 0x%8.8x , size 0x%8.8x\n",  uPhysicalAddr,  uSize);

    if(uMaxHandles < 1 || NULL == pLogicalAddr)
    {
        return NULL;
    }

    //ok, let's check if the input paramaters are ok
    if (uPhysicalAddr % __PM_ALLOC_UNIT ) // first, lets align these parameters 
    {
        UINT uInc = __PM_ALLOC_UNIT - uPhysicalAddr % __PM_ALLOC_UNIT;
        if (uSize < uInc)  // hi, remainning is too small
            return NULL;
        uPhysicalAddr += uInc;
        (BYTE *)pLogicalAddr += uInc;
        uSize -= uInc;
    }

    if (uSize < __PM_ALLOC_UNIT + os_tell_pool_buffer_size(uMaxHandles, sizeof(struct __MEM_HANDLE_T_STRUCT)))  
        // we should request at least this size
        return NULL;

    if ((uPhysicalAddr + uSize) < uPhysicalAddr) // how can we wrap around ?!
        return NULL;

    pPMRoot = MALLOC(sizeof(PM_ALLOC_ROOT_T));

    if(!pPMRoot)
    {
        return NULL;    // hi, I cann't do this
    }
    
    pPMRoot->pLogicalAddress = pLogicalAddr;

    //ok, we are going to set up every thing else

    PML_INIT();  // init the sync lock

    // firstly, the pPMRoot
    pPMRoot->uPhysicalAddress = uPhysicalAddr;
    pPMRoot->uTotalMem = uSize;    // remember the total size
    pPMRoot->uTotalUnits = (uSize - os_tell_pool_buffer_size(uMaxHandles, 
                            sizeof(struct __MEM_HANDLE_T_STRUCT))) / __PM_ALLOC_UNIT;
    pPMRoot->uFreeUnits = pPMRoot->uTotalUnits; // every block is free
    pPMRoot->npFreeList = 0;    // the first block to do
    // and the initial huge block
    {
        PM_ALLOC_FREE_NODE_T *pNode = (PM_ALLOC_FREE_NODE_T *) pPMRoot->pLogicalAddress;
        pNode->npNext = -1;    // no next
        pNode->npPrev = -1;    // no prev
        pNode->npAddr = 0;  // my addr
        pNode->uUnits = pPMRoot->uFreeUnits;
        INIT_OVERRUN_DET(pPMRoot, 0);
    }
    pPMRoot->uMaxHandles = uMaxHandles;

    // then, the handle table
    pPMRoot->pHandlePoolBuffer = (void *)(pPMRoot->pLogicalAddress + pPMRoot->uTotalUnits*__PM_ALLOC_UNIT);
    os_create_pool(&pPMRoot->handlePool, pPMRoot->pHandlePoolBuffer, uMaxHandles, sizeof(struct __MEM_HANDLE_T_STRUCT));
    {
        UINT i;
        MEM_HANDLE_T pHandle;
        for(i=0; i<uMaxHandles; i++)
        {
            pHandle = os_walk_up_pool(&pPMRoot->handlePool, i);
            pHandle->pLogical = NULL;
            pHandle->uPhysical = 0;
            pHandle->uSize = 0;
        }
    }
    // ok, any other issues ??

    pPMRoot->init_magic = __PM_INIT_MAGIC;

    return pPMRoot;
}

INT __pm_alloc_physical_deinit(void *pRoot)
{
    PM_ALLOC_ROOT_T *pPMRoot = (PM_ALLOC_ROOT_T *) pRoot;

    if(!pPMRoot || pPMRoot->init_magic != __PM_INIT_MAGIC) 
    {
        PFATAL("Tried to use an invalid heap root pointer !\n");
        return 0;
    } 

    if (0 == pPMRoot->uTotalUnits)  
    {
        PDEBUG("DEINIT: Tries to deinit before successful init !\n");
        return -1;        // you are going to deallocate an non existing one
    }
     __pm_alloc_physical_heap_walk(pRoot);

    if (pPMRoot->uFreeUnits != pPMRoot->uTotalUnits) 
    {
        int i;

        PDEBUG("DEINIT: Some allocations are still in use !\n");
        // dump these in use handles
        for(i=0; i<pPMRoot->uMaxHandles; i++)
        {
            MEM_HANDLE_T pHandle = os_walk_up_pool(&pPMRoot->handlePool, i);
            if (pHandle->uSize > 0) // in use handle
            {
                PDEBUG("  Handle 0x%8.8x, l_addr=0x%8.8x, p_addr=0x%8.8x, size=0x%8.8x units\n", pHandle, pHandle->pLogical, pHandle->uPhysical, pHandle->uSize);
            }
        }
    }
    // and the lock
    PML_DEINIT();
    FREE(pPMRoot);
    return 0;
}



void __pm_alloc_physical_heap_walk(void *pRoot)
{
#ifdef  __PM_ALLOC_DEBUG
    PM_ALLOC_FREE_NODE_T *pNode;
    INT npNode;
    UINT nodes, sum;
    PM_ALLOC_ROOT_T *pPMRoot = (PM_ALLOC_ROOT_T *) pRoot;

    if(!pPMRoot || pPMRoot->init_magic != __PM_INIT_MAGIC) 
    {
        PFATAL("Tried to use an invalid heap root pointer !\n");
        return;
    } 

    //########################################################################
    PML_ENTER();
    
    PDEBUGE("\n\n++++++++++++++++ Heap Dump +++++++++++++++\n");
    if(pPMRoot->npFreeList < 0)
    {
        PDEBUG("Heap empty\n");
        goto __os_pm_heap_walk_out;
    }
    nodes = sum = 0;
    npNode = pPMRoot->npFreeList;
    PDEBUGE("Start Trace Free Space\n");
    while(npNode >= 0)
    {
        pNode = __PM_GET_BLOCK_POINTER(npNode);
        PDEBUGE(" Node 0x%8.8x, Addr= 0x%8.8x, size=0x%8.8x, prev=0x%8.8x, next=0x%8.8x\n",
                npNode, pNode->npAddr, pNode->uUnits, pNode->npPrev, pNode->npNext);
        PDEBUGE("     P Addr= 0x%8.8x,  L Addr=0x%8.8x\n",
                    (UINT)npNode*__PM_ALLOC_UNIT + pPMRoot->uPhysicalAddress, 
                    (UINT)npNode*__PM_ALLOC_UNIT + pPMRoot->pLogicalAddress);
        OVERRUN_DET(pPMRoot, npNode);
        nodes ++;
        sum += pNode->uUnits;
        if(pNode->npNext >= 0 && npNode >= pNode->npNext)
        {
            PFATAL("Heap Corrupted !!!\n");
            break;  
        }
        npNode = pNode->npNext;   // next one
    }    
    PDEBUGE("Total nodes = %d, total size = %d. \n", nodes, sum);
    PDEBUGE("++++++++++++++++++++++++++++++++++++++++++\n\n");
    
__os_pm_heap_walk_out:

   PML_LEAVE();
   //########################################################################
   return;
#endif
}

