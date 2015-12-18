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
| File:   vid_osi.c
| Purpose: video decoder osi layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01	create                                                     SL
| 10-Sep-03     added  vid_osi_enable_disp_14_9() and vid_osi_disable_disp_14_9()
|               to support a 14:9 Pillarbox image in a 16:9 frame. This is used 
|               when displaying on a 4:3 monitor. The microcode will do a
|               horizontal expansion of the 14:9 image by 8/7 to remove the 
|               black borders on each side.                                MD
+----------------------------------------------------------------------------*/
#include <os/helper-queue.h>
#include <os/os-interrupt.h>
#include <os/drv_debug.h>
#include <os/pm-alloc.h>
#include <hw/hardware.h>

//lingh added
#include <linux/sched.h>

#include "vid_osi.h"
#include "vid_osi_scr.h"
#include "vid_atom_hw.h"
#include "vid_atom_local.h"
#include "vid_atom.h"


/*#ifdef MODULE
MODULE_PARM(vid, "i");
MODULE_PARM_DESC(osd_afl,
                 "select antiflick level: 0-no, 1- mild, 2-medium, 3-maximum");
#endif*/

extern VDEC _videoDecoder;

//lingh added
//static DECLARE_WAIT_QUEUE_HEAD(WaitQ);
volatile int __VID_PLAY_PENDING = 0; 
volatile VIDPLAYSTATUS vid_play_stat;
vidsfm_t vid_sfm = VID_SFM_NORMAL;
static int saved_LB_top_border = 0;

/*Basic initialization*/
INT vid_osi_init()
{
    VDEC_CON conf;
    UINT     fmt;
    ULONG    reg;


    if(_videoDecoder.uOpenFlag == 1)
        return 0;

    vid_sfm = VID_SFM_NORMAL;

    //memory configuration
    PDEBUG("vid_osi_init: _videoDecoder.user.uAddr = 0x%8.8x, _videoDecoder.user.uLen = 0x%8.8x\n",_videoDecoder.user.uAddr,_videoDecoder.user.uLen);
    conf.user.uAddr = _videoDecoder.user.uAddr;
    conf.user.uLen  = _videoDecoder.user.uLen;
    conf.user.ulLogicalAddr = _videoDecoder.user.ulLogicalAddr;

    PDEBUG("vid_osi_init: _videoDecoder.framebuf.uAddr = 0x%8.8x, _videoDecoder.framebuf.uLen = 0x%8.8x\n",_videoDecoder.framebuf.uAddr,_videoDecoder.framebuf.uLen);
    conf.framebuf.uAddr = _videoDecoder.framebuf.uAddr;
    conf.framebuf.uLen  = _videoDecoder.framebuf.uLen;
    conf.framebuf.ulLogicalAddr = _videoDecoder.framebuf.ulLogicalAddr;

    PDEBUG("vid_osi_init: _videoDecoder.ratebuf.uAddr = 0x%8.8x, _videoDecoder.ratebuf.uLen = 0x%8.8x\n",_videoDecoder.ratebuf.uAddr,_videoDecoder.ratebuf.uLen);
    conf.ratebuf.uAddr = _videoDecoder.ratebuf.uAddr;
    conf.ratebuf.uLen  = _videoDecoder.ratebuf.uLen;
    conf.ratebuf.ulLogicalAddr = _videoDecoder.ratebuf.uLen;

    if(vid_atom_config_vdec(&conf) != 0)
        return -1;

    //Get denc information from scrman
    //_videoDecoder.uDenc = scrman_osi_get_denc_id();

    //Get denc format from scrman
    fmt = scrman_osi_get_fmt();
    if(fmt == SCRMAN_FMT_NTSC)
        _videoDecoder.fmt = VID_DISPFMT_NTSC;
    else if(fmt == SCRMAN_FMT_PAL)
        _videoDecoder.fmt = VID_DISPFMT_PAL;
    else
        _videoDecoder.fmt = VID_DISPFMT_UNKNOWN;

    PDEBUG("video fmt = %d\n", _videoDecoder.fmt);

    //set video decoder to NORMAL mode
    if( vid_osi_set_dispmode(VID_DISPMODE_NORM) != 0)
    {
        PDEBUG("set disp NORMAL error\n");
        return -1;
    }

    PDEBUG("video mode = %d\n", _videoDecoder.mode);
    //set video screen size to 4:3
    if( vid_osi_set_dispsize(VID_DISPSIZE_4x3) != 0)
    {
        PDEBUG("set disp size to 4:3 error\n");
        return -1;
    }

    PDEBUG("video size = %d\n", _videoDecoder.size);
    //set video display border to (0,0)
    vid_atom_set_dispborder(0,0);

    // reset the video ratebuffer in case there is leftover data.

    vid_atom_reset_ratebuf(0);

    // clear the video frame buffers to black.

    vid_atom_clear_framebuf();

    // To finish initializing the video output display regs, force
    // two frame switches setting the initialization flag on the
    // first one (the start decode bit must be set during this 
    // operation). Then execute the reset ratebuffer command 
    // chained to freeze frame - this will force the video decoder
    // into freeze frame mode. When the normal play command is finally
    // executed it will not begin displaying video until after two 
    // reference frames have been decoded (normal exit from freeze frame).

    
    reg = MF_DCR(VID_CHIP_CTRL);
    MT_DCR(VID_CHIP_CTRL, reg | DECOD_CHIP_CONTROL_SVD); 

    if(vid_atom_force_frame_switch(1) != 0)
      printk("vid_atom_force_frame_switch(1) failed\n");

    if(vid_atom_force_frame_switch(0) != 0)
      printk("vid_atom_force_frame_switch(0) failed\n");

    MT_DCR(VID_CHIP_CTRL, reg);   // restore start decode

    vid_atom_reset_ratebuf_freeze();

    os_disable_irq(IRQ_VID);
    //add interrupt task
    if(os_add_irq_task(IRQ_VID,
                       vid_osi_task,
                       sizeof(TASK_MSG),
                       32) != 0)
    {
        PDEBUG("install task error\n");
        return -1;
    }

    vid_atom_init_irq_mask();
    os_enable_irq(IRQ_VID);

    PDEBUG("install irq handler and task OK\n");
    memset((void *)(&vid_play_stat),0,sizeof(VIDPLAYSTATUS));
    __VID_PLAY_PENDING = 0;
    _videoDecoder.src = VID_SOURCE_NO;
    _videoDecoder.state = VID_STOPPED;
    _videoDecoder.uOpenFlag = 1;
    return 0;
}


