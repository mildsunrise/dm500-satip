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
| File:   vid_atom_hw.h
| Purpose: video decoder hardware definition PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#ifndef PALLAS_VID_ATOM_HW_H
#define PALLAS_VID_ATOM_HW_H

#include <hw/hardware.h>

#define VIDEO_CMD_WAIT_TIME             10
#define VIDEO_CMD_RETRY                 3

#define CICCR                           0x030
#define CICSEL1                         0x031
#define CICSEL2                         0x032


#define CICVCR                          0x033
#define VID_DCR_BASE                    0x140

/*video chip registers address map*/
#define VID_CHIP_CTRL                   VID_DCR_BASE + 0x00
#define VID_CHIP_MODE                   VID_DCR_BASE + 0x01
#define VID_SYNC_STC0                   VID_DCR_BASE + 0x02
#define VID_SYNC_STC1                   VID_DCR_BASE + 0x03
#define VID_SYNC_PTS0                   VID_DCR_BASE + 0x04
#define VID_SYNC_PTS1                   VID_DCR_BASE + 0x05
#define VID_FIFO                        VID_DCR_BASE + 0x06
#define VID_FIFO_STAT                   VID_DCR_BASE + 0x07
#define VID_CMD                         VID_DCR_BASE + 0x08
#define VID_CMD_DATA                    VID_DCR_BASE + 0x09
#define VID_CMD_STAT                    VID_DCR_BASE + 0x0a
#define VID_CMD_ADDR                    VID_DCR_BASE + 0x0b
#define VID_PROC_IADDR                  VID_DCR_BASE + 0x0c
#define VID_PROC_IDATA                  VID_DCR_BASE + 0x0d

#ifdef  __DRV_FOR_PALLAS__
#define VID_DISP_CNTL                   VID_DCR_BASE + 0x11
#else
#define VID_OSD_MODE                    VID_DCR_BASE + 0x11
#endif 

#define VID_HOST_INT                    VID_DCR_BASE + 0x12
#define VID_MASK                        VID_DCR_BASE + 0x13
#define VID_DISP_MODE                   VID_DCR_BASE + 0x14
#define VID_DISP_DLY                    VID_DCR_BASE + 0x15
#define VID_VBI_CNTL                    VID_DCR_BASE + 0x16
#define VID_TTX_CNTL                    VID_DCR_BASE + 0x17
#define VID_DISP_BOR                    VID_DCR_BASE + 0x18

#ifdef  __DRV_FOR_PALLAS__
#define VID_CURGRAPIC_CNTL              VID_DCR_BASE + 0x1a
#else
#define VID_OSDG_LINK_ADR               VID_DCR_BASE + 0x19
#define VID_OSDI_LINK_ADR               VID_DCR_BASE + 0x1a
#endif 

#define VID_RB_THRE                     VID_DCR_BASE + 0x1b

#ifndef __DRV_FOR_PALLAS__
#define VID_OSDC_LINK_ADR               VID_DCR_BASE + 0x1c
#endif

#define VID_STC_COM_ADR                 VID_DCR_BASE + 0x1d
#define VID_PTS_DELTA                   VID_DCR_BASE + 0x1e
#define VID_PTS_CTRL                    VID_DCR_BASE + 0x1f
#define VID_WRT_PROT                    VID_DCR_BASE + 0x25
#define VID_VCLIP_ADR                   VID_DCR_BASE + 0x27
#define VID_VCLIP_LEN                   VID_DCR_BASE + 0x28
#define VID_BLOCK_SIZE                  VID_DCR_BASE + 0x29
#define VID_SRC_ADR                     VID_DCR_BASE + 0x2a
#define VID_USERDATA_BASE               VID_DCR_BASE + 0x2b

#define VID_VBI_BASE                    VID_DCR_BASE + 0x2c

#ifndef __DRV_FOR_PALLAS__
#define VID_OSDI_BASE                   VID_DCR_BASE + 0x2d
#define VID_OSDG_BASE                   VID_DCR_BASE + 0x2e
#endif 

#define VID_RB_BASE                     VID_DCR_BASE + 0x2f
#define VID_DRAM_ADR                    VID_DCR_BASE + 0x30
#define VID_DRAM_DATA                   VID_DCR_BASE + 0x31
#define VID_DRAM_CMD                    VID_DCR_BASE + 0x32
#define VID_CLIP_WAR                    VID_DCR_BASE + 0x33
#define VID_CLIP_WLR                    VID_DCR_BASE + 0x34
#define VID_SEG0                        VID_DCR_BASE + 0x35
#define VID_SEG1                        VID_DCR_BASE + 0x36
#define VID_SEG2                        VID_DCR_BASE + 0x37
#define VID_SEG3                        VID_DCR_BASE + 0x38
#define VID_FRAME_BUF                   VID_DCR_BASE + 0x39

