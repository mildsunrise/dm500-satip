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
| File:   aud_atom.c
| Purpose: audio driver atom layer PALLAS
| Changes:
| Date:     Comment:
| -----     --------
| 15-Oct-01 create 
| 21-Jan-03 Prevent audio pops due to turning on/off clocks to the DAC.
|           This main cause for this is due to the ADISO bit in CTRL0
|           being cleared by the reset to clear the rate buffer and
|           by the loading of microcode.  Fixed by using the reset cmd
|           that will specifically clear the rate buffer instead of the
|           global audio reset (which also resets the CTRL regs), and
|           by not clearing the ADISO bit during microcode loading.
| 17-Jun-03 Initial implementation to get audio status.  MPEG/AC3/UPCM
| 17-Jun-03 Added UPCM selection for 8-bit UPCM streams
| 20-Jun-03 Change init/stop/start to use halt decoding (pause) instead of 
|           start parsing.  This should prevent "losing" stream data between 
|           stop/start and provided a quicker stop (pause).  The forcing 
|           on/off of mute was also removed (see aud_osi.c)
| 25-Jun-03 Added support for DTS/LPCM coreload and the AC3 decoding coreloads
| 25-Jun-03 Force encoded SPDIF output if decoding AC3 not available
| 25-Jun-03 Added stream status support for DTS/LPCM coreload
| 26-Jun-03 Changed default of _mute var since mute not forced on during open
| 22-Jul-03 Fixed LPCM status bit definitions
| 23-Jul-03 Fill audio microcode description structure variable
| 25-Jul-03 Added check to be sure that the audio physical memory size defined was large enough
+----------------------------------------------------------------------------*/
#include <os/os-io.h>
#include <os/os-interrupt.h>
#include <os/drv_debug.h>
#include <os/os-sync.h>
#include "aud_osi.h"
#include "aud_atom.h"
#include "aud_atom_hw.h"

#include "aud_uc.h"
#include "astb_d.h"
struct aud_ucode_info stb_info = {
  "s5amp3",
  aud_ucode_stb, sizeof(aud_ucode_stb_len),
  astb_d, sizeof(astb_d),
};
#ifdef AC3_ENABLE
#include "aud_dvd.h"
#include "advd_d.h"
struct aud_ucode_info dvd_info = {
  "s5advdr", 
  aud_ucode_dvd, sizeof(aud_ucode_dvd),
  advd_d, sizeof(advd_d),
};
#endif
#ifdef DTS_ENABLE
#include "aud_dts.h"
#include "adts_d.h"
struct aud_ucode_info dts_info = {
  "s5adts", 
  aud_ucode_dts, sizeof(aud_ucode_dts),
  adts_d, sizeof(adts_d),
};
#endif

struct aud_ucode_info *aud_ucode_info[] = {
  &stb_info,
#if AC3_ENABLE
  &dvd_info,
#endif
#if AC3_ENABLE
  &dts_info,
#endif
  NULL
};

/*notify channel change done to demux*/
extern  int aud_cc_done();
extern  void vid_atom_set_stc(STC_T *pData);

void*   _seg1_log = NULL;

int     _mute = 0;

/*local definition*/
#define DEF_AUD_IRQ_MASK        DECOD_AUD_IMR_CCC | \
                                DECOD_AUD_IMR_CM  | \
                                DECOD_AUD_IMR_CM2 | \
                                DECOD_AUD_IMR_RTBC | \
                                DECOD_AUD_IMR_AMSI

int aud_atom_load_microcode(const unsigned short *code, int count);
INT aud_atom_load_dfile(void *base, const unsigned char *dfile, 
                        int df_len, unsigned long rb_off);
inline void aud_atom_reset_rb();

