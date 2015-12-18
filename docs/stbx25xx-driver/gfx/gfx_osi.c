//vulcan/drv/gfx/gfx_osi.c
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
//  Oct/29/2001                                                  Created by YYD
//  Jun/05/2002            Implemented basic gfx routines using software by YYD
#include "os/os-types.h"
#include "os/os-generic.h"
#include "gfx_surface.h"
#include "gfx_osi.h"
#include "gfx_osi_engine.h"
#include "hw/physical-mem.h"
#include "gfx_osi_local.h"
#include "os/pm-alloc.h"
#include <asm/io.h>
#include <linux/ioport.h>

//#define __G2D_FAST_RESIZE

//#define  __GFX_OSI_DEBUG
 
#ifdef __GFX_OSI_DEBUG
    #define __DRV_DEBUG
#endif
#include "os/drv_debug.h"


#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif


INT gfx_osi_init(void)
{
    PDEBUG("gfx_osi_init:  __STB_GRAPHICS_MEM_SIZE = 0x%0x \n",__STB_GRAPHICS_MEM_SIZE);
    
      
    if(__STB_GRAPHICS_MEM_SIZE>0)
    {
       void *pmapped;
    
       if (gpGraphicsPMRoot) // it should have been initialized, so error
       {
           return PM_ALLOC_ERROR;    
       }

       PDEBUG("\nGraphics physical mem at 0x%8.8x , size 0x%8.8x\n",  __STB_GRAPHICS_MEM_BASE_ADDR,  __STB_GRAPHICS_MEM_SIZE);

       if ((__STB_GRAPHICS_MEM_BASE_ADDR + __STB_GRAPHICS_MEM_SIZE) < __STB_GRAPHICS_MEM_BASE_ADDR) // how can we wrap around ?!
          return PM_ALLOC_INVALID_PARM;

       if (!request_region(__STB_GRAPHICS_MEM_BASE_ADDR, __STB_GRAPHICS_MEM_SIZE, __PM_MODULE_NAME))
       {
          printk("ERROR: failed to lock graphics memory\n");
          return PM_ALLOC_ERROR;    // hi, I cann't do this
       }

       pmapped = (void *)ioremap(__STB_GRAPHICS_MEM_BASE_ADDR, __STB_GRAPHICS_MEM_SIZE);


       if (NULL == pmapped)
       {
          release_region(__STB_GRAPHICS_MEM_BASE_ADDR, __STB_GRAPHICS_MEM_SIZE);
          return PM_ALLOC_ERROR;    // hi, I cann't do this
       }

       guGraphicsPhysicalAddr = __STB_GRAPHICS_MEM_BASE_ADDR;
       guGraphicsPhysicalSize = __STB_GRAPHICS_MEM_SIZE;
       guGraphicsLogicalAddr = pmapped;
       guGraphicsPhysicalBase = __STB_GRAPHICS_MEM_BASE_ADDR;

       gpGraphicsPMRoot = __pm_alloc_physical_init(guGraphicsPhysicalAddr, pmapped, guGraphicsPhysicalSize,4096);
    
       PDEBUG("gfx_osi_init gpGraphicsPMRoot = %0x\n", gpGraphicsPMRoot);

       if(!gpGraphicsPMRoot)
       {
          // failed
          iounmap(guGraphicsLogicalAddr);
          release_region(guGraphicsPhysicalAddr, guGraphicsPhysicalSize);
          return PM_ALLOC_ERROR;
       }
       else
       {
          return PM_ALLOC_SUCCESS;
       }  
       
       
    }    
    
  return 0;
}

INT gfx_osi_deinit(void)
{
    
    if(__STB_GRAPHICS_MEM_SIZE>0)
    {
       if (!gpGraphicsPMRoot)  
       {
           PDEBUG("DEINIT: Tries to deinit before successful init !\n");
           return -1;        // you are going to deallocate an non existing one
       }

       __pm_alloc_physical_deinit(gpGraphicsPMRoot);

       gpGraphicsPMRoot = NULL;

       iounmap(guGraphicsLogicalAddr);
       release_region(guGraphicsPhysicalAddr, guGraphicsPhysicalSize);
       guGraphicsPhysicalAddr = 0;
       guGraphicsPhysicalSize = 0;
       guGraphicsLogicalAddr = NULL;
    }       
    return 0;

}



INT gfx_osi_wait_for_engine(INT nTimeout)
{
    return 0;
}



INT gfx_osi_run_engine(INT  nWait)
{
    return 0;
}


INT gfx_osi_reset_engine(void)
{
    return 0;     
}



// rect clip of BLT routines
// return  0 if clip result is not zero
//        -1 if clip result is zero
INT gfx_osi_ClipBLTRect(UINT uSrcWidth, UINT uSrcHeight, 
                                UINT uDesWidth, UINT uDesHeight, 
                                UINT uSrcX,     UINT uSrcY,
                                UINT uDesX,     UINT uDesY,
                                UINT *uWidth,   UINT *uHeight
                                )
{
    if(   !*uWidth || !*uHeight
       || uSrcX >= uSrcWidth || uSrcY >= uSrcHeight
       || uDesX >= uDesWidth || uDesY >= uDesWidth )
    {
        return -1;  // empty clip
    }

    if(uSrcX + *uWidth > uSrcWidth)
    {
        *uWidth = uSrcWidth - uSrcX;
    }
    if(uDesX + *uWidth > uDesWidth)
    {
        *uWidth = uDesWidth - uDesX;
    }

    if(uSrcY + *uHeight > uSrcHeight)
    {
        *uHeight = uSrcHeight - uSrcY;
    }
    if(uDesY + *uHeight > uDesHeight)
    {
        *uHeight = uDesHeight - uDesY;
    }

    return  (*uHeight)&&(*uWidth) ? 0 : -1;
}

// rect clip of BLT routines
// return  0 if clip result is not zero
//        -1 if clip result is zero
INT gfx_osi_ClipRect(UINT uDesWidth, UINT uDesHeight, 
                                UINT uDesX,     UINT uDesY,
                                UINT *uWidth,   UINT *uHeight
                                )
{
    if(   !*uWidth || !*uHeight
       || uDesX >= uDesWidth || uDesY >= uDesWidth )
    {
        return -1;  // empty clip
    }

    if(uDesX + *uWidth > uDesWidth)
    {
        *uWidth = uDesWidth - uDesX;
    }

    if(uDesY + *uHeight > uDesHeight)
    {
        *uHeight = uDesHeight - uDesY;
    }

    return  (*uHeight)&&(*uWidth) ? 0 : -1;
}


