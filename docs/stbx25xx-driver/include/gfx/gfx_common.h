//vulcan/drv/include/gfx/gfx_common.h
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
//  commonly used gfx definations
//Revision Log:   
//  Oct/29/2001                          Created by YYD

#ifndef  _DRV_INCLUDE_GFX_GFX_COMMON_H_INC_
#define  _DRV_INCLUDE_GFX_GFX_COMMON_H_INC_

#include "os/os-types.h"

/* 2D engine Graphics pixel formats */
#define G2D_CLUT1             (0x1)
#define G2D_CLUT8             (0x2)
#define G2D_YCBCR422          (0x3) 
#define G2D_YCBCR420          (0x4) 
#define G2D_AYCBCR422         (0x5) 
#define G2D_AYCBCR420         (0x6) 
#define G2D_ARGB_8888         (0x7)             // for compatibility only


#define GFX_SURFACE_MAX_WIDTH      (0x3fc)      // 255*4
#define GFX_SURFACE_MAX_HEIGHT     (0x3fe)      // 255*4
#define GFX_SURFACE_LINE_JUSTIFY   (4)          // 4 bytes also pixels
#define GFX_SURFACE_ADDR_JUSTIFY   (0x200)       // for OSD it's 512, for G2D and Scaller, it not required

#define G2D_SURFACE_MAX_WIDTH      (0x3fc)      // G2D only
#define G2D_SURFACE_MAX_HEIGHT     (0x3fe)      // G2D only

#define OSD_SURFACE_MAX_WIDTH      (0x3fc)      // OSD only, incompatible with G2D
#define OSD_SURFACE_MAX_HEIGHT     (0x3fc)      // OSD only, incompatible with G2D

#define SCALER_SURFACE_MAX_WIDTH   (0x3fc)       // Scaler limitations only, incompatible with GFX
#define SCALER_SURFACE_MAX_HEIGHT  (0x3fe)      

// surface_compatibility flags
#define GFX_COMP_OSDGFX            (0x01000)
#define GFX_COMP_OSDCUR            (0x02000)
#define GFX_COMP_OSDIMG            (0x04000)
#define GFX_COMP_OSDALL            (0x07000)
#define GFX_COMP_G2DS              (0x10000)
#define GFX_COMP_G2DD              (0x20000)
#define GFX_COMP_G2D               (0x30000)
#define GFX_COMP_SCALERS           (0x40000)
#define GFX_COMP_SCALERD           (0x80000)
#define GFX_COMP_SCALER            (0xC0000)

#define GFX_SURFACE_MAX_SUBPLANES   (3)

#define GFX_SURFACE_ONE_SUBPLANE	(0x100000)
#define GFX_SURFACE_TWO_SUBPLANE	(0x200000)
#define GFX_SURFACE_THREE_SUBPLANE	(0x300000)


// surface defines 
// 0xf00 (GFX pixel format) + 0x3f (OSD pixel format) + compatibilities + numplanes

// CLUT surface 
//  mode number is
//  rgb, pix_res , high_color, color_res
#define GFX_SURFACE_CLUT8BPP_AYCBCR      ((G2D_CLUT8 << 8)  +0x02 + GFX_COMP_G2D + GFX_COMP_SCALER + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT8BPPP_AYCBCR     (                   0x06 + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT4BPP_AYCBCR      (                   0x00 + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT4BPPP_AYCBCR     (                   0x04 + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT2BPP_AYCBCR      (                   0x01 + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT2BPPP_AYCBCR     (                   0x05 + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)

#define GFX_SURFACE_CLUT8BPP_ARGB       ((G2D_CLUT8 << 8)  +0x0a + GFX_COMP_G2D + GFX_COMP_SCALER + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT8BPPP_ARGB      (                   0x0e + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT4BPP_ARGB       (                   0x08 + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT4BPPP_ARGB      (                   0x0c + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT2BPP_ARGB       (                   0x09 + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CLUT2BPPP_ARGB      (                   0x0d + GFX_COMP_OSDGFX  + GFX_COMP_OSDIMG + GFX_SURFACE_ONE_SUBPLANE)

