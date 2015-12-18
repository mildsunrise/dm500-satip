//vulcan/drv/gfx/gfx_osi_engine.c
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

#include "os/os-types.h"
#include "os/os-generic.h"
#include "gfx_surface.h"
#include "gfx_osi.h"
#include "gfx_osi_engine.h"

//#define  __GFX_OSI_G2D_DEBUG
 
#ifdef __GFX_OSI_G2D_DEBUG
    #define __DRV_DEBUG
#endif
#include "os/drv_debug.h"


#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif


GFX_PALETTE_T *__gfx_osi_g2d_select_palette(GFX_SURFACE_T *pSrc, GFX_SURFACE_T *pDes, GFX_PALETTE_T *pTempPal)
{
    int i, numpal, k, steps;
    GFX_PALETTE_T *pPal;

    if(!IS_GFX_SURFACE_CLUT(pSrc->uPlaneConfig)) return NULL;
    if(!pTempPal) return pSrc->pPalette;

    if(!pSrc->pPalette)     // we need to create raw palette
    {
        numpal = 1<<pSrc->plane[0].uPixelSize;
        steps = 255/(numpal-1);
        for(i=0, k=0; i<numpal; i++, k+=steps)
            pTempPal[i].a = pTempPal[i].r = pTempPal[i].g = pTempPal[i].b = k;
        return pTempPal;
    }

    if(IS_GFX_SURFACE_YUV(pSrc->uPlaneConfig))
    {
        if(G2D_CLUT8 == GET_GFX_SURFACE_DATA_TYPE(pDes->uPlaneConfig) || IS_GFX_SURFACE_YUV(pDes->uPlaneConfig))
            pPal = pSrc->pPalette;
        else if(!IS_GFX_SURFACE_YUV(pDes->uPlaneConfig))
        {
            // yuv 2 rgb palette
            pPal = pTempPal;
            numpal = 1<<pSrc->plane[0].uPixelSize;
            for(i=numpal-1; i>=0; i--)
            {
                gfx_osi_ycbcr2rgb(pSrc->pPalette[i].r, pSrc->pPalette[i].g, pSrc->pPalette[i].b,  &pPal[i].r, &pPal[i].g, &pPal[i].b);
                pPal[i].a = pSrc->pPalette[i].a;
            }
        }
    }
    else  // rgb palette
    {
        if(IS_GFX_SURFACE_CLUT(pDes->uPlaneConfig) || !IS_GFX_SURFACE_YUV(pDes->uPlaneConfig))
            pPal = pSrc->pPalette;
        else
        {
            // yuv 2 rgb palette
            pPal = pTempPal;
            numpal = 1<<pSrc->plane[0].uPixelSize;
            for(i=numpal-1; i>=0; i--)
            {
                gfx_osi_rgb2ycbcr(pSrc->pPalette[i].r, pSrc->pPalette[i].g, pSrc->pPalette[i].b,  &pPal[i].r, &pPal[i].g, &pPal[i].b);
                pPal[i].a = pSrc->pPalette[i].a;
            }
        }
    }
    return pPal;
}



