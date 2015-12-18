//pallas/drv/gfx/test/gfxlib.h
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
//  Nov/07/2001                          Created by YYD

#ifndef __GFX_GFXLIB_H_INC__
#define __GFX_GFXLIB_H_INC__

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
 
#include <stdio.h>

#include "gfx/gfx_inf.h"


typedef struct __GFX_SURFACE_LOCK_INFO_T_STRUCTURE__
{
    unsigned int uPlaneConfig;

    struct __GFX_SURFACE_LOCK_PLANE_INFO_T_STRUCTURE__
    {
        void *pPlane;       // frame buffer pointer
        unsigned int uWidth;        // allocated subpalne width
        unsigned int uHeight;       // allocated subplane height
        unsigned int uPixelSize;         // pixel size (bits) of the subplane
        unsigned int uPixelJustify;      // pixel justification requirement (pixel)
        unsigned int uBytePerLine;       // byte per line of the subplane
        GFX_PIXEL_INFO_T a, r, g, b;     // pixel config

        GFX_PHYSICAL_INFO_T plane;      // physical parm;
    } plane[GFX_SURFACE_MAX_SUBPLANES];

} GFX_SURFACE_LOCK_INFO_T;


typedef enum __GFX_FONT_ENCODING_T__
{
    GFX_FONT_ASCII,
    GFX_FONT_GB2312,
    GFX_FONT_UNICODE
} GFX_FONT_ENCODING_T;

typedef struct __GFX_FONT_INFO_T_
{
    int hFont;          // 8 bit antialiased font
    int hMonoFont;      // 1 bit mono font
    int hScratch;       // scratch pad for drawing fonts

    GFX_SURFACE_LOCK_INFO_T infoFont;
    GFX_SURFACE_LOCK_INFO_T infoMono;
    
    GFX_FONT_ENCODING_T encoding;
    unsigned int uMaxChar, uMinChar;
    unsigned int uHeight;
    unsigned int uMaxWidth;
    unsigned int *uWidth;
    unsigned int *uPosX, *uPosY;
} GFX_FONT_INFO_T;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


int gfx_open(void);

int gfx_close(int fdGfxDev);


int gfx_ClipBLTRect(unsigned int uSrcWidth, unsigned int uSrcHeight, 
                                unsigned int uDesWidth, unsigned int uDesHeight, 
                                unsigned int uSrcX,     unsigned int uSrcY,
                                unsigned int uDesX,     unsigned int uDesY,
                                unsigned int *uWidth,   unsigned int *uHeight
                                );

int gfx_ClipRect(unsigned int uDesWidth, unsigned int uDesHeight, 
                                unsigned int uDesX,     unsigned int uDesY,
                                unsigned int *uWidth,   unsigned int *uHeight
                                );

void gfx_pixelJustify(unsigned int uJustify, unsigned int *uX, unsigned int *uWidth);

unsigned int gfx_plane_pixel_color(unsigned int uPlaneConfig, unsigned char a, unsigned char r, unsigned char g, unsigned char b);

void gfx_rgb2ycbcr(BYTE r, BYTE g,  BYTE b,  BYTE *y, BYTE *cb, BYTE *cr);
void gfx_ycbcr2rgb(BYTE y, BYTE cb,  BYTE cr,  BYTE *r, BYTE *g, BYTE *b);

int gfx_create_surface(int fdGfxDev, 
    unsigned int uWidth,        // width of buffer
    unsigned int uHeight,       // height of buffer
    unsigned int uPlaneConfig,  // plane config
    GFX_VISUAL_DEVICE_ID_T graphDev,    // which device to be used
    unsigned int  uFillColor     // which color to fill
   );

int gfx_destroy_surface(int fdGfxDev, int hSurface);

int gfx_get_subplane_pseudo_surface(int fdGfxDev, int hSourceSurface, unsigned int uPlaneID, unsigned int uPlaneConfig, unsigned int uExpectWidth);

int gfx_lock_surface(int fdGfxDev, int hSurface, GFX_SURFACE_LOCK_INFO_T *pParm);

int gfx_unlock_surface(int fdGfxDev, int hSurface, GFX_SURFACE_LOCK_INFO_T *pParm);

