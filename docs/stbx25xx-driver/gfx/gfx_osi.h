//vulcan/drv/gfx/gfx_osi.h
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
//  OSI function of GFX controls 
//Revision Log:   
//  Sept/26/2001                          Created by YYD

#ifndef  _DRV_INCLUDE_GFX_GFX_OSI_H_INC_
#define  _DRV_INCLUDE_GFX_GFX_OSI_H_INC_

#include "os/os-types.h"
#include "gfx/gfx_common.h"
#include "gfx_surface.h"


INT gfx_osi_init();

INT gfx_osi_deinit(void);

INT gfx_osi_run_engine(INT nWait);

INT gfx_osi_wait_for_engine(INT nTimeout);

INT gfx_osi_reset_engine(void);


INT gfx_osi_ClipBLTRect(UINT uSrcWidth, UINT uSrcHeight, 
                                UINT uDesWidth, UINT uDesHeight, 
                                UINT uSrcX,     UINT uSrcY,
                                UINT uDesX,     UINT uDesY,
                                UINT *uWidth,   UINT *uHeight
                                );

INT gfx_osi_ClipRect(UINT uDesWidth, UINT uDesHeight, 
                                UINT uDesX,     UINT uDesY,
                                UINT *uWidth,   UINT *uHeight
                                );


INT gfx_osi_ClipExtBLTRect(GFX_RECT_T *pSrcClip,
                              GFX_RECT_T *pDesClip, 
                                INT *nSrcX,     INT *nSrcY,
                                INT *nDesX,     INT *nDesY,
                                UINT *uWidth,   UINT *uHeight,
                                UINT *uAdjustX, UINT *uAdjustY
                                );

INT gfx_osi_ClipExtRect(GFX_RECT_T *pDesClip, 
                                INT *nDesX,    INT *nDesY,
                                UINT *uWidth,  UINT *uHeight
                                );


void gfx_osi_pixelJustify(UINT uJustify, UINT *uX, UINT *uWidth);


// pClip == NULL will reset to default
// else pClip will hold the previous settings
INT gfx_osi_set_surface_clip_region(GFX_SURFACE_T *pSurface, GFX_RECT_T *pClip);

INT gfx_osi_bitBLT(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                GFX_SURFACE_T *pSrc,  INT nSrcX,  INT nSrcY,
                ALPHA_SELECT *pAlphaSelect, char enableGammaCorrection);

INT gfx_osi_advancedBitBLT(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight,
            GFX_SURFACE_T *pSrc, INT nSrcX, INT nSrcY,
            GFX_SURFACE_T  *pMask, INT nMaskX, INT nMaskY,
            GFX_ROP_CODE_T ROP,
            char enablePixelBitMask, UINT32 uPixelBitMask,
            ALPHA_SELECT *pAlphaSelect);


INT gfx_osi_fillBLT(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight, UINT32 uFillColor);


INT gfx_osi_advancedFillBLT(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight, UINT32 uFillColor, /* formatted in the dest format color if rgb, index if clut */
            GFX_SURFACE_T  *pMask, INT nMaskX, INT nMaskY,
            GFX_ROP_CODE_T ROP, char transparencyEnable,    // 
            UINT32 uBackGroundColor, char enablePixelBitMask, UINT32 uPixelBitMask);


INT gfx_osi_blend(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                GFX_SURFACE_T *pSrc, INT nSrcX, INT nSrcY,
                ALPHA_BLEND_SELECT *pBlendSelect);

INT gfx_osi_advancedBlend(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                GFX_SURFACE_T *pSrc, INT nSrcX, INT nSrcY,
                GFX_SURFACE_T *pAlpha, INT nAlphaX, INT nAlphaY,
                ALPHA_BLEND_SELECT *pBlendSelect);

INT gfx_osi_colorKey(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                GFX_SURFACE_T *pSrc, INT nSrcX, INT nSrcY,
                COLOR_KEY_SELECT *pColorKeySelect, ALPHA_SELECT *pAlphaSelect);    


// no clipping support currently
INT gfx_osi_resize(GFX_SURFACE_T *pDes, UINT uDesX, UINT uDesY, UINT uDesWidth, UINT uDesHeight,
            GFX_SURFACE_T *pSrc, UINT uSrcX, UINT uSrcY, UINT uSrcWidth, UINT uSrcHeight,
            BYTE destAlpha, char enableGammaCorrection);


#endif  // _DRV_INCLUDE_OSD_OSD_OSI_H_INC_