void __gfx_osi_g2d_fillblt8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, UINT uFill)
{
    int y;
    pdes = pdes + nDesX + nDesY*bpldes;
    for(y=0; y<uHeight; y++)
    {
        _OS_MEMSET(pdes, uFill, uWidth);
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_fillblt32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, UINT32 uFill)
{
    UINT y;

    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    for(y=0; y<uHeight; y++)
    {
        register UINT32 *pBuf = (UINT32 *)pdes;
        register UINT x;
        for(x=0; x<uWidth; x++)
            *(pBuf++) = uFill;
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_fillblt1(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, UINT uFill)
{
    UINT y, s;
    register BYTE *pBuf;
    pBuf = pdes + (nDesX>>3) + nDesY*bpldes;
    if(uFill & 1) uFill = 0xff;
    else uFill = 0;
    s = nDesX & 0x07;
    if(s || uWidth < 8) // start
    {
        register BYTE umsk, ufil;

        if(uWidth + s >= 8)
        {
            umsk = 0xff << (8-s);
            ufil = uFill & ~ umsk;
            uWidth -= 8-s;
        }
        else
        {
            umsk = (0xff << (8-s)) | (0xff >> (s+uWidth));
            ufil = uFill & ~ umsk;
            uWidth = 0;
        }
        for(y=0; y<uHeight; y++)
        {
            *pBuf = (*pBuf & umsk) | ufil;
            pBuf += bpldes;
        }
        nDesX += s;
        pBuf = pdes + (nDesX>>3) + nDesY*bpldes;
    }

    s = uWidth&7;
    if(s)   // tailing
    {
        register BYTE umsk, ufil;
        umsk = 0xff >> s;
        ufil = uFill & ~ umsk;
        pBuf = pdes + uWidth/8;
        for(y=0; y<uHeight; y++)
        {
            *pBuf = (*pBuf & umsk) | ufil;
            pBuf += bpldes;
        }
        if(uWidth < 8)
            return;
        pBuf = pdes + (nDesX>>3) + nDesY*bpldes;
    }

    uWidth >>=3;
    for(y=0; y<uHeight; y++)
    {
        _OS_MEMSET(pBuf, uFill, uWidth);
        pBuf += bpldes;
    }
}


void __gfx_osi_g2d_fillblt_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, UINT uFill)
{
    int y,x;
    pdes = pdes + nDesX + nDesY*bpldes;
    for(y=0; y<uHeight; y++)
    {
        UINT16 *puv = (UINT16 *)pdes;
        for(x=0; x<uWidth; x+=2)
        {
            *(puv++) = uFill;
        }
        pdes += bpldes;
    }
}


void __gfx_osi_g2d_maskflt8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uForeGround)
{
    UINT y;
    register UINT x, s;
    register BYTE *pBuf;
    pBuf = psrc + (nSrcX>>3) + nSrcY*bplsrc;

    pdes = pdes + nDesX + nDesY*bpldes;
    
    for(y=0; y<uHeight; y++)
    {
        s = nSrcX&7;
        for(x=0; x<uWidth; x++, s++)
            if(pBuf[s>>3] & (0x80 >> (s&7)))
                pdes[x] = uForeGround;

        pBuf += bplsrc;
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_maskflt32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT32 uForeGround)
{
    UINT y;
    register UINT x, s;
    register BYTE *pBuf;
    pBuf = psrc + (nSrcX>>3) + nSrcY*bplsrc;

    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    

    for(y=0; y<uHeight; y++)
    {
        register UINT32 *pDestBuf = (UINT32 *)pdes;
        s = nSrcX&7;
        for(x=0; x<uWidth; x++, s++)
            if(pBuf[s>>3] & (0x80 >> (s&7)))
                pDestBuf[x] = uForeGround;

        pBuf += bplsrc;
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_maskflt_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uForeGround)
{
    UINT y;
    register UINT x, s;
    register BYTE *pBuf;
    BYTE uf, vf;
    pBuf = psrc + (nSrcX>>3) + nSrcY*bplsrc;

    pdes = pdes + nDesX + nDesY*bpldes;
    
    uf = uForeGround>>8;
    vf = uForeGround&0xff;

    for(y=0; y<uHeight; y++)
    {
        s = nSrcX&7;
        for(x=0; x<uWidth; x++, s++)
        {
            if(pBuf[s>>3] & (0x80 >> (s&7)))
            {
                if(x&1) // u
                    pdes[x] =  uf;
                else  // v
                    pdes[x] = vf;
            }
        }
        pBuf += bplsrc;
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_bitblt8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY)
{
    int y;
    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + nDesX + nDesY*bpldes;
    for(y=0; y<uHeight; y++)
    {
        _OS_MEMCPY(pdes, psrc, uWidth);
        psrc += bplsrc;
        pdes += bpldes;
    }
}

// uColor = 0/a, 1/r, 2/g, 3/b
void __gfx_osi_g2d_bitblt_clut_8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, GFX_PALETTE_T *pPal, UINT uColor)
{
    int y, x;
    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + nDesX + nDesY*bpldes;
    switch(uColor)
    {
    case 0:   // a
        for(y=0; y<uHeight; y++)
        {
            for(x=0; x<uWidth; x++)
            {
                pdes[x] = pPal[psrc[x]].a;
            }
            psrc += bplsrc;
            pdes += bpldes;
        }
        break;
    case 1: // r
        for(y=0; y<uHeight; y++)
        {
            for(x=0; x<uWidth; x++)
            {
                pdes[x] = pPal[psrc[x]].r;
            }
            psrc += bplsrc;
            pdes += bpldes;
        }
        break;
    case 2: // g
        for(y=0; y<uHeight; y++)
        {
            for(x=0; x<uWidth; x++)
            {
                pdes[x] = pPal[psrc[x]].g;
            }
            psrc += bplsrc;
            pdes += bpldes;
        }
        break;
    case 3: // b
        for(y=0; y<uHeight; y++)
        {
            for(x=0; x<uWidth; x++)
            {
                pdes[x] = pPal[psrc[x]].b;
            }
            psrc += bplsrc;
            pdes += bpldes;
        }
        break;
    default:
        break;
    }
}

void __gfx_osi_g2d_bitblt_clut_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, GFX_PALETTE_T *pPal)
{
    int y, x;
    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + nDesX + nDesY*bpldes;
    for(y=0; y<uHeight; y++)
    {
        for(x=0; x<uWidth; x+=2)
        {
            // instead of averaging, we  choose one color components from each pixel in pairs
            pdes[x  ] = pPal[psrc[x  ]].g;  // u
            pdes[x+1] = pPal[psrc[x+1]].b;  // v
        }
        psrc += bplsrc;
        pdes += bpldes;
    }
}


void __gfx_osi_g2d_bitblt_1_8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uForeGround, UINT uBackGround)
{
    UINT y;
    register UINT x, s;
    register BYTE *pBuf;
    pBuf = psrc + (nSrcX>>3) + nSrcY*bplsrc;

    pdes = pdes + nDesX + nDesY*bpldes;
    

    PDEBUGE("1bit SrcX = %d, Y = %d, W = %d, H= %d, bpl=%d, s=%d\n", nSrcX, nSrcY, uWidth, uHeight, bplsrc, s);

    for(y=0; y<uHeight; y++)
    {
        s = nSrcX&7;
        for(x=0; x<uWidth; x++, s++)
            pdes[x] = (pBuf[s>>3] & (0x80 >> (s&7))) ? uForeGround : uBackGround;

        pBuf += bplsrc;
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_bitblt_1_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT32 uForeGround, UINT32 uBackGround)
{
    UINT y;
    register UINT x, s;
    register BYTE *pBuf;
    pBuf = psrc + (nSrcX>>3) + nSrcY*bplsrc;

    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    

    for(y=0; y<uHeight; y++)
    {
        register UINT32 *pDestBuf = (UINT32 *)pdes;
        s = nSrcX&7;
        for(x=0; x<uWidth; x++, s++)
            pDestBuf[x] = (pBuf[s>>3] & (0x80 >> (s&7))) ? uForeGround : uBackGround;

        pBuf += bplsrc;
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_bitblt_1_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uForeGround, UINT uBackGround)
{
    UINT y;
    register UINT x, s;
    register BYTE *pBuf;
    BYTE uf, vf, ub, vb;
    pBuf = psrc + (nSrcX>>3) + nSrcY*bplsrc;

    pdes = pdes + nDesX + nDesY*bpldes;
    
    uf = uForeGround>>8;
    vf = uForeGround&0xff;
    ub = uBackGround>>8;
    vb = uBackGround&0xff;

    for(y=0; y<uHeight; y++)
    {
        s = nSrcX&7;
        for(x=0; x<uWidth; x++, s++)
        {
            if(x&1) // u
                pdes[x] = (pBuf[s>>3] & (0x80 >> (s&7))) ? uf : ub;
            else  // v
                pdes[x] = (pBuf[s>>3] & (0x80 >> (s&7))) ? vf : vb;
        }
        pBuf += bplsrc;
        pdes += bpldes;
    }
}


void __gfx_osi_g2d_bitblt_clut_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, GFX_PALETTE_T *pPal)
{
    int y, x;
    UINT32 *pPal32 = (UINT32 *)pPal;
    // we assume that the palette is the same as argb32 color
    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    for(y=0; y<uHeight; y++)
    {
        UINT32 *pBuf = (UINT32 *)pdes;
        for(x=0; x<uWidth; x++)
        {
            pBuf[x] = pPal32[psrc[x]];
        }
        psrc += bplsrc;
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_bitblt_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY)
{
    int y;
    psrc = psrc + (nSrcX<<2) + nSrcY*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    uWidth <<= 2;
    for(y=0; y<uHeight; y++)
    {
        _OS_MEMCPY(pdes, psrc, uWidth);
        psrc += bplsrc;
        pdes += bpldes;
    }
}


void __gfx_osi_g2d_bitblt_yuv422_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrcy, BYTE *psrcuv, BYTE *psrca, INT bplsrc, INT nSrcX,  INT nSrcY)
{
    int y, x, n;
    // we assume that the palette is the same as argb32 color
    psrcy = psrcy + nSrcX + nSrcY*bplsrc;
    psrcuv = psrcuv + (nSrcX&0xfffffffe) + nSrcY*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    if(psrca)
    {
        psrca = psrca + nSrcX + nSrcY*bplsrc;
        for(y=0; y<uHeight; y++)
        {
            for(x=0,n=0; x<uWidth; x++, n+=4)
            {
                gfx_osi_ycbcr2rgb(psrcy[x], psrcuv[x&0xfffffffe], psrcuv[((x)&0xfffffffe) + 1], pdes + n+1, pdes + n+2, pdes + n+3);
                pdes[n] = psrca[x];
            }
            psrcy += bplsrc;
            psrcuv += bplsrc;
            psrca += bplsrc;
            pdes += bpldes;
        }
    }
    else
    {
        for(y=0; y<uHeight; y++)
        {
            for(x=0,n=0; x<uWidth; x++, n+=4)
            {
                gfx_osi_ycbcr2rgb(psrcy[x], psrcuv[x&0xfffffffe], psrcuv[((x)&0xfffffffe) + 1], pdes + n+1, pdes + n+2, pdes + n+3);
                pdes[n] = 255;
            }
            psrcy += bplsrc;
            psrcuv += bplsrc;
            pdes += bpldes;
        }
    }
}


void __gfx_osi_g2d_bitblt_yuv420_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrcy, BYTE *psrcuv, BYTE *psrca, INT bplsrc, INT nSrcX,  INT nSrcY)
{
    int y, x, n;
    // we assume that the palette is the same as argb32 color
    psrcy = psrcy + nSrcX + nSrcY*bplsrc;
    psrcuv = psrcuv + (nSrcX&0xfffffffe) + (nSrcY>>1)*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    if(psrca)
    {
        psrca = psrca + nSrcX + nSrcY*bplsrc;
        for(y=0; y<uHeight; y++)
        {
            for(x=0,n=0; x<uWidth; x++, n+=4)
            {
                gfx_osi_ycbcr2rgb(psrcy[x], psrcuv[x&0xfffffffe], psrcuv[((x)&0xfffffffe) + 1], pdes + n+1, pdes + n+2, pdes + n+3);
                pdes[n] = psrca[x];
            }
            psrcy += bplsrc;
            if((y+nSrcY)&1) psrcuv += bplsrc;
            psrca += bplsrc;
            pdes += bpldes;
        }
    }
    else
    {
        for(y=0; y<uHeight; y++)
        {
            for(x=0,n=0; x<uWidth; x++, n+=4)
            {
                gfx_osi_ycbcr2rgb(psrcy[x], psrcuv[x&0xfffffffe], psrcuv[((x)&0xfffffffe) + 1], pdes + n+1, pdes + n+2, pdes + n+3);
            }
            psrcy += bplsrc;
            if((y+nSrcY)&1) psrcuv += bplsrc;
            pdes += bpldes;
        }
    }
}


void __gfx_osi_g2d_bitblt_32_yuv422(BYTE *pdesy, BYTE *pdesuv, BYTE *pdesa,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY)
{
    int y, x, n;
    // we assume that the palette is the same as argb32 color
    psrc = psrc + (nSrcX<<2) + nSrcY*bplsrc;
    pdesy = pdesy + nDesX + nDesY*bpldes;
    pdesuv = pdesuv + (nDesX&0xfffffffe) + nDesY*bpldes;
    if(pdesa)
    {
        pdesa = pdesa + nDesX + nDesY*bpldes;
        for(y=0; y<uHeight; y++)
        {
            for(x=0,n=0; x<uWidth; x++, n+=4)
            {
                gfx_osi_rgb2ycbcr(psrc[n+1], psrc[n+2], psrc[n+3], pdesy+x, pdesuv + (x&0xfffffffe), pdesuv + (x&0xfffffffe)+1);
                pdesa[x] = psrc[n];
            }
            pdesy += bpldes;
            pdesuv += bpldes;
            pdesa += bpldes;
            psrc += bplsrc;
        }
    }
    else
    {
        for(y=0; y<uHeight; y++)
        {
            for(x=0,n=0; x<uWidth; x++, n+=4)
            {
                gfx_osi_rgb2ycbcr(psrc[n+1], psrc[n+2], psrc[n+3], pdesy+x, pdesuv + (x&0xfffffffe), pdesuv + (x&0xfffffffe)+1);
            }
            pdesy += bpldes;
            pdesuv += bpldes;
            psrc += bplsrc;
        }
    }
}

void __gfx_osi_g2d_bitblt_32_yuv420(BYTE *pdesy, BYTE *pdesuv, BYTE *pdesa,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY)
{
    int y, x, n;
    // we assume that the palette is the same as argb32 color
    psrc = psrc + (nSrcX<<2) + nSrcY*bplsrc;
    pdesy = pdesy + nDesX + nDesY*bpldes;
    pdesuv = pdesuv + (nDesX&0xfffffffe) + (nDesY>>1)*bpldes;
    if(pdesa)
    {
        pdesa = pdesa + nDesX + nDesY*bpldes;
        for(y=0; y<uHeight; y++)
        {
            for(x=0,n=0; x<uWidth; x++, n+=4)
            {
                gfx_osi_rgb2ycbcr(psrc[n+1], psrc[n+2], psrc[n+3], pdesy+x, pdesuv + (x&0xfffffffe), pdesuv + (x&0xfffffffe)+1);
                pdesa[x] = psrc[n];
            }
            pdesy += bpldes;
            if((y+nDesY)&1) pdesuv += bpldes;
            pdesa += bpldes;
            psrc += bplsrc;
        }
    }
    else
    {
        for(y=0; y<uHeight; y++)
        {
            for(x=0,n=0; x<uWidth; x++, n+=4)
            {
                gfx_osi_rgb2ycbcr(psrc[n+1], psrc[n+2], psrc[n+3], pdesy+x, pdesuv + (x&0xfffffffe), pdesuv + (x&0xfffffffe)+1);
            }
            pdesy += bpldes;
            if((y+nDesY)&1) pdesuv += bpldes;
            psrc += bplsrc;
        }
    }
}



void __gfx_osi_g2d_blend32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY,
                BYTE *palpha, INT bplalpha,  INT nPixIncA, BYTE *pdesta, INT bpldesta,  INT nPixIncDestA)
{
    int y, x;
    register BYTE *ps, *pd, *pa, *pda;
    psrc = psrc + (nSrcX<<2) + nSrcY*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    uWidth <<= 2;
    for(y=0; y<uHeight; y++)
    {
        ps = psrc;
        pd = pdes;
        pa = palpha;
        pda = pdesta;
        for(x=0; x<uWidth; x++)
        {
            *(pd + 1) = ((UINT16)*(ps + 1)*(*pa) + (UINT16)*(pd + 1)*(255 - *pa))>>8;   // should be /255
            *(pd + 2) = ((UINT16)*(ps + 2)*(*pa) + (UINT16)*(pd + 2)*(255 - *pa))>>8;   // should be /255
            *(pd + 3) = ((UINT16)*(ps + 3)*(*pa) + (UINT16)*(pd + 3)*(255 - *pa))>>8;   // should be /255
            *(pd    ) = *pda;
            ps += 4;
            pd += 4;
            pa += nPixIncA;
            pda += nPixIncDestA;
        }
        psrc += bplsrc;
        pdes += bpldes;
        palpha += bplalpha;
        pdesta += bpldesta;
    }
}