#ifdef  __DRV_FOR_PALLAS__
#define VID_CURSOR_POS                  VID_DCR_BASE + 0x3a
#define VID_SCALE_SIZE                  VID_DCR_BASE + 0x3b
#define VID_CROP_SIZE                   VID_DCR_BASE + 0x3c
#define VID_SCALE_BORDER                VID_DCR_BASE + 0x3d
#define VID_CROP_OFFSET                 VID_DCR_BASE + 0x3e
#else
#define VID_OSDC_BASE                   VID_DCR_BASE + 0x3a
#define VID_LBOX                        VID_DCR_BASE + 0x3b
#define VID_TRANS_DLY                   VID_DCR_BASE + 0x3c
#define VID_SMALL_BORDER                VID_DCR_BASE + 0x3d
#define VID_ZOOM_OFFSET                 VID_DCR_BASE + 0x3e
#endif

#define VID_RB_SIZE                     VID_DCR_BASE + 0x3f



#define DECOD_CHIP_CONTROL_CHIP_V_MASK  0x0000F000
#define DECOD_CHIP_CONTROL_VID_CLIP     0x00000800
#define DECOD_CHIP_CONTROL_AUTO_SYNC    0x00000080
#define DECOD_CHIP_CONTROL_AUD_MAS      0x00000040
#define DECOD_CHIP_CONTROL_VID_MAS      0x00000020
#define DECOD_CHIP_CONTROL_DIS_SYNC     0x00000010
#define DECOD_CHIP_CONTROL_BLANK_VID    0x00000008
#define DECOD_CHIP_CONTROL_SVP          0x00000002
#define DECOD_CHIP_CONTROL_SVD          0x00000001

#define DECOD_CHIP_MODE_B_PICT_UNDER    0x00002000
#define DECOD_CHIP_MODE_ENABLE_PIP      0x00001000
#define DECOD_CHIP_MODE_LOAD_STC        0x00000400
#define DECOD_CHIP_MODE_ENABLE_OVER_R   0x00000020
#define DECOD_CHIP_MODE_STC_TIME_BASE   0x00000010
#define DECOD_CHIP_MODE_BYTE_SWAP       0x00000002
#define DECOD_CHIP_MODE_NO_B_FRAME_MODE 0x00000001

#define DECOD_DISP_MODE_BK_MASK         0xFFFF0000
#define DECOD_DISP_MODE_BK_Y_MASK       0xFF000000
#define DECOD_DISP_MODE_BK_CB_MASK      0x00F00000
#define DECOD_DISP_MODE_BK_CR_MASK      0x000F0000
#define DECOD_DISP_MODE_BK_BLACK        0x00880000
#define DECOD_DISP_MODE_COMP_BLANKING   0x00002000
#define DECOD_DISP_MODE_HSC_VSC         0x00004000
#define DECOD_DISP_MODE_SYNC_TRAILING   0x00006000
#define DECOD_DISP_MODE_CCIR_656        0x0000A000
#define DECOD_DISP_MODE_SYNC_LEADING    0x0000E000
#define DECOD_DISP_MODE_HSC_POL_ACT_HI  0x00001000
#define DECOD_DISP_MODE_VSC_POL_HIGH    0x00000800
#define DECOD_DISP_MODE_PAL_MODE        0x00000400
#define DECOD_DISP_MODE_TRANS_POL_LOW   0x00000200
#define DECOD_DISP_MODE_YCBCR_CLIP_254  0x00000000
#define DECOD_DISP_MODE_YCBCR_NO_CLIP   0x00000040
#define DECOD_DISP_MODE_YCBCR_CLIP_253  0x00000080
#define DECOD_DISP_MODE_YCBCR_CLIP      0x000000C0
#define DECOD_DISP_MODE_SFM_MASK        0x00000030
#define DECOD_DISP_MODE_NORM_DISP       0x00000000
#define DECOD_DISP_MODE_BOTTOM_ONLY     0x00000010
#define DECOD_DISP_MODE_TOP_ONLY        0x00000020
#define DECOD_DISP_MODE_FIRST_ONLY      0x00000030

#define DECOD_DISP_MODE_LETTERBOX_DISP  0x00000002
#define DECOD_DISP_MODE_1_2_h_v         0x00000006
#define DECOD_DISP_MODE_1_4_h_v         0x00000008
#define DECOD_DISP_MODE_2x              0x0000000A
#define DECOD_DISP_MODE_DISABLE_EXP     0x0000000E
#define DECOD_DISP_MODE_MASK            0x0000000E
#define DECOD_DISP_MODE_16_9_MONITOR    0x00000001


#define DECOD_VBI_CTL_TTX_INDEX_MASK    0x00000100
#define DECOD_VBI_CTL_TELETEXT_REVERSE  0x00000080
#define DECOD_VBI_CTL_TLTX_7182         0x00000000
#define DECOD_VBI_CTL_TLTX_VBI          0x00000020
#define DECOD_VBI_CTL_TLTX_7120         0x00000040
#define DECOD_VBI_CTL_TLTX_7120_EXT     0x00000060
#define DECOD_VBI_CTL_VBI_LINE_MASK     0x0000001F

#define DECOD_HOST_INT_SERV_PICT        0x80000000
#define DECOD_HOST_INT_FF_STATUS        0x40000000
#define DECOD_HOST_INT_PS_STATUS        0x20000000

//lingh added
#define DECOD_HOST_INT_IPDC             0x10000000

