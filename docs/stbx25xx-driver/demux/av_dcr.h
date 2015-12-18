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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|   Author    :  Lin Guo Hui
|   Component :  xp
|   File      :  av_dcr.c
|   Purpose   :  DCR access to A/V Registers for Chan Chan using
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  --------------------------------------------------------
|       30-Sep-01  LGH  Create
|
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Video decoder.  Suspect 0x179, 0x169, 0x16a, 0x152 (rc).
+----------------------------------------------------------------------------*/
#define v_c_cntl        0x140   /* control decoder operation */
#define v_c_mode        0x141   /* video operational mode */
#define v_s_stc0        0x142   /* STC high order bits 31:0 */
#define v_s_stc1        0x143   /* STC low order bit 32 */
#define v_s_pts0        0x144   /* wPTS high order bits 31:0 */
#define v_s_pts1        0x145   /* wPTS low order bit 32 */
#define v_fifo          0x146   /* FIFO data port */
#define v_fifo_s        0x147   /* FIFO status */
#define v_cmd           0x148   /* wsend command to decoder */
#define v_cmd_d         0x149   /* port for command params */
#define v_cmd_st        0x14A   /* command status */
#define v_cmd_ad        0x14B   /* command address */
#define v_procia        0x14C   /* instruction store */
#define v_procid        0x14D   /* data port for I_Store */
#define v_osd_m         0x151   /* OSD mode control */
#define v_host_i        0x152   /* rbase interrupt register */
#define v_mask          0x153   /* interrupt mask register */
#define v_dispm         0x154   /* operational mode for Disp */
#define v_dispd         0x155   /* setting for 'Sync' delay */
#define v_vb_ctl        0x156   /* VBI */
#define v_displb        0x157   /* set left border of display */
#define v_disptb        0x158   /* top border of display */
#define v_osd_la        0x159   /* first link address for OSD */
#define v_rb_thr        0x15B   /* rate buffer threshold */
#define v_stc_ca        0x15D   /* STC common address */
#define v_ptsctl        0x15F   /* PTS Control */
#define v_w_prot        0x165   /* write protect for I_Store */
#define v_vc_qa         0x167   /* video clip queued block Ad */
#define v_vc_ql         0x168   /* video clip queued block Le */
#define v_udbase        0x16B   /* base mem add for user data */
#define v_v0base        0x16C   /* base mem add for VBI-0 */
#define v_v1base        0x16D   /* base mem add for VBI-1 */
#define v_osbase        0x16E   /* base mem add for OSD data */
#define v_rbbase        0x16F   /* base mem add for video buf */
#define v_dramad        0x170   /* DRAM address */
#define v_dramdt        0x171   /* data port for DRAM access */
#define v_dramcs        0x172   /* DRAM command and statusa */
#define v_vc_wa         0x173   /* rv clip work address */
#define v_vc_wl         0x174   /* rv clip work length */
#define v_m_seg0        0x175   /* segment address 0 */
#define v_m_seg1        0x176   /* segment address 1 */
#define v_m_seg2        0x177   /* segment address 2 */
#define v_m_seg3        0x178   /* segment address 3 */
#define v_fbuff_base    0x179   /* frame buffer base memory */
#define v_tl_border     0x17B   /* top left border */
#define v_tr_dly        0x17C   /* transparency gate delay */
#define v_small_board   0x17D   /* left/top small pict. bord. */
#define v_hv_zoom       0x17E   /* hor/ver zoom window */
#define v_rb_sz         0x17F   /* rate buffer size read */