INT aud_atom_init(ADEC_CON *pAConf)
{
    unsigned long reg;
    const USHORT *code; 
    const unsigned char *data;
    int count, dcount;
    unsigned long control;

    /*-------------------------------------------------------------------------+
    | Disable interrupts during reset.
    +-------------------------------------------------------------------------*/
    // disable_irq(STB_AUD_INT);  // YYD disabled
    /*-------------------------------------------------------------------------+
    | Reset the core.
    +-------------------------------------------------------------------------*/

    aud_atom_reset_rb();

    /*-------------------------------------------------------------------------+
    | After soft reset here must be at least 20 processor cycles before
    | another read/write to MPEG audio DCR.
    +-------------------------------------------------------------------------*/
    os_cpu_clock_delay(30);

    // clear all buffer
    //mpeg_a_buf_blank(pADEC);
    /*-------------------------------------------------------------------------+
    | Write the MPEG audio code segment registers.
    +-------------------------------------------------------------------------*/
    if ((pAConf->seg1 & DECOD_AUD_SEG_ALIGN) != 0)
    {
        printk("ERROR: MPEG_A_MEM_START value must be 0x%8.8x byte aligned\n",DECOD_AUD_SEG_ALIGN);
        return (-1);
    }

    if ((pAConf->seg2 & DECOD_AUD_SEG_ALIGN) != 0)
    {
        printk("ERROR: Audio segment 2 value must be 0x%8.8x byte aligned\n",DECOD_AUD_SEG_ALIGN);
        return (-1);
    }

    if ((pAConf->rb_base & DECOD_AUD_SEG_ALIGN) != 0)
    {
        printk("ERROR: Audio rate buffer base must be 0x%8.8x byte aligned\n",DECOD_AUD_SEG_ALIGN);
        return (-1);
    }

    PDEBUG("MPEG_A_SEGMENT1 = 0x%8.8x and ends at 0x%8.8x\n",MPEG_A_SEGMENT1,MPEG_A_SEGMENT1+MPEG_A_SEGMENT1_SIZE);
    PDEBUG("MPEG_A_SEGMENT3 = 0x%8.8x and ends at 0x%8.8x\n",MPEG_A_SEGMENT3,MPEG_A_SEGMENT3+MPEG_A_SEGMENT3_SIZE);
    PDEBUG("MPEG_A_SEGMENT2 = 0x%8.8x and ends at 0x%8.8x\n",MPEG_A_SEGMENT2,MPEG_A_SEGMENT2+MPEG_A_SEGMENT2_SIZE);
    PDEBUG("MPEG_A_CLIP_BUF_START = 0x%8.8x and ends at 0x%8.8x\n",MPEG_A_CLIP_BUF_START,MPEG_A_CLIP_BUF_START+MPEG_A_CLIP_BUF_LEN);
    PDEBUG("MPEG_A_MIXER_BUF_START = 0x%8.8x and ends at 0x%8.8x\n",MPEG_A_MIXER_BUF_START,MPEG_A_MIXER_BUF_START+MPEG_A_MIXER_BUF_LEN);

    /*-------------------------------------------------------------------------+
    | Make sure the defined audio size is large enough.
    +-------------------------------------------------------------------------*/
    if(((MPEG_A_MIXER_BUF_START + MPEG_A_MIXER_BUF_LEN)- MPEG_A_SEGMENT1) > MPEG_A_MEM_SIZE)
    {
       printk("ERROR: MPEG_A_MEM_SIZE 0x%8.8x is not large enough to cover the audio memory allocations 0x%8.8x\n"
       ,MPEG_A_MEM_SIZE,(int)((MPEG_A_MIXER_BUF_START + MPEG_A_MIXER_BUF_LEN)- MPEG_A_SEGMENT1) );
       return (-1);
    }

    PDEBUG("setting audio segment 1 to 0x%8x\n", pAConf->seg1);
    MT_DCR(AUD_SEG1, pAConf->seg1 >> DECOD_AUD_SEG_SH);
    PDEBUG("setting audio segment 2 to 0x%8x\n", pAConf->seg2);
    MT_DCR(AUD_SEG2, pAConf->seg2 >> DECOD_AUD_SEG_SH);
    /*-------------------------------------------------------------------------+
    | 64K rate buffer is configured in following statement.
    +-------------------------------------------------------------------------*/
    PDEBUG("setting audio segment 3 to 0x%8x\n", pAConf->rb_base);
    MT_DCR(AUD_SEG3, (pAConf->rb_base >> DECOD_AUD_SEG_SH) | (0xF << 28));
    //       ((pAConf->rb_size & 0xF) << 28));
    /*-------------------------------------------------------------------------+
    | Initialize offsets for bank1.
    +-------------------------------------------------------------------------*/
    MT_DCR(AUD_OFFSETS, pAConf->rb_off);

    /*-------------------------------------------------------------------------+
    | Check the microcode load type.
    +-------------------------------------------------------------------------*/
    code = (const USHORT*)aud_ucode_stb; /* Set default coreload to mpeg      */
    count = aud_ucode_stb_len;
    data = astb_d; 
    dcount = astb_d_len;

    switch (pAConf->mode)
    {
        case AUD_MODE_AC3:
#ifdef AC3_ENABLE
	  if (aud_ucode_dvd_len > 0) {
            code = (const USHORT*)aud_ucode_dvd;
            count = aud_ucode_dvd_len;
	    data = advd_d;
	    dcount = advd_d_len;
	    control = DECOD_AUD_CTRL2_DM |
	      DECOD_AUD_CTRL2_LL | DECOD_AUD_CTRL2_HL |
	      DECOD_AUD_CTRL2_DN | DECOD_AUD_CTRL2_AC3;
	  } else 
#endif
	    if (aud_ucode_stb_len > 0) {
	      control = DECOD_AUD_CTRL2_ID | DECOD_AUD_CTRL2_AC3;
	    } else {
	      return (-1);
	    }
	  break;

        case AUD_MODE_STB_MPEG:
	  if (aud_ucode_stb_len == 1)
            {
	      return (-1);
            }
	  control = DECOD_AUD_CTRL2_MPEG;
	  break;

        case AUD_MODE_STB_PCM:
	  if (aud_ucode_stb_len == 1)
            {
	      return (-1);
            }
	  control = DECOD_AUD_CTRL2_PCM;
	  break;

#ifdef DTS_ENABLE
        case AUD_MODE_DTS:
	  if (aud_ucode_dts_len > 0) {
            code = (const USHORT*)aud_ucode_dts;
            count = aud_ucode_dts_len;
	    data = adts_d;
	    dcount = adts_d_len;
            control = DECOD_AUD_CTRL2_ID | 
	              DECOD_AUD_CTRL2_DTS;
	  } else {
	    return (-1);
	  }
	  break;

        case AUD_MODE_LPCM:
	  if (aud_ucode_dts_len > 0) {
            code = (const USHORT*)aud_ucode_dts;
            count = aud_ucode_dts_len;
	    data = adts_d;
	    dcount = adts_d_len;
            control = DECOD_AUD_CTRL2_LPCM;
	  } else {
	    return (-1);
	  }
	  break;
#endif
    default:
      return (-1);
    }

    if(aud_atom_load_microcode(code, count/2) != 0)
    {
        PDEBUG("load microcode error\n");
        return -1;
    }
    PDEBUG("load microcode OK\n");

    if(_seg1_log != NULL)
    {
        if( aud_atom_load_dfile((void*)_seg1_log, data, dcount, pAConf->rb_off) < 0)
        {
            PDEBUG("load data file error\n");
            return -1;
        }
    }

    /*-------------------------------------------------------------------------+
    | Setup stream ID register.
    +-------------------------------------------------------------------------*/
    if(pAConf->mode == AUD_MODE_STB_MPEG)  /* If MPEG, set MPEG pes id's      */
    {
        MT_DCR(AUD_STREAM_ID, DECOD_AUD_DEF_ID);
    }
    else                                   /* All others use private stream id*/
    {
        MT_DCR(AUD_STREAM_ID, DECOD_AUD_DEF_AC3_ID);
    }

    /*-------------------------------------------------------------------------+
    | Start DSP.  Download end flag must be set.
    +-------------------------------------------------------------------------*/
    MT_DCR(AUD_CTRL0,
               DECOD_AUD_CTRL0_START_DECODER | 
               DECOD_AUD_CTRL0_DOWNLOAD_END  |
               DECOD_AUD_CTRL0_START_PARSING |
               DECOD_AUD_CTRL0_ENABLE_INT);
    /*-------------------------------------------------------------------------+
    | Enable DACs
    +-------------------------------------------------------------------------*/
#ifdef CONFIG_OLIVIA // ShaoLin add for Olivia

    MT_DCR(AUD_CTRL1, MF_DCR(AUD_CTRL1) | DECOD_AUD_CTRL1_DAC_EN | 0x08 | 0x01);

#endif

    reg = MF_DCR(AUD_CTRL1) | DECOD_AUD_CTRL1_DAC_EN;
    MT_DCR(AUD_CTRL1, reg);

    /*-------------------------------------------------------------------------+
    | Set correct mode.
    +-------------------------------------------------------------------------*/
    MT_DCR(AUD_CTRL2, control);
    aud_atom_reset_rb(); // YYD, as required by uc 3.01 after set stream format

    return (0);
}