void vid_osi_close()
{
    if (_videoDecoder.uOpenFlag == 0)
        return;

    PDEBUG("vid osi close\n");

/*    if(_videoDecoder.state != VID_STOPPED)
        vid_atom_freeze();

    vid_atom_close();
    _videoDecoder.state = VID_STOPPED;

    //delete clip device
    if(_videoDecoder.clipdev)
    {
        clipdev_delete(_videoDecoder.clipdev);
        _videoDecoder.clipdev = NULL;
    }*/

    if(_videoDecoder.src == VID_SOURCE_DEMUX)
        vid_osi_close_tv();
    else if(_videoDecoder.src == VID_SOURCE_MEMORY)
        vid_osi_close_clip();

    os_delete_irq_task(IRQ_VID);    //remove irq task

    _videoDecoder.ulVidLogAdr = 0;
    _videoDecoder.uOpenFlag = 0;

    vid_atom_clear_framebuf();
//    mpeg_v_buf_blank(pVDEC, FBUFFERS_FLUSH);
//    mpeg_v_buf_blank(pVDEC, FBUFFERS_RB_FLUSH);
}


INT vid_osi_get_v_info(VIDEOINFO* v_info)
{

    ULONG adr = _videoDecoder.user.ulLogicalAddr;

    if(adr == 0)
        return -1;
    v_info->h_size = ((*((unsigned short*)(adr + USER_DATA_H_SIZE))) & 0xFFF0) >> 4;
    v_info->pel_aspect_ratio = (*((unsigned char*)(adr + USER_DATA_PEL_ASPECT_RATIO))) & 0x000F;
    v_info->v_size = ((*((unsigned short*)(adr + USER_DATA_V_SIZE))) & 0xFFF0) >> 4;
    v_info->frame_rate = ((*((unsigned char*)(adr + USER_DATA_FRAME_RATE))) & 0x000f);
    //printk("v_info hsize = %d, v_size = %d, frame rate = %d\n", v_info->h_size, v_info->v_size, v_info->frame_rate);
    return 0;
}

