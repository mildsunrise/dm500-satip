//vulcan/drv/include/gfx/gfx_inf_struct.h
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
//  data structure of GFX INF controls 
//Revision Log:   
//  Nov/06/2001                          Created by YYD
//  Oct/21/2002                          Added structures for several new IOCTLs

#ifndef  _DRV_INCLUDE_GFX_GFX_INF_STRUCT_H_INC_
#define  _DRV_INCLUDE_GFX_GFX_INF_STRUCT_H_INC_

#include "gfx/gfx_common.h"

#define GFX_ARGB_COLOR(a,r,g,b)      (UINT32)( ((UINT32)(a)<<24) | ((UINT32)(r)<<16) | ((UINT32)(g)<<8) | ((UINT32)(b)))
#define GFX_ARGB8888_COLOR(a,r,g,b)  (UINT32)( ((UINT32)(a)<<24) | ((UINT32)(r)<<16) | ((UINT32)(g)<<8) | ((UINT32)(b)))
#define GFX_RGB565_COLOR(a,r,g,b)    (UINT32)(((UINT32)((r)&0xf8)<<8) | ((UINT32)((g)&0xfc)<<3) | ((UINT32)((b)>>3)))
#define GFX_ARGB1555_COLOR(a,r,g,b)  (UINT32)( ((UINT32)((a)&0x80)<<8) | ((UINT32)((r)&0xf8)<<7) | ((UINT32)((g)&0xf8)<<2) | ((UINT32)((b)>>3)))
#define GFX_ARGB4444_COLOR(a,r,g,b)  (UINT32)( ((UINT32)((a)&0xf0)<<8) | ((UINT32)((r)&0xf0)<<4) | ((UINT32)((g)&0xf0)) | ((UINT32)((b)>>4)))

//  -------------------  PACKED -----------------------
#ifdef __GNUC__
#pragma pack(1)
#endif

typedef struct __GFX_PHYSICAL_INFO_T_STRUCTURE__
{
    ULONG   uBase;
    ULONG   uSize;
    ULONG   uOffset;
} GFX_PHYSICAL_INFO_T;


typedef struct __GFX_SURFACE_INFO_T_STRUCTURE__
{
    int hSurface;
    UINT uPlaneConfig;

    struct __GFX_SURFACE_PLANE_INFO_T_STRUCTURE__
    {
        GFX_PHYSICAL_INFO_T plane;      // physical info of plane
        UINT uWidth;        // allocated subpalne width
        UINT uHeight;       // allocated subplane height
        UINT uPixelSize;         // pixel size (bits) of the subplane
        UINT uPixelJustify;      // pixel justification requirement (pixel)
        UINT uBytePerLine;       // byte per line of the subplane
        GFX_PIXEL_INFO_T a, r, g, b;     // pixel config
    } plane[GFX_SURFACE_MAX_SUBPLANES];

} GFX_SURFACE_INFO_T;




typedef struct __GFX_SURFACE_VDEV_PARM_T_STRUCTURE__
{
    int hSurface;
    GFX_VISUAL_DEVICE_ID_T graphDev;    // which device to be used
} GFX_SURFACE_VDEV_PARM_T;


/*
 * Structure used to swap two surfaces, optionally synchronized with the OSD
 * animation interrupt -- BJC 102102
 */
typedef struct __GFX_2SURFACE_VDEV_PARM_T_STRUCTURE__
{
    int hOldSurface;                    // surface to detach (0 for attach only)
    int hNewSurface;                    // surface to attach
    GFX_VISUAL_DEVICE_ID_T graphDev;    // which device to be used
    BYTE bUseOsdAnimInt;                // swap on next OSD animation interrupt
    BYTE bWaitForSwap;                  // do not return until surfaces swapped
                                        //   (0=asynch 1=synch)
    UINT missedInts;                    // OSD anim interrupts since last call
} GFX_2SURFACE_VDEV_PARM_T;


typedef struct __GFX_SURFACE_ACCESS_PALETTE_PARM_T_STRUCTURE__
{
    int hSurface;
    UINT uStart;
    UINT uCount;
    GFX_PALETTE_T *pPalette;
} GFX_SURFACE_ACCESS_PALETTE_PARM_T;


typedef struct __GFX_COORDINATE_T_STRUCTURE__
{
    int nCursorX, nCursorY;
} GFX_COORDINATE_T;

typedef struct __GFX_CURSOR_ATTRUBUTE_PARM_T_STRUCTURE__
{
    int     hCursor;
    UINT    uIndex;
    GFX_CURSOR_ATTRIBUTE_T attr;
} GFX_CURSOR_ATTRUBUTE_PARM_T;

typedef struct __GFX_SCREEN_INFO_T_STRUCTURE__
{
    UINT uLeft, uUpper;
    UINT uWidth, uHeight;
} GFX_SCREEN_INFO_T;


typedef struct __GFX_SURFACE_DISPLAY_T_STRUCTURE__
{
    int hSurface;
    UINT uStartX, uStartY;
    UINT uWinX, uWinY;
    UINT uWinWidth, uWinHeight;
    BYTE bFlickerCorrect;
} GFX_SURFACE_DISPLAY_T;