// cursor plane only
#define GFX_SURFACE_CURSOR4BPP_YCBCR    (                   0x00 + GFX_COMP_OSDCUR  + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CURSOR4BPPP_YCBCR   (                   0x04 + GFX_COMP_OSDCUR  + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CURSOR2BPP_YCBCR    (                   0x01 + GFX_COMP_OSDCUR  + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CURSOR2BPPP_YCBCR   (                   0x05 + GFX_COMP_OSDCUR  + GFX_SURFACE_ONE_SUBPLANE)
 
#define GFX_SURFACE_CURSOR4BPP_RGB      (                   0x08 + GFX_COMP_OSDCUR  + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CURSOR4BPPP_RGB     (                   0x0c + GFX_COMP_OSDCUR  + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CURSOR2BPP_RGB      (                   0x09 + GFX_COMP_OSDCUR  + GFX_SURFACE_ONE_SUBPLANE)
#define GFX_SURFACE_CURSOR2BPPP_RGB     (                   0x0d + GFX_COMP_OSDCUR  + GFX_SURFACE_ONE_SUBPLANE)


// multi plane 8 bit YUV surface
#define GFX_SURFACE_YCBCR_422_888        ((G2D_YCBCR422<<8) +0x03 + GFX_COMP_G2D + GFX_COMP_SCALER + GFX_COMP_OSDGFX + GFX_COMP_OSDIMG + GFX_SURFACE_TWO_SUBPLANE)  /* requires 2 buffers (Y, CBCR) */
#define GFX_SURFACE_YCBCR_420_888        ((G2D_YCBCR420<<8) +0x07 + GFX_COMP_G2D + GFX_COMP_SCALER + GFX_COMP_OSDGFX + GFX_COMP_OSDIMG + GFX_SURFACE_TWO_SUBPLANE)  /* requires 2 buffers (Y, CBCR) */
#define GFX_SURFACE_AYCBCR_422_8888      ((G2D_AYCBCR422<<8)+0x33 + GFX_COMP_G2D + GFX_COMP_SCALER + GFX_COMP_OSDGFX + GFX_SURFACE_THREE_SUBPLANE)  /* requires 3 buffers (Y, CBCR, A) */
#define GFX_SURFACE_AYCBCR_420_8888      ((G2D_AYCBCR420<<8)+0x77 + GFX_COMP_G2D + GFX_COMP_SCALER + GFX_COMP_OSDGFX + GFX_SURFACE_THREE_SUBPLANE)  /* requires 3 buffers (Y, CBCR, A) */

// only for g2d and scaler, not for display    
#define GFX_SURFACE_ARGB_8888            ((G2D_ARGB_8888<<8)+0xff + GFX_COMP_G2D + GFX_COMP_SCALER + GFX_SURFACE_ONE_SUBPLANE)
// 8 bit raw plane, only for g2d and scaler, not for display, actually is clut8 without parm and palette
#define GFX_SURFACE_RAW8BPP              ((G2D_CLUT8<<8)    +0xfe + GFX_COMP_G2D + GFX_COMP_SCALERS  +  GFX_SURFACE_ONE_SUBPLANE)
// 1 bit raw plane, only for g2d, not for display, actually is clut1 without parm and palette
#define GFX_SURFACE_RAW1BPP              ((G2D_CLUT1<<8)    +0xfe + GFX_COMP_G2D +                   +  GFX_SURFACE_ONE_SUBPLANE)


// get plane attributes
#define GET_OSD_SURFACE_DATA_TYPE(a)  ((a)&0x77)	        // two 3 bits for osd_plane data type
#define GET_GFX_SURFACE_DATA_TYPE(a)  (((a)&0xf00)>>8)	    // 4 bits for gfx_plane data type
#define GET_GFX_SURFACE_CONFIG(a)     ((a)&0xff000)         // which plane can use this config
#define GET_GFX_SURFACE_SUBPLANES(a)  (((a)&0x300000)>>20)  // get the number of buffers needed