void vid_osi_set_disp()
{
    UINT uFlag;

    uFlag = _videoDecoder.mode;
    if(_videoDecoder.size == VID_DISPSIZE_16x9)
        uFlag |= VID_MODE_16_9;

    vid_atom_set_dispmode(uFlag);
}

INT vid_osi_set_dispfmt(vidDispFmt_t fmt)
{
    VIDEOCMD vc;

    if(fmt != _videoDecoder.fmt)
    {
        PDEBUG("WARNING: try to set different display format\n");
        if(_videoDecoder.fmt == VID_DISPFMT_NTSC)
        {
            //scrman set the display format of both denc and video decoder
            if(scrman_osi_set_fmt(SCRMAN_FMT_PAL) != 0)
                return -1;
            _videoDecoder.fmt = VID_DISPFMT_PAL;
        }
        else
        {
            //scrman set the display format of both denc and video decoder
            if(scrman_osi_set_fmt(SCRMAN_FMT_NTSC) != 0)
                return -1;
            _videoDecoder.fmt = VID_DISPFMT_NTSC;
        }
  
        vc.uCmd = DECOD_COM_CONF;
        vc.uNum = 1;
        vc.uPara[0] = 0;
        vc.uChained = 0;
        vc.uRetry = DECOD_TIMEOUT;
        if (vid_atom_exec_cmd(&vc) != 0)
        {
          PDEBUG("execute config command failed\n");
          return -1;
        }
    }
    return 0;
}

INT vid_osi_set_dispmode(vidDispMode_t mode)
{
    _videoDecoder.mode = mode;
    vid_osi_set_disp();
    return 0;
}

INT vid_osi_set_sfm(vidsfm_t sfm)
{
  int ret;
  
  ret = 0;  
  switch(sfm)
  {
    case VID_SFM_NORMAL:
        vid_sfm = VID_SFM_NORMAL;  // unlock sfm
	vid_atom_set_sfm(sfm);
        break;
	
    case VID_SFM_BOTTOM_ONLY:
        vid_sfm = VID_SFM_NORMAL;
	vid_atom_set_sfm(sfm);
	vid_sfm = sfm;
        break;
	    
    case VID_SFM_TOP_ONLY:
        vid_sfm = VID_SFM_NORMAL;
	vid_atom_set_sfm(sfm);
	vid_sfm = sfm;
        break;
	    
    case VID_SFM_FIRST_ONLY:
        vid_sfm = VID_SFM_NORMAL;
	vid_atom_set_sfm(sfm);
	vid_sfm = sfm;
        break;
	    
    default:  
        ret = -1;
	break;
  }	
  return ret;
}

INT vid_osi_set_dispsize(vidDispSize_t size)
{
    _videoDecoder.size = size;
    vid_osi_set_disp();
    return 0;
}

inline void vid_osi_copy_rect(RECT *pDes, RECT *pSrc)
{
    pDes->hori_off = pSrc->hori_off;
    pDes->hori_size = pSrc->hori_size;
    pDes->vert_off = pSrc->vert_off;
    pDes->vert_size = pSrc->vert_size;
}


void vid_osi_set_scalepos(RECT *prectSrc, RECT *prectDes)
{
    //set new scale position to video decoder
    vid_atom_set_scalepos(prectSrc, prectDes);
    
    vid_osi_copy_rect(&(_videoDecoder.rectSrc), prectSrc);
    vid_osi_copy_rect(&(_videoDecoder.rectDes), prectDes);
}

void vid_osi_scale_on(UINT uFlag)
{
#ifdef __DRV_FOR_PALLAS__
    vid_atom_scale_on(uFlag);
    _videoDecoder.mode = VID_DISPMODE_SCALE;
#else
    vid_osi_set_disp();		//set dispmode to last mode
#endif
}

