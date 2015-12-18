//pallas/drv/include/os/drv_debug.h
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
//  Common debug print routines for Linux  
//Revision Log:   
//  Sept/10/2001                          Created by YYD

#ifndef _DRV_INCLUDE_OS_DRV_DEBUG_H_INC_
#define _DRV_INCLUDE_OS_DRV_DEBUG_H_INC_

#include <linux/kernel.h>

// due to the limitation of gcc 2.95.3, we could not add __LINE__

#ifdef __DRV_DEBUG
#if ( __GNUC__ > 2)
    #define PDEBUG(fmt, args...)    printk(KERN_NOTICE "%s:%s: " fmt,__FILE__, __FUNCTION__, ##args)
#else
    #define PDEBUG(fmt, args...)    printk(KERN_NOTICE __FILE__ ":" __FUNCTION__ ": " fmt, ##args)
#endif
    #define PDEBUGE(fmt, args...)    printk(fmt, ##args)

#else
    #define PDEBUG(fmt, args...)
    #define PDEBUGE(fmt, args...)
#endif

#if ( __GNUC__ > 2)
#define PFATAL(fmt, args...)    printk(KERN_CRIT "*** %s:%s: " fmt, __FILE__, __FUNCTION__,  ##args)
#else
#define PFATAL(fmt, args...)    printk(KERN_CRIT "*** " __FILE__ ":" __FUNCTION__ ": " fmt, ##args)
#endif
#define PFATALE(fmt, args...)   printk(KERN_CRIT  fmt, ##args)

#if ( __GNUC__ > 2)
#define PINFO(fmt, args...)    printk(KERN_NOTICE "*** %s:%s: " fmt, __FILE__, __FUNCTION__,##args)
#else
#define PINFO(fmt, args...)    printk(KERN_NOTICE "*** " __FILE__ ":" __FUNCTION__ ": " fmt, ##args)
#endif
#define PINFOE(fmt, args...)   printk(KERN_NOTICE  fmt, ##args)

#endif //_DRV_INCLUDE_OS_DRV_DEBUG_H_INC_
