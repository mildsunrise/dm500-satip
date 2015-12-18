//pallas/drv/include/os/pm-alloc.h
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
//  Physical memory block allocation routines for Linux
//Revision Log:
//  Aug/31/2001                                                 Created by YYD
//  Oct/17/2001                                     Add justified alloc by YYD
//  May/27/2002                     Exposed generic heap management API by YYD
//  Jun/03/2002        Slightly modified __os_alloc_init API for Vulcan by YYD

#ifndef  _DRV_INCLUDE_OS_PM_ALLOC_H_INC_
#define  _DRV_INCLUDE_OS_PM_ALLOC_H_INC_

#include "os-types.h"

struct __MEM_HANDLE_T_STRUCT  // Please don't access these internal data structures
{
    UINT                   uPhysical;
    void                   *pLogical;
    UINT                   uSize;
};

typedef struct __MEM_HANDLE_T_STRUCT * MEM_HANDLE_T;

MEM_HANDLE_T os_alloc_physical (UINT32 uNumBytes);

MEM_HANDLE_T os_alloc_physical_justify(UINT32 uNumBytes, UINT32 uJustify);
// justification is rounded to 2^n boundary

void os_free_physical (MEM_HANDLE_T hPhysicalMem);

static inline void *os_get_logical_address(MEM_HANDLE_T hPhysicalMem) { return hPhysicalMem->pLogical; }

static inline ULONG os_get_physical_address(MEM_HANDLE_T hPhysicalMem) { return hPhysicalMem->uPhysical; }

ULONG os_get_actual_physical_size(MEM_HANDLE_T hPhysicalMem);

UINT32 os_get_physical_base(void);

void *os_get_logical_base(void);

UINT32 os_get_physical_total_size(void);






// only availble when debug is enabled
void __os_alloc_physical_heap_walk(void);


// Only used during initialize and deinitialize
INT __os_alloc_physical_init(UINT32 uPhysicalAddr, UINT32 uSize, UINT32 uPhysicalBase);
INT __os_alloc_physical_deinit(void);


// Error Codes

#define PM_ALLOC_SUCCESS         0  // everything is ok
#define PM_ALLOC_ERROR          -1  // undefined error
#define PM_ALLOC_INVALID_PARM   -2  // invalid input parameter
#define PM_ALLOC_OUT_OF_MEMORY  -3  // out of memory
#define PM_ALLOC_OUT_OF_HANDLE  -4  // out of memory handle space


// common physical heap helper routines

MEM_HANDLE_T pm_alloc_physical (void * pRoot, UINT uNumBytes);

MEM_HANDLE_T pm_alloc_physical_justify(void * pRoot, UINT uNumBytes, UINT uJustify);
// justification is rounded to 2^n boundary

void pm_free_physical (void * pRoot, MEM_HANDLE_T hPhysicalMem);

static inline void *pm_get_logical_address(MEM_HANDLE_T hPhysicalMem) { return hPhysicalMem->pLogical; }

static inline ULONG pm_get_physical_address(MEM_HANDLE_T hPhysicalMem) { return hPhysicalMem->uPhysical; }

ULONG pm_get_actual_physical_size(MEM_HANDLE_T hPhysicalMem);

UINT pm_get_physical_base(void * pRoot);

void *pm_get_logical_base(void * pRoot);

UINT pm_get_physical_total_size(void * pRoot);


// only availble when debug is enabled
void __pm_alloc_physical_heap_walk(void * pRoot);


// Only used during initialize and deinitialize
void * __pm_alloc_physical_init(UINT uPhysicalAddr, void *pLogicalAddr, UINT uSize, UINT uMaxHandles);
INT __pm_alloc_physical_deinit(void * pRoot);


#endif  //  _DRV_INCLUDE_OS_PM_ALLOC_H_INC_