// for gfx_osd_create_surface
typedef struct __GFX_CREATE_SURFACE_PARM_T_STRUCTURE__
{
    int  hSurface;      // return value
    UINT uWidth;        // width of buffer
    UINT uHeight;       // height of buffer
    UINT uPlaneConfig;  // plane config
    GFX_VISUAL_DEVICE_ID_T graphDev;    // which device to be used
    UINT32  uFillColor;     // which color to fill
} GFX_CREATE_SURFACE_PARM_T;

typedef struct __GFX_GET_SUBPLANE_PSEUDO_SURFACE_PARM_T__
{
    int  hSurface;      // return value
    int  hSourceSurface;    //source surface
    UINT uPlaneID;
    UINT uPlaneConfig;  // plane config, can only be GFX_SURFACE_RAW8BPP, GFX_SURFACE_RAW4BPP and GFX_SURFACE_RAW1BPP
    UINT uExpectWidth;  // expect width = 0 to use default (based on original byteperline)
}  GFX_GET_SUBPLANE_PSEUDO_SURFACE_PARM_T;


typedef struct __GFX_SET_CLIP_PARM_T_STRUCTURE__
{
    int hSurface; 
    GFX_RECT_T  rect;
} GFX_SET_CLIP_PARM_T;

typedef struct __GFX_BITBLT_PARM_T_STRUCTURE__
{
    int hDesSurface; 
    INT uDesX, uDesY;
    UINT uWidth, uHeight;
    int hSrcSurface;
    INT uSrcX, uSrcY;
    ALPHA_SELECT alphaSelect;
    char enableGammaCorrection;
} GFX_BITBLT_PARM_T;


typedef struct __GFX_ADV_BITBLT_PARM_T_STRUCTURE__
{
    int hDesSurface; 
    INT uDesX, uDesY;
    UINT uWidth, uHeight;
    int hSrcSurface;
    INT uSrcX, uSrcY;
    int hMaskSurface;
    INT uMaskX, uMaskY;
    ALPHA_SELECT alphaSelect;
    GFX_ROP_CODE_T ROP;
    char enablePixelBitMask;
    UINT32 uPixelBitMask;
} GFX_ADV_BITBLT_PARM_T;


typedef struct __GFX_FILLBLT_PARM_T_STRUCTURE__
{
    int hDesSurface; 
    INT uDesX, uDesY;
    UINT uWidth, uHeight;
    UINT32 uFillColor;
} GFX_FILLBLT_PARM_T;


typedef struct __GFX_ADV_FILLBLT_PARM_T_STRUCTURE__
{
    int hDesSurface; 
    INT uDesX, uDesY;
    UINT uWidth, uHeight;
    UINT32 uFillColor;
    int hMaskSurface;
    INT uMaskX, uMaskY;
    GFX_ROP_CODE_T ROP;
    char transparencyEnable;
    UINT32 uBackGroundColor;
    char enablePixelBitMask;
    UINT32 uPixelBitMask;
} GFX_ADV_FILLBLT_PARM_T;


typedef struct __GFX_BLEND_PARM_T_STRUCTURE__
{
    int hDesSurface; 
    INT uDesX, uDesY;
    UINT uWidth, uHeight;
    int hSrcSurface;
    INT uSrcX, uSrcY;
    ALPHA_BLEND_SELECT blendSelect;
} GFX_BLEND_PARM_T;


typedef struct __GFX_ADV_BLEND_PARM_T_STRUCTURE__
{
    int hDesSurface; 
    INT uDesX, uDesY;
    UINT uWidth, uHeight;
    int hSrcSurface;
    INT uSrcX, uSrcY;
    int hAlphaSurface;
    INT uAlphaX, uAlphaY;
    ALPHA_BLEND_SELECT blendSelect;
} GFX_ADV_BLEND_PARM_T;


typedef struct __GFX_COLORKEY_PARM_T_STRUCTURE__
{
    int hDesSurface; 
    INT uDesX, uDesY;
    UINT uWidth, uHeight;
    int hSrcSurface;
    INT uSrcX, uSrcY;
    COLOR_KEY_SELECT colorKeySelect;
    ALPHA_SELECT alphaSelect;
} GFX_COLORKEY_PARM_T;


typedef struct __GFX_RESIZE_PARM_T_STRUCTURE__
{
    int hDesSurface; 
    UINT uDesX, uDesY;
    UINT uDesWidth, uDesHeight;
    int hSrcSurface;
    UINT uSrcX, uSrcY;
    UINT uSrcWidth, uSrcHeight;
    BYTE destAlpha;
    char enableGammaCorrection;
} GFX_RESIZE_PARM_T;

/* 
 * type of memory (contiguous/discontiguous) to be transferred -- BJC 102102
 */
typedef enum __GFX_DMABLT_SRCMEM_T_ENUMERATION__
{
    GFX_DMABLT_SRCMEM_PHYS,  // source memory is physically contiguous
    GFX_DMABLT_SRCMEM_USER   // source memory was allocated by a user-space
                             //     malloc()-like routine and is physically
			                 //     discontiguous
} GFX_DMABLT_SRCMEM_T;

