//vulcan/drv/gfx/gfx_osi_engine.h
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
//  Software implementation of basic 2D graphics engine
//Revision Log:   
//  Jun/18/2002                                                  Created by YYD

#ifndef  _DRV_INCLUDE_GFX_GFX_OSI_ENGINE_H_INC_
#define  _DRV_INCLUDE_GFX_GFX_OSI_ENGINE_H_INC_

#include "os/os-types.h"
#include "gfx/gfx_common.h"

GFX_PALETTE_T *__gfx_osi_g2d_select_palette(GFX_SURFACE_T *pSrc, GFX_SURFACE_T *pDes, GFX_PALETTE_T *pTempPal);

void __gfx_osi_g2d_fillblt8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, UINT uFill);

void __gfx_osi_g2d_fillblt32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, UINT32 uFill);

void __gfx_osi_g2d_fillblt1(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, UINT uFill);

void __gfx_osi_g2d_fillblt_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, UINT uFill);

void __gfx_osi_g2d_maskflt8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uForeGround);

void __gfx_osi_g2d_maskflt32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT32 uForeGround);

void __gfx_osi_g2d_maskflt_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uForeGround);

void __gfx_osi_g2d_bitblt8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY);

void __gfx_osi_g2d_bitblt_clut_8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, GFX_PALETTE_T *pPal, UINT uColor);

void __gfx_osi_g2d_bitblt_clut_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, GFX_PALETTE_T *pPal);

void __gfx_osi_g2d_bitblt_1_8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uForeGround, UINT uBackGround);

void __gfx_osi_g2d_bitblt_1_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT32 uForeGround, UINT32 uBackGround);

void __gfx_osi_g2d_bitblt_1_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uForeGround, UINT uBackGround);

void __gfx_osi_g2d_bitblt_clut_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, GFX_PALETTE_T *pPal);

void __gfx_osi_g2d_bitblt_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY);

void __gfx_osi_g2d_bitblt_yuv422_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrcy, BYTE *psrcuv, BYTE *psrca, INT bplsrc, INT nSrcX,  INT nSrcY);

void __gfx_osi_g2d_bitblt_yuv420_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrcy, BYTE *psrcuv, BYTE *psrca, INT bplsrc, INT nSrcX,  INT nSrcY);

void __gfx_osi_g2d_bitblt_32_yuv422(BYTE *pdesy, BYTE *pdesuv, BYTE *pdesa,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY);

void __gfx_osi_g2d_bitblt_32_yuv420(BYTE *pdesy, BYTE *pdesuv, BYTE *pdesa,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY);

void __gfx_osi_g2d_blend32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY,
                BYTE *palpha, INT bplalpha,  INT nPixIncA, BYTE *pdesta, INT bpldesta,  INT nPixIncDestA);

void __gfx_osi_g2d_blend32_a(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY,
                BYTE *palpha, INT bplalpha,  INT nPixIncA);

void __gfx_osi_g2d_resize_nn8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight);

void __gfx_osi_g2d_resize_bl8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight);

void __gfx_osi_g2d_resize_clut_8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, GFX_PALETTE_T *pPal, UINT uColor);

void __gfx_osi_g2d_resize_clut_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, GFX_PALETTE_T *pPal);

void __gfx_osi_g2d_resize_clut_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, GFX_PALETTE_T *pPal);

void __gfx_osi_g2d_resize_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight);

void __gfx_osi_g2d_resize_32(BYTE *pdes, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight);

void __gfx_osi_g2d_resize_32_yuv422(BYTE *pdesy, BYTE *pdesuv, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight);

void __gfx_osi_g2d_resize_32_yuv420(BYTE *pdesy, BYTE *pdesuv, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight);

void __gfx_osi_g2d_resize_yuv422_32(BYTE *pdes, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrcy, BYTE *psrcuv, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, BYTE a);

void __gfx_osi_g2d_resize_yuv420_32(BYTE *pdes, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrcy, BYTE *psrcuv, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, BYTE a);














#endif  // _DRV_INCLUDE_GFX_OSI_ENGINE_H_INC_