/*----------------------------------------------------------------------------+
| Audio decoder. Suspect 0x1ad, 0x1b4, 0x1a3, 0x1a5 (read/write status)
+----------------------------------------------------------------------------*/
#define a_ctrl0         0x1a0   /* control 0 */
#define a_ctrl1         0x1a1   /* control 1 */
#define a_ctrl2         0x1a2   /* control 2 */
#define a_command       0x1a3   /* wcommand register */
#define a_isr           0x1a4   /* rinterrupt status register */
#define a_imr           0x1a5   /* winterrupt mask register */
#define a_dsr           0x1a6   /* rdecoder status register */
#define a_stc           0x1a7   /* system time clock */
#define a_csr           0x1a8   /* channel status register */
#define a_latcnt        0x1a9   /* PLB Latency count */
#define a_pts           0x1aa   /* rpresentation time stamp */
#define a_tgctrl        0x1ab   /* tone generation control */
#define a_tgval         0x1ac   /* tone generation value */
#define a_auxd          0x1ad   /* raux data */
#define a_strmid        0x1ae   /* stream ID */
#define a_sqar          0x1af   /* queued address register */
#define a_dsp_st        0x1b0   /* rDSP status */
#define a_qlr           0x1b1   /* queued len address */
#define a_dsp_c         0x1b2   /* DSP control */
#define a_inst_d        0x1b4   /* winstruction download */
#define a_war           0x1b5   /* rworking address register */
#define a_seg1r         0x1b6   /* segment 1 base register */
#define a_seg2r         0x1b7   /* segment 2 base register */
#define a_atf           0x1b9   /* audio att value front */
#define a_atr           0x1ba   /* audio att value rear */
#define a_atc           0x1bb   /* audio att value center */
#define a_seg3r         0x1bc   /* segment 3 base register */
#define a_offset        0x1bd   /* offset address */
#define a_wrl           0x1be   /* working length register */
#define a_plb_pr        0x1bf   /* PLB priority */

/*-----------------------------------------------------------------------------+
| Bit definitions for the MPEG audio registers.
+-----------------------------------------------------------------------------*/
#define DECOD_AUD_CTRL0_MODE_MASK       0x0000C000
#define DECOD_AUD_CTRL0_MODE_L          0x00000000
#define DECOD_AUD_CTRL0_MODE_SL         0x00004000
#define DECOD_AUD_CTRL0_MODE_C          0x00008000
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

#define DECOD_AUD_CTRL2_KM1             0x00008000
#define DECOD_AUD_CTRL2_MUTE            0x00004000
#define DECOD_AUD_CTRL2_KM2             0x00002000
#define DECOD_AUD_CTRL2_IP              0x00001000
#define DECOD_AUD_CTRL2_ID              0x00000800
#define DECOD_AUD_CTRL2_HD              0x00000400
#define DECOD_AUD_CTRL2_DM              0x00000200
#define DECOD_AUD_CTRL2_RF              0x00000100
#define DECOD_AUD_CTRL2_LL              0x00000080
#define DECOD_AUD_CTRL2_HL              0x00000040
#define DECOD_AUD_CTRL2_DN              0x00000020
#define DECOD_AUD_CTRL2_DE              0x00000010
#define DECOD_AUD_CTRL2_OM              0x00000008
#define DECOD_AUD_CTRL2_KM0             0x00000004
#define DECOD_AUD_CTRL2_AC3             0x00000000
#define DECOD_AUD_CTRL2_MPEG            0x00000001
#define DECOD_AUD_CTRL2_PCM             0x00000003

#define DECOD_AUD_CMD_RESET             0x00000000

#define DECOD_AUD_INT_CCC               0x00008000
#define DECOD_AUD_INT_RTBC              0x00004000
#define DECOD_AUD_INT_ED                0x00000800
#define DECOD_AUD_INT_PE                0x00000400
#define DECOD_AUD_INT_BE                0x00000200
#define DECOD_AUD_INT_BF                0x00000100
#define DECOD_AUD_INT_PSE               0x00000080
#define DECOD_AUD_INT_PTO               0x00000040
#define DECOD_AUD_INT_ADO               0x00000020
#define DECOD_AUD_INT_ADD               0x00000010
#define DECOD_AUD_INT_CM                0x00000001
#define DECOD_AUD_INT_ALL               0x0000CFF1