void aud_atom_close()
{
    unsigned long reg;
    /*-------------------------------------------------------------------------+
    | Reset the rate buffers.
    +-------------------------------------------------------------------------*/
    aud_atom_reset_rb();

    //stop decoder and disable interrupt
    reg = MF_DCR(AUD_CTRL0) & 
                (~(DECOD_AUD_CTRL0_START_DECODER | 
                   DECOD_AUD_CTRL0_ENABLE_INT));

    MT_DCR(AUD_CTRL0, reg);
}

INT aud_atom_set_stream_type(audStream_t stream)
{
    unsigned long stream_type;
    unsigned long reg;
    //    unsigned long ctrl2;

    /*-------------------------------------------------------------------------+
    | Reset the core.
    +-------------------------------------------------------------------------*/

    aud_atom_reset_rb();

    /*-------------------------------------------------------------------------+
    | After soft reset here must be at least 20 processor cycles before
    | another read/write to MPEG audio DCR.
    +-------------------------------------------------------------------------*/
    os_cpu_clock_delay(30);

    reg = MF_DCR(AUD_CTRL0) & (~DECOD_AUD_CTRL0_TYPE_MPEG1);
    //    ctrl2 = MF_DCR(AUD_CTRL2) & (~DECOD_AUD_CTRL2_PCM);
    switch (stream)
    {
        case AUD_STREAM_TYPE_MPEG1:
            stream_type = DECOD_AUD_CTRL0_TYPE_MPEG1;
	    //            ctrl2 |= DECOD_AUD_CTRL2_MPEG;
            break;

        case AUD_STREAM_TYPE_PES:
            stream_type = DECOD_AUD_CTRL0_TYPE_PES;
	    //            ctrl2 |= DECOD_AUD_CTRL2_MPEG;
            break;

        case AUD_STREAM_TYPE_ES:
            stream_type = DECOD_AUD_CTRL0_TYPE_ES;
	    //            ctrl2 |= DECOD_AUD_CTRL2_MPEG;
            break;

        case AUD_STREAM_TYPE_PCM:  // YYD, add for PCM support
            stream_type = DECOD_AUD_CTRL0_TYPE_ES;
	    //            ctrl2 |= DECOD_AUD_CTRL2_PCM;
            break;

        default:
            return (-1);
    }

    PDEBUG("new stream type =%ld\n", stream_type);
    MT_DCR(AUD_CTRL0, reg | stream_type | DECOD_AUD_CTRL0_START_DECODER);

    return 0;
}

/* Stops (pauses) stream decoding at end of frame */
void aud_atom_stop()
{
  MT_DCR(AUD_CTRL2, MF_DCR(AUD_CTRL2) | DECOD_AUD_CTRL2_HD);
}

/* re-starts paused stream decoding */
void aud_atom_play()
{
  MT_DCR(AUD_CTRL2, MF_DCR(AUD_CTRL2) & ~DECOD_AUD_CTRL2_HD);
}