/* 
 * type of transfer (synchronous/asynchronous) -- BJC 102102
 */
typedef enum __GFX_DMABLT_MODE_T_ENUMERATION__
{
    GFX_DMABLT_MODE_SYNCH,  // synchronous; functions do not return until transfer finished
    GFX_DMABLT_MODE_ASYNCH  // asynchronous; functions return before transfer finishes
} GFX_DMABLT_MODE_T;

/* 
 * structure used to specify parameters for transferring video frame from
 * memory to OSD plane using a system DMA controller; rudimentary
 * low-overhead vertical scaling is supported -- BJC 102102
 */
typedef struct __GFX_DMABLT_PARM_T_STRUCTURE__
{
	BYTE* dstY;           // luminance buffer of destination frame buffer
	BYTE* dstUV;          // chrominance buffer of destination frame buffer
	UINT  dstStride;      // line size of destination frame, in pixels
		
	BYTE* srcY;           // luminance buffer of source frame in memory
	BYTE* srcUV;          // chrominance buffer of source frame in memory
	UINT  srcStride;      // line size of source frame, in pixels

	UINT  imageHeight;    // number of lines to transfer from source to dest
	UINT  draw1xLines;    // lines to copy one time (see documentation)
	UINT  drawMxLines;    // lines to copy multiple times (see documentation)
	UINT  drawMxCount;    // times to draw each drawMxLine (see documentation)
	UINT  srcLineOffset;  // first line (from 0) to transfer from source to dest
    GFX_DMABLT_SRCMEM_T srcMemType; // indicates whether the source frame memory
                                    // is physically contiguous or not
    GFX_DMABLT_MODE_T synchMode; // synchronous or asynchronous
} GFX_DMABLT_PARM_T;


/*
 * structures used by the physical memory allocator -- BJC 102102
 */
typedef enum
{
    GFX_PMALLOC_ALLOCATE,  // allocate memory
    GFX_PMALLOC_FREE,      // free memory allocated by this allocator
    GFX_PMALLOC_CHKALIGN   // translate logical address to physical address
} GFX_PMALLOC_MODE_T;

#include <os/pm-alloc.h>
typedef struct __GFX_PMALLOC_PARM_T_STRUCTURE__
{
    GFX_PMALLOC_MODE_T mode;         // entity from GFX_PMALLOC_MODE_T
    UINT                uAllocSize;  // see documentation
    MEM_HANDLE_T        hBuffer;     // see documentation
    UINT                uPhysical;   // see documentation
    void                *pLogical;   // see documentation
    UINT                uSize;       // see documentation
} GFX_PMALLOC_PARM_T;

typedef enum
{
    GFX_DISP_CNTL_VALPHA,    // Video alpha, 8 bits attr  00 transparnet, ff opaque
    GFX_DISP_CNTL_AVALPHA,   // Alternative video alpha, 8 bits attr  00 transparnet, ff opaque
    GFX_DISP_CNTL_AFVP,      // Anti-flicker video plane, 0 disable, 1 enable
    GFX_DISP_CNTL_EDAF,      // Enable display anti-flicker, 0 disable, 1 enable
    GFX_DISP_CNTL_AFDT,      // Anti-flicker detection threshold, 2 bits attr
    GFX_DISP_CNTL_VPAFC,     // Video plane anti-flicker correction, 2 bits attr
    GFX_DISP_CNTL_FP,        // Force progressive, 0 don't, 1 upsample progressive
    GFX_DISP_CNTL_BACKCOLOR, // 16 bits background color in y8:u4:v4
    GFX_DISP_CNTL_OSDIRQ,    // OSD Animation interrupt, 0 disable, 1 enable      -- BJC 102102
    GFX_DISP_CNTL_OSDHSR     // Custom Horizontal FIR Scaling Ratio, 9 bits attr  -- BJC 102102    
} GFX_DISPLAY_CONTROL_T;

typedef struct __GFX_DISPLAY_CONTROL_PARM_T_STRUCUTRE__
{
    GFX_DISPLAY_CONTROL_T parm;
    UINT uAttr;
} GFX_DISPLAY_CONTROL_PARM_T;


typedef enum
{
   GFX_VISUAL_DEVICE_GLOBAL_ALPHA,        //   0x100 enable / 0x000 disable | 00-ff alpha
   GFX_VISUAL_DEVICE_ENABLE               //   0x001 enable / 0x000 disable 
} GFX_VISUAL_DEVICE_CONTROL_T;

typedef struct __GFX_VISUAL_DEVICE_CONTROL_PARM_T_STRUCUTRE__
{
    GFX_VISUAL_DEVICE_ID_T graphDev;
    GFX_VISUAL_DEVICE_CONTROL_T cntl;
    UINT uAttr;
} GFX_VISUAL_DEVICE_CONTROL_PARM_T;


#ifdef __GNUC__
#pragma pack()
#endif
//  -------------------  UNPACKED ---------------------


#endif // _DRV_INCLUDE_GFX_GFX_INF_STRUCT_H_INC_
