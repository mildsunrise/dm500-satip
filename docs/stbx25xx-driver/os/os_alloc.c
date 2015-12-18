//vulcan/drv/os/os_alloc.c  version 0.1
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
//  May/27/2002                               Seperated from pm-alloc.c by YYD
//  Jun/03/2002        Slightly modified __os_alloc_init API for Vulcan by YYD

// Begin Linux Specific Header
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/stddef.h>
#include <linux/ioport.h>
#include <linux/string.h>    
#include <asm/io.h>
// End Linux Specific Header

#include "os/pm-alloc.h"
#include "os/helper-pool.h"
#include "pm-alloc-local.h"

#ifdef __OS_ALLOC_DEBUG
#define __DRV_DEBUG
#endif
#include "os/drv_debug.h"


// defined the root allocation data structure
static void *gpPMRoot;
static UINT32 guPhysicalAddr;
static UINT32 guPhysicalSize;
static UINT32 guPhysicalBase;
static void *guLogicalAddr;


MEM_HANDLE_T os_alloc_physical_justify(UINT32 uNumBytes, UINT32 uJustify)
{
    return pm_alloc_physical_justify(gpPMRoot, uNumBytes, uJustify);
}

MEM_HANDLE_T os_alloc_physical (UINT32 uNumBytes)
{
    return pm_alloc_physical_justify(gpPMRoot, uNumBytes, 0);
}

void os_free_physical (MEM_HANDLE_T hPhysicalMem)
{
    return pm_free_physical(gpPMRoot, hPhysicalMem);
}

ULONG os_get_actual_physical_size(MEM_HANDLE_T hPhysicalMem)
{
    if (NULL == hPhysicalMem) return 0;
    return (ULONG)hPhysicalMem->uSize*__PM_ALLOC_UNIT;
}


UINT32 os_get_physical_base(void)
{
    return guPhysicalBase;
}

void *os_get_logical_base(void)
{
    return pm_get_logical_base(gpPMRoot);
}

UINT32 os_get_physical_total_size(void)
{
    return pm_get_physical_total_size(gpPMRoot);
}


// Only used during initialize and deinitialize
INT __os_alloc_physical_init(UINT32 uPhysicalAddr, UINT32 uSize, UINT32 uPhysicalBase)
{
    void *pmapped;
    
    if (gpPMRoot) // it should have been initialized, so error
    {
        return PM_ALLOC_ERROR;    //  Fixme:  How about some further checks
    }

    PDEBUG(" Physical mem at 0x%8.8x , size 0x%8.8x\n",  uPhysicalAddr,  uSize);

    if ((uPhysicalAddr + uSize) < uPhysicalAddr) // how can we wrap around ?!
        return PM_ALLOC_INVALID_PARM;

    if (!request_region(uPhysicalAddr, uSize, __PM_MODULE_NAME))
    {
        return PM_ALLOC_ERROR;    // hi, I cann't do this
    }
    
    pmapped = (void *)ioremap(uPhysicalAddr, uSize);
    if (NULL == pmapped) // Wa, fail !
    {
        release_region(uPhysicalAddr, uSize);
        return PM_ALLOC_ERROR;    // hi, I cann't do this
    }

    guPhysicalAddr = uPhysicalAddr;
    guPhysicalSize = uSize;
    guLogicalAddr = pmapped;
    guPhysicalBase = uPhysicalBase;

    gpPMRoot = __pm_alloc_physical_init(uPhysicalAddr, pmapped, uSize, 4096);
    if(gpPMRoot)
        return PM_ALLOC_SUCCESS;

    // failed
    iounmap(guLogicalAddr);
    release_region(guPhysicalAddr, guPhysicalSize);
    
    return PM_ALLOC_ERROR;
}

INT __os_alloc_physical_deinit(void)
{
    if (!gpPMRoot)  
    {
        PDEBUG("DEINIT: Tries to deinit before successful init !\n");
        return -1;        // you are going to deallocate an non existing one
    }

    __pm_alloc_physical_deinit(gpPMRoot);

    gpPMRoot = NULL;

    iounmap(guLogicalAddr);
    release_region(guPhysicalAddr, guPhysicalSize);
    guPhysicalAddr = 0;
    guPhysicalSize = 0;
    guLogicalAddr = NULL;
    return 0;
}



void __os_alloc_physical_heap_walk()
{
    return __pm_alloc_physical_heap_walk(gpPMRoot);
}