int gfx_get_surface_info(int fdGfxDev, int hSurface, GFX_SURFACE_INFO_T *pParm);

int gfx_attach_surface(int fdGfxDev, int hSurface, GFX_VISUAL_DEVICE_ID_T graphDev);

int gfx_detach_surface(int fdGfxDev, int hSurface, GFX_VISUAL_DEVICE_ID_T graphDev);

int gfx_get_surface_palette(int fdGfxDev, int hSurface, unsigned int uStart, unsigned int uCount, GFX_PALETTE_T *pPalette);

int gfx_set_surface_palette(int fdGfxDev, int hSurface, unsigned int uStart, unsigned int uCount, GFX_PALETTE_T *pPalette);

int gfx_get_shared_surface(int fdGfxDev);

int gfx_set_shared_surface(int fdGfxDev, int hSurface);

int gfx_get_surface_display_parm(int fdGfxDev, int hSurface, GFX_SURFACE_DISPLAY_T *pParm);

int gfx_set_surface_display_parm(int fdGfxDev, int hSurface, GFX_SURFACE_DISPLAY_T *pParm);

int gfx_get_screen_info(int fdGfxDev, GFX_SCREEN_INFO_T *pInfo);

int gfx_set_screen_info(int fdGfxDev, GFX_SCREEN_INFO_T *pInfo);

int gfx_set_display_attr(int fdGfxDev, GFX_DISPLAY_CONTROL_T control, UINT uattr);
int gfx_get_display_attr(int fdGfxDev, GFX_DISPLAY_CONTROL_T control, UINT *puattr);

int gfx_set_display_device_attr(int fdGfxDev,  GFX_VISUAL_DEVICE_ID_T graphDev, GFX_VISUAL_DEVICE_CONTROL_T cntrol,  UINT uattr);

int gfx_get_display_device_attr(int fdGfxDev,  GFX_VISUAL_DEVICE_ID_T graphDev, GFX_VISUAL_DEVICE_CONTROL_T cntrol,  UINT *puattr);

int gfx_move_cursor(int fdGfxDev, int x, int y);

int gfx_report_cursor(int fdGfxDev, int *x, int *y);

int gfx_set_cursor_attributes(int fdGfxDev, int hCursorSurface, unsigned int uIndex, GFX_CURSOR_ATTRIBUTE_T attr);

int gfx_wait_for_engine(int fdGfxDev, int nTimeout);

int gfx_reset_engine(int fdGfxDev);

int gfx_set_engine_mode(int fdGfxDev, int sync_mode);

int gfx_get_engine_mode(int fdGfxDev);

int gfx_set_surface_clip_rect(int fdGfxDev, int hSurface, GFX_RECT_T *pClip);

int gfx_bitBLT(int fdGfxDev, int hDes, int nDesX, int nDesY,
                unsigned int uWidth, unsigned int uHeight,
                int hSrc,  int nSrcX,  int nSrcY,
                ALPHA_SELECT *pAlphaSelect, char enableGammaCorrection);

int gfx_advancedBitBLT(int fdGfxDev, int hDes, int nDesX, int nDesY,
            unsigned int uWidth, unsigned int uHeight,
            int hSrc, int nSrcX, int nSrcY,
            int hMask, int nMaskX, int nMaskY,
            GFX_ROP_CODE_T ROP,
            char enablePixelBitMask, unsigned int uPixelBitMask,
            ALPHA_SELECT *pAlphaSelect);

int gfx_fillBLT(int fdGfxDev, int hDes, int nDesX, int nDesY,
            unsigned int uWidth, unsigned int uHeight, unsigned int uFillColor);

int gfx_advancedFillBLT(int fdGfxDev, int hDes, int nDesX, int nDesY,
            unsigned int uWidth, unsigned int uHeight, unsigned int uFillColor, /* formatted in the dest format color if rgb, index if clut */
            int hpMask, int nMaskX, int nMaskY,
            GFX_ROP_CODE_T ROP, char transparencyEnable,    // 
            unsigned int uBackGroundColor, char enablePixelBitMask, unsigned int uPixelBitMask);

