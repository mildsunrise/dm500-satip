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

INT aud_osi_init_clip(ULONG ulClipBufPhyAdr, ULONG ulClipBufLen)
{
    if(_adec.uOpenFlag == 0)
    {
        PDEBUG("audio decoder not initialized\n");
        return -1;
    }
    if(_adec.src != AUD_SOURCE_NO)
    {
        PDEBUG("audio decoder is in use, src = %d\n", _adec.src);
        return -1;
    }
    _adec.ulClipBufPhyAdr = ulClipBufPhyAdr;
    _adec.ulClipBufLen  = ulClipBufLen;

    aud_atom_init_clip();

    //create clip device
    if(ulClipBufLen%(AUD_CLIP_QUEUE_SIZE*256) != 0)
    {
        PDEBUG("clip buffer not 4k aligned \n");
        return -1;
    }

    if((_adec.clipdev = clipdev_create(ulClipBufLen / AUD_CLIP_QUEUE_SIZE,
                                       AUD_CLIP_QUEUE_SIZE,
                                       aud_osi_clip_write,
                                       aud_atom_buf_ready)) == NULL)
    {
	    PDEBUG("create clip device error\n");
        return -1;
    }
    _adec.src           = AUD_SOURCE_MEMORY;
    return 0;
}

void aud_osi_close_clip()
{
    clipdev_stop(_adec.clipdev);
    aud_atom_close_clip();

    if(_adec.state != AUD_STOPPED)
        aud_osi_stop();
    
    //delete clip device
    if(_adec.clipdev)
    {
        clipdev_delete(_adec.clipdev);
        _adec.clipdev = NULL;
    }

    _adec.src = AUD_SOURCE_NO;
}

INT aud_osi_clip_write(CLIPINFO *info)
{
    //clip start address = clip base physical address + start offset
    info->uClipAdrOff += _adec.ulClipBufPhyAdr;
    aud_atom_write_clip(info);
    return 0;
}


INT aud_osi_reset_clipbuf()
{
    //soft reset audio dsp
    aud_atom_reset_rb();

    if(_adec.clipdev)
    {
        clipdev_delete(_adec.clipdev);
        _adec.clipdev = NULL;
    }
    if((_adec.clipdev = clipdev_create(_adec.ulClipBufLen / AUD_CLIP_QUEUE_SIZE,
                                       AUD_CLIP_QUEUE_SIZE,
                                       aud_osi_clip_write,
                                       aud_atom_buf_ready)) == NULL)
    {
		PDEBUG("create clip device error\n");
        return -1;
    }
	return 0;
}

INT aud_osi_clip_flush(CLIPDEV_T clipdev)
{
    if(clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }
    /* Stop clip dev from queing any more buffers and clear the buffer que */
    clipdev_stop(clipdev);

    /* reset the hardware clip buffers */
    aud_atom_close_clip();
    
    /* re-enable clip mode */
    aud_atom_init_clip();
    return 0;
}

int aud_osi_set_pcm_fmt(AUD_PCM_FORMAT_CONFIG *pcm_fmt)
{
    _adec.pcm_fmt.channel = pcm_fmt->channel;
    _adec.pcm_fmt.bits = pcm_fmt->bits;
    _adec.pcm_fmt.freq = pcm_fmt->freq;
    _adec.pcm_fmt.sign = pcm_fmt->sign;
    _adec.pcm_fmt.endian = pcm_fmt->endian;

    return aud_atom_set_pcm_fmt(pcm_fmt);
}

inline CLIPDEV_T aud_osi_get_clipdev()
{
    return _adec.clipdev;
}

inline CLIPDEV_T aud_osi_get_mixerdev()
{
    return _adec.mixer;
}
