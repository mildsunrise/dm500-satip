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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| File:   aud_osi.h
| Purpose: audio driver osi layer PALLAS
| Changes:
| Date:     Comment:
| -----     --------
| 15-Oct-01 create
| 17-Jun-03 Added definition for audio status area in temp region
| 17-Jun-03 Added definition for call to get the current audio status
| 23-Jul-03 Added definition for a microcode description structure
| 25-Jul-03 Added MPEG_A_MEM_SIZE define
+----------------------------------------------------------------------------*/
#ifndef PALLAS_AUD_OSI_H
#define PALLAS_AUD_OSI_H

#include <hw/physical-mem.h>
#include <os/helper-queue.h>
#include "clip/clip.h"
#include "aud_atom.h"
#include "hw/physical-mem.h"

#define ADDR4K(x) (PAGE_SIZE*((x+PAGE_SIZE-1)/PAGE_SIZE))

#define MPEG_A_MEM_START __STB_A_MEM_BASE_ADDR
#define MPEG_A_MEM_SIZE  __STB_A_MEM_SIZE
//modified by shaol extend temp area from 32k to 96k for MP3 playback
#define MPEG_A_SEGMENT1_SIZE		0x00028000
#define MPEG_A_SEGMENT2_SIZE            0x00000800
#define MPEG_A_SEGMENT3_SIZE            0x00010000

//swith physical start area because mmap must start from the edge of 4k
#define MPEG_A_SEGMENT1                 MPEG_A_MEM_START
#define MPEG_A_SEGMENT3                 MPEG_A_SEGMENT1+ MPEG_A_SEGMENT1_SIZE
#define MPEG_A_SEGMENT2                 MPEG_A_SEGMENT3+ MPEG_A_SEGMENT3_SIZE

#define MPEG_A_CLIP_BUF_START           ADDR4K(MPEG_A_SEGMENT2 + MPEG_A_SEGMENT2_SIZE)
#define MPEG_A_CLIP_BUF_LEN             3*(32*1024)

#define MPEG_A_MIXER_BUF_START          ADDR4K(MPEG_A_CLIP_BUF_START + MPEG_A_CLIP_BUF_LEN)
#define MPEG_A_MIXER_BUF_LEN            3*(32*1024)

#define MPEG_A_RB_OFFSET                0x00080010

#define MPEG_A_STATUS_START (MPEG_A_SEGMENT1 + ((MPEG_A_RB_OFFSET & 0xff) << 12))
#define MPEG_A_STATUS_SIZE  64

//The end of audio memory is 0xA0069000 if the __STB_A_MEM_BASE_ADDR
//is set so that all the buffers fall on the correct alignment

#define AUD_CLIP_QUEUE_SIZE             4

/*typedef enum
{
    AUD_SOURCE_NO,
    AUD_SOURCE_DEMUX,
    AUD_SOURCE_MEMORY
}audSource_t;*/

typedef enum
{
    AUD_STOPPED,
    AUD_PLAYING,
}audState_t;

struct aud_ucode_info {
  char *name;
  char *ucode;
  int  ucode_len;
  char *data;
  int  data_len;
};

extern struct aud_ucode_info *aud_ucode_info[];

typedef struct tagADEC
{
    /*-------------------------------------------------------
    | audio decoder status
    +--------------------------------------------------------*/
    audSource_t 	src;
    audState_t      state;
    audMode_t       mode;
    audChannel_t    channel;
    audStream_t     stream;
    
    UINT            uSync;    
    UINT            uMute;
    UINT            uOpenFlag;

    ULONG           *ulStatusArea; /* virtual addr of status area */

    /*--------------------------------------------------------
    | clip information for main clip channel
    +---------------------------------------------------------*/
    CLIPDEV_T       clipdev;
    ULONG           ulClipBufPhyAdr;
    ULONG           ulClipBufLen;

    /*--------------------------------------------------------
    | clip information for main pcm mixing channel
    +---------------------------------------------------------*/
    CLIPDEV_T       mixer;
    ULONG           ulMixerBufPhyAdr;
    ULONG           ulMixerBufLen;   

    //PCM funtions
    AUD_PCM_FORMAT_CONFIG pcm_fmt;
}ADEC;

INT     aud_osi_init(audMode_t mode);
void    aud_osi_close();
//INT     aud_osi_set_source(audSource_t src);
INT     aud_osi_get_status(audioStatus *pStatus);
INT     aud_osi_get_openflag();
INT     aud_osi_play();
INT     aud_osi_stop();
INT     aud_osi_mute();
INT     aud_osi_unmute();
INT     aud_osi_set_vol(AUDVOL *vol);
INT     aud_osi_get_vol(AUDVOL *vol);
INT     aud_osi_set_stream_type(audStream_t stream);
INT     aud_osi_set_channel(audChannel_t channel);
INT     aud_osi_get_microcode_ver(ULONG *pVer);
INT     aud_osi_load_dfile(void* base, const unsigned char *dfile, int df_len);
CLIPDEV_T aud_osi_get_clipdev();
CLIPDEV_T aud_osi_get_mixerdev();

//Synchronization group:
INT     aud_osi_set_pts(STC_T *pData);
INT     aud_osi_get_pts(STC_T *pData);
INT     aud_osi_set_stc(STC_T *pData);
INT     aud_osi_get_stc(STC_T *pData);
INT     aud_osi_enable_sync(UINT uAudMaster);
INT     aud_osi_disable_sync();

//TV group
INT     aud_osi_init_tv();
void    aud_osi_close_tv();

//Clip group
INT     aud_osi_init_clip(ULONG ulClipBufPhyAdr, ULONG ulClipBufLen);
void    aud_osi_close_clip();
INT     aud_osi_set_pcm_fmt(AUD_PCM_FORMAT_CONFIG *pcm_fmt);
INT     aud_osi_clip_write(CLIPINFO *info);
INT     aud_osi_reset_clipbuf();
INT     aud_osi_clip_flush(CLIPDEV_T clipdev);

//mixer group
INT     aud_osi_init_mixer(ULONG ulMixerBufPhyAdr, ULONG ulMixerBufLen);
void    aud_osi_close_mixer();
INT     aud_osi_set_mixer_fmt(AUD_PCM_FORMAT_CONFIG *pcm_fmt);
INT     aud_osi_mixer_write(CLIPINFO *info);
INT     aud_osi_set_mixer_vol(AUDVOL *vol);
INT     aud_osi_get_mixer_vol(AUDVOL *vol);


//Task
void    aud_osi_task(QUEUE_T *pQueue);

#endif