#define DECOD_AUD_DSR_CHAN_CH           0x00008000
#define DECOD_AUD_DSR_TB_CH_IN_PROC     0x00004000
#define DECOD_AUD_DSR_ERROR_MASK        0x00000C00
#define DECOD_AUD_DSR_AUX_DATA          0x00000010
#define DECOD_AUD_DSR_COMMAND_COM       0x00000002

#define DECOD_AUD_TONE_CTRL_TR          0x00000008
#define DECOD_AUD_TONE_CTRL_BA_MASK     0x00000007

#define DECOD_AUD_TONE_VALUE_BD_MASK    0x00001F00
#define DECOD_AUD_TONE_VALUE_FI_MASK    0x00001F7F

#define DECOD_AUD_QLR_BV                0x80000000
#define DECOD_AUD_QLR_SB                0x20000000
#define DECOD_AUD_QLR_QL_MASK           0x001FFFFF

#define DECOD_AUD_SEG_ALIGN             0x0000007F
#define DECOD_AUD_SEG_SH                7

/*-----------------------------------------------------------------------------+
| MPEG audio defines.
+-----------------------------------------------------------------------------*/
#define DECOD_AUD_AC3                   0x00000001
#define DECOD_AUD_LPCM                  0x00000002
#define DECOD_AUD_MPEG                  0x00000003
#define DECOD_AUD_PCM                   0x00000004

#define DECOD_AUD_STREAM_MPEG           0
#define DECOD_AUD_STREAM_PES            1
#define DECOD_AUD_STREAM_ES             2

#define AUDIO_CMD_RETRY                 50

#define DECOD_BEEP_FREQ_TBL_MAX         48
#define DECOD_AUD_BEEP_MAX_DUR          31
#define DECOD_AUD_BEEP_MIN_ATT          7

#define DECOD_AUD_DEF_ID                0xE0C0
#define DECOD_AUD_DEF_AC3_ID            0xFFBD

/*-----------------------------------------------------------------------------+
| Bit definitions for the MPEG video registers.
+-----------------------------------------------------------------------------*/
#define DECOD_RESET_CHIP                0x04000000

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

#define DECOD_VBI_CTL_VBI_MASK          0x00001F00
#define DECOD_VBI_CTL_TELETEXT_REVERSE  0x00000080
#define DECOD_VBI_CTL_TLTX_7128         0x00000000
#define DECOD_VBI_CTL_TLTX_VBI          0x00000020
#define DECOD_VBI_CTL_TLTX_7120         0x00000040
#define DECOD_VBI_CTL_TLTX_7120_EXT     0x00000060
#define DECOD_VBI_CTL_VBI_LINE_MASK     0x0000001F

#define DECOD_COMD_CHAIN                0x00000001

#define DECOD_COMD_STAT_PENDING         0x00000001

#define DECOD_HOST_INT_SERV_PICT        0x80000000
#define DECOD_HOST_INT_FF_STATUS        0x40000000
#define DECOD_HOST_INT_SAVED_PTS        0x08000000
#define DECOD_HOST_INT_ZOOM_OFF_OVER    0x02000000
#define DECOD_HOST_INT_CHAN_CHAN        0x01000000
#define DECOD_HOST_INT_PLB_ERROR        0x00800000
#define DECOD_HOST_INT_BLOCK_READ       0x00040000
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
#define DECOD_HOST_MASK_SAVED_PTS       0x08000000
#define DECOD_HOST_MASK_ZOOM_OFF_OVER   0x02000000
#define DECOD_HOST_MASK_CHAN_CHAN       0x01000000
#define DECOD_HOST_MASK_PLB_ERROR       0x00800000
#define DECOD_HOST_MASK_BLOCK_READ      0x00040000
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

