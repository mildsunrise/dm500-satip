//pallas/drv/include/os/os-generic.h
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
//  A common OS abstraction interface for Linux  
//Revision Log:   
//  Sept/03/2001			Created by YYD
//  Sept/28/2001			Add _OS_MEM helper, YYD

#ifndef  _DRV_INCLUDE_OS_OS_GENERIC_H_INC_
#define  _DRV_INCLUDE_OS_OS_GENERIC_H_INC_

#include "os/os-types.h"

#include <linux/mm.h>
#include <linux/slab.h>  // where kmalloc is defined ??
#include <linux/interrupt.h>  // where in_interrupt defined

#define MALLOC(s) kmalloc((s), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL)
#define FREE(s)   kfree((s))

#include <asm/string.h>    

#define _OS_MEMCPY(a,b,c)   memcpy(a,b,c)
#define _OS_MEMMOVE(a,b,c)  memmove(a,b,c)
#define _OS_MEMSET(a,b,c)   memset(a,b,c)


void * _OS_MAP_PHYSICAL_MEM(UINT uPhyAddr, UINT uSize);
// return NULL for fail and logical address on success

INT    _OS_UNMAP_PHYSICAL_MEM(UINT uPhyAddr, void *pLogicalAddr);
// return -1 on fail, 0 on success


#endif  //  _DRV_INCLUDE_OS_OS_GENERIC_H_INC_