void vid_osi_scale_off()
{
    vid_atom_scale_off();
    //_videoDecoder.mode = VID_DISPMODE_NORM;
}

INT  vid_osi_get_openflag()
{
    return _videoDecoder.uOpenFlag;
}

INT  vid_osi_play()
{
    INT flag;
  
    flag = os_enter_critical_section();
    memset((void *)(&vid_play_stat),0,sizeof(VIDPLAYSTATUS));
    __VID_PLAY_PENDING = 1;
    vid_atom_set_irq_mask(vid_atom_get_irq_mask() | DECOD_HOST_MASK_PSTART);
    os_leave_critical_section(flag);

    if(_videoDecoder.src == VID_SOURCE_NO)
    {
        return -1;
    }

    if(_videoDecoder.state == VID_PLAYING)
    {
        return 0;
    }
    
    //start playing
    if( vid_atom_play() != 0)
        return -1;

    vid_osi_enable_sync();

    _videoDecoder.state = VID_PLAYING;
    return 0;
}

INT vid_osi_play_status(VIDPLAYSTATUS *pstatus)
{
  int flag;
  
  flag = os_enter_critical_section();
  memcpy(pstatus, (void *)&vid_play_stat, sizeof(VIDPLAYSTATUS));
  if(pstatus->picture_start == 1)
  {
    os_leave_critical_section(flag);
    vid_atom_set_irq_mask(vid_atom_get_irq_mask() | DECOD_HOST_MASK_PSTART); // clear any pending pstart int
    flag = os_enter_critical_section();
    memset((void *)&vid_play_stat,0,sizeof(VIDPLAYSTATUS));
  }
  __VID_PLAY_PENDING = 1;
  vid_atom_set_irq_mask(vid_atom_get_irq_mask() | DECOD_HOST_MASK_PSTART);
  os_leave_critical_section(flag);
  return(0);
}


INT vid_osi_stop(UINT uFlag)
{
    if(_videoDecoder.state == VID_STOPPED)
        return 0;

    vid_osi_disable_sync();

    if (uFlag)
      // blank screen
      vid_atom_blank();

    // screen freeze
    vid_atom_freeze();

    vid_atom_stop();
    _videoDecoder.state = VID_STOPPED;
    return 0;
}

INT vid_osi_pause()
{
    if(_videoDecoder.state == VID_PAUSED)
        return 0;
    if(vid_atom_pause() != 0)
    {
        //return -1;  make sure this operation will success
		;
    }
    _videoDecoder.state = VID_PAUSED;
    return 0;
}

INT  vid_osi_freeze()
{
    if(_videoDecoder.state == VID_FREEZED)
        return 0;
    if(vid_atom_freeze() !=0 )
    {
        //return -1; make sure this operation will success
		;
    }
    _videoDecoder.state = VID_FREEZED;
    return 0;
}

INT vid_osi_single_frame(int mode)
{
    unsigned reg;
    
    if(_videoDecoder.state == VID_SINGLE_FRAME) 
    {
      // already in single frame mode -- advance one frame
      reg = MF_DCR(VID_CHIP_CTRL);
      MT_DCR(VID_CHIP_CTRL, reg | DECOD_CHIP_CONTROL_SVD); 
    }
    else if(vid_atom_single_frame(mode) != 0)
    {
        PDEBUG("vid_atom_single_frame(%d) failed\n",mode);
    }
    _videoDecoder.state = VID_SINGLE_FRAME;
    return 0;
}

void vid_osi_blank()
{
    PDEBUG("video blank\n");
    vid_atom_blank();
}

void vid_osi_show()
{
    PDEBUG("video show\n");
    vid_atom_show();
}

/*---------------------------------------------------------------------------+
| GET microcode version
+----------------------------------------------------------------------------*/
INT vid_osi_get_microcode_ver(ULONG *pVer)
{
   return vid_atom_get_microcode_ver(pVer);
}


