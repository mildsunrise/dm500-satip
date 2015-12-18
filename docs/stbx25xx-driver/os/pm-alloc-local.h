//pallas/drv/os/pm-alloc-local.h
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
//  Local header of physical memory block allocation routines for Linux  
//Revision Log:   
//  Sept/03/2001            Created by YYD

#ifndef  _DRV_OS_PM_ALLOC_LOCAL_H_INC_
#define  _DRV_OS_PM_ALLOC_LOCAL_H_INC_

#include <asm/page.h>           // where the system page size is defined
#define __SYSTEM_PAGE_SIZE_DEF  PAGE_SIZE

#include "os/os-types.h"
#include "os/pm-alloc.h"
#include "os/helper-pool.h"

#define __PM_ALLOC_UNIT           (__SYSTEM_PAGE_SIZE_DEF) // the minimal allocation granularity is at system page size

#define __PM_ALLOC_DUMMY1_SIZE    (__PM_ALLOC_UNIT/2)
#define __PM_ALLOC_DUMMY2_SIZE    (__PM_ALLOC_UNIT/2 - sizeof(UINT)*4) // subtract the used members

//#define __PM_MAX_HANDLES          (4096)      // the maximum number of allocation handles we can have

#define __PM_INIT_MAGIC         0xbadbad88

// #define __PM_HANDLE_POOL_SIZE     os_tell_pool_buffer_size(__PM_MAX_HANDLES, sizeof(struct __MEM_HANDLE_T_STRUCT))

struct __PM_ALLOC_FREE_NODE_STRUCT   // the size should be the same as __PM_ALLOC_UNIT
{
    BYTE bDummy1[__PM_ALLOC_DUMMY1_SIZE];  // Dummy buffer to protect and detect overrun
    INT  npAddr;        // Relative address of this block in __PM_ALLOC_UNIT, for check only
    UINT uUnits;        // block size in __PM_ALLOC_UNIT
    INT  npNext;        // Next Free Block in chain
    INT  npPrev;        // previous Free Block in chain
    BYTE bDummy2[__PM_ALLOC_DUMMY2_SIZE]; // Dummy buffer to protect and detect overrun
};

typedef struct __PM_ALLOC_FREE_NODE_STRUCT PM_ALLOC_FREE_NODE_T;

struct __PM_ALLOC_ROOT_STRUCT
{
    UINT uPhysicalAddress;      // The Physical Address
    BYTE *pLogicalAddress;      // The Logical Address
    UINT uTotalUnits;           // The number of allocable units in total
    UINT uTotalMem;             // Total physical mem size including handle pool
    INT  npFreeList;            // Logical Pointer (in Units) to the first free node 
    UINT uFreeUnits;            // The number of Free units remainning
    void *pHandlePoolBuffer;    // The start address of handle area 
    POOL_T handlePool;          // handle pool
    UINT32 uMaxHandles;	        // max handles
    UINT32 sync;                // sync storage
    UINT32 init_magic;          // magic number
};

typedef struct __PM_ALLOC_ROOT_STRUCT PM_ALLOC_ROOT_T;

#define __PM_MODULE_NAME  "pm-alloc"

// Physical memory allocation
//
//  uPhysicalAddress 
//     PM_ALLOC_FREE_NODE 
//          |    ^
//          V    |
//     PM_ALLOC_FREE_NODE 
//          |    ^
//          V    |
//     PM_ALLOC_FREE_NODE 
//          |    ^
//          V    |
//     PM_ALLOC_FREE_NODE 
//
// [Start of Handle Pool]   pHandlePoolBuffer 
//      __PM_HANDLE_POOL_SIZE
// End of Physical Mempory

#endif  //  _DRV_OS_PM_ALLOC_LOCAL_H_INC_



