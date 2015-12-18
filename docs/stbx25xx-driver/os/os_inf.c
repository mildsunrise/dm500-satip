//vulcan/drv/os/os_inf.c
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
//Comment: 
//  Dummy Linux Moduler interface for OS core
//Revision Log:   
//  Sept/24/2001                           Created by YYD
//  Jun/03/2002              Modified for segments by YYD

// The necessary header files
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>

#include "hw/physical-mem.h"
#include "hw/hardware.h"
#include "os/pm-alloc.h"
#include "os/os-interrupt.h"
#include "os/drv_debug.h"
#include "os/os-io.h"


#include "os/pversion.h"

#ifdef MODULE
MODULE_AUTHOR("IBM CRL");
#endif

#ifdef __DRV_FOR_VULCAN__
#define OS_CORE_DRIVER_NAME   "STBx25xx OS-Core"
#ifdef MODULE
MODULE_DESCRIPTION("OS Core support driver for IBM STBx25xx Drivers");
#endif
#elif defined(__DRV_FOR_PALLAS__)
#define OS_CORE_DRIVER_NAME   "STB04xxx OS-Core"
#ifdef MODULE
MODULE_DESCRIPTION("OS Core support driver for IBM STB04xxx Drivers");
#endif
#elif defined(__DRV_FOR_VESTA__)
#define OS_CORE_DRIVER_NAME   "STB03xxx OS-Core"
#ifdef MODULE
MODULE_DESCRIPTION("OS Core support driver for IBM STB04xxx Drivers");
#endif
#else   
#error "Unsupported architecture, please specify it in 'include/hw/hardware.h'"
#endif



static int dummy_inf_init(void)
{
    // print the driver verision info for futher reference
    PVERSION(OS_CORE_DRIVER_NAME);
       
    if(__os_alloc_physical_init(__STB_ALLOC_MEM_BASE_ADDR, __STB_ALLOC_MEM_SIZE, __STB_ALLOC_MEM_BASE_ADDR) < 0)
    {
       PFATAL("PM-alloc failed!\n");
       return -1;
    }
    if(os_irq_init() < 0)
    {
       PFATAL("os-interrupt init failed!\n");
       __os_alloc_physical_deinit();
       return -1;
    }

    return 0;
}

static void dummy_inf_deinit(void)
{
    __os_alloc_physical_deinit();
}

module_init(dummy_inf_init);
module_exit(dummy_inf_deinit);