INT vid_osi_init_tv()
{
    if(_videoDecoder.uOpenFlag == 0)
    {
        PDEBUG("video decoder not initialized\n");
        return -1;
    }
    if(_videoDecoder.src != VID_SOURCE_NO)
    {
        PDEBUG("video decoder is in use, src = %d\n", _videoDecoder.src);
        return -1;
    }
    
    if(vid_atom_init_tv(&(_videoDecoder.ratebuf), 0) != 0)
        return -1;
    _videoDecoder.src = VID_SOURCE_DEMUX;
    return 0;
}

void vid_osi_close_tv()
{
    if(_videoDecoder.state != VID_STOPPED)
        vid_osi_stop(1);
    _videoDecoder.src = VID_SOURCE_NO;
}


INT vid_osi_init_clip(UINT uClipBufPhyAdr, UINT uClipBufLen)
{
PDEBUG("vid_osi_init_clip: start\n");
    if(_videoDecoder.uOpenFlag == 0)
    {
        PDEBUG("video decoder not initialized\n");
        return -1;
    }
    if(_videoDecoder.src != VID_SOURCE_NO)
    {
        PDEBUG("video decoder is in use, src = %d\n", _videoDecoder.src);
        return -1;
    }

    //make sure clip buffer is 128bytes aligned and the size
    //is 32 bytes aligned.
    _videoDecoder.clipbuf.uAddr = uClipBufPhyAdr;
    _videoDecoder.clipbuf.uLen  = uClipBufLen;
    //_videoDecoder.uClipBufLogAdr = uClipBufLogAdr;

    if(vid_atom_init_clip(&(_videoDecoder.clipbuf)) != 0)
        return -1;

    //create clip device
    if(uClipBufLen%(VID_CLIP_QUEUE_SIZE*256) != 0)
    {
        PDEBUG("clip buffer not 4k aligned \n");
        return -1;
    }

    if((_videoDecoder.clipdev = clipdev_create(uClipBufLen / VID_CLIP_QUEUE_SIZE,
                                       VID_CLIP_QUEUE_SIZE,
                                       vid_osi_clip_write,
                                       vid_atom_buf_ready)) == NULL)
    {
	PDEBUG("create clip device error\n");
        return -1;
    }

    _videoDecoder.src = VID_SOURCE_MEMORY;

    //clear frame buffer
    vid_atom_clear_framebuf();

    PDEBUG("clip mode init OK\n");
    //vid_atom_reg_dump();
    return 0;
}

void vid_osi_close_clip()
{
    if(_videoDecoder.state != VID_STOPPED)
        vid_osi_stop(1);
    
    vid_atom_close_clip();
    if(_videoDecoder.clipdev)
    {
        clipdev_delete(_videoDecoder.clipdev);
        _videoDecoder.clipdev = NULL;
    }

    _videoDecoder.src = VID_SOURCE_NO;
}

INT vid_osi_reset_clipbuf()
{
    if(_videoDecoder.clipdev)
    {
        clipdev_delete(_videoDecoder.clipdev);
        _videoDecoder.clipdev = NULL;
    }
    if((_videoDecoder.clipdev = clipdev_create(_videoDecoder.clipbuf.uLen / VID_CLIP_QUEUE_SIZE,
                                       VID_CLIP_QUEUE_SIZE,
                                       vid_osi_clip_write,
                                       vid_atom_buf_ready)) == NULL)
    {
		PDEBUG("create clip device error\n");
        return -1;
    }
	return 0;
}


void* vid_osi_get_stillp_buf(int color)
{
    //int flag;
    //ULONG adr;

    if(_videoDecoder.state != VID_STILLP)
        return NULL;
    
    return (void*)vid_atom_get_stillp_buf(color);
    //return os_get_logical_address(_videoDecoder.spBuf);
    //flag = ((*((unsigned short*)(_videoDecoder.ulUserBufLogAdr + USER_DATA_H_SIZE))) & 0xFFF0) >> 4;
}