INT gfx_osi_ClipExtRect(GFX_RECT_T *pDesClip, 
                                INT *nDesX,    INT *nDesY,
                                UINT *uWidth,  UINT *uHeight
                                )
{
    register INT v1, v2;
    if(!*uWidth || !*uHeight)
    {
        return -1;  // empty clip
    }
    v1 = *nDesX;
    v2 = v1 + (INT)*uWidth-1;
    if(pDesClip->x1 > v1) v1 = pDesClip->x1;
    if(pDesClip->x2 < v2) v2 = pDesClip->x2;
    if(v1 > v2) // no overlap
    {
        return -1;
    }
    *nDesX = v1;
    *uWidth = v2-v1+1;

    v1 = *nDesY;
    v2 = v1 + (INT)*uHeight-1;
    if(pDesClip->y1 > v1) v1 = pDesClip->y1;
    if(pDesClip->y2 < v2) v2 = pDesClip->y2;
    if(v1 > v2) // no overlap
    {
        return -1;
    }
    *nDesY = v1;
    *uHeight = v2-v1+1;

    return 0;
}



INT gfx_osi_ClipExtBLTRect(GFX_RECT_T *pSrcClip,
                              GFX_RECT_T *pDesClip, 
                                INT *nSrcX,     INT *nSrcY,
                                INT *nDesX,     INT *nDesY,
                                UINT *uWidth,   UINT *uHeight,
                                UINT *uAdjustX, UINT *uAdjustY
                                )
{
    INT x1, y1, xadj, yadj;

    // forward clip
    x1 = *nSrcX;    // save for checking coordination move
    y1 = *nSrcY;
    if(gfx_osi_ClipExtRect(pSrcClip, nSrcX, nSrcY, uWidth, uHeight))
        return -1;      // no overlap
    xadj = *nSrcX - x1;
    yadj = *nSrcY - y1;
    *nDesX += xadj;  // we need to move dest coordination too
    *nDesY += yadj;

    // backward clip
    x1 = *nDesX;    // save for checking coordination move
    y1 = *nDesY;
    if(gfx_osi_ClipExtRect(pDesClip, nDesX, nDesY, uWidth, uHeight))
        return -1;      // no overlap
    *nSrcX += *nDesX - x1;  // we need to move src coordination too
    *nSrcY += *nDesY - y1;

    if(uAdjustX) *uAdjustX = xadj + *nDesX - x1;
    if(uAdjustY) *uAdjustY = yadj + *nDesY - y1;

    return 0;
}


void gfx_osi_pixelJustify(UINT uJustify, UINT *uX, UINT *uWidth)
{
    if(uJustify < 2) return;
    if(*uX % uJustify)
        *uX += uJustify - *uX % uJustify;
    
    if(*uWidth % uJustify)
        *uWidth += uJustify - *uWidth % uJustify;
}



INT gfx_osi_set_surface_clip_region(GFX_SURFACE_T *pSurface, GFX_RECT_T *pClip)
{
    if(!gfx_osi_pSurface_valid(pSurface))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }
    if(!pClip)  // reset to default
    {
        pSurface->clip.x1 = pSurface->clip.y1 = 0;
        pSurface->clip.x2 = pSurface->plane[0].uAllocWidth - 1;
        pSurface->clip.y2 = pSurface->plane[0].uAllocHeight - 1;
    }
    else
    {
        GFX_RECT_T clip = *pClip;

        *pClip = pSurface->clip;

        if(clip.x1 > clip.x2 || clip.x1 >= (INT)pSurface->plane[0].uAllocWidth) return -1;
        if(clip.y1 > clip.y2 || clip.y1 >= (INT)pSurface->plane[0].uAllocHeight) return -1;
        if(clip.x2 < 0 || clip.y2 < 0) return -1;

        if(clip.x1 < 0) clip.x1 = 0;
        if(clip.y1 < 0) clip.y1 = 0;
        if(clip.x2 >= (INT)pSurface->plane[0].uAllocWidth) clip.x2 = (INT)pSurface->plane[0].uAllocWidth-1;
        if(clip.y2 >= (INT)pSurface->plane[0].uAllocHeight) clip.y2 = (INT)pSurface->plane[0].uAllocHeight-1;

        gfx_osi_pixelJustify(pSurface->plane[0].uPixelJustify, (UINT *)&clip.x1, &clip.x2);

        pSurface->clip = clip;
    }
    return 0;
}