#define IS_SURFACE_OSD_COMP(a)        (((a)&GFX_COMP_OSDALL) != 0)      // check if it is OSD compatible
#define IS_SURFACE_G2D_COMP(a)        (((a)&0xf00) != 0)                // check if it is G2D compatible

#define IS_SURFACE_CURSOR_COMP(a)     (((a)&GFX_COMP_OSDCUR) != 0)      // check if it is OSD cursor compatible


#define IS_OSD_SURFACE_YUV(a)         (((a)&0x08) == 0)     // check if it is YUV config
#define IS_OSD_SURFACE_CLUT(a)        (((a)&0x03) != 3)     // check if it is clut config

#define IS_GFX_SURFACE_YUV(a)         (((a)&0x08) == 0)     // check if it is YUV config
#define IS_GFX_SURFACE_CLUT(a)        (((a)&0x03) != 3)     // check if it is clut config


//  -------------------  PACKED -----------------------
#ifdef __GNUC__
#pragma pack(1)
#endif

typedef struct __GFX_PALETTE_STRUCTURE__
{
    BYTE    a;      // alpha, 5 level  0%,  25%, 50%, 75%, 100%
    BYTE    r;      // red / Y
    BYTE    g;      // green / Cb
    BYTE    b;      // blue / Cr
} GFX_PALETTE_T;

typedef struct __GFX_PIXEL_INFO_T_STRUCTURE__
{
    USHORT uNumbits;
    USHORT uOffset;
} GFX_PIXEL_INFO_T;

typedef struct __GFX_RECT_T_STRUCTURE__
{
    UINT x, y;   // where x, y is upper left corner
    UINT w, h;   //       w, h is width and height
} GFX_NORM_RECT_T;

typedef struct __GFX_RECT_NORM_T_STRUCTURE__
{
    INT32 x1, y1;    // where x1, y1 must be upper left corner 
    INT32 x2, y2;    //       x2, y2 must be lwer right corner

} GFX_RECT_T;



#ifdef __GNUC__
#pragma pack()
#endif
//  -------------------  UNPACKED ---------------------



typedef enum
{
    GFX_VDEV_OSDCUR    = 0,
    GFX_VDEV_OSDGFX    = 1,
    GFX_VDEV_OSDIMG    = 2,
    GFX_VDEV_NULL      = -1
} GFX_VISUAL_DEVICE_ID_T;

#define GFX_NUMOF_VISUAL_DEVICES     (3)


typedef enum
{
    GFX_CURSOR_NO_PEEP_NO_STEADY = 0,        // peep = 0, steady = 0
    GFX_CURSOR_NO_PEEP_STEADY    = 1,        // peep = 0, steady = 1
    GFX_CURSOR_PEEP_NO_STEADY    = 2,        // peep = 1, steady = 0
    GFX_CURSOR_PEEP_STEADY       = 3,        // peep = 1, steady = 1
} GFX_CURSOR_ATTRIBUTE_T;


typedef enum
{
    GFX_DEST_ALPHA_FROM_GIVEN=0,        // alpha from given value
    GFX_DEST_ALPHA_FROM_SOURCE=1,       // alpha from source pixel
    GFX_DEST_ALPHA_FROM_DESTINATION=2,  // alpha from destination pixel
    GFX_DEST_ALPHA_FROM_BLEND=3         // alpha from blended source and destination
} GFX_DEST_ALPHA_SELECT_T;