#define DECOD_VCLIP_BLOCK_VALID         0x80000000
#define DECOD_VCLIP_END_OF_STREAM       0x40000000
#define DECOD_VCLIP_STREAM_BUISY        0x20000000

#define DECOD_DRAM_BUF2_LUM             0x00000000
#define DECOD_DRAM_BUF2_CH              0x00000100
#define DECOD_DRAM_BUF1_LUM             0x00000600
#define DECOD_DRAM_BUF1_CH              0x00000700
#define DECOD_DRAM_BUF0_LUM             0x00000400
#define DECOD_DRAM_BUF0_CH              0x00000500

#define DECOD_DRAM_BUF4_LUM             0x00000800
#define DECOD_DRAM_BUF4_CH              0x00000900
#define DECOD_DRAM_BUF6_LUM             0x00000A00
#define DECOD_DRAM_BUF6_CH              0x00000B00

#define DECOD_WR_PROT_DISABLE           0x00000001
#define DECOD_WR_PROT_ENABLE            0x00000000

/*-----------------------------------------------------------------------------+
| MPEG video decoder memory address definitions and memory usage.
+-----------------------------------------------------------------------------*/
#define DECOD_SP_ADDR                   0x100
#define DECOD_DRAM_SS1                  0x1F4
#define DECOD_DRAM_SS2                  0x1F6
#define DECOD_DRAM_PS                   0x1F8
#define DECOD_DRAM_TR1                  0x1FC
#define DECOD_DRAM_TR2                  0x1FE

#define DECOD_USER_MEM_SIZE             512
#define DECOD_MEM_ALIGN                 128
#define DECOD_RB_ALIGN                  32
#define DECOD_NTSC_FRAME_BUFFER_SIZE    1601280
#define DECOD_PAL_FRAME_BUFFER_SIZE     1785600
#define DECOD_PAL2M_FRAME_BUFFER_SIZE   1633024

#define DECOD_MIN_RATE_BUFF_SIZE        200000
#define DECOD_DRAM_MULT                 128
#define DECOD_OSD_BASE_ADDR             0x0900

#define DECOD_TIMEOUT                   25
#define DECOD_CLIP_TIMEOUT              500

#define MAX_VID_SERV_PICTURES           16
#define VID_SP_MODE_OFF                 0x00000000
#define VID_SP_MODE_INIT_PENDING        0x00000001
#define VID_SP_MODE_INIT_COMPLETE       0x00000002
#define VID_SP_MODE_DISP_PENDING        0x00000003
#define VID_SP_MODE_DISP_COMPLETE       0x00000004

/*-----------------------------------------------------------------------------+
| MPEG video decoder command definitions.
+-----------------------------------------------------------------------------*/
#define DECOD_COM_PLAY                  (0x0000<<1)
#define DECOD_COM_PAUSE                 (0x0001<<1)
#define DECOD_COM_SFRAME                (0x0002<<1)
#define DECOD_COM_FF                    (0x0003<<1)
#define DECOD_COM_SLOWMO                (0x0004<<1)
#define DECOD_COM_IMM_NOR_PLAY          (0x0005<<1)
#define DECOD_COM_NO_PAN_SCAN           (0x0006<<1)
#define DECOD_COM_FFRAME                (0x0007<<1)
#define DECOD_COM_RES_VID_BUF           (0x0008<<1)
#define DECOD_COM_CONF                  (0x0009<<1)
#define DECOD_COM_CH_SW                 (0x000A<<1)
#define DECOD_COM_INIT_SP               (0x000A<<1)
#define DECOD_COM_DISP_SP               (0x000B<<1)
#define DECOD_COM_CH_SW_PREOP           (0x000C<<1)
#define DECOD_COM_DEL_SP                (0x000C<<1)
#define DECOD_COM_STILL_P               (0x000E<<1)

#define DECOD_COM_PAL_4M                0x0000  /* PAL 4 Meg */
#define DECOD_COM_PAL                   0x8000