void aud_atom_mute(void)
{

    unsigned long reg;

    reg = MF_DCR(AUD_CTRL1) | DECOD_AUD_CTRL1_SOFT_MUTE;
    MT_DCR(AUD_CTRL1, reg);
    reg = MF_DCR(AUD_CTRL2) | DECOD_AUD_CTRL2_MUTE;
    MT_DCR(AUD_CTRL2, reg);
    _mute = 1;
    PDEBUG("mute\n");
}

void aud_atom_unmute(void)
{

    unsigned long reg;

    reg = MF_DCR(AUD_CTRL1) & (~DECOD_AUD_CTRL1_SOFT_MUTE);
    MT_DCR(AUD_CTRL1, reg);
    reg = MF_DCR(AUD_CTRL2) & (~DECOD_AUD_CTRL2_MUTE);
    MT_DCR(AUD_CTRL2, reg);
    _mute = 0;
    PDEBUG("unmute\n");
}

#ifdef DTS_ENABLE
/* LPCM tables */
#define A_LPCM_STAT_EMPH_MASK 0x8000

#define A_LPCM_STAT_BIT_MASK  0x00C0
#define A_LPCM_STAT_BIT_SHIFT 6
#define A_LPCM_STAT_BIT_16    0x0000
#define A_LPCM_STAT_BIT_20    0x0040
#define A_LPCM_STAT_BIT_24    0x0080
#define A_LPCM_STAT_BIT_INV   0x00C0

#define A_LPCM_STAT_FS_MASK   0x0030
#define A_LPCM_STAT_FS_SHIFT  4
#define A_LPCM_STAT_FS_48KHZ  0x0000
#define A_LPCM_STAT_FS_96KHZ  0x0010
#define A_LPCM_STAT_FS_INV    0x0020

#define A_LPCM_CHAN_MASK      0x0007

long lpcm_fs[] = { 48000, 96000, 0, 0 };
long lpcm_bit[] = { 16, 20, 24, 0 };

INT aud_atom_lpcm_status (audioStatus *pStatus)
{
  unsigned long astat;

  astat = pStatus->raw[2] >> 16;

  if (0 == (pStatus->sample_rate = 
	    lpcm_fs[(astat & A_LPCM_STAT_FS_MASK) >> A_LPCM_STAT_FS_SHIFT])) {
    return 0;
  }

  if (0 == (pStatus->bit_rate = 
	    lpcm_bit[(astat & A_LPCM_STAT_BIT_MASK) >> A_LPCM_STAT_BIT_SHIFT])) {
    return 0;
  }

  pStatus->bit_rate *= 
    pStatus->sample_rate * ((astat & A_LPCM_CHAN_MASK) + 1);

  return 1;
}

/* DTS tables  */
#define A_DTS_NBLKS ((pStatus->raw[2] & 0x007f0000) >> 16)
#define A_DTS_FSIZE  (pStatus->raw[2] & 0x00003fff)
#define A_DTS_FS    ((pStatus->raw[3] & 0x01e00000) >> 21)
#define A_DTS_RATE  ((pStatus->raw[3] & 0x001f0000) >> 16)
#define A_DTS_XTEND  (pStatus->raw[3] & 0x00000010)
#define A_DTS_XVAL  ((pStatus->raw[3] & 0x000000e0) >> 5)

long dts_fs[16] = {
  0, 8000, 16000, 32000,
  0,  0, 11025, 22050, 44100,
  0,  0, 12000, 24000, 48000,
  0,  0
};

long dts_rate[32] = {
   32000,   56000,   64000,   96000,  112000,  128000,  192000,  224000,
  256000,  320000,  384000,  448000,  512000,  576000,  640000,  768000,
  960000, 1024000, 1152000, 1280000, 1344000, 1408000, 1411200, 1472000,
 1536000, 1920000, 2048000, 3072000, 3840000,       0,       0,       0 
};

long dts_chan[16] = {
  0+1  /* A */,
  0+2  /* A+B dual mono */,
  0+2  /* L+R stereo    */,
  0+2  /* (L+R) + (L-R) sum-difference */,
  0+2  /* LT + RT left/right total */,
  0+3  /* C + L + R */,
  0+3  /* L + R + S */,
  0+4  /* C + L + R + S */,
  0+4  /* L + R + SL + SR */,
  0+5  /* C + L + R + SL + SR */,
  0+6  /* CL + CR + L + R + SL + SR */,
  0+6  /* C + L + R + LR + RR + OV  */,
  0+6  /* CF + CR + LF +RF + LF + RR */,
  0+7  /* CL + C + CR + L + R + SL + SR */,
  0+8  /* CL + CR + L + R + SL1 + SL2 + SR + SR2 */,
  0+8  /* CL + C + CR + L + R + SL + S + SR */,
}; /* values >= 16 are user-defined */

INT aud_atom_dts_status (audioStatus *pStatus)
{
  pStatus->bit_rate = dts_rate[A_DTS_RATE];
  pStatus->sample_rate = dts_fs[A_DTS_FS];
  if (A_DTS_XTEND) {
    if ((A_DTS_XVAL == 2) || (A_DTS_XVAL == 3))
      pStatus->sample_rate *= 2;
  }

  return 1;
}
#endif

/* UPCM tables */
unsigned long upcm_sample_rate[16] = {
  0, 16000, 22050, 24000,
  0, 32000, 44100, 48000,
  0,  8000, 11025, 12000,
  0, 64000, 88200, 96000
};

