//vulcan/drv/gfx/osd_dcr.h
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
//  DCR defination of of OSD controls 
//Revision Log:   
//  Jun/04/2002                          Created by YYD

#ifndef  _DRV_GFX_OSD_DCR_H_INC_
#define  _DRV_GFX_OSD_DCR_H_INC_

#include "os/os-io.h"

#define DCR_VID0_OSD_MODE       (0x0151)        // display control register
#define DCR_VID0_GSLA           (0x0159)        // graphics start link register
#define DCR_VID0_ISLA           (0x015a)        // image start link register
#define DCR_VID0_CSLA           (0x015c)        // cursor start link register
#define DCR_VID0_IPBASE         (0x016d)        // image layer OSD base address
#define DCR_VID0_GPBASE         (0x016e)        // graphics layer OSD base address
#define DCR_VID0_CPBASE         (0x017a)        // cursor layer OSD base address
#define DCR_VID0_DISP_MODE      (0x0154)

#endif // _DRV_GFX_OSD_DCR_H_INC_
