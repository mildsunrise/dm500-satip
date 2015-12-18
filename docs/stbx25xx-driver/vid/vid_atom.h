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
| File:   vid_atom.h
| Purpose: video decoder atom layer for PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01     create                                                      SL
+----------------------------------------------------------------------------*/

#ifndef PALLAS_VID_ATOM_H
#define PALLAS_VID_ATOM_H

#include <os/helper-queue.h>
#include "clip/clip.h"
#include "vid/vid_types.h"
#include "hw/physical-mem.h"
#include "vid_atom_hw.h"

/*----------------------------------------------------------------------------+
| Defines
+----------------------------------------------------------------------------*/
#ifndef NULL
#define NULL                0
#endif

#define VID_VBI_LINES               16
#define VID_FRAME_MEM_BASE          __STB_V_FB_MEM_BASE_ADDR
#define VID_FRAME_MEM_SIZE          __STB_V_FB_MEM_SIZE

#define VID_MEM_BASE                __STB_V_MEM_BASE_ADDR
#define VID_MEM_SIZE                __STB_V_MEM_SIZE
#define VID_USER_MEM_BASE           VID_MEM_BASE            // start of video memory area
#define VID_USER_MEM_SIZE           512                     // fixed at 512
#define VID_VBI0_MEM_BASE           VID_MEM_BASE+ VID_USER_MEM_SIZE
#define VID_VBI0_MEM_SIZE           (VID_VBI_LINES*1440)
#define VID_VBI1_MEM_BASE           VID_MEM_BASE+ VID_USER_MEM_SIZE+VID_VBI0_MEM_SIZE
#define VID_VBI1_MEM_SIZE           (VID_VBI_LINES*1440)
#define VID_RB_MEM_BASE             VID_MEM_BASE+ VID_USER_MEM_SIZE+VID_VBI0_MEM_SIZE+VID_VBI1_MEM_SIZE
#define VID_RB_MEM_SIZE             VID_MEM_SIZE- VID_USER_MEM_SIZE-VID_VBI0_MEM_SIZE-VID_VBI1_MEM_SIZE

#define VID_MODE_NORMAL             0x0000
#define VID_MODE_LETTER_BOX         0x0001
#define VID_MODE_SCALE              0x0006
#define VID_MODE_DISEXP             0x0007
#define VID_MODE_16_9               0x0100
#define VID_MODE_14_9               0x0101
#define VID_MODE_PAL                0x1000

#define VID_IS_PAL_MODE(x)          ((x) & VID_MODE_PAL) ? 1:0;
#define VID_IS_16_9_MODE(x)         ((x) & VID_MODE_16_9) ? 1:0;

//#define VID_NO_SOURCE               0
//#define VID_SOURCE_DEMUX            1
//#define VID_SOURCE_MEMORY           2

#define VID_RB_NORMAL_PLAY        1
#define VID_RB_SFM                2
#define VID_RB_OTHER              3

#define DECOD_DLY_DENC_INTERNAL     0x00004714
#define DECOD_DLY_DENC_EXTERNAL     0x00004714

/*message constant*/
#define VID_MSG_BLOCK_READ      0x00000001
#define VID_MSG_SEQUENCE_END    0x00000002

/*interrupt notify function*/
#define VID_NOTIFY_MAX          16
#define VID_NOTIFY_VALID_MASK ( DECOD_HOST_MASK_SERV_PICT         |           \
                                DECOD_HOST_MASK_FF_STATUS         |           \
                                DECOD_HOST_MASK_PS_STATUS         |           \
                                DECOD_HOST_MASK_IPDC              |           \
                                DECOD_HOST_MASK_SAVED_PTS         |           \
                                DECOD_HOST_MASK_VSOR              |           \
                                DECOD_HOST_MASK_ZOOM_OFF_OVER     |           \
                                DECOD_HOST_MASK_CHAN_CHAN         |           \
                                DECOD_HOST_MASK_PLB_ERROR         |           \
                                DECOD_HOST_MASK_BLOCK_READ        |           \
                                DECOD_HOST_MASK_SS                |           \
                                DECOD_HOST_MASK_SERROR            |           \
                                DECOD_HOST_MASK_SEND              |           \
                                DECOD_HOST_MASK_SMPTE             |           \
                                DECOD_HOST_MASK_PSKIP             |           \
                                DECOD_HOST_MASK_PSTART            |           \
                                DECOD_HOST_MASK_PRESOL            |           \
                                DECOD_HOST_MASK_USR_DATA          |           \
                                DECOD_HOST_MASK_VBI_START         |           \
                                DECOD_HOST_MASK_VIDEO_START       |           \
                                DECOD_HOST_MASK_FF_VIDEO_START    |           \
                                DECOD_HOST_MASK_BLK_MOVE_COMPL    |           \
                                DECOD_HOST_MASK_TIMER_BASE_CH     |           \
                                DECOD_HOST_MASK_VID_RB_TH         |           \
                                DECOD_HOST_MASK_VID_RB_OV         |           \
                                DECOD_HOST_MASK_OSD_DATA)