INT aud_atom_upcm_status (audioStatus *pStatus)
{
  unsigned long astat;
  int num_bits = 1;

  astat = MF_DCR(AUD_DSP_CTRL);

  pStatus->sample_rate = upcm_sample_rate[astat & 0x0f];
  
  switch (astat & AUD_DSP_PCM_BIT_MASK) {
  case AUD_DSP_PCM_16BIT:
    num_bits = 16;
    break;

  case AUD_DSP_PCM_8BIT:
    num_bits = 8;
    break;

  default:
    num_bits = 24;
  };

  if (!(astat & AUD_DSP_PCM_MONO)) num_bits *= 2;
  pStatus->bit_rate = pStatus->sample_rate * num_bits;

  return 1;
}

/* Dolby AC3 status tables */
unsigned long ac3_sample_rate[4] =  { 48000, 44100, 32000, 0};
unsigned long ac3_bit_rate[32] = {
  32,  40,  48,  56,  64,  80,  96, 112,
 128, 160, 192, 224, 256, 320, 384, 448,
 512, 576, 640
};
#if 0
int ac3_num_channel = {
  2 /* 1+1 */, 1 /* mono */, 2 /* stereo */, 3 /* LCR */, 
  3 /* LRS */, 4 /* LCRS */, 4 /* LRLsRs */, 5 /* LCRLsRs */
}
#endif

INT aud_atom_ac3_status (audioStatus *pStatus)
{
  int i;
  unsigned long astat;

  astat = pStatus->raw[2];

  /* Lookup current sample rate */
  pStatus->sample_rate = ac3_sample_rate[(astat >> 16) & 0x03];

  /* Lookup current bit rate */
  i = (astat >> 23) & 0x1f;
  pStatus->bit_rate = (i > 18) ? -1 : ac3_bit_rate[i];

  return 1;
}

/* MPEG status tables */
unsigned long mpeg_sample_rate[4] = { 44100, 48000, 32000, 0};
unsigned long mpeg_bit_rate[2][3][16] = {
  {    /* ID == 0 (LSF) */
    {   0,  32,  48,  56,  64,  80,  96, 112, /* Layer I   */
	128, 144, 160, 176, 192, 224, 256, -1}, 
    {   0,   8,  16,  24,  32,  40,  48,  56, /* Layer II  */
	 64,  80,  96, 112, 128, 144, 160, -1}, 
    {   0,   8,  16,  24,  32,  40,  48,  56, /* Layer III */
	 64,  80,  96, 112, 128, 144, 160, -1}, 
  },
  {    /* ID == 1 */
    {   0,  32,  64,  96, 128, 160, 192, 224, /* Layer I   */
	256, 288, 320, 352, 384, 416, 448, -1}, 
    {   0,  32,  48,  56,  64,  80,  96, 112, /* Layer II  */
	128, 160, 192, 224, 256, 320, 384, -1}, 
    {   0,  32,  40,  48,  56,  64,  80,  96, /* Layer III */
	112, 128, 160, 192, 224, 256, 320, -1}
  }
};

INT aud_atom_mpeg_status (audioStatus *pStatus)
{
  unsigned long astat, astat1;
  int layer, id;

  astat = pStatus->raw[2];
  astat1 = pStatus->raw[3];
  layer = ((astat >> 17) & 0x03) ^ 0x03;
  id = (astat >> 19) & 0x01;

  if (layer == 3) return 0; /* if invalid layer, exit */

  /* Lookup current sample rate */
  pStatus->sample_rate = mpeg_sample_rate[((astat >> 10) & 0x3)];
  
  if (id == 0)
    pStatus->sample_rate /= 2;

  /* Lookup current bit rate */
  pStatus->bit_rate = 
    mpeg_bit_rate[id][layer][(astat >> 12) & 0x0f];
    
  return 1;
}

INT aud_atom_get_status(unsigned long *aud_status_mem_ptr,
			audioStatus *pStatus)
{
  int i;

  /* Get and store audio status register */
  pStatus->dsp_status = MF_DCR(AUD_DSP_STATUS);

  if (!(pStatus->dsp_status & 0x02)) { /* check if valid play */
    pStatus->bit_rate = 0;
    pStatus->sample_rate = 0;
    for (i = 0; i < (64 / sizeof(unsigned long)); i++)
      pStatus->raw[i] = 0;

    return 0;
  }

  /* Get and store raw status from status memory area */
  for (i = 0; i < (64 / sizeof(unsigned long)); i++)
    pStatus->raw[i] = *aud_status_mem_ptr++;

  /* call appropriate decode status generation subroutine */
  switch (pStatus->stream_decode_type) {
  case AUD_MODE_STB_PCM:
    return aud_atom_upcm_status(pStatus);

  case AUD_MODE_AC3:
    return aud_atom_ac3_status(pStatus);

#ifdef DTS_ENABLE
  case AUD_MODE_DTS:
    return aud_atom_dts_status(pStatus);

  case AUD_MODE_LPCM:
    return aud_atom_lpcm_status(pStatus);
#endif

  case AUD_MODE_STB_MPEG:
    return aud_atom_mpeg_status(pStatus);
  }

  return 0;
}

INT aud_atom_get_mute_state()
{
    return _mute;
}