void __gfx_osi_g2d_blend32_a(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uWidth, UINT uHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY,
                BYTE *palpha, INT bplalpha,  INT nPixIncA)
{
    int y, x;
    register BYTE *ps, *pd, *pa;
    psrc = psrc + (nSrcX<<2) + nSrcY*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    uWidth <<= 2;
    for(y=0; y<uHeight; y++)
    {
        ps = psrc;
        pd = pdes;
        pa = palpha;
        for(x=0; x<uWidth; x++)
        {
            *(pd + 1) = ((UINT16)*(ps + 1)*(*pa) + (UINT16)*(pd + 1)*(255 - *pa))>>8;   // should be /255
            *(pd + 2) = ((UINT16)*(ps + 2)*(*pa) + (UINT16)*(pd + 2)*(255 - *pa))>>8;   // should be /255
            *(pd + 3) = ((UINT16)*(ps + 3)*(*pa) + (UINT16)*(pd + 3)*(255 - *pa))>>8;   // should be /255
            *(pd    ) = ((UINT16)*(ps + 0)*(*pa) + (UINT16)*(pd + 0)*(255 - *pa))>>8;   // should be /255
            ps += 4;
            pd += 4;
            pa += nPixIncA;
        }
        psrc += bplsrc;
        pdes += bpldes;
        palpha += bplalpha;
    }
}




