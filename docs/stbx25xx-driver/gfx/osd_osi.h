//vulcan/drv/gfx/osd_osi.h
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
//  OSI function of OSD controls 
//Revision Log:   
//  Sept/26/2001                          Created by YYD
//   Oct/21/2002           Added OSD_DISP_CNTL_ANIM, OSD_DISP_CNTL_ANIMR, and
//                         OSD_DISP_CNTL_CHFSR to OSD_DISPLAY_CONTROL_T -- BJC


#ifndef  _DRV_INCLUDE_GFX_OSD_OSI_H_INC_
#define  _DRV_INCLUDE_GFX_OSD_OSI_H_INC_

#include "os/os-types.h"
#include "os/pm-alloc.h"
#include "gfx_surface.h"



typedef  enum
{
   OSD_GRAPH_DEVICE_GLOBAL_ALPHA,        //   will always fail
   OSD_GRAPH_DEVICE_ENABLE               //   0x001 enable / 0x000 disable
}  OSD_GRAPH_DEVICE_PARM_T;

typedef enum
{
    OSD_GRAPH_SURFACE_FLICKER_CORRECTION,       // r/w the antiflicker filter value (v1)
    OSD_GRAPH_SURFACE_SCREEN_OFFSET,            // r/w the offset of surface to screen (v1 as horizontal, v2 as vertical)
    OSD_GRAPH_SURFACE_SINGLE_PALETTE,           // r/w access a single palette (v1 as index and v2 as value)
    OSD_GRAPH_SURFACE_ALL_PALETTE,              // r/w access all palette from 0 (v1 as num of entries, v2 is used as pointer to palette array)
                                                //  if v1 == NULL, the default palette is used
} OSD_GRAPH_SURFACE_PARM_T;

typedef enum
{
    OSD_DISP_CNTL_AFVP,      // Anti-flicker video plane, 0 disable, 1 enable
    OSD_DISP_CNTL_EDAF,      // Enable display anti-flicker, 0 disable, 1 enable
    OSD_DISP_CNTL_AFDT,      // Anti-flicker detection threshold, 2 bits attr
    OSD_DISP_CNTL_VPAFC,     // Video plane anti-flicker correction, 2 bits attr
    OSD_DISP_CNTL_BACKCOLOR, // 16 bits background color in y8:u4:v4
    OSD_DISP_CNTL_ANIM,      // Animation mode, 0 no, 1 yes -- BJC 102102
    OSD_DISP_CNTL_ANIMR,     // Animation rate, 3 bits attr -- BJC 102102
    OSD_DISP_CNTL_CHFSR,     // Custom horizontal finite scaling ratio, 9 bits attr -- BJC 102102
} OSD_DISPLAY_CONTROL_T;


INT osd_osi_init(void);
INT osd_osi_deinit(void);

INT osd_osi_attach_comp_gfx_surface(GFX_VISUAL_DEVICE_ID_T graphDev, GFX_SURFACE_T * pSurface);
// attach will also detach previous surface if needed.

INT osd_osi_detach_comp_gfx_surface(GFX_VISUAL_DEVICE_ID_T graphDev, GFX_SURFACE_T * pSurface);

INT osd_osi_set_device_parm(GFX_VISUAL_DEVICE_ID_T graphDev, OSD_GRAPH_DEVICE_PARM_T parm, UINT uValue);
UINT osd_osi_get_device_parm(GFX_VISUAL_DEVICE_ID_T graphDev, OSD_GRAPH_DEVICE_PARM_T parm);

INT osd_osi_set_display_parm(OSD_DISPLAY_CONTROL_T parm, UINT uAttr);
UINT osd_osi_get_display_parm(OSD_DISPLAY_CONTROL_T parm);

INT osd_osi_set_comp_gfx_surface_parm(GFX_SURFACE_T * pSurface, OSD_GRAPH_SURFACE_PARM_T parm, ULONG lValue1, ULONG lValue2);
INT osd_osi_get_comp_gfx_surface_parm(GFX_SURFACE_T *pSurface, OSD_GRAPH_SURFACE_PARM_T parm, ULONG *plValue1, ULONG *plValue2);

INT osd_osi_create_cursor(GFX_SURFACE_T *pCursor, UINT uPlaneConfig, UINT uWidth, UINT uHeight);
INT osd_osi_destory_cursor(GFX_SURFACE_T *pCursor);
INT osd_osi_attach_cursor(GFX_SURFACE_T *pCursor);
INT osd_osi_detach_cursor(GFX_SURFACE_T *pCursor);
INT osd_osi_set_cursor_position(INT nHorizontal, INT nVertical);
INT osd_osi_get_cursor_position(INT *pnHorizontal, INT *pnVertical);
INT osd_osi_set_cursor_attributes(GFX_SURFACE_T *pCursor, UINT uIndex, GFX_CURSOR_ATTRIBUTE_T attr);

#endif  // _DRV_INCLUDE_OSD_OSD_OSI_H_INC_
