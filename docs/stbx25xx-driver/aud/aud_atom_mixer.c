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
| File:   aud_atom.c
| Purpose: audio driver atom layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#include <os/os-io.h>
#include <os/drv_debug.h>
#include <os/os-sync.h>
#include "aud_atom.h"
#include "aud_atom_hw.h"

void aud_atom_init_mixer()
{
    unsigned long reg;

    PDEBUG("init mixer\n");
    reg = MF_DCR(AUD_CTRL0);

    MT_DCR(AUD_CTRL0, reg | DECOD_AUD_CTRL0_MIX_EN);
}

void aud_atom_close_mixer()
{
    unsigned long reg;

    reg = MF_DCR(AUD_CTRL0) & (~DECOD_AUD_CTRL0_MIX_EN);

    MT_DCR(AUD_CTRL0, reg);
}

int aud_atom_mixer_buf_ready()
{
    return (MF_DCR(AUD_QLR2) & DECOD_AUD_QLR_BV) ? 0 : 1;
}

void aud_atom_mixer_write(CLIPINFO *pInfo)
{
    unsigned long reg;

    PDEBUG("mixer write start = 0x%8.8lx, len = 0x%8.8lx \n", pInfo->uClipAdrOff, pInfo->uClipLen);

    reg = pInfo->uClipLen & 0x1fffff;
    //write to audio decoder
    MT_DCR(AUD_QAR2, pInfo->uClipAdrOff);  //at most 2M
    MT_DCR(AUD_QLR2, reg | DECOD_AUD_QLR_BV);
}

void aud_atom_set_mixer_vol(AUDVOL* pVol)
{
    unsigned long reg;
    unsigned char vol;

    if (pVol->frontleft > 63)
        vol = 63;
    else
        vol = pVol->frontleft;

    reg = MF_DCR(AUD_DSP_CTRL);
    reg = (reg & ~(0x3f << 16)) | ((vol & 0x03f) << 16);
    MT_DCR(AUD_DSP_CTRL, reg);
}

void aud_atom_get_mixer_vol(AUDVOL* pVol)
{
    unsigned long reg;
    reg = MF_DCR(AUD_DSP_CTRL) & AUD_DSP_MIX_VOL;
    pVol->center = reg >> 16;
    pVol->frontleft = reg >> 16;
    pVol->frontright = reg >> 16;
    pVol->lfe = reg >> 16;
    pVol->rearleft = reg >> 16;
    pVol->rearright = reg >> 16;
}

// YYD, added for PCM support
int aud_atom_set_mixer_fmt(AUD_PCM_FORMAT_CONFIG *pcmcfg)
{
   unsigned long val=0;
   PDEBUG("PCM_FMT: sign = %d, channel = %d,  bits = %d, freq = %d, endian=%d\n",
        pcmcfg->sign, pcmcfg->channel, pcmcfg->bits, pcmcfg->freq, pcmcfg->endian);

   // set parameteres
   val = MF_DCR(AUD_DSP_CTRL);
   printk("[dspc =0x%8.8x\n", (unsigned int)val);

   val &= ~AUD_DSP_MIX_MASK;  // clear current

   switch(pcmcfg->sign)
   {
   case AUD_PCM_SIGNED:
      val |= AUD_DSP_PCM_SIGNED;
      break;
   case AUD_PCM_UNSIGNED:
      val |= AUD_DSP_PCM_UNSIGNED;
      break;
   }
   
   switch(pcmcfg->channel)
   {
   case AUD_PCM_MONO:
      val |= AUD_DSP_MIX_MONO;
      break;
   case AUD_PCM_STEREO:
      val |= AUD_DSP_MIX_STEREO;
      break;
   }

   switch(pcmcfg->bits)
   {
   case AUD_PCM_16BIT:
   default:
      val |= AUD_DSP_MIX_16BIT;
      break;
   }
   
   switch(pcmcfg->freq)
   {
   case AUD_PCM_16KHZ:
      val |= AUD_DSP_MIX_16KHZ;
      break;
   case AUD_PCM_32KHZ:
      val |= AUD_DSP_MIX_32KHZ;
      break;
   case AUD_PCM_22_05KHZ:
      val |= AUD_DSP_MIX_22_05KHZ;
      break;
   case AUD_PCM_44_1KHZ:
      val |= AUD_DSP_MIX_44_1KHZ;
      break;
   case AUD_PCM_24KHZ:
      val |= AUD_DSP_MIX_24KHZ;
      break;
   case AUD_PCM_48KHZ:
      val |= AUD_DSP_MIX_48KHZ;
      break;
   // untested modes
   case AUD_PCM_8KHZ:
      val |= AUD_DSP_MIX_8KHZ;
      break;
   case AUD_PCM_11_025KHZ:
      val |= AUD_DSP_MIX_11_025KHZ;
      break;
   case AUD_PCM_12KHZ:
      val |= AUD_DSP_MIX_12KHZ;
      break;
   case AUD_PCM_64KHZ:
      val |= AUD_DSP_MIX_64KHZ;
      break;
   case AUD_PCM_88_2KHZ:
      val |= AUD_DSP_MIX_88_2KHZ;
      break;
   case AUD_PCM_96KHZ:
      val |= AUD_DSP_MIX_96KHZ;
      break;
   default:
      val |= AUD_DSP_MIX_44_1KHZ;
      break;
   }

   printk("mixer pcm value = 0x%8.8x]\n", (unsigned int)val);
   MT_DCR(AUD_DSP_CTRL, val);

   // set the endian of audio
/*
#if 1
   val = MF_DCR(AUD_CTRL2);

   val &= ~DECOD_AUD_CTRL2_ENDIAN_MASK;  // clear current

   switch(pcmcfg->endian)
   {
   case AUD_PCM_BIG_ENDIAN:
      val |= DECOD_AUD_CTRL2_BIG_ENDIAN;
      break;
   case AUD_PCM_LITTLE_ENDIAN:
      val |= DECOD_AUD_CTRL2_LITTLE_ENDIAN;
      break;
   }

   MT_DCR(AUD_CTRL2, val);
   
#endif

   MT_DCR(AUD_CMD, DECOD_AUD_CMD_DSP_RESET); // YYD, as required by uc 3.01 after set stream format
*/
   return 0;
}
// end of YYD added
