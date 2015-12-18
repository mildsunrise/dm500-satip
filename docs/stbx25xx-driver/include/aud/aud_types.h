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
|       COPYRIGHT   I B M   CORPORATION 2001, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| File:   aud_types.h
| Purpose: Definitions for various audio control structures
| Changes:
| Date:     Comment:
| -----     --------
| 15-Oct-01 create
| 17-Jun-03 Added defintion for audioStatus structure
| 17-Jun-03 Added definitions for 8-bit UPCM
| 25-Jun-03 Added support for DTS/LPCM coreload
+----------------------------------------------------------------------------*/
#ifndef PALLAS_AUD_TYPES_H
#define PALLAS_AUD_TYPES_H

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

typedef enum{
   AUD_SOURCE_DEMUX,
   AUD_SOURCE_MEMORY,
   AUD_SOURCE_NO,
}audSource_t;

typedef enum
{
    AUD_CHANNEL_STEREO,
    AUD_CHANNEL_MONOLEFT,
    AUD_CHANNEL_MONORIGHT
}audChannel_t;

typedef enum
{
    AUD_STREAM_TYPE_ES,
    AUD_STREAM_TYPE_PCM,
    AUD_STREAM_TYPE_PES,
    AUD_STREAM_TYPE_MPEG1,
    AUD_STREAM_TYPE_UNKNOWN
}audStream_t;


typedef enum
{
    AUD_MODE_AC3,
    AUD_MODE_STB_MPEG,
    AUD_MODE_STB_PCM,
    AUD_MODE_DTS,
    AUD_MODE_LPCM
}audMode_t;


typedef struct tagAudVol
{
    unsigned char frontleft;
    unsigned char frontright;
    unsigned char rearleft;
    unsigned char rearright;
    unsigned char center;
    unsigned char lfe;
}AUDVOL;


typedef struct audioStatus
{
  unsigned long dsp_status;
  unsigned long stream_decode_type;
  unsigned long sample_rate;
  unsigned long bit_rate;
  unsigned long raw[64 / sizeof(unsigned long)];
} audioStatus;

// YYD, July 17, 2001
// Add for PCM support
typedef enum 
{
   AUD_PCM_STEREO   = 0,
   AUD_PCM_MONO
}
AUD_PCM_CHANNELS;

typedef enum
{
   AUD_PCM_20BIT    = 0,
   AUD_PCM_18BIT,
   AUD_PCM_16BIT,
   AUD_PCM_8BIT
}
AUD_PCM_SAMPLE_BITS;

typedef enum
{
   AUD_PCM_16KHZ    = 1,
   AUD_PCM_22_05KHZ = 2,
   AUD_PCM_24KHZ    = 3,
   AUD_PCM_32KHZ    = 5,
   AUD_PCM_44_1KHZ  = 6,
   AUD_PCM_48KHZ    = 7,

   // untested modes
   AUD_PCM_8KHZ     = 9,
   AUD_PCM_11_025KHZ= 10,
   AUD_PCM_12KHZ    = 11,
   AUD_PCM_64KHZ    = 13,
   AUD_PCM_88_2KHZ  = 14,
   AUD_PCM_96KHZ    = 15
}
AUD_PCM_SAMPLE_FREQ;

typedef enum
{
   AUD_PCM_SIGNED = 0,
   AUD_PCM_UNSIGNED
}
AUD_PCM_DATA_SIGN;

typedef enum
{
   AUD_PCM_BIG_ENDIAN = 0,
   AUD_PCM_LITTLE_ENDIAN
}
AUD_PCM_DATA_ENDIAN;

typedef struct tagPCMFormat
{
   AUD_PCM_CHANNELS     channel;
   AUD_PCM_SAMPLE_BITS  bits;
   AUD_PCM_SAMPLE_FREQ  freq;
   AUD_PCM_DATA_SIGN    sign;
   AUD_PCM_DATA_ENDIAN  endian;
}AUD_PCM_FORMAT_CONFIG; 
// end of YYD add

#define AUD_SETTING_UNMUTE	0
#define AUD_SETTING_MUTE	1

#endif
