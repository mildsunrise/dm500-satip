//pallas/drv/include/os/helper-pool.c
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
//  Common Pool helper routines  
//Revision Log:   
//  Sept/27/2001            Created by YYD


#include "os/helper-pool.h"
#include "helper-pool-local.h"

#ifndef __HELPER_POOL_TEST__
    #include "os/os-generic.h"      // for MALLOC, FREE, _OS_MEMCPY
#else
    #include <malloc.h>
    #include <string.h>
    #define MALLOC(a) malloc(a)
    #define FREE(a) free(a)
    #define _OS_MEMCPY(a,b,c) memcpy(a,b,c)
#endif

#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif


INT os_create_pool (POOL_T *pPool, void *pBuffer, UINT uElements, UINT uElementSize)
{
    UINT i;

    if (NULL == pPool) return -1;
    if (0 == uElements*uElementSize) return -1;
    // init the buffer
    if (NULL != pBuffer) // user provides an buffer
    {
        pPool->pBuffer = pBuffer;
        pPool->uAttr = 0;
    }
    else
    {
        pPool->pBuffer = MALLOC(os_tell_pool_buffer_size(uElements, uElementSize));
        if (NULL == pPool->pBuffer) return -1;
        pPool->uAttr = __HELPER_POOL_MEMALLOC;
    }
    
    pPool->uElements = uElements;
    pPool->uElementSize_plus_pointer = uElementSize+sizeof(POOL_CHAIN_T *);
    pPool->uFreeElements = uElements;
 
    // initialize pool chain
    for(i=0; i<uElements; i++)
    {
        ((POOL_CHAIN_T *)((char *)pPool->pBuffer + i*pPool->uElementSize_plus_pointer))->pNext = 
            (POOL_CHAIN_T *)((char *)pPool->pBuffer + (i+1)*pPool->uElementSize_plus_pointer);
    }
    pPool->pPoolHead = pPool->pBuffer;
    ((POOL_CHAIN_T *)((char *)pPool->pBuffer + (uElements-1)*pPool->uElementSize_plus_pointer))->pNext = NULL;

    return 0;
}

INT os_delete_pool (POOL_T *pPool)
{
    if (NULL == pPool || NULL == pPool->pBuffer) return -1;
    pPool->pPoolHead = NULL;
    pPool->uElements = 0;
    pPool->uElementSize_plus_pointer = 0;
    pPool->uFreeElements = 0;
    if(pPool->uAttr & __HELPER_POOL_MEMALLOC) FREE(pPool->pBuffer);
    pPool->pBuffer = NULL;
    return 0;
}


void * os_get_from_pool (POOL_T *pPool)
{
    POOL_CHAIN_T  *pCurr;
    if (NULL == pPool || NULL == pPool->pBuffer
        || NULL == pPool->pPoolHead) 
        return NULL;
    pCurr = (POOL_CHAIN_T *)pPool->pPoolHead;
    pPool->pPoolHead = ((POOL_CHAIN_T *)pPool->pPoolHead)->pNext;
    pCurr->pNext = NULL;
//    pPool->uFreeElements --;
    return (char *)pCurr + sizeof(POOL_CHAIN_T *);
}

INT os_put_back_to_pool(POOL_T *pPool, void *pElement)
{
    POOL_CHAIN_T  *pCurr;
    if (NULL == pPool || NULL == pPool->pBuffer
        || NULL == pElement) 
        return -1;
    pCurr = (POOL_CHAIN_T  *)((char *)pElement - sizeof(POOL_CHAIN_T *));
    if(pCurr->pNext != NULL) return -1; // Should I check this ?
    pCurr->pNext = (POOL_CHAIN_T  *)pPool->pPoolHead;
    pPool->pPoolHead = pCurr;
//    pPool->uFreeElements ++;
    return 0;
}


INT os_get_pool_status(POOL_T *pPool)
{
    if (NULL == pPool || NULL == pPool->pBuffer) return -1;
    return  pPool->pPoolHead != NULL ? 1 : 0 ;
}

INT os_validate_pool_element(POOL_T *pPool, void *pElement)
{
    UINT uAddr;
    if (NULL == pPool || NULL == pPool->pBuffer
        || NULL == pElement) 
        return -1;
    uAddr = (UINT)((char *)pElement - sizeof(POOL_CHAIN_T *));
    if(uAddr < (UINT)pPool->pBuffer) return -1;
    uAddr -= (UINT)pPool->pBuffer;
    if(uAddr % pPool->uElementSize_plus_pointer != 0) return -1;
    if(uAddr >= pPool->uElementSize_plus_pointer*pPool->uElements) return -1;
    return 0;
}

void * os_walk_up_pool(POOL_T *pPool, UINT uWhichElement)
{
    POOL_CHAIN_T  *pCurr;
    if (NULL == pPool || NULL == pPool->pBuffer
        || NULL == pPool->pPoolHead || pPool->uElements <= uWhichElement)
        return NULL;
    pCurr = (POOL_CHAIN_T *)((char *)pPool->pBuffer + uWhichElement*pPool->uElementSize_plus_pointer);
    return (char *)pCurr + sizeof(POOL_CHAIN_T *);
}

