//vulcan/drv/gfx/test/gfxlib.c
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
//  Linux device driver interface of GFX controls 
//Revision Log:   
//  Nov/07/2001                                   Created by YYD
//  Jun/27/2002                 Adopted for Vulcan driver by YYD

#include <sys/ioctl.h>

#include "gfxlib.h"


int gfx_open(void)
{
    return open(GFX_DEV_NAME, O_RDWR);
}

int gfx_close(int fdGfxDev)
{
    return close(fdGfxDev);
}



INT gfx_ClipBLTRect(UINT uSrcWidth, UINT uSrcHeight, 
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



INT gfx_ClipRect(UINT uDesWidth, UINT uDesHeight, 
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



void gfx_pixelJustify(UINT uJustify, UINT *uX, UINT *uWidth)
{
    if(uJustify < 2) return;
    if(*uX % uJustify)
        *uX += uJustify - *uX % uJustify;
    
    if(*uWidth % uJustify)
        *uWidth += uJustify - *uWidth % uJustify;
}


unsigned int gfx_plane_pixel_color(unsigned int uPlaneConfig, unsigned char a, unsigned char r, unsigned char g, unsigned char b)
{
    switch(GET_GFX_SURFACE_DATA_TYPE(uPlaneConfig))
    {
        case G2D_CLUT1     :
#ifdef G2D_CLUT4
        case G2D_CLUT4     :
#endif
        case G2D_CLUT8     :
            return r;
#ifdef G2D_ARGB_1555
        case G2D_ARGB_1555 : 
            return GFX_ARGB1555_COLOR(a,r,g,b);
#endif
#ifdef G2D_ARGB_4444
        case G2D_ARGB_4444 : 
            return GFX_ARGB4444_COLOR(a,r,g,b);
#endif
#ifdef G2D_RGB_565
        case G2D_RGB_565   :
            return GFX_RGB565_COLOR(a,r,g,b);
#endif
        case G2D_ARGB_8888 :
#ifdef G2D_YCBCR
        case G2D_YCBCR     : /* 4:2:2, requires 2 buffers */ 
        case G2D_AYCBCR    : /* 4:2:2, requires 3 buffers */
#elif defined(G2D_YCBCR422) 
        case G2D_YCBCR422     : /* 4:2:2, requires 2 buffers */ 
        case G2D_AYCBCR422    : /* 4:2:2, requires 3 buffers */
        case G2D_YCBCR420     : /* 4:2:2, requires 2 buffers */ 
        case G2D_AYCBCR420    : /* 4:2:2, requires 3 buffers */
#endif
            return GFX_ARGB8888_COLOR(a,r,g,b);
        default:
            return GFX_ARGB8888_COLOR(a,r,g,b);
    }
}


void gfx_rgb2ycbcr(BYTE r, BYTE g,  BYTE b,  BYTE *y, BYTE *cb, BYTE *cr)
{
    // Y  =  0.257*R + 0.504*G + 0.098*B + 16
    // CB = -0.148*R - 0.291*G + 0.439*B + 128
    // CR =  0.439*R - 0.368*G - 0.071*B + 128
    *y  = (BYTE)((8432*(ULONG)r + 16425*(ULONG)g + 3176*(ULONG)b + 16*32768)>>15);
    *cb = (BYTE)((128*32768 + 14345*(ULONG)b - 4818*(ULONG)r -9527*(ULONG)g)>>15);
    *cr = (BYTE)((128*32768 + 14345*(ULONG)r - 12045*(ULONG)g-2300*(ULONG)b)>>15);
}

void gfx_ycbcr2rgb(BYTE y, BYTE cb,  BYTE cr,  BYTE *r, BYTE *g, BYTE *b)
{
    // R = 1.164*(Y - 16) + 1.596*(CR - 128)
    // G = 1.164*(Y - 16) - 0.813*(CR - 128) - 0.391*(CB - 128)
    // B = 1.164*(Y - 16)                    + 2.018*(CB - 128)

    // R = 1.164*Y + 1.596*CR - 222.4[1.164*16 + 1.592*128]
    // G = 1.164*Y + 135.488[0.813*128 + 0.391*128 - 1.164*16] - 0.813*CR - 0.391*CB
    // B = 1.164*Y + 2.018*CB - 276.928[1.164*16 + 2.018*128]
    int rr, gg, bb, my;

    my = 38142*(int)y;

    rr = (my + 52298*(int)cr - 7287603)>>15;

    if(rr > 255) *r = 255;
    else if(rr < 0) *r = 0;
    else *r = (BYTE)rr;

    gg = (my + 4439671 - 26640*cr - 12812*cb)>> 15;

    if(gg > 255) *g = 255;
    else if(gg < 0) *g = 0;
    else *g = (BYTE)gg;
    
    bb = (my + 66126*cb - 9074377)>>15;

    if(bb > 255) *b = 255;
    else if(bb < 0) *b = 0;
    else *b = (BYTE)bb;

}



int gfx_create_surface(int fdGfxDev, 
    UINT uWidth,        // width of buffer
    UINT uHeight,       // height of buffer
    UINT uPlaneConfig,  // plane config
    GFX_VISUAL_DEVICE_ID_T graphDev,    // which device to be used
    UINT32  uFillColor     // which color to fill
   )
{
    GFX_CREATE_SURFACE_PARM_T parm;

    parm.graphDev = graphDev;
    parm.uFillColor = uFillColor;
    parm.uWidth = uWidth;
    parm.uHeight =  uHeight;
    parm.uPlaneConfig = uPlaneConfig;

    if(ioctl(fdGfxDev, IOC_GFX_CREATE_SURFACE, &parm) >= 0)
    {
        return parm.hSurface;
    }
    return -1;
}


int gfx_destroy_surface(int fdGfxDev, int hSurface)
{
    return ioctl(fdGfxDev, IOC_GFX_DESTROY_SURFACE, hSurface);
}


int gfx_get_subplane_pseudo_surface(int fdGfxDev, int hSourceSurface, unsigned int uPlaneID, unsigned int uPlaneConfig, unsigned int uExpectWidth)
{
    GFX_GET_SUBPLANE_PSEUDO_SURFACE_PARM_T parm;

    parm.hSourceSurface = hSourceSurface;
    parm.uPlaneConfig = uPlaneConfig;
    parm.uPlaneID = uPlaneID;
    parm.uExpectWidth = uExpectWidth;

    if(ioctl(fdGfxDev, IOC_GFX_GET_SUBPLANE_PSEUDO_SURFACE, &parm) >= 0)
        return parm.hSurface;
    return -1;
}



int gfx_get_surface_info(int fdGfxDev, int hSurface, GFX_SURFACE_INFO_T *pParm)
{
    if(!pParm) return -1;
    
    memset(pParm, 0, sizeof(GFX_SURFACE_INFO_T));

    pParm->hSurface = hSurface;

    return ioctl(fdGfxDev, IOC_GFX_GET_SURFACE_INFO, pParm);
}


int gfx_lock_surface(int fdGfxDev, int hSurface, GFX_SURFACE_LOCK_INFO_T *pParm)
{
    GFX_SURFACE_INFO_T parm;
    int rtn, i;
    
    memset(&parm, 0, sizeof(parm));

    parm.hSurface = hSurface;

    rtn = ioctl(fdGfxDev, IOC_GFX_LOCK_SURFACE, &parm);
    if( rtn < 0)
    {
        return rtn;
    }

    memset(pParm, 0, sizeof(*pParm));

    for(i=0; i<GET_GFX_SURFACE_SUBPLANES(parm.uPlaneConfig); i++)
    {
        pParm->plane[i].pPlane = mmap(0, parm.plane[i].plane.uSize, 
            PROT_WRITE|PROT_READ, MAP_SHARED,  fdGfxDev, parm.plane[i].plane.uBase);
        if(MAP_FAILED == pParm->plane[i].pPlane)
            break;
    }
    if(i<GET_GFX_SURFACE_SUBPLANES(parm.uPlaneConfig))  // failed
    {
        for(i--;i>=0; i--)
        {
            munmap(pParm->plane[i].pPlane, parm.plane[i].plane.uSize);
        }
        ioctl(fdGfxDev, IOC_GFX_UNLOCK_SURFACE, hSurface);
        return -1;
    }

    pParm->uPlaneConfig = parm.uPlaneConfig;

    for(i=0; i<GET_GFX_SURFACE_SUBPLANES(parm.uPlaneConfig); i++)
    {
        // printf("mapped from 0x%08x to 0x%08x\n", parm.plane[i].plane.uBase, (UINT)pParm->plane[i].pPlane);
        pParm->plane[i].pPlane += parm.plane[i].plane.uOffset;
        pParm->plane[i].plane = parm.plane[i].plane;
        pParm->plane[i].uWidth = parm.plane[i].uWidth;
        pParm->plane[i].uHeight = parm.plane[i].uHeight;
        pParm->plane[i].uPixelSize = parm.plane[i].uPixelSize;
        pParm->plane[i].uPixelJustify = parm.plane[i].uPixelJustify;
        pParm->plane[i].uBytePerLine = parm.plane[i].uBytePerLine;
        pParm->plane[i].a = parm.plane[i].a;
        pParm->plane[i].r = parm.plane[i].r;
        pParm->plane[i].g = parm.plane[i].g;
        pParm->plane[i].b = parm.plane[i].b;
    }

    return 0;
}


int gfx_unlock_surface(int fdGfxDev, int hSurface, GFX_SURFACE_LOCK_INFO_T *pParm)
{
    int rtn, i;
    
    for(i=0; i<GET_GFX_SURFACE_SUBPLANES(pParm->uPlaneConfig); i++)
    {
        munmap(pParm->plane[i].pPlane - pParm->plane[i].plane.uOffset, 
            pParm->plane[i].plane.uSize);
    }

    rtn = ioctl(fdGfxDev, IOC_GFX_UNLOCK_SURFACE, hSurface);

    if(!rtn)
        memset(pParm, 0, sizeof(*pParm));

    return rtn;
}


int gfx_attach_surface(int fdGfxDev, int hSurface, GFX_VISUAL_DEVICE_ID_T graphDev)
{
    GFX_SURFACE_VDEV_PARM_T parm;
    parm.graphDev = graphDev;
    parm.hSurface = hSurface;

    return ioctl(fdGfxDev, IOC_GFX_ATTACH_SURFACE, &parm);
}


int gfx_detach_surface(int fdGfxDev, int hSurface, GFX_VISUAL_DEVICE_ID_T graphDev)
{
    GFX_SURFACE_VDEV_PARM_T parm;
    parm.graphDev = graphDev;
    parm.hSurface = hSurface;

    return ioctl(fdGfxDev, IOC_GFX_DETACH_SURFACE, &parm);
}


int gfx_get_surface_palette(int fdGfxDev, int hSurface, UINT uStart, UINT uCount, GFX_PALETTE_T *pPalette)
{
    GFX_SURFACE_ACCESS_PALETTE_PARM_T parm;
    parm.hSurface = hSurface;
    parm.pPalette = pPalette;
    parm.uStart = uStart;
    parm.uCount = uCount;

    return ioctl(fdGfxDev, IOC_GFX_GET_SURFACE_PALETTE, &parm);
}


int gfx_set_surface_palette(int fdGfxDev, int hSurface, UINT uStart, UINT uCount, GFX_PALETTE_T *pPalette)
{
    GFX_SURFACE_ACCESS_PALETTE_PARM_T parm;
    parm.hSurface = hSurface;
    parm.pPalette = pPalette;
    parm.uStart = uStart;
    parm.uCount = uCount;

    return ioctl(fdGfxDev, IOC_GFX_SET_SURFACE_PALETTE, &parm);
}


int gfx_get_shared_surface(int fdGfxDev)
{
    return ioctl(fdGfxDev, IOC_GFX_GET_SHARED_SURFACE, 0);
}

int gfx_set_shared_surface(int fdGfxDev, int hSurface)
{
    return ioctl(fdGfxDev, IOC_GFX_SET_SHARED_SURFACE, hSurface);
}


int gfx_get_surface_display_parm(int fdGfxDev, int hSurface, GFX_SURFACE_DISPLAY_T *pParm)
{
    pParm->hSurface = hSurface;

    return ioctl(fdGfxDev, IOC_GFX_GET_SURFACE_DISP_PARM, pParm);
}


int gfx_set_surface_display_parm(int fdGfxDev, int hSurface, GFX_SURFACE_DISPLAY_T *pParm)
{
    pParm->hSurface = hSurface;

    return ioctl(fdGfxDev, IOC_GFX_SET_SURFACE_DISP_PARM, pParm);
}


int gfx_get_screen_info(int fdGfxDev, GFX_SCREEN_INFO_T *pInfo)
{
    return ioctl(fdGfxDev, IOC_GFX_GET_SCREEN_INFO, pInfo);
}


int gfx_set_screen_info(int fdGfxDev, GFX_SCREEN_INFO_T *pInfo)
{
    return ioctl(fdGfxDev, IOC_GFX_SET_SCREEN_INFO, pInfo);
}


int gfx_move_cursor(int fdGfxDev, int x, int y)
{
    GFX_COORDINATE_T parm;
    parm.nCursorX = x;
    parm.nCursorY = y;

    return ioctl(fdGfxDev, IOC_GFX_MOVE_CURSOR, &parm);
}

int gfx_report_cursor(int fdGfxDev, int *x, int *y)
{
    GFX_COORDINATE_T parm;
    if(ioctl(fdGfxDev, IOC_GFX_MOVE_CURSOR, &parm) >= 0)
    {
        if(x) *x = parm.nCursorX;
        if(y) *y = parm.nCursorY;
        return 0;
    }
    else return -1;
}

int gfx_set_cursor_attributes(int fdGfxDev, int hCursorSurface, UINT uIndex, GFX_CURSOR_ATTRIBUTE_T attr)
{
    GFX_CURSOR_ATTRUBUTE_PARM_T parm;
    parm.attr = attr;
    parm.hCursor = hCursorSurface;
    parm.uIndex = uIndex;

    return ioctl(fdGfxDev, IOC_GFX_SET_CURSOR_ATTRIBUTE, &parm);
}

int gfx_set_display_attr(int fdGfxDev, GFX_DISPLAY_CONTROL_T control, UINT uattr)
{
    GFX_DISPLAY_CONTROL_PARM_T parm;
    parm.parm = control;
    parm.uAttr = uattr;

    return ioctl(fdGfxDev, IOC_GFX_SET_DISPLAY_CONTROL, &parm);
}

int gfx_get_display_attr(int fdGfxDev, GFX_DISPLAY_CONTROL_T control, UINT *puattr)
{
    GFX_DISPLAY_CONTROL_PARM_T parm;
    parm.parm = control;
    if(ioctl(fdGfxDev, IOC_GFX_GET_DISPLAY_CONTROL, &parm) >= 0)
    {
        if(puattr) *puattr = parm.uAttr;
        return 0;
    }
    return -1;
}

int gfx_set_display_device_attr(int fdGfxDev,  GFX_VISUAL_DEVICE_ID_T graphDev, GFX_VISUAL_DEVICE_CONTROL_T control,  UINT uattr)
{
    GFX_VISUAL_DEVICE_CONTROL_PARM_T parm;
    parm.graphDev = graphDev;
    parm.cntl = control;
    parm.uAttr = uattr;

    return ioctl(fdGfxDev, IOC_GFX_SET_VISUAL_DEVICE_CONTROL, &parm);
}

int gfx_get_display_device_attr(int fdGfxDev,  GFX_VISUAL_DEVICE_ID_T graphDev, GFX_VISUAL_DEVICE_CONTROL_T control,  UINT *puattr)
{
    GFX_VISUAL_DEVICE_CONTROL_PARM_T parm;
    parm.graphDev = graphDev;
    parm.cntl = control;
    if(ioctl(fdGfxDev, IOC_GFX_GET_VISUAL_DEVICE_CONTROL, &parm) >= 0)
    {
        if(puattr) *puattr = parm.uAttr;
        return 0;
    }
    return -1;
}


int gfx_wait_for_engine(int fdGfxDev, int nTimeout)
{
    return ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, nTimeout);
}

int gfx_reset_engine(int fdGfxDev)
{
    return ioctl(fdGfxDev, IOC_GFX_RESET_ENGINE, 0);
}

int gfx_set_engine_mode(int fdGfxDev, int sync_mode)
{
    return ioctl(fdGfxDev, IOC_GFX_SET_ENGINE_MODE, sync_mode);
}

int gfx_get_engine_mode(int fdGfxDev)
{
    return ioctl(fdGfxDev, IOC_GFX_GET_ENGINE_MODE, 0);
}

int gfx_set_surface_clip_rect(int fdGfxDev, int hSurface, GFX_RECT_T *pClip)
{
    GFX_SET_CLIP_PARM_T parm;
    parm.hSurface = hSurface;
    if(pClip)  parm.rect = *pClip;
    else
    {
        parm.rect.x1 = 0;
        parm.rect.y1 = 0;
        parm.rect.x2 = 1<<(sizeof(int)*8 - 2);  // large enough
        parm.rect.y2 = 1<<(sizeof(int)*8 - 2);  // large enough
    }
    if(ioctl(fdGfxDev, IOC_GFX_SET_SURFACE_CLIP_RECT, &parm) < 0)
       return -1;
    if(pClip) *pClip = parm.rect;
    return 0;
}

INT gfx_bitBLT(int fdGfxDev, int hDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                int hSrc,  INT nSrcX,  INT nSrcY,
                ALPHA_SELECT *pAlphaSelect, char enableGammaCorrection)
{
    GFX_BITBLT_PARM_T   parm;
    parm.hDesSurface = hDes;
    parm.uDesX = nDesX;
    parm.uDesY = nDesY;
    parm.uWidth = uWidth;
    parm.uHeight = uHeight;
    parm.hSrcSurface = hSrc;
    parm.uSrcX = nSrcX;
    parm.uSrcY = nSrcY;
    if(pAlphaSelect)
        parm.alphaSelect = *pAlphaSelect;
    else
    {
        parm.alphaSelect.globalAlphaValue = 0;
        parm.alphaSelect.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
    }
    parm.enableGammaCorrection = enableGammaCorrection;

    return ioctl(fdGfxDev, IOC_GFX_BITBLT, &parm);
}


INT gfx_advancedBitBLT(int fdGfxDev, int hDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight,
            int hSrc, INT nSrcX, INT nSrcY,
            int hMask, INT nMaskX, INT nMaskY,
            GFX_ROP_CODE_T ROP,
            char enablePixelBitMask, UINT32 uPixelBitMask,
            ALPHA_SELECT *pAlphaSelect)
{
    GFX_ADV_BITBLT_PARM_T   parm;
    parm.hDesSurface = hDes;
    parm.uDesX = nDesX;
    parm.uDesY = nDesY;
    parm.uWidth = uWidth;
    parm.uHeight = uHeight;
    parm.hSrcSurface = hSrc;
    parm.uSrcX = nSrcX;
    parm.uSrcY = nSrcY;
    parm.hMaskSurface = hMask;
    parm.uMaskX = nMaskX;
    parm.uMaskY = nMaskY;
    parm.ROP = ROP;
    parm.enablePixelBitMask = enablePixelBitMask;
    parm.uPixelBitMask = uPixelBitMask;
    if(pAlphaSelect)
        parm.alphaSelect = *pAlphaSelect;
    else
    {
        parm.alphaSelect.globalAlphaValue = 0;
        parm.alphaSelect.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
    }

    return ioctl(fdGfxDev, IOC_GFX_ADV_BITBLT, &parm);
}


INT gfx_fillBLT(int fdGfxDev, int hDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight, UINT32 uFillColor)
{
    GFX_FILLBLT_PARM_T   parm;
    parm.hDesSurface = hDes;
    parm.uDesX = nDesX;
    parm.uDesY = nDesY;
    parm.uWidth = uWidth;
    parm.uHeight = uHeight;
    parm.uFillColor = uFillColor;

    return ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm);
}


