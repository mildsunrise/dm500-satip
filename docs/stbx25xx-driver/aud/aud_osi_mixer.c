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
| File:   aud_osi.c
| Purpose: audio driver osi layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#include <os/os-interrupt.h>
#include <os/drv_debug.h>
#include "aud_osi.h"

extern ADEC    _adec;

INT aud_osi_init_mixer(ULONG ulMixerBufPhyAdr, ULONG ulMixerBufLen)
{
    if(_adec.uOpenFlag == 0)
    {
        PDEBUG("audio decoder not initialized\n");
        return -1;
    }
    _adec.ulMixerBufPhyAdr = ulMixerBufPhyAdr;
    _adec.ulMixerBufLen  = ulMixerBufLen;

    aud_atom_init_mixer();

    //create clip device
    if(ulMixerBufLen%(AUD_CLIP_QUEUE_SIZE*256) != 0)
    {
        PDEBUG("clip buffer not 4k aligned \n");
        return -1;
    }

    if((_adec.mixer = clipdev_create(ulMixerBufLen / AUD_CLIP_QUEUE_SIZE,
                                       AUD_CLIP_QUEUE_SIZE,
                                       aud_osi_mixer_write,
                                       aud_atom_mixer_buf_ready)) == NULL)
    {
	    PDEBUG("create clip device error\n");
        return -1;
    }
    return 0;
}

void aud_osi_close_mixer()
{   
    PDEBUG("close mixer\n");
    aud_atom_close_mixer();
    //delete clip device
    if(_adec.mixer)
    {
        clipdev_delete(_adec.mixer);
        _adec.mixer = NULL;
    }
}

INT aud_osi_mixer_write(CLIPINFO *info)
{
    //clip start address = clip base physical address + start offset
    info->uClipAdrOff += _adec.ulMixerBufPhyAdr;
    aud_atom_mixer_write(info);
    return 0;
}

int aud_osi_set_mixer_fmt(AUD_PCM_FORMAT_CONFIG *pcm_fmt)
{
    return aud_atom_set_mixer_fmt(pcm_fmt);
}

INT  aud_osi_set_mixer_vol(AUDVOL *vol)
{
    aud_atom_set_vol(vol);
    return 0;
}
INT  aud_osi_get_mixer_vol(AUDVOL *vol)
{
    aud_atom_get_vol(vol);
    return 0;
}