INT gfx_osi_bitBLT(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                GFX_SURFACE_T *pSrc,  INT nSrcX,  INT nSrcY,
                ALPHA_SELECT *pAlphaSelect, char enableGammaCorrection)
{
    INT rtn = 0;

    if(!gfx_osi_pSurface_valid(pSrc) || !gfx_osi_pSurface_valid(pDes) ||
       !IS_SURFACE_G2D_COMP(pSrc->uPlaneConfig) || !IS_SURFACE_G2D_COMP(pDes->uPlaneConfig))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }
    gfx_osi_pixelJustify(pSrc->plane[0].uPixelJustify, (UINT *)&nSrcX, &uWidth);
    gfx_osi_pixelJustify(pDes->plane[0].uPixelJustify, (UINT *)&nDesX, &uWidth);

    if(gfx_osi_ClipExtBLTRect(&pSrc->bound, &pDes->clip, 
                            &nSrcX, &nSrcY, &nDesX, &nDesY,  &uWidth, &uHeight, NULL, NULL))
    {
        PDEBUG("No overlapping rect is found!\n");
        return 0;       // just done
    }


    {
        if(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig) == G2D_CLUT1)
        {
            GFX_PALETTE_T yuvpal[2];
            GFX_PALETTE_T *pPal;
            pPal = __gfx_osi_g2d_select_palette(pSrc, pDes, yuvpal);
            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_CLUT8:
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, 1, 0);
                break;
            case G2D_AYCBCR422:
                if(!pAlphaSelect || GFX_DEST_ALPHA_FROM_SOURCE == pAlphaSelect->storedAlphaSelect)
                    __gfx_osi_g2d_bitblt_1_8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal[1].a, pPal[0].a);  // a
                else if(GFX_DEST_ALPHA_FROM_GIVEN == pAlphaSelect->storedAlphaSelect)
                    __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pAlphaSelect->globalAlphaValue);  // a
            case G2D_YCBCR422:
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal[1].r, pPal[0].r);  // y
                __gfx_osi_g2d_bitblt_1_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, ((UINT)pPal[1].g<<8) | pPal[1].b, ((UINT)pPal[0].g<<8) | pPal[0].b);  // uv
                break;
            case G2D_AYCBCR420:
                if(!pAlphaSelect || GFX_DEST_ALPHA_FROM_SOURCE == pAlphaSelect->storedAlphaSelect)
                    __gfx_osi_g2d_bitblt_1_8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal[1].a, pPal[0].a);  // a
                else if(GFX_DEST_ALPHA_FROM_GIVEN == pAlphaSelect->storedAlphaSelect)
                    __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pAlphaSelect->globalAlphaValue);  // a
            case G2D_YCBCR420:
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal[1].r, pPal[0].r);  // y
                __gfx_osi_g2d_bitblt_1_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY/2,
                    uWidth, uHeight/2, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine*2, nSrcX, nSrcY/2, ((UINT)pPal[1].g<<8) | pPal[1].b, ((UINT)pPal[0].g<<8) | pPal[0].b);  // uv
                break;
            case G2D_ARGB_8888:
                __gfx_osi_g2d_bitblt_1_32(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, *(UINT32 *)&pPal[1], *(UINT32 *)&pPal[0]);
            default:
                rtn = -1;
                break;
            }
        }
        else if(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig) == G2D_CLUT8)
        {
            GFX_PALETTE_T yuvpal[256];
            GFX_PALETTE_T *pPal;
            pPal = __gfx_osi_g2d_select_palette(pSrc, pDes, yuvpal);

            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_CLUT8:
                __gfx_osi_g2d_bitblt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                break;
            case G2D_AYCBCR422:
                if(!pAlphaSelect || GFX_DEST_ALPHA_FROM_SOURCE == pAlphaSelect->storedAlphaSelect)
                    __gfx_osi_g2d_bitblt_clut_8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal, 0);  // a
                else if(GFX_DEST_ALPHA_FROM_GIVEN == pAlphaSelect->storedAlphaSelect)
                    __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pAlphaSelect->globalAlphaValue);  // a
            case G2D_YCBCR422:
                __gfx_osi_g2d_bitblt_clut_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal, 1);  // y
                __gfx_osi_g2d_bitblt_clut_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal);  // uv
                break;
            case G2D_AYCBCR420:
                if(!pAlphaSelect || GFX_DEST_ALPHA_FROM_SOURCE == pAlphaSelect->storedAlphaSelect)
                    __gfx_osi_g2d_bitblt_clut_8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal, 0);  // a
                else if(GFX_DEST_ALPHA_FROM_GIVEN == pAlphaSelect->storedAlphaSelect)
                    __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pAlphaSelect->globalAlphaValue);  // a
            case G2D_YCBCR420:
                __gfx_osi_g2d_bitblt_clut_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal, 1);  // y
                __gfx_osi_g2d_bitblt_clut_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY/2,
                    uWidth, uHeight/2, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine*2, nSrcX, nSrcY/2, pPal);  // uv
                break;
            case G2D_ARGB_8888:
                __gfx_osi_g2d_bitblt_clut_32(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY, pPal);  // y
                break;
            default:
                rtn = -1;
                break;
            }
        }
        else if(IS_GFX_SURFACE_CLUT(pDes->uPlaneConfig))
        {
            rtn = -1;
        }
        else    // we all are direct color modes
        {
            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_YCBCR422:      // des is 422
            case G2D_AYCBCR422:     
                switch(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig))
                {
                case G2D_YCBCR422:      // src is 422
                case G2D_AYCBCR422:     
                    // y first
                    __gfx_osi_g2d_bitblt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                    // uv
                    __gfx_osi_g2d_bitblt8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine, nSrcX, nSrcY);
                    break;
                case G2D_YCBCR420:      // src is 420
                case G2D_AYCBCR420:     // we should expand to 422
                    // y first
                    __gfx_osi_g2d_bitblt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                    // uv
                    // even lines
                    __gfx_osi_g2d_bitblt8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine*2, nDesX, nDesY/2,
                        uWidth, uHeight/2, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine, nSrcX, nSrcY/2);
                    // odd lines
                    __gfx_osi_g2d_bitblt8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine*2, nDesX + pDes->plane[1].uBytePerLine, nDesY/2,
                        uWidth, uHeight/2, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine, nSrcX, nSrcY/2);
                    break;
                case G2D_ARGB_8888: // src is argb8888
                    __gfx_osi_g2d_bitblt_32_yuv422(pDes->plane[0].pBuffer, pDes->plane[1].pBuffer, pDes->plane[2].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                    break;
                default:
                    rtn = -1;
                }
                break;

            case G2D_YCBCR420:          // des is 420
            case G2D_AYCBCR420:
                switch(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig))
                {
                case G2D_YCBCR422:      // src is 422
                case G2D_AYCBCR422:     // we need drop odd lines
                    // y first
                    __gfx_osi_g2d_bitblt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                    // uv
                    __gfx_osi_g2d_bitblt8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY/2,
                        uWidth, uHeight/2, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine*2, nSrcX, nSrcY/2);
                    break;
                case G2D_YCBCR420:      // src is 420
                case G2D_AYCBCR420:     
                    // y first
                    __gfx_osi_g2d_bitblt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                    // uv
                    __gfx_osi_g2d_bitblt8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY/2,
                        uWidth, uHeight/2, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine, nSrcX, nSrcY/2);
                    break;
                case G2D_ARGB_8888: // src is argb8888
                    __gfx_osi_g2d_bitblt_32_yuv420(pDes->plane[0].pBuffer, pDes->plane[1].pBuffer, pDes->plane[2].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                default:
                    rtn = -1;
                }
                break;

            case G2D_ARGB_8888:     // des is rgb 8888
                switch(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig))
                {
                case G2D_YCBCR422:      // src is 422
                case G2D_AYCBCR422:     // we need drop odd lines
                    __gfx_osi_g2d_bitblt_yuv422_32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[1].pBuffer, pSrc->plane[2].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                    break;
                case G2D_YCBCR420:      // src is 420
                case G2D_AYCBCR420:     
                    __gfx_osi_g2d_bitblt_yuv420_32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[1].pBuffer, pSrc->plane[2].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                    break;
                case G2D_ARGB_8888: // src is argb8888
                    __gfx_osi_g2d_bitblt_32(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY);
                    break;
                default:
                    rtn = -1;
                }
                break;
            default:
                rtn = -1;
            }

            // a for yuv colors
            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_AYCBCR422:     // dest has alpha
            case G2D_AYCBCR420:
                switch(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig))
                {
                case G2D_YCBCR422:      // src has no alpha
                case G2D_YCBCR420:
                    if(!pAlphaSelect || GFX_DEST_ALPHA_FROM_SOURCE == pAlphaSelect->storedAlphaSelect)
                        __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                          uWidth, uHeight, 255);  // a
                    else if(pAlphaSelect && GFX_DEST_ALPHA_FROM_GIVEN == pAlphaSelect->storedAlphaSelect)
                        __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                          uWidth, uHeight, pAlphaSelect->globalAlphaValue);  // a
                    break;
                case G2D_AYCBCR422:     // src has alpha
                case G2D_AYCBCR420:     
                    if(!pAlphaSelect || GFX_DEST_ALPHA_FROM_SOURCE == pAlphaSelect->storedAlphaSelect)
                        __gfx_osi_g2d_bitblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                          uWidth, uHeight, pSrc->plane[2].pBuffer, pSrc->plane[2].uBytePerLine, nSrcX, nSrcY);
                    else if(GFX_DEST_ALPHA_FROM_GIVEN == pAlphaSelect->storedAlphaSelect)
                        __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                        uWidth, uHeight, pAlphaSelect->globalAlphaValue);  // a
                    break;
                }
                break;
            default: // no alpha at all
                break;
            }
        }
    }

    return rtn;
}