INT gfx_advancedFillBLT(int fdGfxDev, int hDes, INT nDesX, INT nDesY,
            UINT uWidth, UINT uHeight, UINT32 uFillColor, /* formatted in the dest format color if rgb, index if clut */
            int hMask, INT nMaskX, INT nMaskY,
            GFX_ROP_CODE_T ROP, char transparencyEnable,    // 
            UINT32 uBackGroundColor, char enablePixelBitMask, UINT32 uPixelBitMask)
{
    GFX_ADV_FILLBLT_PARM_T   parm;
    parm.hDesSurface = hDes;
    parm.uDesX = nDesX;
    parm.uDesY = nDesY;
    parm.uWidth = uWidth;
    parm.uHeight = uHeight;
    parm.uFillColor = uFillColor;
    parm.hMaskSurface = hMask;
    parm.uMaskX = nMaskX;
    parm.uMaskY = nMaskY;
    parm.ROP = ROP;
    parm.transparencyEnable = transparencyEnable;
    parm.uBackGroundColor = uBackGroundColor;
    parm.enablePixelBitMask = enablePixelBitMask;
    parm.uPixelBitMask = uPixelBitMask;

    return ioctl(fdGfxDev, IOC_GFX_ADV_FILLBLT, &parm);
}


INT gfx_blend(int fdGfxDev, int hDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                int hSrc, INT nSrcX, INT nSrcY,
                ALPHA_BLEND_SELECT *pBlendSelect)
{
    GFX_BLEND_PARM_T   parm;
    parm.hDesSurface = hDes;
    parm.uDesX = nDesX;
    parm.uDesY = nDesY;
    parm.uWidth = uWidth;
    parm.uHeight = uHeight;
    parm.hSrcSurface = hSrc;
    parm.uSrcX = nSrcX;
    parm.uSrcY = nSrcY;
    if(pBlendSelect)
        parm.blendSelect = *pBlendSelect;
    else
    {
        parm.blendSelect.globalAlphaValue = 0x80;
        parm.blendSelect.blendInputSelect = GFX_BLEND_ALPHA_FROM_SOURCE;
        parm.blendSelect.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
    }

    return ioctl(fdGfxDev, IOC_GFX_BLEND, &parm);
}


