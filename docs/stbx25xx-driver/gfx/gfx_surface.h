//vulcan/drv/gfx/gfx_surface.h
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
//  Common defines of GFX surfaces 
//Revision Log:   
//  Sept/28/2001                              Created by YYD
//  Oct/17/2001       Modified to suppport G2D/SCALER by YYD

#ifndef  _DRV_INCLUDE_GFX_GFX_SURFACE_H_INC_
#define  _DRV_INCLUDE_GFX_GFX_SURFACE_H_INC_

#include "os/os-types.h"
#include "os/pm-alloc.h"

#include "gfx/gfx_common.h"

//  -------------------  PACKED -----------------------
#ifdef __GNUC__
#pragma pack(1)
#endif

typedef struct __GFX_PLANE_CONFIG_STRUCTURE__
{
        void *pBuffer;           // logocal pointer to subplane video buffer
        UINT uBMLA;              // BMLA to subplane video buffer
        UINT uAllocWidth;        // allocated subpalne width
        UINT uAllocHeight;       // allocated subplane height
        UINT uPixelSize;         // pixel size (bits) of the subplane
        UINT uPixelJustify;      // pixel justification requirement (pixel)
        UINT uBytePerLine;       // byte per line of the subplane
        GFX_PIXEL_INFO_T a, r, g, b;    // a, r, g, b pixel info
        UINT uAllocSize;         // Added to hold the actually allocated size for later use
} GFX_PLANE_CONFIG_T;


typedef struct __GRAPH_SURFACE_T_STRUCTURE__
{
// public: 
    UINT uPlaneConfig;                              
    UINT uAttr;
    GFX_VISUAL_DEVICE_ID_T attachedDev;


    GFX_PLANE_CONFIG_T plane[GFX_SURFACE_MAX_SUBPLANES];    // this one is public now
    GFX_PALETTE_T *pPalette;                                // logical palette

// private:   :-( Hide me
    MEM_HANDLE_T hBuffer;                                 // subplane video buffer handle
    void *pPal;                                           // pointer to physical palette
    UINT32       uBufferBase;
    INT twinheader;                                       // for 32 bits color mode, there are two headers
    UINT32       uBufferBase2;

    GFX_RECT_T clip;                                 // clipping area
    GFX_RECT_T bound;                                // bounding area, for gfx convenience only

    struct __GRAPH_SURFACE_T_STRUCTURE__ *pNextAttach;    // for attached chain on the same surface

} GFX_SURFACE_T;

typedef struct __GRAPH_SURFACE_PHYSICAL_PARM_T_STRUCTURE__
{
    ULONG   uGFXBufferPhysicalBase;
    ULONG   uGFXBufferPhysicalSize;

    ULONG   uPlanePhysicalBaseAddr[GFX_SURFACE_MAX_SUBPLANES];
    ULONG   uPlanePhysicalOffset[GFX_SURFACE_MAX_SUBPLANES];
    ULONG   uPlanePhysicalSize[GFX_SURFACE_MAX_SUBPLANES];

} GFX_SURFACE_PHYSICAL_PARM_T;


#ifdef __GNUC__
#pragma pack()
#endif
//  -------------------  UNPACKED ---------------------

#define __GFX_SURFACE_ALLOC        (0x00000001)
#define __GFX_SURFACE_OSDCURSOR    (0x00000002)    // for cursor surface

void gfx_osi_init_surface_t(GFX_SURFACE_T * pSurface);
// used to empty a new unused surface_t for next steps

INT gfx_osi_create_surface(GFX_SURFACE_T *pSurface, GFX_VISUAL_DEVICE_ID_T graphDev, UINT uPlaneConfig, UINT uWidth, UINT uHeight);
// an surface that we can attach to compatible device to be displayed

INT gfx_osi_destroy_surface(GFX_SURFACE_T *pSurface);

INT gfx_osi_reconfig_surface_dimension(GFX_SURFACE_T *pSurface, UINT uNewWidth, UINT uNewHeight);

INT gfx_osi_set_surface_palette(GFX_SURFACE_T *pSurface, GFX_PALETTE_T *pPal, UINT uStart, UINT uNumEntry);
INT gfx_osi_get_surface_palette(GFX_SURFACE_T *pSurface, GFX_PALETTE_T *pPal, UINT uStart, UINT uNumEntry);

INT gfx_osi_reset_surface_palette(GFX_SURFACE_T *pSurface);

INT gfx_osi_get_surface_physical_parm(GFX_SURFACE_PHYSICAL_PARM_T *pParm, GFX_SURFACE_T *pSurface);

void gfx_osi_rgb2ycbcr(BYTE r, BYTE g,  BYTE b,  BYTE *y, BYTE *cb, BYTE *cr);
void gfx_osi_ycbcr2rgb(BYTE y, BYTE cb,  BYTE cr,  BYTE *r, BYTE *g, BYTE *b);

#define gfx_osi_pSurface_valid(a) ((a) && ((a)->uAttr & __GFX_SURFACE_ALLOC))

#define gfx_osi_pSurface_alloc(a) ((a)->uAttr & __GFX_SURFACE_ALLOC)

#define gfx_osi_pSurface_cursor(a) ((a) && ((a)->uAttr & __GFX_SURFACE_OSDCURSOR))

extern GFX_PALETTE_T  gGFX_RGB8_Palette[256];
extern GFX_PALETTE_T  gGFX_RGB4_Palette[16];
extern GFX_PALETTE_T  gGFX_RGB2_Palette[4];

#endif //  _DRV_INCLUDE_GFX_GFX_SURFACE_H_INC_