INT gfx_osi_advancedBitBLT(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight,
            GFX_SURFACE_T *pSrc, INT nSrcX, INT nSrcY,
            GFX_SURFACE_T  *pMask, INT nMaskX, INT nMaskY,
            GFX_ROP_CODE_T ROP,
            char enablePixelBitMask, UINT32 uPixelBitMask,
            ALPHA_SELECT *pAlphaSelect)
{
    INT rtn, xadj, yadj;

    if(!gfx_osi_pSurface_valid(pSrc) || !gfx_osi_pSurface_valid(pDes))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }

#if 0   // fixme: CLUT1 is not here
    if(gfx_osi_pSurface_valid(pMask) && GET_GFX_SURFACE_DATA_TYPE(pMask->uPlaneConfig) !=  G2D_CLUT1)
    {
        PDEBUG("invalid mask surface!\n");
        return -1;
    }
#endif

    gfx_osi_pixelJustify(pSrc->plane[0].uPixelJustify, (UINT *)&nSrcX, &uWidth);    gfx_osi_pixelJustify(pDes->plane[0].uPixelJustify, (UINT *)&nDesX, &uWidth);

    if(gfx_osi_ClipExtBLTRect(&pSrc->bound, &pDes->clip, 
                            &nSrcX, &nSrcY, &nDesX, &nDesY,  &uWidth, &uHeight, &xadj, &yadj))
    {
        PDEBUG("No overlapping rect is found!\n");
        return 0;       // just done
    }
 
    // Fixme:  I don't know how to deal with too small a mask by default
    // So I assume we clip also by mask bound
    if(pMask)
    {
        UINT uMaskWidth, uMaskHeight;
        INT nx, ny;
        uMaskWidth = uWidth;
        uMaskHeight = uHeight;
        nMaskX += xadj;
        nMaskY += yadj;
        nx = nMaskX;
        ny = nMaskY;
        // check for pattern area
        if(gfx_osi_ClipExtRect(&pMask->bound,
            &nMaskX, &nMaskY, &uMaskWidth, &uMaskHeight))
        {
            PDEBUG("No overlapping mask rect is found!\n");
            return -1;
        }
        else if(uMaskWidth < uWidth || uMaskHeight < uHeight)
        {
            uWidth = uMaskWidth;
            uHeight = uMaskHeight;
            nx = nMaskX - nx;
            ny = nMaskY - ny;
            nSrcX += nx; 
            nDesX += nx; 
            nSrcY += ny; 
            nDesY += ny; 
        }
    }
   
    rtn = -1;

    return rtn;
}


INT gfx_osi_fillBLT(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight, UINT32 uFillColor)
{
    INT rtn = 0;

    if(!gfx_osi_pSurface_valid(pDes))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }

    gfx_osi_pixelJustify(pDes->plane[0].uPixelJustify, (UINT *)&nDesX, &uWidth);

    if(gfx_osi_ClipExtRect(&pDes->clip, 
                            &nDesX, &nDesY, &uWidth, &uHeight))
    {
        PDEBUG("No in-clip rect is found!\n");
        return 0;       // that's it
    }

    if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_CLUT1)
    {
        PDEBUG("CLUT1\n");
        __gfx_osi_g2d_fillblt1(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
            uWidth, uHeight, uFillColor);
    }
    else if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_CLUT8)
    {
        PDEBUG("CLUT8\n");
        __gfx_osi_g2d_fillblt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
            uWidth, uHeight, uFillColor);
    }
    else if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_ARGB_8888)
    {
        PDEBUG("ARGB8888\n");
        __gfx_osi_g2d_fillblt32(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
            uWidth, uHeight, uFillColor);
    }
    else 
    {
        BYTE y,u,v;
        gfx_osi_rgb2ycbcr((uFillColor>>16)&0xff, (uFillColor>>8)&0xff, (uFillColor)&0xff,  &y, &u, &v);
        PDEBUG("Color = Y %02x, U %02x, V %02x\n",  y,u,v);
        switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
        {
        case G2D_AYCBCR422:     
            PDEBUG(" Alpha = %08x\n", pDes->plane[2].pBuffer);
            __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, uFillColor>>24);   // a
        case G2D_YCBCR422:
            PDEBUG(" Y = %08x\n", pDes->plane[0].pBuffer);
            __gfx_osi_g2d_fillblt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, y);
            PDEBUG(" UV = %08x\n", pDes->plane[1].pBuffer);
            __gfx_osi_g2d_fillblt_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, ((UINT)u<<8)|v);
            break;
        case G2D_AYCBCR420:
            PDEBUG(" Alpha = %08x\n", pDes->plane[2].pBuffer);
            __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, uFillColor>>24);   // a
        case G2D_YCBCR420:          // des is 420
            PDEBUG(" Y = %08x\n", pDes->plane[0].pBuffer);
            __gfx_osi_g2d_fillblt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, y); // y
            PDEBUG(" UV = %08x\n", pDes->plane[1].pBuffer);
            __gfx_osi_g2d_fillblt_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY/2,
                uWidth, uHeight/2, ((UINT)u<<8)|v);
            break;
        default:
            rtn = -1;
             break;
        }
    }

    return rtn;
}


