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
| File:   aud_atom_hw.h
| Purpose: audio driver atom layer hardware definition PALLAS
| Changes:
| Date:     Comment:
| -----     --------
| 15-Oct-01 create
| 17-Jun-03 Added definitions for 8-bit UPCM, and masks for sample rate and
|           number of bits per sample.
| 20-Jun-03 Added definition for halt decoding (HD) in ctrl2 register
| 25-Jun-03 Added support for DTS/LPCM coreload
+----------------------------------------------------------------------------*/
#ifndef PALLAS_AUD_ATOM_HW_H
#define PALLAS_AUD_ATOM_HW_H

#include <hw/hardware.h>

/*audio register address*/
#define AUD_CTRL0               0x1A0
#define AUD_CTRL1               0x1A1
#define AUD_CTRL2               0x1A2
#define AUD_CMD                 0x1A3
#define AUD_ISR                 0x1A4
#define AUD_IMR                 0x1A5
#define AUD_DSR                 0x1A6
#define AUD_STC                 0x1A7
#define AUD_CSR                 0x1A8

#define AUD_QAR2                0x1A9
#define AUD_PTS                 0x1AA
#define AUD_TONE_GEN_CTRL       0x1AB
#define AUD_QLR2                0x1AC
#define AUD_ACL_DATA            0x1AD
#define AUD_STREAM_ID           0x1AE
#define AUD_QAR                 0x1AF
#define AUD_DSP_STATUS          0x1B0
#define AUD_QLR                 0x1B1
#define AUD_DSP_CTRL            0x1B2
#define AUD_WLR2                0x1B3
#define AUD_IMFD                0x1B4
#define AUD_WAR                 0x1B5
#define AUD_SEG1                0x1B6
#define AUD_SEG2                0x1B7

#ifdef __DRV_FOR_PALLAS__
#define RB_RBF                  0x1B8
#endif

#define AUD_ATTEN_VAL_FRONT     0x1B9
#define AUD_ATTEN_VAL_REAR      0x1BA
#define AUD_ATTEN_VAL_CENTER    0x1BB
#define AUD_SEG3                0x1BC
#define AUD_OFFSETS             0x1BD
#define AUD_WLR                 0x1BE
#define AUD_WAR2                0x1BF


#define DECOD_AUD_SEG_ALIGN             0x0000007F
#define DECOD_AUD_SEG_SH                7


#define DECOD_AUD_AC3                   0x00000001
#define DECOD_AUD_LPCM                  0x00000002
#define DECOD_AUD_MPEG                  0x00000003
#define DECOD_AUD_PCM                   0x00000004
#define DECOD_AUD_STREAM_MPEG           0
#define DECOD_AUD_STREAM_PES            1
#define DECOD_AUD_STREAM_ES             2
#define DECOD_AUD_BEEP_MAX_DUR          31
#define DECOD_AUD_BEEP_MIN_ATT          7
#define DECOD_AUD_DEF_ID                0xE0C0
#define DECOD_AUD_DEF_AC3_ID            0xFFBD

/*-----------------------------------------------------------------------------+
| Mpeg Audio Register Definitions
+-----------------------------------------------------------------------------*/
#define DECOD_AUD_CTRL0_MODE_MASK       0x0000C000
#define DECOD_AUD_CTRL0_MODE_L          0x00000000
#define DECOD_AUD_CTRL0_MODE_SL         0x00004000
#define DECOD_AUD_CTRL0_MODE_C          0x00008000
#define DECOD_AUD_CTRL0_MIX_EN          0x00000800
#define DECOD_AUD_CTRL0_START_DECODER   0x00000400
#define DECOD_AUD_CTRL0_ENABLE_SYNC     0x00000200
#define DECOD_AUD_CTRL0_START_PARSING   0x00000100
#define DECOD_AUD_CTRL0_CLIP_EN         0x00000080
#define DECOD_AUD_CTRL0_DOWNLOAD_END    0x00000040
#define DECOD_AUD_CTRL0_DOWNLOAD_EN     0x00000008
#define DECOD_AUD_CTRL0_ENABLE_INT      0x00000004
#define DECOD_AUD_CTRL0_TYPE_MASK       0x00000003
#define DECOD_AUD_CTRL0_TYPE_ES         0x00000001
#define DECOD_AUD_CTRL0_TYPE_PES        0x00000002
#define DECOD_AUD_CTRL0_TYPE_MPEG1      0x00000003
#define DECOD_AUD_CTRL1_SOFT_MUTE       0x00000400
#define DECOD_AUD_CTRL1_STEREO          0x00000000
#define DECOD_AUD_CTRL1_MONO_LEFT       0x00002000
#define DECOD_AUD_CTRL1_MONO_RIGHT      0x00004000
#define DECOD_AUD_CTRL1_CHANNEL_MASK    0x00006000