#define DECOD_HOST_INT_SAVED_PTS        0x08000000
#define DECOD_HOST_INT_VSOR             0x04000000
#define DECOD_HOST_INT_ZOOM_OFF_OVER    0x02000000
#define DECOD_HOST_INT_CHAN_CHAN        0x01000000
#define DECOD_HOST_INT_PLB_ERROR        0x00800000
#define DECOD_HOST_INT_BLOCK_READ       0x00400000
#define DECOD_HOST_INT_STC_READY        0x00020000
#define DECOD_HOST_INT_DECOD_HANG       0x00010000
#define DECOD_HOST_INT_SS               0x00008000
#define DECOD_HOST_INT_SERROR           0x00004000
#define DECOD_HOST_INT_SEND             0x00002000
#define DECOD_HOST_INT_SMPTE            0x00001000
#define DECOD_HOST_INT_PSKIP            0x00000800
#define DECOD_HOST_INT_PSTART           0x00000400
#define DECOD_HOST_INT_PRESOL           0x00000200
#define DECOD_HOST_INT_USR_DATA         0x00000100
#define DECOD_HOST_INT_VBI_START        0x00000080
#define DECOD_HOST_INT_VIDEO_START      0x00000040
#define DECOD_HOST_INT_FF_VIDEO_START   0x00000020
#define DECOD_HOST_INT_BLK_MOVE_COMPL   0x00000010
#define DECOD_HOST_INT_TIMER_BASE_CH    0x00000008
#define DECOD_HOST_INT_VID_RB_TH        0x00000004
#define DECOD_HOST_INT_VID_RB_OV        0x00000002
#define DECOD_HOST_INT_OSD_DATA         0x00000001

#define DECOD_HOST_MASK_SERV_PICT       0x80000000
#define DECOD_HOST_MASK_FF_STATUS       0x40000000
//shaol added
#define DECOD_HOST_MASK_PS_STATUS       0x20000000
//lingh added
#define DECOD_HOST_MASK_IPDC            0x10000000


#define DECOD_HOST_MASK_SAVED_PTS       0x08000000
#define DECOD_HOST_MASK_VSOR            0x04000000
#define DECOD_HOST_MASK_ZOOM_OFF_OVER   0x02000000
#define DECOD_HOST_MASK_CHAN_CHAN       0x01000000
#define DECOD_HOST_MASK_PLB_ERROR       0x00800000
#define DECOD_HOST_MASK_BLOCK_READ      0x00400000
#define DECOD_HOST_MASK_STC_READY       0x00020000
#define DECOD_HOST_MASK_DECOD_HANG      0x00010000
#define DECOD_HOST_MASK_SS              0x00008000
#define DECOD_HOST_MASK_SERROR          0x00004000
#define DECOD_HOST_MASK_SEND            0x00002000
#define DECOD_HOST_MASK_SMPTE           0x00001000
#define DECOD_HOST_MASK_PSKIP           0x00000800
#define DECOD_HOST_MASK_PSTART          0x00000400
#define DECOD_HOST_MASK_PRESOL          0x00000200
#define DECOD_HOST_MASK_USR_DATA        0x00000100
#define DECOD_HOST_MASK_VBI_START       0x00000080
#define DECOD_HOST_MASK_VIDEO_START     0x00000040
#define DECOD_HOST_MASK_FF_VIDEO_START  0x00000020
#define DECOD_HOST_MASK_BLK_MOVE_COMPL  0x00000010
#define DECOD_HOST_MASK_TIMER_BASE_CH   0x00000008
#define DECOD_HOST_MASK_VID_RB_TH       0x00000004
#define DECOD_HOST_MASK_VID_RB_OV       0x00000002
#define DECOD_HOST_MASK_OSD_DATA        0x00000001

#define DECOD_HOST_INT_CHAN_CHAN_BIT	7
#define DECOD_HOST_MASK_CHAN_CHAN_BIT	7

#define DECOD_SCALE_SIZE_SHSIZE         0x03FF0000
#define DECOE_SCALE_SIZE_SVSIZE         0x000003FF

#define DECOD_CROP_SIZE_SS              0x80000000
#define DECOD_CROP_SIZE_CHSIZE          0x03FF0000
#define DECOD_CROP_SIZE_CVSIZE          0x000003FF

#define DECOD_SCALE_BORD_BDC            0x80000000
#define DECOD_SCALE_BORD_LEFTSC         0x03FF0000
#define DECOD_SCALE_BORD_TOPSC          0x000003FF

#define DECOD_CROP_OFF_CROPHOS          0x03FF0000
#define DECOD_CROP_OFF_CROPVOS          0x000003FF


#define DECOD_VCLIP_BLOCK_VALID         0x80000000
#define DECOD_VCLIP_END_OF_STREAM       0x40000000
#define DECOD_VCLIP_STREAM_BUISY        0x20000000

#define DECOD_MEM_ALIGN                 128
#define DECOD_RB_ALIGN                  32

#define DECOD_WR_PROT_DISABLE           0x00000001
#define DECOD_WR_PROT_ENABLE            0x00000000

#endif
