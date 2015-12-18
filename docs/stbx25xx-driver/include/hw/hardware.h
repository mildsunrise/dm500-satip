//drv/include/hw/hardware.h
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
//  Hardware defination of IBM STB  controller
//Revision Log:
//  Oct/12/2001                 Created by YYD
//  Jun/28/2002         Added defination for platform id    YYD
//  Nov/13/2002         Added defination for cache line size    YYD

#ifndef  _DRV_INCLUDE_HW_HARDWARE_H_INC_
#define  _DRV_INCLUDE_HW_HARDWARE_H_INC_

#define __STB_SYS_CLK       (63000000)      // 63MHz SYS_CLK of Vulcan
#define __TMRCLK            (27000000)      // 27MHz hardware timer

#include "os/os-io.h"       // for MF_SPR

#define SPR_PVR    (0x3e8)
#define SPR_TBL    (0x10C)

#define GET_PPC_VERSION()  MF_SPR(SPR_PVR)

#define PPC_PALLAS_PBA     (0x41810890) // initial release of pallas
#define PPC_PALLAS_PBB     (0x418108d1) // pallas 2
#define PPC_VESTA          (0x40310083) // Vesta PBE
#define PPC_VULCAN_PBA     (0x40000000) // unknown currently

#define PPC_STB_CACHE_LINE_SIZE         32

//#define __DRV_FOR_PALLAS__      // defined for Pallas driver
#define __DRV_FOR_VULCAN__    // defined for Vulcan driver
//#define __DRV_FOR_VESTA__     // defined for Vesta driver
#endif  // _DRV_INCLUDE_HW_HARDWARE_H_INC_
