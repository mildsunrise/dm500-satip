//vulcan/drv/gfx/osd_local.h
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
//  Common defines of OSD functions 
//Revision Log:   
//  Sept/28/2001                          Created by YYD

#ifndef  _DRV_GFX_OSD_LOCAL_H_INC_
#define  _DRV_GFX_OSD_LOCAL_H_INC_

#include "os/os-types.h"
#include "gfx_surface.h"

// defines osd planer configs

// to distinguish field applicables
// z/Z  : not used
// c/C  : cursor plane
// g/G  : graphics plane
// s/S  : still plane
// b/B  : back ground plane
// a/A  : all plane


#define OSD_PLANE_MAX_DIMENSIONS    (0x1fff)
#define OSD_CURSOR_POSITION_RANGE   (0x3ff)

#define OSD_PLANE_GRAPHICS          GFX_COMP_OSDGFX
#define OSD_PLANE_CURSOR            GFX_COMP_OSDCUR
#define OSD_PLANE_IMAGE             GFX_COMP_OSDIMG

#define OSD_PLANE_MAX_SUBPLANES     GFX_SURFACE_MAX_SUBPLANES

#define OSD_PLANE_ONE_SUBPLANE	    GFX_SURFACE_ONE_SUBPLANE
#define OSD_PLANE_TWO_SUBPLANE	    GFX_SURFACE_TWO_SUBPLANE
#define OSD_PLANE_THREE_SUBPLANE	GFX_SURFACE_THREE_SUBPLANE

#endif //  _DRV_GFX_OSD_LOCAL_H_INC_