INT vid_osi_init_stillp(UINT uClipBufPhyAdr, UINT uClipBufLen)
{

PDEBUG("vid_osi_init_stillp: start\n");

    if(_videoDecoder.uOpenFlag == 0)
    {
        PDEBUG("video decoder not initialized\n");
        return -1;
    }
    if(_videoDecoder.src != VID_SOURCE_NO)
    {
        PDEBUG("video decoder is in use, src = %d\n", _videoDecoder.src);
        return -1;
    }

    //make sure clip buffer is 128bytes aligned and the size
    //is 32 bytes aligned.
    _videoDecoder.clipbuf.uAddr = uClipBufPhyAdr;
    _videoDecoder.clipbuf.uLen  = uClipBufLen;
    //_videoDecoder.uClipBufLogAdr = uClipBufLogAdr;

    //if(vid_atom_init_clip(&(_videoDecoder.clipbuf)) != 0)
    //    return -1;

    if(vid_atom_enter_stillp(VID_RB_MEM_BASE) < 0)
    {
        //if(_videoDecoder.spBuf)
        //    os_free_physical(_videoDecoder.spBuf);
        return -1;
    }

    //create clip device
    if(uClipBufLen%(VID_CLIP_QUEUE_SIZE*256) != 0)
    {
        PDEBUG("clip buffer not 4k aligned \n");
        return -1;
    }

    if((_videoDecoder.clipdev = clipdev_create(uClipBufLen / VID_CLIP_QUEUE_SIZE,
                                       VID_CLIP_QUEUE_SIZE,
                                       vid_osi_clip_write,
                                       vid_atom_buf_ready)) == NULL)
    {
	    PDEBUG("create clip device error\n");
        return -1;
    }

    _videoDecoder.src = VID_SOURCE_MEMORY;
    PDEBUG("stillp mode init OK\n");
    //vid_atom_reg_dump();

    _videoDecoder.state = VID_STILLP;
    _videoDecoder.seq_end = 1;
    return 0;
}

INT vid_osi_close_stillp()
{   
    vid_atom_exit_stillp();
    
    if(_videoDecoder.clipdev)
    {
        clipdev_delete(_videoDecoder.clipdev);
        _videoDecoder.clipdev = NULL;
    }

    _videoDecoder.src = VID_SOURCE_NO;
//    _videoDecoder.state = VID_PLAYING;
    return 0;
}


INT vid_osi_stillp_ready()
{
    UINT32 flags;

    flags = os_enter_critical_section();
    if(_videoDecoder.seq_end == 1)
    {
        _videoDecoder.seq_end = 0;
        os_leave_critical_section(flags);
        return 0;
    }
    os_leave_critical_section(flags);
    return -1;
}
INT vid_osi_clip_cc()
{
   unsigned reg;

   clipdev_stop(_videoDecoder.clipdev);

   MT_DCR(VID_VCLIP_LEN,0);                           /* clear block valid bit */

   MT_DCR(VID_CMD_STAT, 0);                         /* reset video rate buffer */
   MT_DCR(VID_PROC_IADDR, 0x00008200);
   vid_atom_reset_ratebuf(0x8000);

    reg = MF_DCR(VID_CHIP_CTRL);
    MT_DCR(VID_CHIP_CTRL, reg | DECOD_CHIP_CONTROL_SVD);

   _videoDecoder.state = VID_PLAYING;
   return 0;
}

INT vid_osi_clip_flush()
{
   clipdev_stop(_videoDecoder.clipdev);

   MT_DCR(VID_VCLIP_LEN,0);                           /* clear block valid bit */

   MT_DCR(VID_CMD_STAT, 0);                         /* reset video rate buffer */
   MT_DCR(VID_PROC_IADDR, 0x00008200);
   vid_atom_reset_ratebuf(0x0000);

   return 0;
}

void vid_osi_task(QUEUE_T *pQueue)
{
    TASK_MSG msg;

    PDEBUG("vid task\n");
    while( os_get_queue_status(pQueue) > 0)
    {
        //get first message
        if( os_dequeue(pQueue, &msg) != 0 )
        {
            PDEBUG("no message\n");
            return;
        }
        if( msg.uMsgType == VID_MSG_BLOCK_READ)
        {
            PDEBUG("block read msg\n");
            clipdev_clipinfo_done(_videoDecoder.clipdev);
            //write next clip
            clipdev_hw_write(_videoDecoder.clipdev);
            continue;
        }
        if( msg.uMsgType == VID_MSG_SEQUENCE_END)
        {
            UINT32 flags;
            flags = os_enter_critical_section();
            _videoDecoder.seq_end = 1;
            os_leave_critical_section(flags);
            continue;
        }
    }
}