void aud_atom_get_vol(AUDVOL* pVol)
{
    unsigned long val;
    unsigned long reg;

    val = MF_DCR(AUD_ATTEN_VAL_FRONT);
    reg = val & 0x0000003F;
    pVol->frontright = reg;
    reg = (val >> 8) & 0x0000003F;
    pVol->frontleft = reg;

    val = MF_DCR(AUD_ATTEN_VAL_REAR);
    reg = val & 0x0000003F;
    pVol->rearright = reg;
    reg = (val >> 8) & 0x0000003F;
    pVol->rearleft = reg;

    val = MF_DCR(AUD_ATTEN_VAL_CENTER);
    reg = val & 0x0000003F;
    pVol->lfe = reg;
    reg = (val >> 8) & 0x0000003F;
    pVol->center = reg;
}

void aud_atom_set_vol(AUDVOL *pAudVol)
{
    unsigned long reg1;
    unsigned long reg;
    unsigned long val;

    reg1 = pAudVol->frontleft;
    reg = pAudVol->frontright;

    if (reg1 > 63)
        reg1 = 63;

    if (reg > 63)
        reg = 63;

    val = ((reg1 & 0x0000003F) << 8) | (reg & 0x0000003F);

    MT_DCR(AUD_ATTEN_VAL_FRONT, val);

    reg1 = pAudVol->rearleft;
    reg = pAudVol->rearright;

    if (reg1 > 63)
        reg1 = 63;

    if (reg > 63)
        reg = 63;

    val = ((reg1 & 0x0000003F) << 8) | (reg & 0x0000003F);

    MT_DCR(AUD_ATTEN_VAL_REAR, val);

    reg1 = pAudVol->center;
    reg = pAudVol->lfe;

    if (reg1 > 63)
        reg1 = 63;

    if (reg > 63)
        reg = 63;

    val = ((reg1 & 0x0000003F) << 8) | (reg & 0x0000003F);

    MT_DCR(AUD_ATTEN_VAL_CENTER, val);
}

void aud_atom_set_channel(audChannel_t channel)
{
    unsigned long reg;

    reg = MF_DCR(AUD_CTRL1) & (~DECOD_AUD_CTRL1_CHANNEL_MASK);

    switch (channel)
    {

        case AUD_CHANNEL_STEREO:
            MT_DCR(AUD_CTRL1, reg | DECOD_AUD_CTRL1_STEREO);
            break;

        case AUD_CHANNEL_MONOLEFT:
            MT_DCR(AUD_CTRL1, reg | DECOD_AUD_CTRL1_MONO_LEFT);
            break;

        case AUD_CHANNEL_MONORIGHT:
            MT_DCR(AUD_CTRL1, reg | DECOD_AUD_CTRL1_MONO_RIGHT);
            break;
    }
}

// YYD, added for PCM support
int aud_atom_set_pcm_fmt(AUD_PCM_FORMAT_CONFIG *pcmcfg)
{
   unsigned long val=0;
   PDEBUG("PCM_FMT: sign = %d, channel = %d,  bits = %d, freq = %d, endian=%d\n",
        pcmcfg->sign, pcmcfg->channel, pcmcfg->bits, pcmcfg->freq, pcmcfg->endian);

   // set parameteres
   
   val = MF_DCR(AUD_DSP_CTRL);

   val &= ~AUD_DSP_PCM_MASK;  // clear current

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
      val |= AUD_DSP_PCM_MONO;
      break;
   case AUD_PCM_STEREO:
      val |= AUD_DSP_PCM_STEREO;
      break;
   }

   switch(pcmcfg->bits)
   {
   case AUD_PCM_20BIT:
      val |= AUD_DSP_PCM_20BIT;
      break;
   case AUD_PCM_18BIT:
      val |= AUD_DSP_PCM_18BIT;
      break;
   case AUD_PCM_8BIT:
     val |= AUD_DSP_PCM_8BIT;
     break;
   case AUD_PCM_16BIT:
   default:
      val |= AUD_DSP_PCM_16BIT;
      break;
   }
   
   switch(pcmcfg->freq)
   {
   case AUD_PCM_16KHZ:
      val |= AUD_DSP_PCM_16KHZ;
      break;
   case AUD_PCM_32KHZ:
      val |= AUD_DSP_PCM_32KHZ;
      break;
   case AUD_PCM_22_05KHZ:
      val |= AUD_DSP_PCM_22_05KHZ;
      break;
   case AUD_PCM_44_1KHZ:
      val |= AUD_DSP_PCM_44_1KHZ;
      break;
   case AUD_PCM_24KHZ:
      val |= AUD_DSP_PCM_24KHZ;
      break;
   case AUD_PCM_48KHZ:
      val |= AUD_DSP_PCM_48KHZ;
      break;
   // untested modes
   case AUD_PCM_8KHZ:
      val |= AUD_DSP_PCM_8KHZ;
      break;
   case AUD_PCM_11_025KHZ:
      val |= AUD_DSP_PCM_11_025KHZ;
      break;
   case AUD_PCM_12KHZ:
      val |= AUD_DSP_PCM_12KHZ;
      break;
   case AUD_PCM_64KHZ:
      val |= AUD_DSP_PCM_64KHZ;
      break;
   case AUD_PCM_88_2KHZ:
      val |= AUD_DSP_PCM_88_2KHZ;
      break;
   case AUD_PCM_96KHZ:
      val |= AUD_DSP_PCM_96KHZ;
      break;
   default:
      val |= AUD_DSP_PCM_44_1KHZ;
      break;
   }

   MT_DCR(AUD_DSP_CTRL, val);

   // set the endian of audio
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

   aud_atom_reset_rb(); // YYD, as required by uc 3.01 after set stream format

   return 0;
}
// end of YYD added