INT gfx_osi_advancedFillBLT(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight, UINT32 uFillColor, /* formatted in the dest format color if rgb, index if clut */
            GFX_SURFACE_T  *pMask, INT nMaskX, INT nMaskY,
            GFX_ROP_CODE_T ROP, char transparencyEnable,    // 
            UINT32 uBackGroundColor, char enablePixelBitMask, UINT32 uPixelBitMask)
{
    INT rtn = 0;

    if(!gfx_osi_pSurface_valid(pDes))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }

#if 0  //  fixme: CLUT1 is not here
    if(gfx_osi_pSurface_valid(pMask) && GET_GFX_SURFACE_DATA_TYPE(pMask->uPlaneConfig) !=  G2D_CLUT1)
    {
        PDEBUG("invalid mask surface!\n");
        return -1;
    }
#endif

    gfx_osi_pixelJustify(pDes->plane[0].uPixelJustify, (UINT *)&nDesX, &uWidth);

    if(pMask)
    {
        if(gfx_osi_ClipExtBLTRect(&pMask->bound, &pDes->clip, 
                                &nMaskX, &nMaskY, &nDesX, &nDesY,  &uWidth, &uHeight, NULL,NULL))
        {
            PDEBUG("No overlapping rect is found!\n");
            return 0;       // just done
        }
        PDEBUGE("MaskX = %d, Y = %d, W = %d, H= %d\n", nMaskX, nMaskY, uWidth, uHeight);
    }
    else    // no mask
    {
        if(gfx_osi_ClipExtRect(&pDes->clip, 
                                &nDesX, &nDesY, &uWidth, &uHeight))
        {
            return 0;       // that's it
        }
        return gfx_osi_fillBLT(pDes, nDesX, nDesY, uWidth, uHeight, uFillColor);
    }
    if(!transparencyEnable)
    {
        if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_CLUT1)
        {
            PDEBUG("CLUT1\n");
            return -1;
        }
        else if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_CLUT8)
        {
            PDEBUG("CLUT8\n");
            __gfx_osi_g2d_bitblt_1_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY,
                uFillColor, uBackGroundColor);
        }
        else if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_ARGB_8888)
        {
            PDEBUG("ARGB8888\n");
            __gfx_osi_g2d_bitblt_1_32(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY,
                uFillColor, uBackGroundColor);
        }
        else
        {
            BYTE y,u,v, yb, ub, vb;
            gfx_osi_rgb2ycbcr((uFillColor>>16)&0xff, (uFillColor>>8)&0xff, (uFillColor)&0xff,  &y, &u, &v);
            gfx_osi_rgb2ycbcr((uBackGroundColor>>16)&0xff, (uBackGroundColor>>8)&0xff, (uBackGroundColor)&0xff,  &yb, &ub, &vb);
            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_AYCBCR422:     
                PDEBUG(" Alpha = %08x\n", pDes->plane[2].pBuffer);
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, 
                    pMask->plane[0].uBytePerLine, nMaskX, nMaskY, 
                    (uFillColor>>24)&0xff, (uBackGroundColor>>24)&0xff);
            case G2D_YCBCR422:
                PDEBUG(" Y = %08x\n", pDes->plane[0].pBuffer);
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, 
                    pMask->plane[0].uBytePerLine, nMaskX, nMaskY,
                    y, yb);
                PDEBUG(" UV = %08x\n", pDes->plane[1].pBuffer);
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, 
                    pMask->plane[0].uBytePerLine, nMaskX, nMaskY,
                    ((UINT)u<<8) | v, ((UINT)ub<<8) | vb);
                break;
            case G2D_AYCBCR420:
                PDEBUG(" Alpha = %08x\n", pDes->plane[2].pBuffer);
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, 
                    pMask->plane[0].uBytePerLine, nMaskX, nMaskY,
                    (uFillColor>>24)&0xff, (uBackGroundColor>>24)&0xff);
            case G2D_YCBCR420:          // des is 420
                PDEBUG(" Y = %08x\n", pDes->plane[0].pBuffer);
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, 
                    pMask->plane[0].uBytePerLine, nMaskX, nMaskY,
                    y, yb);
                PDEBUG(" UV = %08x\n", pDes->plane[1].pBuffer);
                __gfx_osi_g2d_bitblt_1_8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY/2,
                    uWidth, uHeight/2, pMask->plane[0].pBuffer, 
                    pMask->plane[0].uBytePerLine*2, nMaskX, nMaskY/2,
                    ((UINT)u<<8) | v, ((UINT)ub<<8) | vb);
                break;
            default:
                rtn = -1;
                break;
            }
        }
    }
    else    // transparent
    {
        if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_CLUT1)
        {
            PDEBUG("CLUT1\n");
            return -1;
        }
        else if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_CLUT8)
        {
            PDEBUG("CLUT8\n");
            __gfx_osi_g2d_maskflt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY, uFillColor);
        }
        else if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_ARGB_8888)
        {
            PDEBUG("ARGB8888\n");
            __gfx_osi_g2d_maskflt32(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY, uFillColor);
        }
        else
        {
            BYTE y,u,v;
            gfx_osi_rgb2ycbcr((uFillColor>>16)&0xff, (uFillColor>>8)&0xff, (uFillColor)&0xff,  &y, &u, &v);
            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_AYCBCR422:     
                PDEBUG(" Alpha = %08x\n", pDes->plane[2].pBuffer);
                __gfx_osi_g2d_maskflt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY, 
                    (uFillColor>>24)&0xff);
            case G2D_YCBCR422:
                PDEBUG(" Y = %08x\n", pDes->plane[0].pBuffer);
                __gfx_osi_g2d_maskflt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY, 
                    y);
                PDEBUG(" UV = %08x\n", pDes->plane[1].pBuffer);
                __gfx_osi_g2d_maskflt8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY, 
                    ((UINT)u<<8) | v);
                break;
            case G2D_AYCBCR420:
                PDEBUG(" Alpha = %08x\n", pDes->plane[2].pBuffer);
                __gfx_osi_g2d_maskflt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY, 
                    (uFillColor>>24)&0xff);
            case G2D_YCBCR420:          // des is 420
                PDEBUG(" Y = %08x\n", pDes->plane[0].pBuffer);
                __gfx_osi_g2d_maskflt8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, nDesX, nDesY,
                    uWidth, uHeight, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine, nMaskX, nMaskY, 
                    y);
                PDEBUG(" UV = %08x\n", pDes->plane[1].pBuffer);
                __gfx_osi_g2d_maskflt8(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, nDesX, nDesY/2,
                    uWidth, uHeight/2, pMask->plane[0].pBuffer, pMask->plane[0].uBytePerLine*2, nMaskX, nMaskY/2, 
                    ((UINT)u<<8) | v);
                break;
            default:
                rtn = -1;
                break;
            }
        }
    }
    return rtn;
}



