/*----------------------------------------------------------------------------+
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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author: Ling shao
| File:   vid_types.h
| Purpose: video decoder data types for PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/

#ifndef PALLAS_VID_TYPES_H
#define PALLAS_VID_TYPES_H

/*stc data sturcture*/
#ifndef STC_STRUCT
#define STC_STRUCT
typedef struct tagSTC
{
    unsigned long  bit32_1;
    unsigned long  bit0;
}STC_T;

typedef struct tagSyncInfo
{
    STC_T stc;
    STC_T pts;
}SYNCINFO;
#endif

//denc type definition
typedef enum
{
  DENC_ID0,
  DENC_ID1
}DENC_ID;

typedef enum
{
  DENC_OUTFMT_RGB,
  DENC_OUTFMT_LBR,
  DENC_OUTFMT_LC,
  DENC_OUTFMT_CVBS
}DENC_OUTFMT;

typedef enum
{
    DENC_MODE_NTSC,
    DENC_MODE_PAL,
    DENC_MODE_BAR_ON,
    DENC_MODE_BAR_OFF
}DENC_MODE;

typedef enum{
   VID_SOURCE_NO,
   VID_SOURCE_DEMUX,
   VID_SOURCE_MEMORY
}vidSource_t;


typedef enum{
   VID_DISPFMT_NTSC,
   VID_DISPFMT_PAL,
   VID_DISPFMT_UNKNOWN
}vidDispFmt_t;


typedef enum
{
   VID_OUTFMT_RGB,
   VID_OUTFMT_YBR,
   VID_OUTFMT_SVIDEO,
   VID_OUTFMT_CVBS
}vidOutFmt_t;

typedef enum{
   VID_DISPMODE_NORM,
   VID_DISPMODE_LETTERBOX,
   VID_DISPMODE_1_2 = 3,
   VID_DISPMODE_1_4 = 4,
   VID_DISPMODE_2x = 5,
   VID_DISPMODE_SCALE = 6,
   VID_DISPMODE_DISEXP
}vidDispMode_t;

typedef enum{
   VID_DISPSIZE_4x3,
   VID_DISPSIZE_16x9
}vidDispSize_t;

typedef enum{
   VID_SYNC_NO,
   VID_SYNC_VID,
   VID_SYNC_AUD
}vidSync_t;

typedef enum
{
    VID_STOPPED,
    VID_PLAYING,
    VID_FREEZED,
    VID_PAUSED,
    VID_FASTFORWORD,
    VID_SLOWMOTION,
    VID_STILLP,
    VID_SINGLE_FRAME
}vidState_t;

typedef struct tagRect
{
    int     hori_off;
    int     vert_off;
    int     hori_size;
    int     vert_size;
}RECT;

typedef struct tagScaleInfo
{
    RECT src;
    RECT des;
}SCALEINFO;


typedef struct tagVideoInfo
{
    unsigned short  h_size;
    unsigned short  v_size;
    unsigned short  pel_aspect_ratio;
    unsigned short  frame_rate;
}VIDEOINFO;

typedef struct tagReadInfo
{
    int     color;      //0:Y, 1:UV
    int     offset;     //read offset
    char*   buf;
    int     size;
}READINFO;

typedef struct tagVidPlayStatus
{
    unsigned reserved       :  30;
    unsigned freeze         :   1;
    unsigned picture_start  :   1;
}VIDPLAYSTATUS;

typedef enum
{
  VID_SFM_NORMAL = 0,
  VID_SFM_BOTTOM_ONLY,
  VID_SFM_TOP_ONLY,
  VID_SFM_FIRST_ONLY
} vidsfm_t;
  
#define VID_FF_IFRAME_ONLY          0
#define VID_FF_IPFRAME              1

#endif
