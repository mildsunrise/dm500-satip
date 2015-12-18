//pallas/drv/gfx/test/gfxbmp.c
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
//  MS Windoze 8/24 bits Bitmap access
//Revision Log:   
//  Nov/13/2001                          Created by YYD

#include <stdlib.h> // for rand
#include <stdio.h> 

// #define __JPEG_LIB_READY__   // IJG's libjpeg 6b

#ifdef __JPEG_LIB_READY__   // IJG's libjpeg 6b

#include <setjmp.h>

#define XMD_H   // force it to ignore it's own define of INT32

#include <jpeglib.h>

#undef XMD_H

#endif

#include "gfxlib.h"




static UINT16 fgetWord(FILE *fp, BYTE intel)
{
    UINT16 i,j;
    i=(UINT16)(fgetc(fp)&0xff);
    j=(UINT16)(fgetc(fp)&0xff);
    if(intel) return(i+(j<<8));
    else return ((i<<8)+j);
}

static UINT32 fgetLong(FILE *fp, BYTE intel)
{
    UINT32 i,j,k,l;
    i=(UINT32)(fgetc(fp) & 0xff);
    j=(UINT32)(fgetc(fp) & 0xff);
    k=(UINT32)(fgetc(fp) & 0xff);
    l=(UINT32)(fgetc(fp) & 0xff);
    if(intel)   return(i+(j<<8)+(k<<16)+(l<<24));
    else  return(l+(k<<8)+(j<<16)+(i<<24));
}

static int fputWord(UINT16 wd, FILE *fp, BYTE intel)
{
    int rtn;
    if(intel) {
        rtn = fputc(wd&0xff, fp);
        rtn = fputc((wd>>8)&0xff, fp);
    }
    else {
        rtn = fputc((wd>>8)&0xff, fp);
        rtn = fputc(wd&0xff, fp);
    }
    return rtn;
}

static int fputLong(UINT32 dw, FILE *fp, BYTE intel)
{
    int rtn;
    if(intel) {
        rtn = fputc(dw&0xff, fp);
        rtn = fputc((dw>>8)&0xff, fp);
        rtn = fputc((dw>>16)&0xff, fp);
        rtn = fputc((dw>>24)&0xff, fp);
    }
    else {
        rtn = fputc((dw>>24)&0xff, fp);
        rtn = fputc((dw>>16)&0xff, fp);
        rtn = fputc((dw>>8)&0xff, fp);
        rtn = fputc(dw&0xff, fp);
    }
    return rtn;
}