#define DECOD_AUD_CTRL1_DAC_EN          0x00000010

#define DECOD_AUD_CTRL2_HD              0x00008000
#define DECOD_AUD_CTRL2_MUTE            0x00004000
#define DECOD_AUD_CTRL2_IP              0x00001000
#define DECOD_AUD_CTRL2_ID              0x00000800
#define DECOD_AUD_CTRL2_DM              0x00000200
#define DECOD_AUD_CTRL2_LL              0x00000080
#define DECOD_AUD_CTRL2_HL              0x00000040
#define DECOD_AUD_CTRL2_DN              0x00000020

#define DECOD_AUD_CTRL2_ENDIAN_MASK     0x00000010 // YYD
#define DECOD_AUD_CTRL2_BIG_ENDIAN      0x00000000 // YYD
#define DECOD_AUD_CTRL2_LITTLE_ENDIAN	0x00000010 // YYD, add according to uc 3.09

#define DECOD_AUD_CTRL2_AC3             0x00000000
#define DECOD_AUD_CTRL2_MPEG            0x00000001
#define DECOD_AUD_CTRL2_PCM             0x00000003
#define DECOD_AUD_CTRL2_DTS             0x00000002
#define DECOD_AUD_CTRL2_LPCM            0x00000001

#define DECOD_AUD_CMD_RESET             0x00000000
#define DECOD_AUD_CMD_PES_RESET		0x00000002
#define DECOD_AUD_CMD_DSP_RESET		0x00000001  // YYD, added according to uc v3.01, requested after change stream format

#define DECOD_AUD_INT_CCC               0x00008000
#define DECOD_AUD_INT_RTBC              0x00004000
#define DECOD_AUD_INT_AMSI              0x00000800
#define DECOD_AUD_INT_PE                0x00000400
#define DECOD_AUD_INT_BE                0x00000200
#define DECOD_AUD_INT_BF                0x00000100
#define DECOD_AUD_INT_PSE               0x00000080
#define DECOD_AUD_INT_PTO               0x00000040
#define DECOD_AUD_INT_ADO               0x00000020
#define DECOD_AUD_INT_ADD               0x00000010
#define DECOD_AUD_INT_CM2               0x00000002
#define DECOD_AUD_INT_CM                0x00000001
#define DECOD_AUD_INT_ALL               0x0000CFF1

#define DECOD_AUD_IMR_CCC               0x00008000
#define DECOD_AUD_IMR_RTBC              0x00004000
#define DECOD_AUD_IMR_AMSI              0x00000800
#define DECOD_AUD_IMR_PE                0x00000400
#define DECOD_AUD_IMR_BE                0x00000200
#define DECOD_AUD_IMR_BF                0x00000100
#define DECOD_AUD_IMR_PSE               0x00000080
#define DECOD_AUD_IMR_PTO               0x00000040
#define DECOD_AUD_IMR_ADO               0x00000020
#define DECOD_AUD_IMR_ADD               0x00000010
#define DECOD_AUD_IMR_CM2               0x00000002
#define DECOD_AUD_IMR_CM                0x00000001

#define DECOD_AUD_DSR_CHAN_CH           0x00008000
#define DECOD_AUD_DSR_TB_CH_IN_PROC     0x00004000
//added by shaol audio sync master
#define DECOD_AUD_DSR_SYNC_MASTER       0x00002000
#define DECOD_AUD_DSR_ERROR_MASK        0x00000C00
#define DECOD_AUD_DSR_AUX_DATA          0x00000010
#define DECOD_AUD_DSR_COMMAND_COM       0x00000002

#define DECOD_AUD_TONE_CTRL_TR          0x00080000
#define DECOD_AUD_TONE_CTRL_BA_MASK     0x00000007
#define DECOD_AUD_TONE_CTRL_BD_MASK     0x1F000000
#define DECOD_AUD_TONE_CTRL_FI_MASK     0x007F0000