INT gfx_osi_blend(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                GFX_SURFACE_T *pSrc, INT nSrcX, INT nSrcY,
                ALPHA_BLEND_SELECT *pBlendSelect)
{
    INT rtn = 0;

    if(!gfx_osi_pSurface_valid(pSrc) || !gfx_osi_pSurface_valid(pDes))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }

    if(gfx_osi_ClipExtBLTRect(&pSrc->bound, &pDes->clip, 
                            &nSrcX, &nSrcY, &nDesX, &nDesY,  &uWidth, &uHeight, NULL, NULL))
    {
        PDEBUG("No overlapping rect is found!\n");
        return 0;       // just done
    }

    if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) != G2D_ARGB_8888 || GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig) != G2D_ARGB_8888)
    {
        rtn = -1;
    }
    else
    {
        BYTE *psrca;
        UINT bplsrca;
        UINT incsrca;
        ALPHA_BLEND_SELECT bs;

        if(!pBlendSelect)
        {
            pBlendSelect = &bs;
            bs.blendInputSelect = GFX_BLEND_ALPHA_FROM_SOURCE;
            bs.globalAlphaValue = 255;
            bs.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
        }

        switch(pBlendSelect->blendInputSelect)
        {
        case GFX_BLEND_ALPHA_FROM_GIVEN:
            psrca = &pBlendSelect->globalAlphaValue;
            bplsrca = 0;
            incsrca = 0;
            break;

        case GFX_BLEND_ALPHA_FROM_DESTINATION:
            psrca = pDes->plane[0].pBuffer + nDesY*pDes->plane[0].uBytePerLine + (nDesX<<2);
            bplsrca = pDes->plane[0].uBytePerLine;
            incsrca = 4;
            break;

        case GFX_BLEND_ALPHA_FROM_SOURCE:
        default:
            psrca = pSrc->plane[0].pBuffer + nSrcY*pSrc->plane[0].uBytePerLine + (nSrcX<<2);
            bplsrca = pSrc->plane[0].uBytePerLine;
            incsrca = 4;
            break;
        
        }

        switch(pBlendSelect->storedAlphaSelect)
        {
        case GFX_DEST_ALPHA_FROM_GIVEN:
            __gfx_osi_g2d_blend32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY, uWidth, uHeight,
                      pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY,
                      psrca, bplsrca, incsrca, &pBlendSelect->globalAlphaValue, 0, 0);
            break;
        case GFX_DEST_ALPHA_FROM_SOURCE:
            __gfx_osi_g2d_blend32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY, uWidth, uHeight,
                      pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY,
                      psrca, bplsrca, incsrca, 
                      pSrc->plane[0].pBuffer + nSrcY*pSrc->plane[0].uBytePerLine + (nSrcX<<2), pSrc->plane[0].uBytePerLine, 4);
            break;
        case GFX_DEST_ALPHA_FROM_DESTINATION:
            __gfx_osi_g2d_blend32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY, uWidth, uHeight,
                      pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY,
                      psrca, bplsrca, incsrca, 
                      pDes->plane[0].pBuffer + nDesY*pDes->plane[0].uBytePerLine + (nDesX<<2), pDes->plane[0].uBytePerLine, 4);
            break;
        case GFX_DEST_ALPHA_FROM_BLEND:
            __gfx_osi_g2d_blend32_a(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY, uWidth, uHeight,
                      pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY,
                      psrca, bplsrca, incsrca);
            break;
        }
    }

    return rtn;
}