/*----------------------------------------------------------------------------+
| Type Definitions
+----------------------------------------------------------------------------*/
typedef struct tagVID_NOTIFY
{
    void (*fn)(unsigned int intreg);
    unsigned int mask;
} VID_NOTIFY;

typedef struct tagMemSeg
{
    ULONG   uAddr;
    UINT    uSegSize;
}MEM_SEG;

typedef struct tagAT_MEM
{
    UINT32  uAddr;  //Start physical address of the memory
    UINT    uLen;   //Size of this memory
    ULONG   ulLogicalAddr; //starting logical address
    ULONG   ulVideoLogicalAddr; //starting address in video logical space
}AT_MEM;

typedef struct tagVDEC_CONFIG
{
  //memory base
    AT_MEM  user;
    AT_MEM  framebuf;
    AT_MEM  ratebuf;
}VDEC_CON;


#ifndef TASK_MSG
typedef struct tagTaskMsg
{
  UINT    uMsgType;
  ULONG   ulPara1;
  ULONG   ulPara2;
}TASK_MSG;
#endif

/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/
//Common group:
INT     vid_atom_init(UINT uDispDelay);
INT     vid_atom_config_vdec(VDEC_CON *pVdec);
void    vid_atom_set_dispmode(UINT uMode);
void    vid_atom_set_dispfmt(UINT uPal);
void    vid_atom_set_dispborder(UINT uLeft, UINT uTop);
void    vid_atom_set_scalepos(RECT *prectSrc, RECT *prectDes);
void    vid_atom_scale_on(UINT uFlag);
void    vid_atom_scale_off();
void    vid_atom_set_sfm(vidsfm_t sfm);
//UINT32  vid_atom_get_irq_mask();
//void    vid_atom_set_irq_mask(UINT32 uMask);
INT     vid_atom_play();
void    vid_atom_stop();
INT     vid_atom_pause();
void    vid_atom_close();
INT     vid_atom_freeze();
void    vid_atom_blank();
void    vid_atom_show();
INT     vid_atom_reset_ratebuf(int mode);
void    vid_atom_clear_rb();
void    vid_atom_clear_framebuf();
void    vid_atom_set_memseg(MEM_SEG *pMem);
INT     vid_atom_get_microcode_ver(ULONG *pVer);
void    vid_atom_reg_dump();
INT     vid_atom_force_frame_switch(ULONG initflag);
INT     vid_atom_reset_ratebuf_freeze();

//Synchronization group:
void    vid_atom_set_pts(STC_T *pData);
void    vid_atom_get_pts(STC_T *pData);
void    vid_atom_set_stc(STC_T *pData);
void    vid_atom_get_stc(STC_T *pData);
void    vid_atom_enable_sync(UINT uVidMaster);
INT     vid_atom_disable_sync();

//TV group:
INT     vid_atom_init_tv(AT_MEM *pRateBase, UINT uRateThreshold);
//void    vid_atom_close_tv();

//Clip group:
INT     vid_atom_init_clip(AT_MEM *pRateBase);
void    vid_atom_close_clip();
void    vid_atom_write_clip(CLIPINFO *pInfo);
INT     vid_atom_buf_ready();
INT     vid_atom_fast_forward(UINT uSpeed);
INT     vid_atom_slow_motion(UINT uSpeed);
//still picture
INT     vid_atom_enter_stillp(UINT uStillpAdr);
INT     vid_atom_exit_stillp(void);
ULONG   vid_atom_get_stillp_buf(int color);

//single frame
int     vid_atom_resume_from_sf(ULONG mode);
INT     vid_atom_svd_has_reset();
int     vid_atom_single_frame(int mode);
void    vid_atom_set_rb_size(ULONG value);

//Interrupt group:
void    vid_atom_irq_handle(UINT uIrq, void *pData);   //interrupt handler
void    vid_atom_init_irq_mask();
UINT32  vid_atom_get_irq_mask();
void    vid_atom_set_irq_mask(UINT32 uMask);

//For Teletext Driver Only!
int     vid_atom_ttx_add_notify(void(*fn)(unsigned int intreq), unsigned int mask);
int     vid_atom_ttx_del_notify(void(*fn)(unsigned int intreq), unsigned int mask);

#endif