int gfx_blend(int fdGfxDev, int hDes, int nDesX, int nDesY,
                unsigned int uWidth, unsigned int uHeight,
                int hSrc, int nSrcX, int nSrcY,
                ALPHA_BLEND_SELECT *pBlendSelect);

int gfx_advancedBlend(int fdGfxDev, int hDes, int nDesX, int nDesY,
                unsigned int uWidth, unsigned int uHeight,
                int hSrc, int nSrcX, int nSrcY,
                int hAlpha, int nAlphaX, int nAlphaY,
                ALPHA_BLEND_SELECT *pBlendSelect);

int gfx_colorKey(int fdGfxDev, int hDes, int nDesX, int nDesY,
                unsigned int uWidth, unsigned int uHeight,
                int hSrc, int nSrcX, int nSrcY,
                COLOR_KEY_SELECT *pColorKeySelect, ALPHA_SELECT *pAlphaSelect);

int gfx_resize(int fdGfxDev, int hDes, unsigned int uDesX, unsigned int uDesY, unsigned int uDesWidth, unsigned int uDesHeight,
            int hSrc, unsigned int uSrcX, unsigned int uSrcY, unsigned int uSrcWidth, unsigned int uSrcHeight,
            BYTE destAlpha, char enableGammaCorrection);


// software based drawing functions
// gfxdraw.c

#define GFX_DRAW_TIMEOUT    30000   // 30 seconds

int gfx_draw_horizontal_line( int fdGfxDev,   int hDes, 
   long X1,    long X2,    long Y,
   unsigned int uColor );
   
int gfx_draw_rectangle( int fdGfxDev,  int hDes, 
   long X1,  long Y1,  long X2,  long Y2, 
   unsigned int uColor,   int fillMode );

int gfx_draw_polygon( int fdGfxDev,  int hDes, 
   long *X,  long *Y,  unsigned int numVertices,
   unsigned int uColor,  int fillMode);

int gfx_draw_ur_triangle( int fdGfxDev,  int hDes, 
   long X1,   long Y1,  long X2,  long Y2,
   unsigned int uColor  );

int gfx_draw_circle( int fdGfxDev,  int hDes, 
   long X, long Y, long radius,
   unsigned int uColor,  int fillMode);

int gfx_draw_line( int fdGfxDev, int hDes, 
   long X1,   long Y1,   long X2,  long Y2,
   unsigned int uColor);
   
int gfx_draw_pixel( int fdGfxDev,  int hDes, 
   long X, long Y,
   unsigned int uColor);


// font manipulation routines
// gfxfont.c

// draw font flags
#define GFX_DRAW_FONT_BACKGROUND    1  //  enable solid background
#define GFX_DRAW_FONT_ANTIALIAS     2  //  enable antialias
#define GFX_DRAW_FONT_WITHALPHA     4  //  update alpha too
#define GFX_DRAW_FONT_DEFAULT       0  //  non antialiased transparent background, no alpha


int gfx_load_font_freetype2( int fdGfxDev,   GFX_FONT_INFO_T *pFont,
   GFX_FONT_ENCODING_T encoding,  unsigned int charStart,  unsigned int charEnd,
   unsigned int uWidth,  unsigned int uHeight,  char * pFontPath  );

int gfx_free_font(int fdGfxDev, GFX_FONT_INFO_T *pFont);

// return font width in pixels
int gfx_draw_font(int fdGfxDev, int hDes, GFX_FONT_INFO_T *pFont, unsigned int uChar, 
   long X,  long Y,  unsigned int uColor,  unsigned int uBackColor, unsigned int uFlag );

// return string length in pixels
int gfx_draw_ascii_string( int fdGfxDev, int hDes, GFX_FONT_INFO_T *pFont, const char *szStr,
   long X, long Y,  unsigned int uColor,  unsigned int uBackColor,  unsigned int uFlag );

// return num char draw
int gfx_draw_ascii_string_rect( int fdGfxDev, int hDes, GFX_FONT_INFO_T *pFont, const char *szStr,
   long X1,  long Y1, long X2, long Y2, int char_space, int line_space, unsigned int uColor,  unsigned int uBackColor,  unsigned int uFlag );
   
