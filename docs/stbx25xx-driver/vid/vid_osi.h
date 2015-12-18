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
| File:   vid_osi.h
| Purpose: video decoder operation system independence layer for PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01             create                          SL
+----------------------------------------------------------------------------*/

#ifndef PALLAS_VID_OSI_H
#define PALLAS_VID_OSI_H

#include <os/pm-alloc.h>
#include "vid_atom.h"
#include "vid/vid_types.h"
#include "clip/clip.h"


#define USER_DATA_H_SIZE                0x1f4
#define USER_DATA_PEL_ASPECT_RATIO      0x1f5
#define USER_DATA_V_SIZE                0x1f6
#define USER_DATA_FRAME_RATE            0x1f7


#define VID_CLIP_QUEUE_SIZE     4


/*typedef struct{
   vidSource_t          src;
   vidDispFmt_t     fmt;
   vidDispMode_t        mode;
   vidDispSize_t        size;
   vidDenc_t            denc;
   vidSync_t            sync;
}VID_MODE;              */


typedef struct tagVDEC
{
    /*-------------------------------------------------------
    | static set of the video decoder
    +--------------------------------------------------------*/
    AT_MEM          user;
    AT_MEM          framebuf;
    AT_MEM          ratebuf;
    AT_MEM          vbi0;
    AT_MEM          vbi1;
    AT_MEM          clipbuf;

    /*-------------------------------------------------------
    | Logical start address of video buffer
    +--------------------------------------------------------*/
    ULONG            ulVidLogAdr;

    /*-------------------------------------------------------
    | video decoder status
    +--------------------------------------------------------*/
    vidSource_t         src;
    vidDispMode_t       mode;
    vidDispSize_t       size;
    vidSync_t           sync;
    vidDispFmt_t        fmt;
    //UINT                  uDenc;

    vidState_t      old_state;
    vidState_t      state;
    UINT            uOpenFlag;
    UINT            uClipStart;

    /*-------------------------------------------------------
    | screen size and postion
    +--------------------------------------------------------*/
    UINT                    uLeft, uTop; //display border in small picture
    RECT                    rectSrc, rectDes; //src and des rect in scale mode

    /*--------------------------------------------------------
    | clip mode waiting clip queue
    +---------------------------------------------------------*/
    CLIPDEV_T       clipdev;
    //UINT            uClipBufLogAdr;

    /*--------------------------------------------------------
    | Teletext and vbi
    +---------------------------------------------------------*/
    UINT            nLineNum;

    /*---------------------------------------------------------
    | Still Picture
    +----------------------------------------------------------*/
    //SEMAPHORE_T     sema;
    UINT            seq_end;
    //MEM_HANDLE_T    spBuf;
}VDEC;

INT     vid_osi_init();
void    vid_osi_close();
//INT     vid_osi_set_source(vidSource_t src);
INT     vid_osi_set_dispfmt(vidDispFmt_t fmt);
INT     vid_osi_set_dispmode(vidDispMode_t mode);
INT     vid_osi_set_dispsize(vidDispSize_t size);
void    vid_osi_set_scalepos(RECT *prectSrc, RECT *prectDes);
void    vid_osi_scale_on(UINT uFlag);
void    vid_osi_scale_off();
INT     vid_osi_get_v_info(VIDEOINFO* v_info);
INT     vid_osi_set_sfm(vidsfm_t sfm);
INT     vid_osi_enable_disp_14_9(int top_border);
INT     vid_osi_disable_disp_14_9();


//INT     vid_osi_get_status(VID_STATUS *pStatus);
INT     vid_osi_play_status(VIDPLAYSTATUS *pstatus);
INT     vid_osi_get_openflag();

INT     vid_osi_play();
INT     vid_osi_stop(UINT uFlag);
INT     vid_osi_pause();
INT     vid_osi_freeze();
INT     vid_osi_single_frame(INT mode);
void    vid_osi_blank();
void    vid_osi_show();
INT     vid_osi_get_microcode_ver(ULONG *pVer);

//Synchronization group:
void    vid_osi_set_pts(STC_T *pData);
void    vid_osi_get_pts(STC_T *pData);
void    vid_osi_set_stc(STC_T *pData);
void    vid_osi_get_stc(STC_T *pData);
void    vid_osi_set_sync_type(vidSync_t sync_t);
INT     vid_osi_enable_sync();
void    vid_osi_disable_sync();
//TV group
INT     vid_osi_init_tv();
void    vid_osi_close_tv();

//Clip group
INT     vid_osi_init_clip(UINT uClipBufPhyAdr,
                          UINT uClipBufLen);  /*uClipBufLen must be multiply of 4k*/
void    vid_osi_close_clip();
INT     vid_osi_clip_write(CLIPINFO *info);
CLIPDEV_T vid_osi_get_clipdev();
ULONG   vid_osi_buf_fill(void *src, ULONG ulLen);
ULONG   vid_osi_buf_fill_blocked(void *src, ULONG ulLen);

INT     vid_osi_fast_forward(UINT uSpeed);
INT     vid_osi_slow_motion(UINT uSpeed);
INT     vid_osi_reset_clipbuf();
INT     vid_osi_clip_cc();
INT     vid_osi_clip_flush();

//still picture
INT     vid_osi_init_stillp(UINT uClipBufPhyAdr,
                                        UINT uClipBufLen);
INT     vid_osi_close_stillp();
INT     vid_osi_stillp_ready();
void*   vid_osi_get_stillp_buf();

//single frame
int     vid_osi_single_frame_complete();
void    vid_osi_ipdc_complete();

//Task
void    vid_osi_task(QUEUE_T *pQueue);


#endif