INT  vid_osi_fast_forward(UINT uSpeed)
{
    /*if(_videoDecoder.src != VID_SOURCE_MEMORY)
    {
        PDEBUG("init memory mode first\n");
        return -1;
    }*/
    if(vid_atom_fast_forward(uSpeed) != 0)
        return -1;
    _videoDecoder.state = VID_FASTFORWORD;
    return 0;
}

INT  vid_osi_slow_motion(UINT uSpeed)
{
    /*if(_videoDecoder.src != VID_SOURCE_MEMORY)
    {
        PDEBUG("init memory mode first\n");
        return -1;
    }*/
    if(vid_atom_slow_motion(uSpeed) != 0)
        return -1;
    _videoDecoder.state  = VID_SLOWMOTION;
    return 0;
}

void  vid_osi_set_sync_type(vidSync_t sync)
{
    _videoDecoder.sync = sync;
}

INT  vid_osi_enable_sync()
{
    if(_videoDecoder.sync == VID_SYNC_NO)
    {
        PDEBUG("not set sync type\n");
        return -1;
    }
    if(_videoDecoder.sync == VID_SYNC_VID)
        vid_atom_enable_sync(1);
    else
        vid_atom_enable_sync(0);
    return 0;
}

void vid_osi_disable_sync()
{
    vid_atom_disable_sync();
}


CLIPDEV_T vid_osi_get_clipdev()
{
    if(_videoDecoder.src != VID_SOURCE_MEMORY)
    {
        PDEBUG("init memory mode first\n");
        return NULL;
    }
    return _videoDecoder.clipdev;
}


INT vid_osi_clip_write(CLIPINFO *info)
{
    vid_atom_write_clip(info);
    return 0;
}

void vid_osi_set_pts(STC_T *pData)
{
    vid_atom_set_pts(pData);
}

void vid_osi_get_pts(STC_T *pData)
{
    //vid_atom_get_pts(pData);
    pData->bit0 = 0;
    pData->bit32_1 = 0;
    return;
}

void vid_osi_set_stc(STC_T *pData)
{
    vid_atom_set_stc(pData);
}

void vid_osi_get_stc(STC_T *pData)
{
    vid_atom_get_stc(pData);
}

int vid_osi_single_frame_complete()
{

    if(MF_DCR(VID_CHIP_CTRL) & DECOD_CHIP_CONTROL_SVD)
      return(-1);
    
    return 0;
}

INT vid_osi_enable_disp_14_9(int top_border)
{
    VIDEOCMD vc;
   
    //set the current letterbox top border value
    saved_LB_top_border = MF_DCR(VID_LBOX);
   
    if(top_border != -1)
       MT_DCR(VID_LBOX, top_border); //set the letterbox top border value

    //note: chip must be in letterbox mode
    if(vid_osi_set_dispmode(VID_DISPMODE_LETTERBOX)!=0) {
       printk("Failed to set LETTERBOX mode\n");
       return -1;
    }
    
    vc.uCmd = DECOD_COM_NO_PAN_SCAN;
    vc.uNum = 1;
    vc.uPara[0] = 0x1000; 
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;
    if (vid_atom_exec_cmd(&vc) != 0){
      PDEBUG("execute config command failed\n");
      return -1;
    }
  
    return 0;
}

INT vid_osi_disable_disp_14_9()
{
   VIDEOCMD vc;
    
    vc.uCmd = DECOD_COM_NO_PAN_SCAN;
    vc.uNum = 1;
    vc.uPara[0] = 0x0000; 
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;
    
    if (vid_atom_exec_cmd(&vc) != 0){
      PDEBUG("execute config command failed\n");
      return -1;
    }
  
    if(vid_osi_set_dispmode(VID_DISPMODE_NORM)!=0) {
       printk("Failed to disable LETTERBOX mode\n");
       return -1;
    }
  
    // restore the top border value back to what it originally was  
    MT_DCR(VID_LBOX, saved_LB_top_border); 
        
    return 0;
}