int gfx_get_ascii_string_length(GFX_FONT_INFO_T *pFont, const char *szStr);

int gfx_get_ascii_string_height(GFX_FONT_INFO_T *pFont, const char *szStr);



//  24 bit can only be save from / load to 16/32 bpp RGB
//  8 bit can only be save from / load to 8 bpp RGB
//  4 bit can only be load to 4 bpp RGB
//  1 bit can only be load to 1 bpp RGB

// return 0 on success
//  -1 on fail

int gfx_SaveBMP24b(const char * fname, GFX_SURFACE_LOCK_INFO_T *pSurface, unsigned int uStartX, unsigned int uStartY, unsigned int uWidth, unsigned int uHeight);
int gfx_SaveBMP8b(const char * fname, GFX_SURFACE_LOCK_INFO_T *pSurface, GFX_PALETTE_T *pPal, unsigned int uStartX, unsigned int uStartY, unsigned int uWidth, unsigned int uHeight);

int gfx_LoadBMP24b(GFX_SURFACE_LOCK_INFO_T *pSurface, unsigned int uDesX, unsigned int uDesY, unsigned int uWidth, unsigned int uHeight, const char * fname, unsigned int uSrcX, unsigned int uSrcY, BYTE alpha);
int gfx_LoadBMP8b(GFX_SURFACE_LOCK_INFO_T *pSurface, GFX_PALETTE_T *pPal, unsigned int uDesX, unsigned int uDesY, unsigned int uWidth, unsigned int uHeight, const char * fname, unsigned int uSrcX, unsigned int uSrcY, BYTE alpha);
int gfx_LoadBMP4b(GFX_SURFACE_LOCK_INFO_T *pSurface, GFX_PALETTE_T *pPal, unsigned int uDesX, unsigned int uDesY, unsigned int uWidth, unsigned int uHeight, const char * fname, unsigned int uSrcX, unsigned int uSrcY, BYTE alpha);
int gfx_LoadBMP1b(GFX_SURFACE_LOCK_INFO_T *pSurface, GFX_PALETTE_T *pPal, unsigned int uDesX, unsigned int uDesY, unsigned int uWidth, unsigned int uHeight, const char * fname, unsigned int uSrcX, unsigned int uSrcY, BYTE alpha);


// Load BMP and create an surface the same type of that BMP
// return the surface handle on success !!!
//    if pSurface != NULL the surface will be locked and return the lock info in pSurface
//    else the surface is not locked
//    if pWidth / pHeight is not NULL,  the width / height value will be returned
// return -1 on failure 

int gfx_LoadBMP_Surface(int fdGfxDev, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT *pWidth, UINT *pHeight, const char * fname, BYTE alpha);


// Load 32bits RGB targa bitmap and create an surface the same type of that bitmap
// return the surface handle on success !!!
//    if pSurface != NULL the surface will be locked and return the lock info in pSurface
//    else the surface is not locked
//    if pWidth / pHeight is not NULL,  the width / height value will be returned
// return -1 on failure 

int gfx_LoadTGA32b_Surface(int fdGfxDev, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT *pWidth, UINT *pHeight, const char * fname);


// Load color/greyscale JPEG bitmap and create an surface the same type of that bitmap
// return the surface handle on success !!!
//    if pSurface != NULL the surface will be locked and return the lock info in pSurface
//    else the surface is not locked
//    if pWidth / pHeight is not NULL,  the width / height value will be returned
// return -1 on failure 
int gfx_LoadJPEG_Surface(int fdGfxDev, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT *pWidth, UINT *pHeight, const char * fname, BYTE alpha);

// Parameter pointers can be NULL which means to ignore that parm
int gfx_GetBMPInfo(const char * fname, unsigned int *upWidth, unsigned int *upHeight, unsigned int *upPixelSize);

// load BMP, TGA or JPEG surfaces by their file name suffixes
int gfx_LoadBitmap_Surface(int fdGfxDev, GFX_SURFACE_LOCK_INFO_T *pSurface, UINT *pWidth, UINT *pHeight, const char * fname, BYTE alpha);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // __GFX_GFXLIB_H_INC__