INT gfx_osi_advancedBlend(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                GFX_SURFACE_T *pSrc, INT nSrcX, INT nSrcY,
                GFX_SURFACE_T *pAlpha, INT nAlphaX, INT nAlphaY,
                ALPHA_BLEND_SELECT *pBlendSelect)
{
    INT rtn = 0, xadj, yadj;

    if(!gfx_osi_pSurface_valid(pSrc) || !gfx_osi_pSurface_valid(pDes))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }

    if(pAlpha && (!gfx_osi_pSurface_alloc(pAlpha)
        || G2D_CLUT8 != GET_GFX_SURFACE_DATA_TYPE(pAlpha->uPlaneConfig)))
    {
        PDEBUG("invalid alpha surface!\n");
        return -1;
    }

    if(gfx_osi_ClipExtBLTRect(&pSrc->bound, &pDes->clip, 
                            &nSrcX, &nSrcY, &nDesX, &nDesY,  &uWidth, &uHeight, &xadj, &yadj))
    {
        PDEBUG("No overlapping rect is found!\n");
        return 0;       // just done
    }
 
    // Fixme:  I don't know how to deal with too small a mask by default
    // So I assume we clip also by mask bound
    if(pAlpha)
    {
        UINT uAlphaWidth, uAlphaHeight;
        INT nx, ny;
        uAlphaWidth = uWidth;
        uAlphaHeight = uHeight;
        nAlphaX += xadj;
        nAlphaY += yadj;
        nx = nAlphaX;
        ny = nAlphaY;
        // check for pattern area
        if(gfx_osi_ClipExtRect(&pAlpha->bound,
            &nAlphaX, &nAlphaY, &uAlphaWidth, &uAlphaHeight))
        {
            PDEBUG("No overlapping mask rect is found!\n");
            return -1;
        }
        else if(uAlphaWidth < uWidth || uAlphaHeight < uHeight)
        {
            uWidth = uAlphaWidth;
            uHeight = uAlphaHeight;
            nx = nAlphaX - nx;
            ny = nAlphaY - ny;
            nSrcX += nx; 
            nDesX += nx; 
            nSrcY += ny; 
            nDesY += ny; 
        }
    }

    if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) != G2D_ARGB_8888 || GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig) != G2D_ARGB_8888)
    {
        rtn = -1;
    }
    else
    {
        BYTE *psrca;
        UINT bplsrca;
        UINT incsrca;
        ALPHA_BLEND_SELECT bs;

        if(!pBlendSelect || 
            (pBlendSelect->blendInputSelect == GFX_BLEND_ALPHA_FROM_PATTERN && !pAlpha)) // no alpha given
        {
            pBlendSelect = &bs;
            bs.blendInputSelect = GFX_BLEND_ALPHA_FROM_SOURCE;
            bs.globalAlphaValue = 255;
            bs.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
            PDEBUG("Alpha mask is not used\n");
        }

        switch(pBlendSelect->blendInputSelect)
        {
        case GFX_BLEND_ALPHA_FROM_GIVEN:
            psrca = &pBlendSelect->globalAlphaValue;
            bplsrca = 0;
            incsrca = 0;
            break;

        case GFX_BLEND_ALPHA_FROM_DESTINATION:
            psrca = pDes->plane[0].pBuffer + nDesY*pDes->plane[0].uBytePerLine + (nDesX<<2);
            bplsrca = pDes->plane[0].uBytePerLine;
            incsrca = 4;
            break;

        case GFX_BLEND_ALPHA_FROM_PATTERN:
            psrca = pAlpha->plane[0].pBuffer + nAlphaY*pAlpha->plane[0].uBytePerLine + nAlphaX;
            bplsrca = pAlpha->plane[0].uBytePerLine;
            incsrca = 1;
            break;

        case GFX_BLEND_ALPHA_FROM_SOURCE:
        default:
            psrca = pSrc->plane[0].pBuffer + nSrcY*pSrc->plane[0].uBytePerLine + (nSrcX<<2);
            bplsrca = pSrc->plane[0].uBytePerLine;
            incsrca = 4;
            break;
        
        }

        PDEBUGE("Alpha p = %08x, bpl=%d, inc=%d\n", psrca, bplsrca, incsrca);

        switch(pBlendSelect->storedAlphaSelect)
        {
        case GFX_DEST_ALPHA_FROM_GIVEN:
            __gfx_osi_g2d_blend32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY, uWidth, uHeight,
                      pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY,
                      psrca, bplsrca, incsrca, &pBlendSelect->globalAlphaValue, 0, 0);
            break;
        case GFX_DEST_ALPHA_FROM_SOURCE:
            __gfx_osi_g2d_blend32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY, uWidth, uHeight,
                      pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY,
                      psrca, bplsrca, incsrca, 
                      pSrc->plane[0].pBuffer + nSrcY*pSrc->plane[0].uBytePerLine + (nSrcX<<2), pSrc->plane[0].uBytePerLine, 4);
            break;
        case GFX_DEST_ALPHA_FROM_DESTINATION:
            __gfx_osi_g2d_blend32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY, uWidth, uHeight,
                      pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY,
                      psrca, bplsrca, incsrca, 
                      pDes->plane[0].pBuffer + nDesY*pDes->plane[0].uBytePerLine + (nDesX<<2), pDes->plane[0].uBytePerLine, 4);
            break;
        case GFX_DEST_ALPHA_FROM_BLEND:
            __gfx_osi_g2d_blend32_a(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, nDesX, nDesY, uWidth, uHeight,
                      pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, nSrcX, nSrcY,
                      psrca, bplsrca, incsrca);
            break;
        }
    }

    return rtn;
}



INT gfx_osi_colorKey(GFX_SURFACE_T *pDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                GFX_SURFACE_T *pSrc, INT nSrcX, INT nSrcY,
                COLOR_KEY_SELECT *pColorKeySelect, ALPHA_SELECT *pAlphaSelect)
{
    INT rtn;

    if(!gfx_osi_pSurface_valid(pSrc) || !gfx_osi_pSurface_valid(pDes))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }
    if(!pColorKeySelect)
    {
        PDEBUG("Color key is not specified!\n");
        return -1;
    }

    if(gfx_osi_ClipExtBLTRect(&pSrc->bound, &pDes->clip, 
                            &nSrcX, &nSrcY, &nDesX, &nDesY,  &uWidth, &uHeight, NULL, NULL))
    {
        PDEBUG("No overlapping rect is found!\n");
        return 0;       // just done
    }

    rtn = -1;

    return rtn;
}



