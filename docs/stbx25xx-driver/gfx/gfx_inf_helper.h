//vulcan/drv/gfx/gfx_inf_helper.h
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
//  Helper function for INF layer of GFX controls 
//Revision Log:   
//  Nov/06/2001                          Created by YYD
//  Oct/21/2002  Declared prototypes for several newly added functions.     BJC

#ifndef  _DRV_GFX_GFX_INF_HELPER_H_INC_
#define  _DRV_GFX_GFX_INF_HELPER_H_INC_

#include "gfx/gfx_common.h"
#include "gfx/gfx_inf_struct.h"
#include "gfx_surface.h"

#define GFX_DEFAULT_SURFACES        256         // a max of 256 surfaces for use

#define OSD_DEFAULT_XRES    640
#define OSD_DEFAULT_YRES    440

#define OSD_DEFAULT_TV_LEFT_MARGIN  30
#define OSD_DEFAULT_TV_UPPER_MARGIN 30


typedef struct __GFX_SURFACE_LOCAL_INFO_T_STRUCTURE__
{
    int hSurface;
    UINT uPlaneConfig;

    // GFX_PALETTE_T   *pPalette;

    struct __GFX_SURFACE_PLANE_LOCAL_INFO_T_STRUCTURE__
    {
        void *pPlane;      // physical info of plane
        UINT uWidth;        // allocated subpalne width
        UINT uHeight;       // allocated subplane height
        UINT uPixelSize;         // pixel size (bits) of the subplane
        UINT uPixelJustify;      // pixel justification requirement (pixel)
        UINT uBytePerLine;       // byte per line of the subplane
        GFX_PIXEL_INFO_T a, r, g, b;     // pixel config
    } plane[GFX_SURFACE_MAX_SUBPLANES];

} GFX_SURFACE_LOCAL_INFO_T;


int gfx_inf_h_init(unsigned int uNumSurface);

int gfx_inf_h_deinit(int nForce);
// nForce :  0      // safest mode
//           1      // stop when surfaces are locked
//           2      // continue anyway


int gfx_inf_h_create_surface(GFX_CREATE_SURFACE_PARM_T *pParm);
//   -1  fail
//    0  success

int gfx_inf_h_destroy_surface(int hSurface);

int gfx_inf_h_reconfig_surface_dimension(int hSurface, UINT uNewWidth, UINT uNewHeight);

int gfx_inf_h_get_subplane_pseudo_surface(GFX_GET_SUBPLANE_PSEUDO_SURFACE_PARM_T *pParm);

int gfx_inf_h_get_surface_info(GFX_SURFACE_INFO_T *pInfo);

int gfx_inf_h_lock_surface(GFX_SURFACE_INFO_T *pInfo);
int gfx_inf_h_unlock_surface(int hSurface);

int gfx_inf_h_attach_surface(GFX_SURFACE_VDEV_PARM_T *pParm);
int gfx_inf_h_detach_surface(GFX_SURFACE_VDEV_PARM_T *pParm);
int gfx_inf_h_swap_surface(GFX_2SURFACE_VDEV_PARM_T *pParm); // BJC 102102

int gfx_inf_h_get_screen_info(GFX_SCREEN_INFO_T *pInfo);
int gfx_inf_h_set_screen_info(GFX_SCREEN_INFO_T *pInfo);

int gfx_inf_h_get_surface_display_parm(GFX_SURFACE_DISPLAY_T *pParm);
int gfx_inf_h_set_surface_display_parm(GFX_SURFACE_DISPLAY_T *pParm);

int gfx_inf_h_get_display_control(GFX_DISPLAY_CONTROL_PARM_T *pParm);
int gfx_inf_h_set_display_control(GFX_DISPLAY_CONTROL_PARM_T *pParm);

int gfx_inf_h_get_visual_device_control(GFX_VISUAL_DEVICE_CONTROL_PARM_T *pParm);
int gfx_inf_h_set_visual_device_control(GFX_VISUAL_DEVICE_CONTROL_PARM_T *pParm);

int gfx_inf_h_get_palette(GFX_SURFACE_ACCESS_PALETTE_PARM_T *pParm);
int gfx_inf_h_set_palette(GFX_SURFACE_ACCESS_PALETTE_PARM_T *pParm);

int gfx_inf_h_move_cursor(GFX_COORDINATE_T *pParm);
int gfx_inf_h_report_cursor_position(GFX_COORDINATE_T *pParm);
int gfx_inf_h_set_cursor_attributes(GFX_CURSOR_ATTRUBUTE_PARM_T *pParm);

int gfx_inf_h_wait_for_engine(int nTimeout);

int gfx_inf_h_reset_engine(void);

int gfx_inf_h_set_engine_mode(int enable_sync);
int gfx_inf_h_get_engine_mode();

int gfx_inf_h_set_surface_clip_rect(GFX_SET_CLIP_PARM_T *pParm);

int gfx_inf_h_bitBLT(GFX_BITBLT_PARM_T *pParm);

int gfx_inf_h_advancedBitBLT(GFX_ADV_BITBLT_PARM_T *pParm);

int gfx_inf_h_fillBLT(GFX_FILLBLT_PARM_T *pParm);

int gfx_inf_h_advancedFillBLT(GFX_ADV_FILLBLT_PARM_T *pParm);

int gfx_inf_h_blend(GFX_BLEND_PARM_T *pParm);

int gfx_inf_h_advancedBlend(GFX_ADV_BLEND_PARM_T *pParm);

int gfx_inf_h_colorKey(GFX_COLORKEY_PARM_T *pParm);    

int gfx_inf_h_resize(GFX_RESIZE_PARM_T *pParm);

int gfx_inf_h_set_shared_surface(int hSurface);

int gfx_inf_h_get_shared_surface(void);

// this is a dirty slow process, but it is needed indeed
int _gfx_inf_h_validate_surface_address(ULONG uPlanePhysicalBaseAddr, ULONG uPlanePhysicalSize);

// limited use only
int _gfx_inf_h_get_surface_local(GFX_SURFACE_LOCAL_INFO_T *pInfo);

int gfx_inf_h_dmablt(GFX_DMABLT_PARM_T *pParm); // BJC 102102
int gfx_inf_h_dmablt_wait(void); // BJC 102102
int gfx_inf_h_pmalloc(GFX_PMALLOC_PARM_T *pParm); // BJC 102102

#endif // _DRV_GFX_GFX_INF_HELPER_H_INC_