INT gfx_advancedBlend(int fdGfxDev, int hDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                int hSrc, INT nSrcX, INT nSrcY,
                int hAlpha, INT nAlphaX, INT nAlphaY,
                ALPHA_BLEND_SELECT *pBlendSelect)
{
    GFX_ADV_BLEND_PARM_T   parm;
    parm.hDesSurface = hDes;
    parm.uDesX = nDesX;
    parm.uDesY = nDesY;
    parm.uWidth = uWidth;
    parm.uHeight = uHeight;
    parm.hSrcSurface = hSrc;
    parm.uSrcX = nSrcX;
    parm.uSrcY = nSrcY;
    parm.hAlphaSurface = hAlpha;
    parm.uAlphaX = nAlphaX;
    parm.uAlphaY = nAlphaY;
    if(pBlendSelect)
        parm.blendSelect = *pBlendSelect;
    else
    {
        parm.blendSelect.globalAlphaValue = 0x80;
        parm.blendSelect.blendInputSelect = GFX_BLEND_ALPHA_FROM_PATTERN;
        parm.blendSelect.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
    }

    return ioctl(fdGfxDev, IOC_GFX_ADV_BLEND, &parm);
}

INT gfx_colorKey(int fdGfxDev, int hDes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight,
                int hSrc, INT nSrcX, INT nSrcY,
                COLOR_KEY_SELECT *pColorKeySelect, ALPHA_SELECT *pAlphaSelect)
{
    GFX_COLORKEY_PARM_T   parm;
    parm.hDesSurface = hDes;
    parm.uDesX = nDesX;
    parm.uDesY = nDesY;
    parm.uWidth = uWidth;
    parm.uHeight = uHeight;
    parm.hSrcSurface = hSrc;
    parm.uSrcX = nSrcX;
    parm.uSrcY = nSrcY;
    parm.colorKeySelect = *pColorKeySelect;
    if(pAlphaSelect)
        parm.alphaSelect = *pAlphaSelect;
    else
    {
        parm.alphaSelect.globalAlphaValue = 0;
        parm.alphaSelect.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
    }

    return ioctl(fdGfxDev, IOC_GFX_COLORKEY, &parm);
}


INT gfx_resize(int fdGfxDev, int hDes, UINT uDesX, UINT uDesY, UINT uDesWidth, UINT uDesHeight,
            int hSrc, UINT uSrcX, UINT uSrcY, UINT uSrcWidth, UINT uSrcHeight,
            BYTE destAlpha, char enableGammaCorrection)
{
    GFX_RESIZE_PARM_T   parm;
    parm.hDesSurface = hDes;
    parm.uDesX = uDesX;
    parm.uDesY = uDesY;
    parm.uDesWidth = uDesWidth;
    parm.uDesHeight = uDesHeight;
    parm.hSrcSurface = hSrc;
    parm.uSrcX = uSrcX;
    parm.uSrcY = uSrcY;
    parm.uSrcWidth = uSrcWidth;
    parm.uSrcHeight = uSrcHeight;
    parm.destAlpha = destAlpha;
    parm.enableGammaCorrection = enableGammaCorrection;

    return ioctl(fdGfxDev, IOC_GFX_RESIZE, &parm);
}