void aud_atom_init_tv()
{
    unsigned long reg;

    reg = MF_DCR(AUD_CTRL0) & (~DECOD_AUD_CTRL0_CLIP_EN);

    MT_DCR(AUD_CTRL0, reg);
}

void aud_atom_init_clip()
{
    unsigned long reg;

    reg = MF_DCR(AUD_CTRL0);

    MT_DCR(AUD_CTRL0, reg | DECOD_AUD_CTRL0_CLIP_EN);
}

void aud_atom_close_clip()
{
    unsigned long reg;
    unsigned long ctrl0;
    unsigned long t1;

   ctrl0 = MF_DCR(AUD_CTRL0) & ~DECOD_AUD_CTRL0_CLIP_EN;

   /*-------------------------------------------------------------------------+
   | Reset the clip regs. Turning clip mode from on to off will reset the      
   | working clip length reg but not the queued clip length reg so we turn the
   | clip mode from off to on twice. The first time to clear the working length 
   | reg. Turning clip mode back on will transfer the queued clip regs to the
   | working clip regs and reset block valid. Then turning clip mode off again 
   | will reset the working length again.
   |
   | Note: DECOD_AUD_CTRL0_START_PARSING mus be turned on to allow the stream
   | busy status to be cleared.
   +-------------------------------------------------------------------------*/

   reg=ctrl0 | DECOD_AUD_CTRL0_START_PARSING;   
   MT_DCR(AUD_CTRL0, reg);

   /*-------------------------------------------------------------------------+
   | Reset the rate buffer. If the rate bufer is full, stream busy will not
   | reset.
   +-------------------------------------------------------------------------*/
   aud_atom_reset_rb();

   /*-------------------------------------------------------------------------+
   | Must wait for stream busy to clear before re-enabling clip mode or the 
   | decoder state machine will get confused
   +-------------------------------------------------------------------------*/
   for(t1=MF_SPR(SPR_TBL);MF_DCR(AUD_QLR) & DECOD_AUD_QLR_SB;) {   
      if(MF_SPR(SPR_TBL) - t1 >= (__TMRCLK/1000)*5) {
         break;
      }
   }
   MT_DCR(AUD_CTRL0, reg | DECOD_AUD_CTRL0_CLIP_EN);
   MT_DCR(AUD_CTRL0,reg);
   aud_atom_reset_rb();

   for(t1=MF_SPR(SPR_TBL);MF_DCR(AUD_QLR) & DECOD_AUD_QLR_SB;) {   
      if(MF_SPR(SPR_TBL) - t1 >= (__TMRCLK/1000)*5) {
         break;
      }
   }
   MT_DCR(AUD_CTRL0,ctrl0);   
}

int aud_atom_buf_ready()
{
    return (MF_DCR(AUD_QLR) & DECOD_AUD_QLR_BV) ? 0 : 1;
}

int aud_atom_buf_ready2()
{
    return (MF_DCR(AUD_QLR2) & DECOD_AUD_QLR_BV) ? 0 : 1;
}

void aud_atom_write_clip(CLIPINFO *pInfo)
{
    unsigned long reg;

    PDEBUG("write clip start = 0x%8.8lx, len = 0x%8.8lx \n", pInfo->uClipAdrOff, pInfo->uClipLen);

    reg = pInfo->uClipLen & 0x1fffff;
    //write to audio decoder
    MT_DCR(AUD_QAR, pInfo->uClipAdrOff);  //at most 2M
    MT_DCR(AUD_QLR, reg | DECOD_AUD_QLR_BV);
}

void aud_atom_write_clip2(CLIPINFO *pInfo)
{
    unsigned long reg;

    PDEBUG("write clip start = 0x%8.8lx, len = 0x%8.8lx \n", pInfo->uClipAdrOff, pInfo->uClipLen);

    reg = pInfo->uClipLen & 0x1fffff;
    //write to audio decoder
    MT_DCR(AUD_QAR2, pInfo->uClipAdrOff);  //at most 2M
    MT_DCR(AUD_QLR2, reg | DECOD_AUD_QLR_BV);
}

void aud_atom_set_irq_mask(UINT32 mask)
{
    MT_DCR(AUD_IMR, mask);
}

UINT32 aud_get_irq_status()
{
    return MF_DCR(AUD_ISR);
}