typedef struct tagBITMAPINFOHEADER
{
        UINT32      biSize;
        INT32       biWidth;
        INT32       biHeight;
        UINT16       biPlanes;
        UINT16       biBitCount;
        UINT32      biCompression;
        UINT32      biSizeImage;
        INT32       biXPelsPerMeter;
        INT32       biYPelsPerMeter;
        UINT32      biClrUsed;
        UINT32      biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagBITMAPFILEHEADER 
{
        BYTE     bfType[2];
        UINT32   bfSize;
        UINT16    bfReserved1;
        UINT16    bfReserved2;
        UINT32   bfOffBits;
} BITMAPFILEHEADER;


// save image as a Windows BMP 24 bits image
//  only 16/32 bit color image is supported
// return 0 if successful
//   -1 on fail

int gfx_SaveBMP24b(const char * fname, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT uStartX, UINT uStartY, UINT uWidth, UINT uHeight)
{
    UINT32    offset;
    BYTE * buf;
    int i, j, k;
    FILE * fp;
    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;
    UINT  mr, sr, mg, sg, mb, sb;

    if(!pSurface || !fname || !fname[0] )
        return -1;
    if( 
#ifdef GFX_SURFACE_ARGB_1555
        pSurface->uPlaneConfig != GFX_SURFACE_ARGB_1555 &&
#endif
#ifdef GFX_SURFACE_ARGB_4444
        pSurface->uPlaneConfig != GFX_SURFACE_ARGB_4444 &&
#endif
#ifdef GFX_SURFACE_RGB_565
        pSurface->uPlaneConfig != GFX_SURFACE_RGB_565 &&
#endif
        pSurface->uPlaneConfig != GFX_SURFACE_ARGB_8888 )
    {
        return -1;
    }
    
    if(gfx_ClipRect(pSurface->plane[0].uWidth, pSurface->plane[0].uHeight, uStartX, uStartY, &uWidth, &uHeight))
    {
        return -1;  // null clip
    }

    if ((buf = (BYTE *)malloc((uWidth*3L+3)&0xffffffc)) == NULL)
        return -1;
    fp=fopen(fname, "wb");
    if(fp == NULL) 
    {
        free(buf); 
        return -1;
    }

    fhead.bfType[0] = 'B';
    fhead.bfType[1] = 'M';
    fhead.bfSize = 0x36L+((uWidth*3L+3)&0xffffffc)*uHeight;
    fhead.bfReserved1 = 0;
    fhead.bfReserved2 = 0;
    fhead.bfOffBits= 0x36L;
    ihead.biSize = 0x28L;
    ihead.biWidth = (INT32)uWidth;
    ihead.biHeight = (INT32)uHeight;
    ihead.biPlanes = 1;
    ihead.biBitCount = 24;
    ihead.biCompression = 0L;
    ihead.biXPelsPerMeter=2835;  // 72 dpi
    ihead.biYPelsPerMeter=2835;
    ihead.biClrUsed=0;
    ihead.biClrImportant=0;
    ihead.biSizeImage  = ((uWidth*3L+3)&0xffffffc)*uHeight;
    
    // write filehead 
    fputc(fhead.bfType[0], fp);
    fputc(fhead.bfType[1], fp);
    fputLong(fhead.bfSize, fp, 1);
    fputWord(fhead.bfReserved1, fp, 1);
    fputWord(fhead.bfReserved2, fp, 1);
    fputLong(fhead.bfOffBits, fp, 1);
    
    // write infohead 
    fputLong(ihead.biSize, fp, 1);
    fputLong((UINT32)ihead.biWidth, fp, 1);
    fputLong((UINT32)ihead.biHeight, fp, 1);
    fputWord(ihead.biPlanes, fp, 1);
    fputWord(ihead.biBitCount, fp, 1);
    fputLong(ihead.biCompression, fp, 1);
    fputLong(ihead.biSizeImage, fp, 1);
    fputLong((UINT32)ihead.biXPelsPerMeter, fp, 1);
    fputLong((UINT32)ihead.biYPelsPerMeter, fp, 1);
    fputLong(ihead.biClrUsed, fp, 1);
    fputLong(ihead.biClrImportant, fp, 1);

    mr = 0xff >> (8-pSurface->plane[0].r.uNumbits);
    sr = pSurface->plane[0].r.uOffset;
    mg = 0xff >> (8-pSurface->plane[0].g.uNumbits);
    sg = pSurface->plane[0].g.uOffset;
    mb = 0xff >> (8-pSurface->plane[0].b.uNumbits);
    sb = pSurface->plane[0].b.uOffset;

    for(i=uHeight-1; i>=0; i--)
    {
        offset = pSurface->plane[0].uBytePerLine * ((UINT32)i+uStartY) + 
                 pSurface->plane[0].uPixelSize/8 * uStartX;
        switch(pSurface->uPlaneConfig)
        {
#if defined(GFX_SURFACE_ARGB_1555) || defined(GFX_SURFACE_ARGB_4444) || defined(GFX_SURFACE_RGB_565)
#ifdef GFX_SURFACE_ARGB_1555
        case GFX_SURFACE_ARGB_1555:
#endif
#ifdef GFX_SURFACE_ARGB_4444
        case GFX_SURFACE_ARGB_4444:
#endif
#ifdef GFX_SURFACE_RGB_565
        case GFX_SURFACE_RGB_565:
#endif
            {
                UINT16 *pImg = (UINT16 *)((BYTE *)pSurface->plane[0].pPlane + offset);
                for(j=0, k=0; j<uWidth; j++, k+=3)
                {
                     buf[k  ] = (BYTE)((pImg[j]>>sb)&mb);
                     buf[k+1] = (BYTE)((pImg[j]>>sg)&mg);
                     buf[k+2] = (BYTE)((pImg[j]>>sr)&mr);
                }
            }
#endif
        case GFX_SURFACE_ARGB_8888:
            {
                UINT32 *pImg = (UINT32 *)((BYTE *)pSurface->plane[0].pPlane + offset);
                for(j=0, k=0; j<uWidth; j++, k+=3)
                {
                     buf[k  ] = (BYTE)((pImg[j]>>sb)&mb);
                     buf[k+1] = (BYTE)((pImg[j]>>sg)&mg);
                     buf[k+2] = (BYTE)((pImg[j]>>sr)&mr);
                }
            }
        }
        fwrite(buf, (uWidth*3L+3)&0xffffffc, 1, fp);
    }
            
  fclose(fp);
  free(buf);
  return 0;
}


// save image as a Windows BMP 8 bits image
//  conversion will be added if needed.
// return 0 if successful

int gfx_SaveBMP8b(const char * fname, GFX_SURFACE_LOCK_INFO_T *pSurface, GFX_PALETTE_T *pPal, UINT uStartX, UINT uStartY, UINT uWidth, UINT uHeight)
{
    UINT32    offset, uJust;
    BYTE * buf;
    int i;
    FILE * fp;
    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;

    if(!pSurface || !fname || !fname[0] )
        return -1;
    if( pSurface->uPlaneConfig != GFX_SURFACE_CLUT8BPP_ARGB &&
        pSurface->uPlaneConfig != GFX_SURFACE_RAW8BPP)
    {
        return -1;
    }
    
    if(gfx_ClipRect(pSurface->plane[0].uWidth, pSurface->plane[0].uHeight, uStartX, uStartY, &uWidth, &uHeight))
    {
        return -1;  // null clip
    }

    uJust = (uWidth&3) ? 4 - (uWidth&3) : 0;

    if ((buf = (BYTE *)malloc(1024)) == NULL)
        return -1;

    fp=fopen(fname, "wb");
    if(fp == NULL) 
    {
        free(buf);
        return -1;
    }

    ihead.biSize = 0x28L;
    ihead.biWidth = (INT32)uWidth;
    ihead.biHeight = (INT32)uHeight;
    ihead.biPlanes = 1;
    ihead.biBitCount = 8;
    ihead.biCompression = 0L;
    ihead.biXPelsPerMeter=2835;  // 72 dpi
    ihead.biYPelsPerMeter=2835;
    ihead.biClrUsed=256;
    ihead.biClrImportant=0;
    ihead.biSizeImage  = (((long)uWidth+3)&0xfffffffc)*uHeight;
    
    fhead.bfType[0] = 'B';
    fhead.bfType[1] = 'M';
    fhead.bfReserved1 = 0;
    fhead.bfReserved2 = 0;
    fhead.bfSize = 0x36L+ihead.biSizeImage+ihead.biClrUsed*4;
    fhead.bfOffBits= 0x36L+ihead.biClrUsed*4;
    
    // write filehead 
    fputc(fhead.bfType[0], fp);
    fputc(fhead.bfType[1], fp);
    fputLong(fhead.bfSize, fp, 1);
    fputWord(fhead.bfReserved1, fp, 1);
    fputWord(fhead.bfReserved2, fp, 1);
    fputLong(fhead.bfOffBits, fp, 1);
    
    // write infohead 
    fputLong(ihead.biSize, fp, 1);
    fputLong((UINT32)ihead.biWidth, fp, 1);
    fputLong((UINT32)ihead.biHeight, fp, 1);
    fputWord(ihead.biPlanes, fp, 1);
    fputWord(ihead.biBitCount, fp, 1);
    fputLong(ihead.biCompression, fp, 1);
    fputLong(ihead.biSizeImage, fp, 1);
    fputLong((UINT32)ihead.biXPelsPerMeter, fp, 1);
    fputLong((UINT32)ihead.biYPelsPerMeter, fp, 1);
    fputLong(ihead.biClrUsed, fp, 1);
    fputLong(ihead.biClrImportant, fp, 1);

    if(pPal)
    {
        for(i=0; i<256; i++) 
        {
            buf[i*4+3] = pPal[i].a;  
            buf[i*4+2] = pPal[i].r;
            buf[i*4+1] = pPal[i].g;
            buf[i*4+0] = pPal[i].b;
        }
    }
    else
    {
        for(i=0; i<256; i++) 
        {
            buf[i*4+3] = 255;  
            buf[i*4+2] = i;
            buf[i*4+1] = i;
            buf[i*4+0] = i;
        }
    }
    fwrite(buf, 1024, 1, fp);        // write palette
    memset(buf, 0, 4);
    
    for(i=uHeight-1; i>=0; i--)
    {
        offset = pSurface->plane[0].uBytePerLine * ((UINT32)i+uStartY) + 
                 pSurface->plane[0].uPixelSize/8 * uStartX;
        fwrite((BYTE *)pSurface->plane[0].pPlane + offset, uWidth, 1, fp);
        if(uJust)
            fwrite(buf, uJust, 1, fp);
    }          
    fclose(fp);
    free(buf);
    return 0;
}




// load Windows BMP 24 bits files
// Return 0 if successful

int gfx_LoadBMP24b(GFX_SURFACE_LOCK_INFO_T *pSurface, UINT uDesX, UINT uDesY, UINT uWidth, UINT uHeight, const char * fname, UINT uSrcX, UINT uSrcY, BYTE alpha)
{
    UINT32    offsets, offsetd, uJust;
    BYTE * buf;
    int i, j, k;
    FILE * fp;
    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;
    UINT  rr, sr, rg, sg, rb, sb;
    int reverse = 0;

    if(!pSurface || !fname || !fname[0] )
        return -1;

    if( 
#ifdef GFX_SURFACE_ARGB_1555
        pSurface->uPlaneConfig != GFX_SURFACE_ARGB_1555 &&
#endif
#ifdef GFX_SURFACE_ARGB_4444
        pSurface->uPlaneConfig != GFX_SURFACE_ARGB_4444 &&
#endif
#ifdef GFX_SURFACE_RGB_565
        pSurface->uPlaneConfig != GFX_SURFACE_RGB_565 &&
#endif
        pSurface->uPlaneConfig != GFX_SURFACE_ARGB_8888 )
    {
        return -1;
    }
    
    fp=fopen(fname, "rb");
    if(fp == NULL)
        return -1; 

    // read filehead 
    fhead.bfType[0] = fgetc(fp);
    fhead.bfType[1] = fgetc(fp);
    fhead.bfSize = fgetLong(fp, 1);
    fhead.bfReserved1 = fgetWord(fp, 1);
    fhead.bfReserved2 = fgetWord(fp, 1);
    fhead.bfOffBits = fgetLong(fp, 1);
    
    // read infohead 
    ihead.biSize = fgetLong(fp, 1);
    ihead.biWidth = (INT32)fgetLong(fp, 1);
    ihead.biHeight = (INT32)fgetLong(fp, 1);
    ihead.biPlanes = fgetWord(fp, 1);
    ihead.biBitCount = fgetWord(fp, 1);
    ihead.biCompression = fgetLong(fp, 1);
    ihead.biSizeImage = fgetLong(fp, 1);
    ihead.biXPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biYPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biClrUsed = fgetLong(fp, 1);
    ihead.biClrImportant = fgetLong(fp, 1);
    
    if (fhead.bfType[0] != 'B' ||
        fhead.bfType[1] != 'M'   // 'BM'
        || ihead.biPlanes != 1
        || ihead.biBitCount != 24
        || ihead.biCompression != 0L)  // not a valid 24bits BMP file
    {
        fclose(fp); 
        return -1;
    }
   
    if(ihead.biHeight < 0) {ihead.biHeight = -ihead.biHeight; reverse = 1; }
    
    if(gfx_ClipBLTRect(ihead.biWidth, ihead.biHeight, 
        pSurface->plane[0].uWidth, pSurface->plane[0].uHeight, 
        uSrcX, uSrcY, uDesX, uDesY, &uWidth, &uHeight))
    {
        fclose(fp);
        return -1;  // null clip
    }
    
    uJust = (ihead.biWidth*3L+3)&0xffffffc;
    if ((buf = (BYTE *)malloc(uJust)) == NULL)
    {
        fclose(fp);
        return -1;
    }
    
    rr = (8-pSurface->plane[0].r.uNumbits);
    sr = pSurface->plane[0].r.uOffset;
    rg = (8-pSurface->plane[0].g.uNumbits);
    sg = pSurface->plane[0].g.uOffset;
    rb = (8-pSurface->plane[0].b.uNumbits);
    sb = pSurface->plane[0].b.uOffset;

    for(i=0; i<uHeight; i++)
    {
        if(reverse)
        {
            offsetd = pSurface->plane[0].uBytePerLine * ((UINT32)i+uDesY) + 
                pSurface->plane[0].uPixelSize/8 * uDesX;
        }
        else
        {
            offsetd = pSurface->plane[0].uBytePerLine * ((UINT32)uHeight-1 - i + uDesY) + 
                pSurface->plane[0].uPixelSize/8 * uDesX;
        }
        
        offsets = fhead.bfOffBits + uJust*(i+ihead.biHeight-uHeight-uSrcY) + uSrcX*3;
        fseek(fp, offsets, SEEK_SET);
        fread(buf, uWidth*3, 1, fp);
        
        switch(pSurface->uPlaneConfig)
        {
#if defined(GFX_SURFACE_ARGB_1555) || defined(GFX_SURFACE_ARGB_4444) || defined(GFX_SURFACE_RGB_565)
#ifdef GFX_SURFACE_ARGB_1555
        case GFX_SURFACE_ARGB_1555:
#endif
#ifdef GFX_SURFACE_ARGB_4444
        case GFX_SURFACE_ARGB_4444:
#endif
#ifdef GFX_SURFACE_RGB_565
        case GFX_SURFACE_RGB_565:
#endif
            {
                UINT16 *pImg = (UINT16 *)((BYTE *)pSurface->plane[0].pPlane + offsetd);
                UINT16 ua =  (UINT16)(alpha >> (8-pSurface->plane[0].a.uNumbits)) << pSurface->plane[0].a.uOffset;
                for(j=0, k=0; j<uWidth; j++, k+=3)
                {
                    pImg[j] = ua | 
                        ((UINT16)(buf[k  ] >> rb)<<sb) | 
                        ((UINT16)(buf[k+1] >> rg)<<sg) |
                        ((UINT16)(buf[k+2] >> rr)<<sr) ;
                }
            }
#endif
        case GFX_SURFACE_ARGB_8888:
            {
                UINT32 *pImg = (UINT32 *)((BYTE *)pSurface->plane[0].pPlane + offsetd);
                UINT32 ua =  (UINT32)(alpha >> (8-pSurface->plane[0].a.uNumbits)) << pSurface->plane[0].a.uOffset;
                for(j=0, k=0; j<uWidth; j++, k+=3)
                {
                    pImg[j] = ua | 
                        ((UINT32)(buf[k  ] >> rb)<<sb) | 
                        ((UINT32)(buf[k+1] >> rg)<<sg) |
                        ((UINT32)(buf[k+2] >> rr)<<sr) ;
                }
            }
        }
    }

    fclose(fp);
    free(buf);
    return 0;
}



// load Windows BMP 8 bits files
// Return 1 if successful

int gfx_LoadBMP8b(GFX_SURFACE_LOCK_INFO_T *pSurface, GFX_PALETTE_T *pPal, UINT uDesX, UINT uDesY, UINT uWidth, UINT uHeight, const char * fname, UINT uSrcX, UINT uSrcY, BYTE alpha)
{
    UINT32  offsets, offsetd, uJust;
    BYTE *buf;
    int  i;
    FILE *fp;
    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;
    int reverse=0;
    
    if(!pSurface || !fname || !fname[0] )
        return -1;

    if( pSurface->uPlaneConfig != GFX_SURFACE_CLUT8BPP_ARGB &&
        pSurface->uPlaneConfig != GFX_SURFACE_RAW8BPP)
    {
        return -1;
    }
    
    fp=fopen(fname, "rb");
    if(fp == NULL)
        return -1; 
    
    // read filehead 
    fhead.bfType[0] = fgetc(fp);
    fhead.bfType[1] = fgetc(fp);
    fhead.bfSize = fgetLong(fp, 1);
    fhead.bfReserved1 = fgetWord(fp, 1);
    fhead.bfReserved2 = fgetWord(fp, 1);
    fhead.bfOffBits = fgetLong(fp, 1);
    
    // read infohead 
    ihead.biSize = fgetLong(fp, 1);
    ihead.biWidth = (INT32)fgetLong(fp, 1);
    ihead.biHeight = (INT32)fgetLong(fp, 1);
    ihead.biPlanes = fgetWord(fp, 1);
    ihead.biBitCount = fgetWord(fp, 1);
    ihead.biCompression = fgetLong(fp, 1);
    ihead.biSizeImage = fgetLong(fp, 1);
    ihead.biXPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biYPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biClrUsed = fgetLong(fp, 1);
    ihead.biClrImportant = fgetLong(fp, 1);
    
    if (fhead.bfType[0] != 'B' ||
        fhead.bfType[1] != 'M'    // 'BM'
        || ihead.biPlanes != 1
        || ihead.biBitCount != 8
        || ihead.biCompression != 0L 
        || ihead.biClrUsed > 256     )  // not a valid 8bits BMP file
    {
        fclose(fp); 
        return -1;
    }
   
    if(ihead.biHeight < 0) {ihead.biHeight = -ihead.biHeight; reverse = 1; }

    if(gfx_ClipBLTRect(ihead.biWidth, ihead.biHeight, 
        pSurface->plane[0].uWidth, pSurface->plane[0].uHeight, 
        uSrcX, uSrcY, uDesX, uDesY, &uWidth, &uHeight))
    {
        fclose(fp);
        return -1;  // null clip
    }

    if(pPal)
    {
        if((buf = (BYTE *)malloc(256*4)) == NULL) 
        {
            fclose(fp);
            return -1;
        }   // can not allocate buffer
        
        if(ihead.biClrUsed == 0) ihead.biClrUsed = 256;  
        
        fread(buf, ihead.biClrUsed*4, 1, fp);
        for(i=0; i<(int)ihead.biClrUsed; i++)
        {
            pPal[i].a = alpha;
            pPal[i].r = buf[i*4+2];
            pPal[i].g = buf[i*4+1];
            pPal[i].b = buf[i*4+0];
        }
        
        for(; i<256; i++) 
        {
            pPal[i].a = 0;
            pPal[i].r = 0;
            pPal[i].g = 0;
            pPal[i].b = 0;
        }
        
        free(buf);
    }

    uJust = (ihead.biWidth+3)&0xffffffc;

    for(i=0; i<uHeight; i++)
    {
        if(reverse)
        {
            offsetd = pSurface->plane[0].uBytePerLine * ((UINT32)i+uDesY) + 
                pSurface->plane[0].uPixelSize/8 * uDesX;
        }
        else
        {
            offsetd = pSurface->plane[0].uBytePerLine * ((UINT32)uHeight-1 - i + uDesY) + 
                pSurface->plane[0].uPixelSize/8 * uDesX;
        }
        
        offsets = fhead.bfOffBits + uJust*(i+ihead.biHeight-uSrcY-uHeight) + uSrcX;
        fseek(fp, offsets, SEEK_SET);
        fread((BYTE *)pSurface->plane[0].pPlane + offsetd, uWidth, 1, fp);
    }
    
    fclose(fp);
    return 0;
}



int gfx_LoadBMP4b(GFX_SURFACE_LOCK_INFO_T *pSurface, GFX_PALETTE_T *pPal, unsigned int uDesX, unsigned int uDesY, unsigned int uWidth, unsigned int uHeight, const char * fname, unsigned int uSrcX, unsigned int uSrcY, BYTE alpha)
{
    UINT32  offsets, offsetd, uJust;
    BYTE *buf;
    int  i;
    FILE *fp;
    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;
    int reverse=0;
    
    if(!pSurface || !fname || !fname[0] )
        return -1;

    if( pSurface->uPlaneConfig != GFX_SURFACE_CLUT4BPP_ARGB 
#ifdef GFX_SURFACE_RAW4BPP
        &&  pSurface->uPlaneConfig != GFX_SURFACE_RAW4BPP
#endif
        )
    {
        return -1;
    }
    
    fp=fopen(fname, "rb");
    if(fp == NULL)
        return -1; 
    
    // read filehead 
    fhead.bfType[0] = fgetc(fp);
    fhead.bfType[1] = fgetc(fp);
    fhead.bfSize = fgetLong(fp, 1);
    fhead.bfReserved1 = fgetWord(fp, 1);
    fhead.bfReserved2 = fgetWord(fp, 1);
    fhead.bfOffBits = fgetLong(fp, 1);
    
    // read infohead 
    ihead.biSize = fgetLong(fp, 1);
    ihead.biWidth = (INT32)fgetLong(fp, 1);
    ihead.biHeight = (INT32)fgetLong(fp, 1);
    ihead.biPlanes = fgetWord(fp, 1);
    ihead.biBitCount = fgetWord(fp, 1);
    ihead.biCompression = fgetLong(fp, 1);
    ihead.biSizeImage = fgetLong(fp, 1);
    ihead.biXPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biYPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biClrUsed = fgetLong(fp, 1);
    ihead.biClrImportant = fgetLong(fp, 1);
    
    if (fhead.bfType[0] != 'B' ||
        fhead.bfType[1] != 'M'    // 'BM'
        || ihead.biPlanes != 1
        || ihead.biBitCount != 4
        || ihead.biCompression != 0L 
        || ihead.biClrUsed > 16     )  // not a valid 4bits BMP file
    {
        fclose(fp); 
        return -1;
    }
   
    if(ihead.biHeight < 0) {ihead.biHeight = -ihead.biHeight; reverse = 1; }


    uSrcX &= 0xfffffffe;
    uDesX &= 0xfffffffe;
    uWidth = (uWidth + 1)&0xfffffffe;
    
    if(gfx_ClipBLTRect(ihead.biWidth, ihead.biHeight, 
        pSurface->plane[0].uWidth, pSurface->plane[0].uHeight, 
        uSrcX, uSrcY, uDesX, uDesY, &uWidth, &uHeight))
    {
        fclose(fp);
        return -1;  // null clip
    }

    if(pPal)
    {
        if((buf = (BYTE *)malloc(16*4)) == NULL) 
        {
            fclose(fp);
            return -1;
        }   // can not allocate buffer
        
        if(ihead.biClrUsed == 0) ihead.biClrUsed = 16;  
        
        fread(buf, ihead.biClrUsed*4, 1, fp);
        for(i=0; i<(int)ihead.biClrUsed; i++)
        {
            pPal[i].a = alpha;
            pPal[i].r = buf[i*4+2];
            pPal[i].g = buf[i*4+1];
            pPal[i].b = buf[i*4+0];
        }
        
        for(; i<16; i++) 
        {
            pPal[i].a = 0;
            pPal[i].r = 0;
            pPal[i].g = 0;
            pPal[i].b = 0;
        }
        
        free(buf);
    }

    uJust = ((ihead.biWidth+1)/2+3)&0xffffffc;

    for(i=0; i<uHeight; i++)
    {
        if(reverse)
        {
            offsetd = pSurface->plane[0].uBytePerLine * ((UINT32)i+uDesY) + 
                pSurface->plane[0].uPixelSize * uDesX / 8;
        }
        else
        {
            offsetd = pSurface->plane[0].uBytePerLine * ((UINT32)uHeight-1 - i + uDesY) + 
                pSurface->plane[0].uPixelSize * uDesX / 8;
        }
        
        offsets = fhead.bfOffBits + uJust*(i+ihead.biHeight-uSrcY-uHeight) + uSrcX/2;
        fseek(fp, offsets, SEEK_SET);
        fread((BYTE *)pSurface->plane[0].pPlane + offsetd, (uWidth+1)/2, 1, fp);
    }
    
    fclose(fp);
    return 0;
}


int gfx_LoadBMP1b(GFX_SURFACE_LOCK_INFO_T *pSurface, GFX_PALETTE_T *pPal, unsigned int uDesX, unsigned int uDesY, unsigned int uWidth, unsigned int uHeight, const char * fname, unsigned int uSrcX, unsigned int uSrcY, BYTE alpha)
{
    UINT32  offsets, offsetd, uJust;
    BYTE *buf;
    int  i;
    FILE *fp;
    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;
    int reverse=0;
    
    if(!pSurface || !fname || !fname[0] )
        return -1;

    if(
#ifdef GFX_SURFACE_CLUT1BPP_ARGB
        pSurface->uPlaneConfig != GFX_SURFACE_CLUT1BPP_ARGB &&
#endif
        pSurface->uPlaneConfig != GFX_SURFACE_RAW1BPP)
    {
        return -1;
    }
    
    fp=fopen(fname, "rb");
    if(fp == NULL)
        return -1; 
    
    // read filehead 
    fhead.bfType[0] = fgetc(fp);
    fhead.bfType[1] = fgetc(fp);
    fhead.bfSize = fgetLong(fp, 1);
    fhead.bfReserved1 = fgetWord(fp, 1);
    fhead.bfReserved2 = fgetWord(fp, 1);
    fhead.bfOffBits = fgetLong(fp, 1);
    
    // read infohead 
    ihead.biSize = fgetLong(fp, 1);
    ihead.biWidth = (INT32)fgetLong(fp, 1);
    ihead.biHeight = (INT32)fgetLong(fp, 1);
    ihead.biPlanes = fgetWord(fp, 1);
    ihead.biBitCount = fgetWord(fp, 1);
    ihead.biCompression = fgetLong(fp, 1);
    ihead.biSizeImage = fgetLong(fp, 1);
    ihead.biXPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biYPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biClrUsed = fgetLong(fp, 1);
    ihead.biClrImportant = fgetLong(fp, 1);
    
    if (fhead.bfType[0] != 'B' ||
        fhead.bfType[1] != 'M'    // 'BM'
        || ihead.biPlanes != 1
        || ihead.biBitCount != 1
        || ihead.biCompression != 0L 
        || ihead.biClrUsed > 2     )  // not a valid 1bits BMP file
    {
        fclose(fp); 
        return -1;
    }
   
    if(ihead.biHeight < 0) {ihead.biHeight = -ihead.biHeight; reverse = 1; }

    uSrcX &= 0xfffffff8;
    uDesX &= 0xfffffff8;
    uWidth = (uWidth + 7)&0xfffffff8;

    if(gfx_ClipBLTRect(ihead.biWidth, ihead.biHeight, 
        pSurface->plane[0].uWidth, pSurface->plane[0].uHeight, 
        uSrcX, uSrcY, uDesX, uDesY, &uWidth, &uHeight))
    {
        fclose(fp);
        return -1;  // null clip
    }

    if(pPal)
    {
        if((buf = (BYTE *)malloc(2*4)) == NULL) 
        {
            fclose(fp);
            return -1;
        }   // can not allocate buffer
        
        if(ihead.biClrUsed == 0) ihead.biClrUsed = 2;  
        
        fread(buf, ihead.biClrUsed*4, 1, fp);
        for(i=0; i<(int)ihead.biClrUsed; i++)
        {
            pPal[i].a = alpha;
            pPal[i].r = buf[i*4+2];
            pPal[i].g = buf[i*4+1];
            pPal[i].b = buf[i*4+0];
        }
        
        for(; i<2; i++) 
        {
            pPal[i].a = 0;
            pPal[i].r = 0;
            pPal[i].g = 0;
            pPal[i].b = 0;
        }
        
        free(buf);
    }

    uJust = ((ihead.biWidth+7)/8+3)&0xffffffc;

    for(i=0; i<uHeight; i++)
    {
        if(reverse)
        {
            offsetd = pSurface->plane[0].uBytePerLine * ((UINT32)i+uDesY) + 
                pSurface->plane[0].uPixelSize * uDesX / 8;
        }
        else
        {
            offsetd = pSurface->plane[0].uBytePerLine * ((UINT32)uHeight-1 - i + uDesY) + 
                pSurface->plane[0].uPixelSize * uDesX / 8;
        }
        
        offsets = fhead.bfOffBits + uJust*(i+ihead.biHeight-uSrcY-uHeight) + uSrcX/8;
        fseek(fp, offsets, SEEK_SET);
        fread((BYTE *)pSurface->plane[0].pPlane + offsetd, (uWidth+7)/8, 1, fp);
    }
    
    fclose(fp);
    return 0;
}



int gfx_GetBMPInfo(const char * fname, UINT *upWidth, UINT *upHeight, UINT *upPixelSize)
{
    FILE *fp;
    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;
    
    if(!fname || !fname[0])
        return -1;

    fp=fopen(fname, "rb");
    if(fp == NULL)
        return -1; 
    
    // read filehead 
    fhead.bfType[0] = fgetc(fp);
    fhead.bfType[1] = fgetc(fp);
    fhead.bfSize = fgetLong(fp, 1);
    fhead.bfReserved1 = fgetWord(fp, 1);
    fhead.bfReserved2 = fgetWord(fp, 1);
    fhead.bfOffBits = fgetLong(fp, 1);
    
    // read infohead 
    ihead.biSize = fgetLong(fp, 1);
    ihead.biWidth = (INT32)fgetLong(fp, 1);
    ihead.biHeight = (INT32)fgetLong(fp, 1);
    ihead.biPlanes = fgetWord(fp, 1);
    ihead.biBitCount = fgetWord(fp, 1);
    ihead.biCompression = fgetLong(fp, 1);
    ihead.biSizeImage = fgetLong(fp, 1);
    ihead.biXPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biYPelsPerMeter = (INT32)fgetLong(fp, 1);
    ihead.biClrUsed = fgetLong(fp, 1);
    ihead.biClrImportant = fgetLong(fp, 1);

    fclose(fp); 
    
    if (fhead.bfType[0] != 'B' ||
        fhead.bfType[1] != 'M'    // 'BM'
        || ihead.biPlanes != 1
        || ihead.biCompression != 0L 
        )  // not a valid 8bits BMP file
    {
        return -1;
    }
   
    if(ihead.biHeight < 0) {ihead.biHeight = -ihead.biHeight; }

    if(upWidth) *upWidth = ihead.biWidth;
    if(upHeight) *upHeight = ihead.biHeight;
    if(upPixelSize) *upPixelSize = ihead.biBitCount;
    return 0;
}


// Load BMP and create an surface the same type of that BMP
// return the surface handle on success !!!
//    if pSurface != NULL the surface will be locked and return the lock info in pSurface
//    else the surface is not locked
//    if pWidth / pHeight is not NULL,  the width / height value will be returned
// return -1 on failure 

int gfx_LoadBMP_Surface(int fdGfxDev, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT *pWidth, UINT *pHeight, const char * fname, BYTE alpha)
{
	unsigned int uW, uH, uP;
    int hbmp, rtn;
    GFX_SURFACE_LOCK_INFO_T sbmp;
    GFX_PALETTE_T *pPal = NULL;

	if(gfx_GetBMPInfo(fname, &uW, &uH, &uP) < 0)
	{
        // invalid bmp
		return -1;
	}

    if(uP <= 8) // palette bmp
    {
        pPal = malloc(256*sizeof(GFX_PALETTE_T));
        if(!pPal)
        {
            return -1;
        }
    }

    hbmp = -1;

    switch(uP)
    {
    case 1:
#ifdef GFX_SURFACE_CLUT1BPP_ARGB
        hbmp = gfx_create_surface(fdGfxDev, uW, uH, GFX_SURFACE_CLUT1BPP_ARGB, GFX_VDEV_NULL, 0);
#elif defined(GFX_SURFACE_RAW1BPP)
        hbmp = gfx_create_surface(fdGfxDev, uW, uH, GFX_SURFACE_RAW1BPP, GFX_VDEV_NULL, 0);
#endif

        break;
    case 4:
        hbmp = gfx_create_surface(fdGfxDev, uW, uH, GFX_SURFACE_CLUT4BPP_ARGB, GFX_VDEV_NULL, 0);
        break;
    case 8:
        hbmp = gfx_create_surface(fdGfxDev, uW, uH, GFX_SURFACE_CLUT8BPP_ARGB, GFX_VDEV_NULL, 0);
        break;
    case 24:
        hbmp = gfx_create_surface(fdGfxDev, uW, uH, GFX_SURFACE_ARGB_8888, GFX_VDEV_NULL, 0);
        break;
    default:    // unsupported BMP format
        free(pPal);
        return -1;
    }
    if(hbmp < 0)
    {
        if(pPal) free(pPal);
        return -1;
    }

    if(gfx_lock_surface(fdGfxDev, hbmp, &sbmp) < 0)
    {
        gfx_destroy_surface(fdGfxDev, hbmp);
        if(pPal) free(pPal);
        return -1;
    }

    switch(uP)
    {
    case 1:
        rtn = gfx_LoadBMP1b(&sbmp, pPal, 0, 0, uW, uH, fname, 0, 0, alpha);
        break;
    case 4:
        rtn = gfx_LoadBMP4b(&sbmp, pPal, 0, 0, uW, uH, fname, 0, 0, alpha);
        break;
    case 8:
        rtn = gfx_LoadBMP8b(&sbmp, pPal, 0, 0, uW, uH, fname, 0, 0, alpha);
        break;
    case 24:
        rtn = gfx_LoadBMP24b(&sbmp, 0, 0, uW, uH, fname, 0, 0, alpha);
        break;
    }
    if(rtn < 0)
    {
        if(pPal) free(pPal);
        gfx_unlock_surface(fdGfxDev, hbmp, &sbmp);
        gfx_destroy_surface(fdGfxDev, hbmp);
        return -1;
    }
    if(uP <= 8) // palette bmp
    {
        gfx_set_surface_palette(fdGfxDev, hbmp, 0, 1<<uP, pPal);
        free(pPal);
    }
    if(pSurface)
    {
        memcpy(pSurface, &sbmp, sizeof(GFX_SURFACE_LOCK_INFO_T));
    }
    else
    {
        gfx_unlock_surface(fdGfxDev, hbmp, &sbmp);
    }
    if(pWidth) *pWidth = uW;
    if(pHeight) *pHeight = uH;
    return hbmp;
}


//  targa 32bits ARGB format support, just for use of the alpha channel :-(

/*  Targa file header*/
typedef struct __TARGAHDR__ {
        BYTE        bIDFieldSize;           /* Characters in ID field */
        BYTE        bClrMapType;            /* Color map type */
        BYTE        bImageType;             /* Image type */
        BYTE        lClrMapSpec[5];         /* Color map specification */
        UINT16      wXOrigin;               /* X origin */
        UINT16      wYOrigin;               /* Y origin */
        UINT16      wWidth;                 /* Bitmap width */
        UINT16      wHeight;                /* Bitmap height */
        BYTE        bBitsPixel;             /* Bits per pixel */
        BYTE        bImageDescriptor;       /* Image descriptor */
} TARGA_HDR;


int gfx_LoadTGA32b_Surface(int fdGfxDev, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT *pWidth, UINT *pHeight, const char * fname)
{
    FILE *fp;
    TARGA_HDR thead;
    int htga;
    int x, y;
    GFX_SURFACE_LOCK_INFO_T stga;

    if(!fname || !fname[0])
        return -1;

    fp=fopen(fname, "rb");
    if(fp == NULL)
        return -1; 

    thead.bIDFieldSize = (BYTE)fgetc(fp);
    thead.bClrMapType = (BYTE)fgetc(fp);
    thead.bImageType = (BYTE)fgetc(fp);
    fread(thead.lClrMapSpec, 5, 1, fp); 
    thead.wXOrigin = fgetWord(fp, 1);
    thead.wYOrigin = fgetWord(fp, 1);
    thead.wWidth = fgetWord(fp, 1);
    thead.wHeight = fgetWord(fp, 1);
    thead.bBitsPixel = (BYTE)fgetc(fp);
    thead.bImageDescriptor = (BYTE)fgetc(fp);

    if ((thead.bImageType!=2) || (thead.bBitsPixel!=32) || thead.bClrMapType != 0)     /* not an TGA 32 bits file*/
    {
        fclose(fp); 
        return -1;
    }

    htga = gfx_create_surface(fdGfxDev, (UINT)thead.wWidth, (UINT)thead.wHeight, GFX_SURFACE_ARGB_8888, GFX_VDEV_NULL, 0);

    if(htga < 0)
    {
        fclose(fp);
        return -1;
    }

    if(gfx_lock_surface(fdGfxDev, htga, &stga) < 0)
    {
        gfx_destroy_surface(fdGfxDev, htga);
        fclose(fp);
        return -1;
    }

    fseek(fp, sizeof(thead) + thead.bIDFieldSize, SEEK_SET);

    // the pixel is stored in BGRA byte sequence order, which is just reversed to our format :-(
    // I'm not care the right to left ones, this should be rare
    if(thead.bImageDescriptor & 0x20) 
    {
        // top to bottom
        for(y=0; y<(int)thead.wHeight; y++)
        {
            BYTE *p;
            p = (BYTE *)stga.plane[0].pPlane + stga.plane[0].uBytePerLine*y;
            fread(p, 4, thead.wWidth, fp);
            for(x=0; x<thead.wWidth*4; x+=4)
            {
                BYTE t; 
                t = p[x];
                p[x] = p[x+3];
                p[x+3] = t;
                t = p[x+1];
                p[x+1] = p[x+2];
                p[x+2] = t;
            }
        }
    }
    else
    {
        // bottom to up
        for(y=(int)thead.wHeight-1; y>=0; y--)
        {
            BYTE *p;
            p = (BYTE *)stga.plane[0].pPlane + stga.plane[0].uBytePerLine*y;
            fread(p, 4, thead.wWidth, fp);
            for(x=0; x<thead.wWidth*4; x+=4)
            {
                BYTE t; 
                t = p[x];
                p[x] = p[x+3];
                p[x+3] = t;
                t = p[x+1];
                p[x+1] = p[x+2];
                p[x+2] = t;
            }
        }
    }
    fclose(fp);
    if(pSurface)
    {
        memcpy(pSurface, &stga, sizeof(GFX_SURFACE_LOCK_INFO_T));
    }
    else
    {
        gfx_unlock_surface(fdGfxDev, htga, &stga);
    }
    if(pWidth) *pWidth = thead.wWidth;
    if(pHeight) *pHeight = thead.wHeight;
    return htga;
}


#ifdef __JPEG_LIB_READY__   // IJG's libjpeg 6b

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;


METHODDEF(void)  __my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


int gfx_LoadJPEG_Surface(int fdGfxDev, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT *pWidth, UINT *pHeight, const char * fname, BYTE alpha)
{
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr __jerr;
    FILE * fp;		/* source file */
    BYTE * buf;
    JSAMPROW row_pointer[1];
    BYTE isgray;
    int hjpg;
    GFX_SURFACE_LOCK_INFO_T sjpg;
    int i,j,k;
    
    
    if(!fname || !fname[0])
        return -1;
    
    fp=fopen(fname, "rb");
    if(fp == NULL)
        return -1; 
    
    /* Step 1: allocate and initialize JPEG decompression object */
    __jerr.pub.error_exit = __my_error_exit;
    cinfo.err = jpeg_std_error(&__jerr.pub);
    /* Establish the setjmp return context for my_error_exit to use. */
    buf = NULL;
    hjpg = -1;
    sjpg.uPlaneConfig = 0;
    if (setjmp(__jerr.setjmp_buffer)) 
    {
        /* If we get here, the JPEG code has signaled an error.
        * We need to clean up the JPEG object, close the input file, and return.
        */
        if(sjpg.uPlaneConfig)
        {
            gfx_unlock_surface(fdGfxDev, hjpg, &sjpg);
        }
        if(hjpg >= 0)
        {
            gfx_destroy_surface(fdGfxDev, hjpg);
        }
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        if(buf) free(buf);
        return -1;
    }
    
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);
    
    /* Step 2: specify data source (eg, a file) */
    
    jpeg_stdio_src(&cinfo, fp);
    
    /* Step 3: read file parameters with jpeg_read_header() */
    
    (void) jpeg_read_header(&cinfo, (unsigned char)1);
    
    if(cinfo.num_components == 3 && cinfo.out_color_space == JCS_RGB) 
    {
        hjpg = gfx_create_surface(fdGfxDev, cinfo.image_width, cinfo.image_height, GFX_SURFACE_ARGB_8888, GFX_VDEV_NULL, 0);
        
        if(hjpg < 0)
        {
            jpeg_destroy_decompress(&cinfo);
            fclose(fp); 
            return -1;  // can not allocate enough memory
        }
        isgray = 0;
    }
    else if(cinfo.num_components == 1 && cinfo.out_color_space == JCS_GRAYSCALE) 
    {
        hjpg = gfx_create_surface(fdGfxDev, cinfo.image_width, cinfo.image_height, GFX_SURFACE_CLUT8BPP_ARGB, GFX_VDEV_NULL, 0);
        
        if(hjpg < 0)
        {
            jpeg_destroy_decompress(&cinfo);
            fclose(fp); 
            return -1;  // can not allocate enough memory
        }
        isgray = 1;
    }
    else 
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        return -1;
    }
    
    if(gfx_lock_surface(fdGfxDev, hjpg, &sjpg) < 0)
    {
        gfx_destroy_surface(fdGfxDev, hjpg);
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        return -1;
    }
    
    
    if(isgray) 
    {
        int i;
        GFX_PALETTE_T *pPal = malloc(256*sizeof(GFX_PALETTE_T));
        if(!pPal)
        {
            gfx_unlock_surface(fdGfxDev, hjpg, &sjpg);
            gfx_destroy_surface(fdGfxDev, hjpg);
            jpeg_destroy_decompress(&cinfo);
            fclose(fp);
            return -1;
        }
        for(i=0; i<256; i++)
        {
            pPal[i].a = alpha;
            pPal[i].r = i;
            pPal[i].g = i;
            pPal[i].b = i;
        }
        gfx_set_surface_palette(fdGfxDev, hjpg, 0, 256, pPal);
        free(pPal);
    }  
    else
    {
        if ((buf = (BYTE *)malloc(cinfo.image_width*3)) == NULL) 
        {
            gfx_unlock_surface(fdGfxDev, hjpg, &sjpg);
            gfx_destroy_surface(fdGfxDev, hjpg);
            jpeg_destroy_decompress(&cinfo);
            fclose(fp);
            return -1;
        }   // can not allocate buffer
    }
    
    /* Step 4: set parameters for decompression */
    
    /* Step 5: Start decompressor */
    
    (void) jpeg_start_decompress(&cinfo);
    
    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */
    if(isgray) 
    {
        BYTE * cr;
        cr = (BYTE *)sjpg.plane[0].pPlane; 
        k=0;
        while (cinfo.output_scanline < cinfo.output_height) 
        {
            row_pointer[0] = cr+ k*sjpg.plane[0].uBytePerLine;
            k++;
            (void) jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }
    }
    else 
    {
        BYTE * cr, *base;
        base = (BYTE *)sjpg.plane[0].pPlane;
        row_pointer[0] = buf;
        while (cinfo.output_scanline < cinfo.output_height) 
        {
            (void) jpeg_read_scanlines(&cinfo, row_pointer, 1);
            cr = base;
            for(i=0,j=0, k=0; i<(int)cinfo.image_width; i++) 
            {
                cr[k++] = alpha;
                cr[k++] = buf[j++];
                cr[k++] = buf[j++];
                cr[k++] = buf[j++];
            }
            base += sjpg.plane[0].uBytePerLine;
        }
    }   
    
    /* Step 7: Finish decompression */
    (void) jpeg_finish_decompress(&cinfo);
    
    /* Step 8: Release JPEG decompression object */
    jpeg_destroy_decompress(&cinfo);
    
    if(buf) free(buf);
    fclose(fp);
    
    if(pWidth) *pWidth = sjpg.plane[0].uWidth;
    if(pHeight) *pHeight = sjpg.plane[0].uHeight;

    if(pSurface)
    {
        memcpy(pSurface, &sjpg, sizeof(GFX_SURFACE_LOCK_INFO_T));
    }
    else
    {
        gfx_unlock_surface(fdGfxDev, hjpg, &sjpg);
    }
    return hjpg;
}


#endif



// we are expecting bitmap file distinguished by .bmp or .tga  suffixes

int gfx_LoadBitmap_Surface(int fdGfxDev, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT *pWidth, UINT *pHeight, const char * fname, BYTE alpha)
{
    int lth;
    if(!fname || !fname[0])
        return -1;
    if((lth = strlen(fname)) < 5) return -1;    // we could not decide the suffix

    if(!strcasecmp(&fname[lth-4], ".BMP"))
        return gfx_LoadBMP_Surface(fdGfxDev, pSurface, pWidth, pHeight, fname, alpha);

    if(!strcasecmp(&fname[lth-4], ".TGA"))
        return gfx_LoadTGA32b_Surface(fdGfxDev, pSurface, pWidth, pHeight, fname);

#ifdef __JPEG_LIB_READY__   // IJG's libjpeg 6b

    if(!strcasecmp(&fname[lth-4], ".JPG"))
        return gfx_LoadJPEG_Surface(fdGfxDev, pSurface, pWidth, pHeight, fname, alpha);

#endif

    return -1;  // unrecognized suffix
}
