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
| File:   aud_atom.h
| Purpose: audio driver atom layer PALLAS
| Changes:
| Date:     Comment:
| -----     --------
| 15-Oct-01 create                  
| 17-Jun-03 added call definition for aud_atom_get_status
+----------------------------------------------------------------------------*/
#ifndef PALLAS_AUD_ATOM_H
#define PALLAS_AUD_ATOM_H

#include <os/os-types.h>
#include "clip/clip.h"
#include "aud/aud_types.h"


/*audio message between interrupt handler and task*/
#define AUD_MSG_BLOCK_READ      1
#define AUD_MSG_BLOCK_READ2     2

#define AUD_SYNC_RW_TIMEOUT     5

typedef struct tagADEC_CON
{
    ULONG           seg1;
    ULONG           seg2;
    ULONG           rb_base;
    UINT            rb_size;    //size: 0-F/ 4k-64k
    ULONG           rb_off;
    audMode_t       mode;       //
    //audStream_t     stream;
}ADEC_CON;


#ifndef TASK_MSG
typedef struct tagTaskMsg
{
  UINT    uMsgType;
  ULONG   ulPara1;
  ULONG   ulPara2;
}TASK_MSG;
#endif


//Common group:
INT     aud_atom_init(ADEC_CON *pAConf);
//void    aud_atom_set_type(UINT uType);
//UINT32  aud_atom_get_irq_mask();
//void    aud_atom_set_irq_mask(UINT32 uMask);
void    aud_atom_play();
void    aud_atom_stop();
void    aud_atom_close();
//void    aud_atom_set_memseg(MEM_SEG *pMem);
INT     aud_atom_set_stream_type(audStream_t stream);
INT     aud_atom_get_microcode_ver(ULONG *pVer);
INT     aud_atom_get_status(unsigned long *, audioStatus *);
void    aud_atom_mute();
void    aud_atom_unmute();
INT     aud_atom_get_mute_state();
void    aud_atom_get_vol(AUDVOL *vol);
void    aud_atom_set_vol(AUDVOL *vol);
void    aud_atom_set_channel(audChannel_t channel);
void    aud_atom_reg_dump();
INT     aud_atom_dsp_ready();
void    aud_atom_reset_rb();

//Synchronization group:
INT     aud_atom_set_pts(STC_T *pData);
INT     aud_atom_get_pts(STC_T *pData);
INT     aud_atom_set_stc(STC_T *pData);
INT     aud_atom_get_stc(STC_T *pData);
void    aud_atom_sync_master(UINT uAudMaster);
void    aud_atom_sync_on();
INT     aud_atom_sync_off();

//TV group:
void    aud_atom_init_tv();

//Clip group:
void    aud_atom_init_clip();
void    aud_atom_close_clip();
void    aud_atom_write_clip(CLIPINFO *pInfo);
INT     aud_atom_buf_ready();
INT     aud_atom_set_pcm_fmt(AUD_PCM_FORMAT_CONFIG *pcmcfg);

//mixer group:
void    aud_atom_init_mixer();
void    aud_atom_close_mixer();
int     aud_atom_mixer_buf_ready();
void    aud_atom_mixer_write(CLIPINFO *pInfo);
void    aud_atom_set_mixer_vol(AUDVOL* pVol);
void    aud_atom_get_mixer_vol(AUDVOL* pVol);
INT     aud_atom_set_mixer_fmt(AUD_PCM_FORMAT_CONFIG *pcmcfg);

//Interrupt group:
void    aud_atom_irq_handler(UINT uIrq, void *pData);   //interrupt handler
void    aud_atom_init_irq_mask();


#endif
