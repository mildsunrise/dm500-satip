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
// Created by YYD

#ifndef __PVERSION_H_INC__
#define __PVERSION_H_INC__

#include <linux/kernel.h>

#define __STB_DRIVER_RELEASE__  "Feb/16/2004"

#ifndef  __DRV_VERSION__  // current version
    #define __DRV_VERSION__  "1.5"
#endif


#ifndef  __DRV_DATE__   // last modified date
    #define __DRV_DATE__  "Feb/16/2004"
#endif


#define PVERSION(module_name)   printk(KERN_NOTICE "\n%s " __DRV_VERSION__ "-" \
                __DRV_DATE__ "@" __STB_DRIVER_RELEASE__ " Built " __DATE__ "/" __TIME__ " for " UTS_RELEASE "\n", module_name)

#endif