typedef enum
{
    GFX_BLEND_ALPHA_FROM_GIVEN=0,       // 0000 Alpha from Destination Control 
    GFX_BLEND_ALPHA_FROM_SOURCE=1,      // 0001 Alpha from source pixel
    GFX_BLEND_ALPHA_FROM_DESTINATION=2, // 0010 Alpha from destination pixel

    // the following is for AdvancedBlend
    GFX_BLEND_ALPHA_FROM_PATTERN=3,     // 0011 Alpha from pattern buffer

    // I did not implement these in software in vulcan driver
    GFX_BLEND_SRC_OVER_DEST=5,          // 0101 Source OVER destination
    GFX_BLEND_DEST_OVER_SRC=6,          // 0110 Destination OVER source
    GFX_BLEND_SRC_PLUS_DEST=7,          // 0111 Source PLUS destination
    GFX_BLEND_FADE_DESTINATION=8,       // 1000 FADE destination
    GFX_BLEND_DEST_IN_SRC=9,            // 1001 Destination IN source
    GFX_BLEND_SRC_IN_DEST=10,           // 1010 Source IN destination
    GFX_BLEND_OPAQUE_DEST=11,           // 1011 OPAQUE destination
    GFX_BLEND_DEST_HELD_SRC=13,         // 1101 Destination HELD OUT BY source
    GFX_BLEND_SRC_HELD_DEST=14,         // 1110 Source HELD OUT BY destination
    GFX_BLEND_DARKEN_DEST=15,           // 1111 DARKEN destination
} GFX_BLEND_SELECT_T;


// I did not implement these in software
typedef enum
{
    GFX_ROP_DISABLE= -1,// disable

    GFX_ROP_CLEAR=0,    // 0000 Clear all bits to zero
    GFX_ROP_NOR=1,      // 0001 COMPL(source) AND COMPL(destination)
    GFX_ROP_AND_INV=2,  // 0010 COMPL(source) AND destination
    GFX_ROP_COPY_INV=3, // 0011 COMPL(source)
    GFX_ROP_AND_REV=4,  // 0100 Source AND COMPL(destination)
    GFX_ROP_INVERT=5,   // 0101 COMPL(destination)
    GFX_ROP_XOR=6,      // 0110 Source XOR destination
    GFX_ROP_NAND=7,     // 0111 COMPL(source) OR COMPL(destination)
    GFX_ROP_AND=8,      // 1000 Source AND destination
    GFX_ROP_EQUIV=9,    // 1001 COMPL(source) XOR destination
    GFX_ROP_NOOP=10,    // 1010 Destination
    GFX_ROP_OR_INV=11,  // 1011 COMPL(source) OR destination
    GFX_ROP_COPY=12,    // 1100 Source
    GFX_ROP_OR_REV=13,  // 1101 Source OR COMPL(destination)
    GFX_ROP_OR=14,      // 1110 Source OR destination
    GFX_ROP_SET=15,     // 1111 Set all bits to one
} GFX_ROP_CODE_T;



// I did not implement these in software
typedef enum
{
    // each source pixel will be compared to the colorKeyCompareValue
    GFX_COLORKEY_COMPARE_SOURCE      = 0,

    // each destination pixel will be compared to the colorKeyCompareValue
    GFX_COLORKEY_COMPARE_DESTINATION = 1,
} GFX_COLORKEY_COMPARE_T;

// I did not implement these in software
typedef enum
{
    //  replacement color will replace the source 
    //  pixel value when the  colorKey Compare Value matches 
    //  the pixel value
    GFX_COLORKEY_OUTPUT_REPLACE     = 0,

    //  destination pixel will be keeped
    //  when the  colorKey Compare Value matches 
    //  the pixel value
    GFX_COLORKEY_OUTPUT_KEEP = 1,
} GFX_COLORKEY_OUTPUT_T;

// I did not implement these in software
typedef struct 
{
    UINT32  colorKeyCompareValue;

    GFX_COLORKEY_COMPARE_T colorKeyCompareSelect;
    GFX_COLORKEY_OUTPUT_T colorKeyOutputSelect;

    UINT32 replacementColor; 
} COLOR_KEY_SELECT; 

typedef struct 
{
    GFX_DEST_ALPHA_SELECT_T storedAlphaSelect;
    BYTE globalAlphaValue; /* required for global or if alphaformat is MONO */
} ALPHA_SELECT; 

typedef struct 
{
    GFX_DEST_ALPHA_SELECT_T storedAlphaSelect;
    BYTE globalAlphaValue;
    GFX_BLEND_SELECT_T blendInputSelect;
} ALPHA_BLEND_SELECT;


#endif // _DRV_INCLUDE_GFX_GFX_COMMON_H_INC_