void __gfx_osi_g2d_resize_nn8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight)
{
    int x, y, xp, yp;
    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + nDesX + nDesY*bpldes;
    for(y=0; y<uDesHeight; y++) 
    {
        yp = (((y*uSrcHeight)<<8)/uDesHeight + 128)>>8;
        yp *= bplsrc;
        for(x=0; x<uDesWidth; x++) 
        {
            xp = (((x*uSrcWidth)<<8)/uDesWidth + 128)>>8;
            pdes[x] = psrc[xp+yp];
        }
        pdes += bpldes;
    }  
}

void __gfx_osi_g2d_resize_bl8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight)
{
    int x, y, xp, yp, ypx;
    int xx, yy, dx, dy;
    BYTE *pp;
    BYTE p00, p01, p10, p11;

    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + nDesX + nDesY*bpldes;
    for(y=0; y<uDesHeight; y++) 
    {
        yy = ((y*uSrcHeight)<<8)/uDesHeight;
        yp = yy>>8;  dy = yy&0xff;
        ypx = yp*bplsrc;
        for(x=0; x<uDesWidth; x++) 
        {
            xx = ((x*uSrcWidth)<<8)/uDesWidth;
            xp = xx>>8; dx = xx&0xff;
            pp = psrc+xp+ypx; 
            p00 = *pp;
            if(xp+1 < uSrcWidth)
            {
                p10 = *(pp+1); 
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(pp+bplsrc);
                    p11 = *(pp+bplsrc+1);
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            else
            {
                p10 = p00;
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(pp+bplsrc);
                    p11 = p01;
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }

            pdes[x] = (BYTE)(((p11*dx+p01*(256-dx))*dy + (p10*dx+p00*(256-dx))*(256-dy))>>16);
        }
        pdes += bpldes;
    }  
}

// uColor = 0/a, 1/r, 2/g, 3/b
void __gfx_osi_g2d_resize_clut_8(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, GFX_PALETTE_T *pPal, UINT uColor)
{
    int x, y, xp, yp;
    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + nDesX + nDesY*bpldes;
    switch(uColor)
    {
    case 0:   // a
        for(y=0; y<uDesHeight; y++) 
        {
            yp = (((y*uSrcHeight)<<8)/uDesHeight + 128)>>8;
            yp *= bplsrc;
            for(x=0; x<uDesWidth; x++) 
            {
                xp = (((x*uSrcWidth)<<8)/uDesWidth + 128)>>8;
                pdes[x] = pPal[psrc[xp+yp]].a;
            }
            pdes += bpldes;
        }
        break;
    case 1:   // r
        for(y=0; y<uDesHeight; y++) 
        {
            yp = (((y*uSrcHeight)<<8)/uDesHeight + 128)>>8;
            yp *= bplsrc;
            for(x=0; x<uDesWidth; x++) 
            {
                xp = (((x*uSrcWidth)<<8)/uDesWidth + 128)>>8;
                pdes[x] = pPal[psrc[xp+yp]].r;
            }
            pdes += bpldes;
        }
        break;
    case 2:   // g
        for(y=0; y<uDesHeight; y++) 
        {
            yp = (((y*uSrcHeight)<<8)/uDesHeight + 128)>>8;
            yp *= bplsrc;
            for(x=0; x<uDesWidth; x++) 
            {
                xp = (((x*uSrcWidth)<<8)/uDesWidth + 128)>>8;
                pdes[x] = pPal[psrc[xp+yp]].g;
            }
            pdes += bpldes;
        }
        break;
    case 3:   // b
        for(y=0; y<uDesHeight; y++) 
        {
            yp = (((y*uSrcHeight)<<8)/uDesHeight + 128)>>8;
            yp *= bplsrc;
            for(x=0; x<uDesWidth; x++) 
            {
                xp = (((x*uSrcWidth)<<8)/uDesWidth + 128)>>8;
                pdes[x] = pPal[psrc[xp+yp]].b;
            }
            pdes += bpldes;
        }
        break;
    }
}

void __gfx_osi_g2d_resize_clut_32(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, GFX_PALETTE_T *pPal)
{
    int x, y, xp, yp;
    
    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    for(y=0; y<uDesHeight; y++) 
    {
        UINT32  *pBuf = (UINT32 *)pdes;
        yp = (((y*uSrcHeight)<<8)/uDesHeight + 128)>>8;
        yp *= bplsrc;
        for(x=0; x<uDesWidth; x++) 
        {
            xp = (((x*uSrcWidth)<<8)/uDesWidth + 128)>>8;
            pBuf[x] = *(UINT32 *)&pPal[psrc[xp+yp]];
        }
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_resize_clut_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, GFX_PALETTE_T *pPal)
{
    int x, y, xp, yp;
    psrc = psrc + nSrcX + nSrcY*bplsrc;
    pdes = pdes + nDesX + nDesY*bpldes;
    uDesWidth &= 0xfffffffe;    // make sure it is even
    for(y=0; y<uDesHeight; y++) 
    {
        yp = (((y*uSrcHeight)<<8)/uDesHeight + 128)>>8;
        yp *= bplsrc;
        for(x=0; x<uDesWidth; x+=2) 
        {
            xp = (((x*uSrcWidth)<<8)/uDesWidth + 128)>>8;
            pdes[x  ] = pPal[psrc[xp+yp]].g;
            pdes[x+1] = pPal[psrc[xp+yp]].b;
        }
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_resize_uv(BYTE *pdes,  INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight)
{
    int x, y, xp, yp, wplsrc, wpldes, srcw, desw;
    UINT16 *ps, *pd;

    ps = (UINT16 *)(psrc + nSrcX + nSrcY*bplsrc);
    pd = (UINT16 *)(pdes + nDesX + nDesY*bpldes);

    wplsrc =  bplsrc>>1;
    wpldes =  bpldes>>1;
    srcw = uSrcWidth>>1;
    desw = uDesWidth>>1;

    for(y=0; y<uDesHeight; y++) 
    {
        yp = (((y*uSrcHeight)<<8)/uDesHeight + 128)>>8;
        yp *= wplsrc;
        for(x=0; x<desw; x++) 
        {
            xp = (((x*srcw)<<8)/desw + 128)>>8;  // on even pairs
            pd[x] = ps[xp+yp];
        }
        pd += wpldes;
    }
}

void __gfx_osi_g2d_resize_32(BYTE *pdes, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight)
{
    int x, y, xp, yp, ypx;
    int xx, yy, dx, dy;
    BYTE *pp;
    UINT32 p00, p01, p10, p11;

    psrc = psrc + (nSrcX<<2) + nSrcY*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    for(y=0; y<uDesHeight; y++) 
    {
        UINT32 *pBuf = (UINT32 *)pdes;
        yy = ((y*uSrcHeight)<<8)/uDesHeight;
        yp = yy>>8;  dy = yy&0xff;
        ypx = yp*bplsrc;
        for(x=0; x<uDesWidth; x++) 
        {
            xx = ((x*uSrcWidth)<<8)/uDesWidth;
            xp = xx>>8; dx = xx&0xff;
            pp = (psrc+(xp<<2)+ypx);
            p00 = *(UINT32 *)pp;
            if(xp+1 < uSrcWidth)
            {
                p10 = *(UINT32 *)(pp+4); 
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(UINT32 *)(pp+bplsrc);
                    p11 = *(UINT32 *)(pp+bplsrc+4);
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            else
            {
                p10 = p00;
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(UINT32 *)(pp+bplsrc);
                    p11 = p01;
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            {
                UINT32 r,g,b,a;
                {
                    BYTE p00r, p01r, p10r, p11r;
                    p00r = (p00>>16)&0xff;
                    p01r = (p01>>16)&0xff;
                    p10r = (p10>>16)&0xff;
                    p11r = (p11>>16)&0xff;
                    r = ((p11r*dx+p01r*(256-dx))*dy + (p10r*dx+p00r*(256-dx))*(256-dy))&0xff0000;
                }
                {
                    BYTE p00g, p01g, p10g, p11g;
                    p00g = (p00>> 8)&0xff;
                    p01g = (p01>> 8)&0xff;
                    p10g = (p10>> 8)&0xff;
                    p11g = (p11>> 8)&0xff;
                    g = (((p11g*dx+p01g*(256-dx))*dy + (p10g*dx+p00g*(256-dx))*(256-dy))>>8)&0xff00;
                }
                {
                    BYTE p00b, p01b, p10b, p11b;
                    p00b = (p00    )&0xff;
                    p01b = (p01    )&0xff;
                    p10b = (p10    )&0xff;
                    p11b = (p11    )&0xff;
                    b = (((p11b*dx+p01b*(256-dx))*dy + (p10b*dx+p00b*(256-dx))*(256-dy))>>16)&0xff;
                }
                {
                    BYTE p00a, p01a, p10a, p11a;
                    p00a = (p00>>24)&0xff;
                    p01a = (p01>>24)&0xff;
                    p10a = (p10>>24)&0xff;
                    p11a = (p11>>24)&0xff;
                    a = (((p11a*dx+p01a*(256-dx))*dy + (p10a*dx+p00a*(256-dx))*(256-dy))<<8)&0xff000000;
                }
                pBuf[x] = a | r | g | b;
            }
        }
        pdes += bpldes;
    }
}


void __gfx_osi_g2d_resize_32_yuv422(BYTE *pdesy, BYTE *pdesuv, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight)
{
    int x, y, xp, yp, ypx;
    int xx, yy, dx, dy;
    BYTE *pp;
    UINT32 p00, p01, p10, p11;

    psrc = psrc + (nSrcX<<2) + nSrcY*bplsrc;
    pdesy = pdesy + nDesX + nDesY*bpldes;
    pdesuv = pdesuv + nDesX + nDesY*bpldes;
    for(y=0; y<uDesHeight; y++) 
    {
        yy = ((y*uSrcHeight)<<8)/uDesHeight;
        yp = yy>>8;  dy = yy&0xff;
        ypx = yp*bplsrc;
        for(x=0; x<uDesWidth; x++) 
        {
            xx = ((x*uSrcWidth)<<8)/uDesWidth;
            xp = xx>>8; dx = xx&0xff;
            pp = (psrc+(xp<<2)+ypx);
            p00 = *(UINT32 *)pp;
            if(xp+1 < uSrcWidth)
            {
                p10 = *(UINT32 *)(pp+4); 
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(UINT32 *)(pp+bplsrc);
                    p11 = *(UINT32 *)(pp+bplsrc+4);
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            else
            {
                p10 = p00;
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(UINT32 *)(pp+bplsrc);
                    p11 = p01;
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            {
                BYTE r,g,b, dummy;
                {
                    BYTE p00r, p01r, p10r, p11r;
                    p00r = (p00>>16)&0xff;
                    p01r = (p01>>16)&0xff;
                    p10r = (p10>>16)&0xff;
                    p11r = (p11>>16)&0xff;
                    r = (BYTE)(((p11r*dx+p01r*(256-dx))*dy + (p10r*dx+p00r*(256-dx))*(256-dy))>>16);
                }
                {
                    BYTE p00g, p01g, p10g, p11g;
                    p00g = (p00>> 8)&0xff;
                    p01g = (p01>> 8)&0xff;
                    p10g = (p10>> 8)&0xff;
                    p11g = (p11>> 8)&0xff;
                    g = (BYTE)(((p11g*dx+p01g*(256-dx))*dy + (p10g*dx+p00g*(256-dx))*(256-dy))>>16);
                }
                {
                    BYTE p00b, p01b, p10b, p11b;
                    p00b = (p00    )&0xff;
                    p01b = (p01    )&0xff;
                    p10b = (p10    )&0xff;
                    p11b = (p11    )&0xff;
                    b = (BYTE)(((p11b*dx+p01b*(256-dx))*dy + (p10b*dx+p00b*(256-dx))*(256-dy))>>16);
                }
                if((x+nDesX)&1) // v
                    gfx_osi_rgb2ycbcr(r,g,b, pdesy+x, &dummy, pdesuv+x);
                else
                    gfx_osi_rgb2ycbcr(r,g,b, pdesy+x, pdesuv+x, &dummy);
            }
        }
        pdesy += bpldes;
        pdesuv += bpldes;
    }
}

void __gfx_osi_g2d_resize_32_yuv420(BYTE *pdesy, BYTE *pdesuv, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrc, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight)
{
    int x, y, xp, yp, ypx;
    int xx, yy, dx, dy;
    BYTE *pp;
    UINT32 p00, p01, p10, p11;

    psrc = psrc + (nSrcX<<2) + nSrcY*bplsrc;
    pdesy = pdesy + nDesX + nDesY*bpldes;
    pdesuv = pdesuv + nDesX + (nDesY/2)*bpldes;
    for(y=0; y<uDesHeight; y++) 
    {
        yy = ((y*uSrcHeight)<<8)/uDesHeight;
        yp = yy>>8;  dy = yy&0xff;
        ypx = yp*bplsrc;
        for(x=0; x<uDesWidth; x++) 
        {
            xx = ((x*uSrcWidth)<<8)/uDesWidth;
            xp = xx>>8; dx = xx&0xff;
            pp = (psrc+(xp<<2)+ypx);
            p00 = *(UINT32 *)pp;
            if(xp+1 < uSrcWidth)
            {
                p10 = *(UINT32 *)(pp+4); 
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(UINT32 *)(pp+bplsrc);
                    p11 = *(UINT32 *)(pp+bplsrc+4);
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            else
            {
                p10 = p00;
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(UINT32 *)(pp+bplsrc);
                    p11 = p01;
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            {
                BYTE r,g,b, dummy;
                {
                    BYTE p00r, p01r, p10r, p11r;
                    p00r = (p00>>16)&0xff;
                    p01r = (p01>>16)&0xff;
                    p10r = (p10>>16)&0xff;
                    p11r = (p11>>16)&0xff;
                    r = (BYTE)(((p11r*dx+p01r*(256-dx))*dy + (p10r*dx+p00r*(256-dx))*(256-dy))>>16);
                }
                {
                    BYTE p00g, p01g, p10g, p11g;
                    p00g = (p00>> 8)&0xff;
                    p01g = (p01>> 8)&0xff;
                    p10g = (p10>> 8)&0xff;
                    p11g = (p11>> 8)&0xff;
                    g = (BYTE)(((p11g*dx+p01g*(256-dx))*dy + (p10g*dx+p00g*(256-dx))*(256-dy))>>16);
                }
                {
                    BYTE p00b, p01b, p10b, p11b;
                    p00b = (p00    )&0xff;
                    p01b = (p01    )&0xff;
                    p10b = (p10    )&0xff;
                    p11b = (p11    )&0xff;
                    b = (BYTE)(((p11b*dx+p01b*(256-dx))*dy + (p10b*dx+p00b*(256-dx))*(256-dy))>>16);
                }
                if((x+nDesX)&1) // v
                    gfx_osi_rgb2ycbcr(r,g,b, pdesy+x, &dummy, pdesuv+x);
                else
                    gfx_osi_rgb2ycbcr(r,g,b, pdesy+x, pdesuv+x, &dummy);
            }
        }
        pdesy += bpldes;
        if(y&1) pdesuv += bpldes;
    }
}


void __gfx_osi_g2d_resize_yuv422_32(BYTE *pdes, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrcy, BYTE *psrcuv, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, BYTE a)
{
    int x, y, xp, yp, ypx;
    int xx, yy, dx, dy;
    BYTE *pp;
    BYTE p00, p01, p10, p11;
    BYTE lum, u, v;

    psrcy = psrcy + nSrcX + nSrcY*bplsrc;
    psrcuv = psrcuv + nSrcX + nSrcY*bplsrc;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    for(y=0; y<uDesHeight; y++) 
    {
        yy = ((y*uSrcHeight)<<8)/uDesHeight;
        yp = yy>>8;  dy = yy&0xff;
        ypx = yp*bplsrc;
        for(x=0; x<uDesWidth; x++) 
        {
            xx = ((x*uSrcWidth)<<8)/uDesWidth;
            xp = xx>>8; dx = xx&0xff;
            pp = (psrcy+xp+ypx);
            p00 = *pp;

            if((xp+nSrcX)&1) // odd x
            {
                u = *(psrcuv+xp+ypx-1);
                v = *(psrcuv+xp+ypx);
            }
            else
            {
                u = *(psrcuv+xp+ypx);
                v = *(psrcuv+xp+ypx+1);
            }

            if(xp+1 < uSrcWidth)
            {
                p10 = *(pp+1); 
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(pp+bplsrc);
                    p11 = *(pp+bplsrc+1);
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            else
            {
                p10 = p00;
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(pp+bplsrc);
                    p11 = p01;
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }

            lum = (BYTE)(((p11*dx+p01*(256-dx))*dy + (p10*dx+p00*(256-dx))*(256-dy))>>16);
            gfx_osi_ycbcr2rgb(lum, u, v, pdes+(x<<2)+1, pdes+(x<<2)+2, pdes+(x<<2)+3);
            pdes[x<<2] = a;
        }
        pdes += bpldes;
    }
}

void __gfx_osi_g2d_resize_yuv420_32(BYTE *pdes, INT bpldes, INT nDesX, INT nDesY,
                UINT uDesWidth, UINT uDesHeight, BYTE *psrcy, BYTE *psrcuv, INT bplsrc, INT nSrcX,  INT nSrcY, UINT uSrcWidth, UINT uSrcHeight, BYTE a)
{
    int x, y, xp, yp, ypx, ypx1;
    int xx, yy, dx, dy;
    BYTE *pp;
    BYTE p00, p01, p10, p11;
    BYTE lum, u, v;

    psrcy = psrcy + nSrcX + nSrcY*bplsrc;
    psrcuv = psrcuv + nSrcX;
    pdes = pdes + (nDesX<<2) + nDesY*bpldes;
    for(y=0; y<uDesHeight; y++) 
    {
        yy = ((y*uSrcHeight)<<8)/uDesHeight;
        yp = yy>>8;  dy = yy&0xff;
        ypx = yp*bplsrc;
        ypx1 = (yp + nSrcY)/2*bplsrc;
        for(x=0; x<uDesWidth; x++) 
        {
            xx = ((x*uSrcWidth)<<8)/uDesWidth;
            xp = xx>>8; dx = xx&0xff;
            pp = (psrcy+xp+ypx);
            p00 = *pp;

            if((xp+nSrcX)&1) // odd x
            {
                u = *(psrcuv+xp+ypx1-1);
                v = *(psrcuv+xp+ypx1);
            }
            else
            {
                u = *(psrcuv+xp+ypx1);
                v = *(psrcuv+xp+ypx1+1);
            }

            if(xp+1 < uSrcWidth)
            {
                p10 = *(pp+1); 
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(pp+bplsrc);
                    p11 = *(pp+bplsrc+1);
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }
            else
            {
                p10 = p00;
                if(yp+1 < uSrcHeight)
                {
                    p01 = *(pp+bplsrc);
                    p11 = p01;
                }
                else
                {
                    p01 = p00;
                    p11 = p10;
                }
            }

            lum = (BYTE)(((p11*dx+p01*(256-dx))*dy + (p10*dx+p00*(256-dx))*(256-dy))>>16);
            gfx_osi_ycbcr2rgb(lum, u, v, pdes+(x<<2)+1, pdes+(x<<2)+2, pdes+(x<<2)+3);
            pdes[x<<2] = a;
        }
        pdes += bpldes;
    }
}