INT gfx_osi_resize(GFX_SURFACE_T *pDes, UINT uDesX, UINT uDesY, UINT uDesWidth, UINT uDesHeight,
            GFX_SURFACE_T *pSrc, UINT uSrcX, UINT uSrcY, UINT uSrcWidth, UINT uSrcHeight,
            BYTE destAlpha, char enableGammaCorrection)
{
    INT rtn = 0;

    if(!gfx_osi_pSurface_valid(pSrc) || !gfx_osi_pSurface_valid(pDes))
    {
        PDEBUG("invalid surface!\n");
        return -1;
    }

    gfx_osi_pixelJustify(pDes->plane[0].uPixelJustify, &uDesX, &uDesWidth);
    gfx_osi_pixelJustify(pSrc->plane[0].uPixelJustify, &uSrcX, &uSrcWidth);

    if(gfx_osi_ClipRect(pDes->plane[0].uAllocWidth, pDes->plane[0].uAllocHeight, 
                            uDesX, uDesY, &uDesWidth, &uDesHeight))
    {
        return 0;       // that's it
    }

    if(gfx_osi_ClipRect(pSrc->plane[0].uAllocWidth, pSrc->plane[0].uAllocHeight, 
                            uSrcX, uSrcY, &uSrcWidth, &uSrcHeight))
    {
        return 0;       // that's it
    }

    {
        if(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig) == G2D_CLUT8)
        {
            GFX_PALETTE_T yuvpal[256];
            GFX_PALETTE_T *pPal;
            pPal = __gfx_osi_g2d_select_palette(pSrc, pDes, yuvpal);
            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_CLUT8:
                __gfx_osi_g2d_resize_nn8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                    uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
                break;
            case G2D_AYCBCR422:
                __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, uDesX, uDesY,
                    uDesWidth, uDesHeight, destAlpha);  // a
            case G2D_YCBCR422:
                // y
                __gfx_osi_g2d_resize_clut_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                    uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight, pPal, 1);
                // uv
                __gfx_osi_g2d_resize_clut_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, uDesX, uDesY,
                    uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight, pPal);
                break;
            case G2D_AYCBCR420:
                __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, uDesX, uDesY,
                    uDesWidth, uDesHeight, destAlpha);  // a
            case G2D_YCBCR420:
                // y
                __gfx_osi_g2d_resize_clut_8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                    uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight, pPal, 1);
                // uv
                __gfx_osi_g2d_resize_clut_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, uDesX, uDesY/2,
                    uDesWidth, uDesHeight/2, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight, pPal);
                break;
            case G2D_ARGB_8888:
                __gfx_osi_g2d_resize_clut_32(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                    uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight, pPal);
                break;
            default:
                rtn = -1;
                break;
            }
        }
        else if(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) == G2D_CLUT8 || IS_GFX_SURFACE_CLUT(pSrc->uPlaneConfig))
        {
            rtn = -1;
        }
        else    // we all are direct color modes
        {
            // uv
            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_YCBCR422:      // des is 422
            case G2D_AYCBCR422:     
                switch(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig))
                {
                case G2D_YCBCR422:      // src is 422
                case G2D_AYCBCR422:     
                    // y first
#ifdef __G2D_FAST_RESIZE
                    __gfx_osi_g2d_resize_nn8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
#else
                    __gfx_osi_g2d_resize_bl8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
#endif
                    // uv
                    __gfx_osi_g2d_resize_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
                    break;
                case G2D_YCBCR420:      // src is 420
                case G2D_AYCBCR420:     // we should expand to 422
                    // y first
#ifdef __G2D_FAST_RESIZE
                    __gfx_osi_g2d_resize_nn8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
#else
                    __gfx_osi_g2d_resize_bl8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
#endif
                    // uv
                    __gfx_osi_g2d_resize_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine, uSrcX, uSrcY/2, uSrcWidth, uSrcHeight/2);
                    break;
                case G2D_ARGB_8888:
                    __gfx_osi_g2d_resize_32_yuv422(pDes->plane[0].pBuffer, pDes->plane[1].pBuffer, pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
                    break;
                default:
                    rtn = -1;
                }
                break;
            case G2D_YCBCR420:          // des is 420
            case G2D_AYCBCR420:
                switch(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig))
                {
                case G2D_YCBCR422:      // src is 422
                case G2D_AYCBCR422:     // we need drop odd lines
                    // y first
#ifdef __G2D_FAST_RESIZE
                    __gfx_osi_g2d_resize_nn8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
#else
                    __gfx_osi_g2d_resize_bl8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
#endif
                    // uv
                    __gfx_osi_g2d_resize_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, uDesX, uDesY/2,
                        uDesWidth, uDesHeight/2, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
                    break;
                case G2D_YCBCR420:      // src is 420
                case G2D_AYCBCR420:     
                    // y first
#ifdef __G2D_FAST_RESIZE
                    __gfx_osi_g2d_resize_nn8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
#else
                    __gfx_osi_g2d_resize_bl8(pDes->plane[0].pBuffer,  pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
#endif
                    // uv
                    __gfx_osi_g2d_resize_uv(pDes->plane[1].pBuffer,  pDes->plane[1].uBytePerLine, uDesX, uDesY/2,
                        uDesWidth, uDesHeight/2, pSrc->plane[1].pBuffer, pSrc->plane[1].uBytePerLine, uSrcX, uSrcY/2, uSrcWidth, uSrcHeight/2);
                    break;
                case G2D_ARGB_8888:
                    __gfx_osi_g2d_resize_32_yuv420(pDes->plane[0].pBuffer, pDes->plane[1].pBuffer, pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
                    break;
                default:
                    rtn = -1;
                }
                break;
            case G2D_ARGB_8888: // des is argb8888
                switch(GET_GFX_SURFACE_DATA_TYPE(pSrc->uPlaneConfig))
                {
                case G2D_YCBCR422:      // src is 422
                case G2D_AYCBCR422:     
                    __gfx_osi_g2d_resize_yuv422_32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[1].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight, destAlpha);
                    break;
                case G2D_YCBCR420:      // src is 420
                case G2D_AYCBCR420:     
                    __gfx_osi_g2d_resize_yuv420_32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[1].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight, destAlpha);
                    break;
                case G2D_ARGB_8888:
                    __gfx_osi_g2d_resize_32(pDes->plane[0].pBuffer, pDes->plane[0].uBytePerLine, uDesX, uDesY,
                        uDesWidth, uDesHeight, pSrc->plane[0].pBuffer, pSrc->plane[0].uBytePerLine, uSrcX, uSrcY, uSrcWidth, uSrcHeight);
                    break;
                default:
                    rtn = -1;
                }
                break;
            default:
                rtn = -1;
            }
            // a
            switch(GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig))
            {
            case G2D_AYCBCR422:     // dest has alpha
            case G2D_AYCBCR420:
                __gfx_osi_g2d_fillblt8(pDes->plane[2].pBuffer,  pDes->plane[2].uBytePerLine, uDesX, uDesY,
                    uDesWidth, uDesHeight, destAlpha);  // a
                break;
            default: // no alpha at all
                break;
            }
        }
    }

    return rtn;
}