#define DECOD_AUD_QLR_BV                0x80000000
#define DECOD_AUD_QLR_SB                0x20000000
#define DECOD_AUD_QLR_QL_MASK           0x001FFFFF


// YYD, add for PCM support
#define AUD_DSP_PCM_FS_MASK    0x0000000F  // mask for sample rate
#define AUD_DSP_PCM_BIT_MASK   0x00000030  // mask for bits per sample
#define AUD_DSP_PCM_MASK       0x000000FF       // bits 24-31
#define AUD_DSP_PCM_SIGNED     0x00000000       // bit 24 = 0
#define AUD_DSP_PCM_UNSIGNED   0x00000080       // bit 24 = 1
#define AUD_DSP_PCM_MONO       0x00000040       // bit 25 = 1
#define AUD_DSP_PCM_STEREO     0x00000000       // bit 25 = 0
#define AUD_DSP_PCM_20BIT      0x00000000       // bit 26,27 = 00
#define AUD_DSP_PCM_18BIT      0x00000010       // bit 26,27 = 01
#define AUD_DSP_PCM_16BIT      0x00000020       // bit 26,27 = 10
#define AUD_DSP_PCM_8BIT       0x00000030       // bit 26,27 = 10
#define AUD_DSP_PCM_16KHZ      0x00000001       // bit 28-31 = 0001
#define AUD_DSP_PCM_32KHZ      0x00000005       // bit 28-31 = 0101
#define AUD_DSP_PCM_22_05KHZ   0x00000002       // bit 28-31 = 0010
#define AUD_DSP_PCM_44_1KHZ    0x00000006       // bit 28-31 = 0110
#define AUD_DSP_PCM_24KHZ      0x00000003       // bit 28-31 = 0011
#define AUD_DSP_PCM_48KHZ      0x00000007       // bit 28-31 = 0111

// untested modes
#define AUD_DSP_PCM_8KHZ       0x00000009
#define AUD_DSP_PCM_11_025KHZ  0x0000000A
#define AUD_DSP_PCM_12KHZ      0x0000000B
#define AUD_DSP_PCM_64KHZ      0x0000000D
#define AUD_DSP_PCM_88_2KHZ    0x0000000E
#define AUD_DSP_PCM_96KHZ      0x0000000F


// YYD, add for PCM support
#define AUD_DSP_MIX_VOL        0x003F0000 
#define AUD_DSP_MIX_MASK       (0x000000FF << 8)
#define AUD_DSP_MIX_SIGNED     (0x00000000 << 8)
#define AUD_DSP_MIX_UNSIGNED   (0x00000080 << 8)
#define AUD_DSP_MIX_MONO       (0x00000040 << 8)
#define AUD_DSP_MIX_STEREO     (0x00000000 << 8)
#define AUD_DSP_MIX_16BIT      (0x00000000 << 8)
#define AUD_DSP_MIX_16KHZ      (0x00000001 << 8)
#define AUD_DSP_MIX_32KHZ      (0x00000005 << 8)
#define AUD_DSP_MIX_22_05KHZ   (0x00000002 << 8)
#define AUD_DSP_MIX_44_1KHZ    (0x00000006 << 8)
#define AUD_DSP_MIX_24KHZ      (0x00000003 << 8)
#define AUD_DSP_MIX_48KHZ      (0x00000007 << 8)

// untested modes
#define AUD_DSP_MIX_8KHZ       (0x00000009 << 8)
#define AUD_DSP_MIX_11_025KHZ  (0x0000000A << 8)
#define AUD_DSP_MIX_12KHZ      (0x0000000B << 8)
#define AUD_DSP_MIX_64KHZ      (0x0000000D << 8)
#define AUD_DSP_MIX_88_2KHZ    (0x0000000E << 8)
#define AUD_DSP_MIX_96KHZ      (0x0000000F << 8)

//mixing code area
#define DECOD_AUD_OFFSETS_TEMP_MASK     0x000000FF
#define DECOD_AUD_OFFSETS_TEMP_HDR_SIZE 64
#define DECOD_AUD_OFFSETS_TEMP_SIZE     (32*1024)

#endif