void aud_atom_irq_handler(UINT uIrq, void *pData)
{
    unsigned int reg;
    TASK_MSG msg;
    PDEBUG("\n[IRQ = %d\n", uIrq);
    
    reg = MF_DCR(AUD_ISR);

    if ((reg & DECOD_AUD_INT_CCC) != 0)
    {
        /*data = MF_DCR(AUD_CTRL0) & (~DECOD_AUD_CTRL0_ENABLE_SYNC);
        MT_DCR(AUD_CTRL0, data);
        DEMUX_CHAN_CHAN_DONE(AUD_CHAN_CHAN_DONE);
        DEMUX_PCR_CALLBACK(XP_CALLBACK_RUN);

        if ((__audio_mute == 0) && (__audio_playing != 0))
        {
            mpeg_a_hw_aud_unmute();
        }*/
        aud_cc_done();
        if(_mute == 0)
            aud_atom_unmute();
        PDEBUG("channel change done\n");
    }

    if ((reg & DECOD_AUD_INT_RTBC) != 0)
    {
        //DEMUX_PCR_CALLBACK(XP_CALLBACK_RUN);
        ;
    }

    if ((reg & DECOD_AUD_INT_AMSI) != 0)
    {
        STC_T stc;
        //audio master
        if(MF_DCR(AUD_DSR) & DECOD_AUD_DSR_SYNC_MASTER)
        {
	        aud_atom_get_pts(&stc);
	        vid_atom_set_stc(&stc);
        }
    }

    if ((reg & DECOD_AUD_INT_CM) != 0)
    {
        //schedule a task for this interrupt
        PDEBUG("main block ready\n");

        msg.uMsgType = AUD_MSG_BLOCK_READ;  //message type
        msg.ulPara1 = reg;                  //interrupt status
        msg.ulPara2 = 0;
        os_call_irq_task(uIrq, &msg);            
    }

    if ((reg & DECOD_AUD_INT_CM2) != 0)
    {
        //schedule a task for this interrupt
        PDEBUG("mixer block ready\n");

        msg.uMsgType = AUD_MSG_BLOCK_READ2;  //message type
        msg.ulPara1 = reg;                  //interrupt status
        msg.ulPara2 = 0;
        os_call_irq_task(uIrq, &msg);            
    }



    PDEBUG("\n]\n");
    return;
}

INT aud_atom_load_microcode(const unsigned short *code, int count)
{
    int i;
    /*-------------------------------------------------------------------------+
    | Enable microcode download.
    +-------------------------------------------------------------------------*/

    i = (MF_DCR(AUD_CTRL0) & ~(DECOD_AUD_CTRL0_DOWNLOAD_EN | DECOD_AUD_CTRL0_DOWNLOAD_END));
    MT_DCR(AUD_CTRL0, i);
    MT_DCR(AUD_CTRL0, i | DECOD_AUD_CTRL0_DOWNLOAD_EN);
    /*-------------------------------------------------------------------------+
    | Load microcode.
    +-------------------------------------------------------------------------*/
    for (i = 0; i < (count / 2); i++)
    {
        MT_DCR(AUD_IMFD, ((unsigned long *) code)[i]);
    }

    /*-------------------------------------------------------------------------+
    | Signal end of microcode load.
    +-------------------------------------------------------------------------*/
    i = (MF_DCR(AUD_CTRL0) & ~(DECOD_AUD_CTRL0_DOWNLOAD_EN | DECOD_AUD_CTRL0_DOWNLOAD_END));
    MT_DCR(AUD_CTRL0, i); // turn off download enable
    MT_DCR(AUD_CTRL0, i | DECOD_AUD_CTRL0_DOWNLOAD_END);// turn on valid microcode
    return (0);
}

INT aud_atom_load_dfile(void *base, const unsigned char *dfile, 
                        int df_len, unsigned long rb_off)
{
   unsigned char *des;

   /*-------------------------------------------------------------------------+
   | Compute where data file should be stored in Audio Offsets Temp Area
   +-------------------------------------------------------------------------*/
   des = (unsigned char *)
         ((base + ( rb_off & DECOD_AUD_OFFSETS_TEMP_MASK) * 4096) +
         DECOD_AUD_OFFSETS_TEMP_HDR_SIZE);

   PDEBUG("dfile des = 0x%8.8x, dfile len = 0x%8.8x\n", des, df_len);
   /*-------------------------------------------------------------------------+
   | Copy Data file into Audio Segment 1 Temp Data Area
   +-------------------------------------------------------------------------*/
   memcpy(des, dfile, df_len);
   return(0);
}



void aud_atom_init_irq_mask()
{
    UINT32 flags;
    
    flags = os_enter_critical_section();
    MT_DCR(AUD_IMR, DEF_AUD_IRQ_MASK);
    os_leave_critical_section(flags);
}

void aud_atom_reg_dump()
{
    PDEBUG("CTRL 0 = 0x%8x\n", MF_DCR(AUD_CTRL0));
    PDEBUG("CTRL 1 = 0x%8.8x\n", MF_DCR(AUD_CTRL1));
    PDEBUG("CTRL 2 = 0x%8.8x\n", MF_DCR(AUD_CTRL2));
    PDEBUG("MEM SEG 1 = 0x%8.8x\n", MF_DCR(AUD_SEG1));
    PDEBUG("MEM SEG 2 = 0x%8.8x\n", MF_DCR(AUD_SEG2));
    PDEBUG("MEM SEG 3 = 0x%8.8x\n", MF_DCR(AUD_SEG3));    
    PDEBUG("INT MASK = 0x%8.8x\n", MF_DCR(AUD_IMR));
    /*PDEBUG("FRAME BUF = 0x%8.8x\n", MF_DCR(VID_FRAME_BUF));
    PDEBUG("RB BUF = 0x%8.8x\n", MF_DCR(VID_RB_BASE));
    PDEBUG("RB SIZE = 0x%8.8x\n", MF_DCR(VID_RB_SIZE));*/
}

inline void aud_atom_reset_rb()
{
    MT_DCR(AUD_CMD, DECOD_AUD_CMD_DSP_RESET);
}

inline INT aud_atom_dsp_ready()
{   
    return !(MF_DCR(AUD_DSR) & DECOD_AUD_DSR_COMMAND_COM); 
}
