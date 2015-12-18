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
| File:   vid_clip.c
| Purpose: video decoder atom layer clip function PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#include <os/helper-queue.h>
#include <os/os-io.h>
#include <os/drv_debug.h>
#include "vid_atom_hw.h"
#include "vid_atom_local.h"
#include "vid_osi.h"

extern VDEC _videoDecoder;
extern unsigned long buf0_lum, buf0_chr, buf1_lum, buf1_chr;
extern ULONG guClipVideoOffset;

//Clip group:
inline INT vid_atom_buf_ready()
{
    return (MF_DCR(VID_VCLIP_LEN) & DECOD_VCLIP_BLOCK_VALID) ? 0 : 1;
}

INT vid_atom_init_clip(AT_MEM *pRateBase)
{
    ULONG reg;
    ULONG val;
    PDEBUG("vid_atom_init_clip: clip base = 0x%8.8lx, size = 0x%8.8lx\n", pRateBase->uAddr, pRateBase->uLen);

    pRateBase->ulVideoLogicalAddr =  guClipVideoOffset;
    
    if(pRateBase != NULL)
    {
        /*-------------------------------------------------------------------------
        | set rate buffer base and size
        +--------------------------------------------------------------------------*/
        if((pRateBase->ulVideoLogicalAddr % DECOD_MEM_ALIGN) == 0)
        {
            val = (pRateBase->ulVideoLogicalAddr / DECOD_MEM_ALIGN) & 0x7fff;
            PDEBUG("\nvid_atom_init_clip:pRateBase->ulVideoLogicalAddr = 0x%8.8x\n",pRateBase->ulVideoLogicalAddr); 
            MT_DCR(VID_RB_BASE, val);
            PDEBUG("\nvid_atom_init_clip: clip base address = 0x%8.8lx\n", val);
        }
        else
        {
            PDEBUG("clip buffer base not 128 aligned\n");
            return -1;
        }
        if((pRateBase->uLen % DECOD_RB_ALIGN) == 0)
        {
            val = (pRateBase->uLen / DECOD_RB_ALIGN) & 0xffff;
            MT_DCR(VID_RB_SIZE, val);
            PDEBUG("\nvid_atom_init_clip: clip base size = 0x%8.8lx\n", val);
        }
        else
        {
            PDEBUG("clip buffer size not 32 aligned\n");
            return -1;
        }
    }


    MT_DCR(VID_VCLIP_LEN, 0x00000000);
    MT_DCR(VID_VCLIP_ADR, 0x00000000);

    reg = MF_DCR(VID_CHIP_CTRL);
    MT_DCR(VID_CHIP_CTRL, reg | DECOD_CHIP_CONTROL_VID_CLIP |
           DECOD_CHIP_CONTROL_SVP | DECOD_CHIP_CONTROL_SVD |
           DECOD_CHIP_CONTROL_BLANK_VID);

	if( vid_atom_reset_ratebuf(VID_RB_NORMAL_PLAY) != 0)
		return -1;
    
    return 0;
}		

void vid_atom_close_clip()
{
    ULONG reg;

    reg = MF_DCR(VID_CHIP_CTRL) & (~DECOD_CHIP_CONTROL_VID_CLIP);
    MT_DCR(VID_CHIP_CTRL, reg);
}

void vid_atom_write_clip(CLIPINFO *pInfo)
{
    unsigned long reg;

    PDEBUG("write clip start = 0x%8.8lx, len = 0x%8.8lx, flag = %d\n",
           pInfo->uClipAdrOff, pInfo->uClipLen, pInfo->uFlag);

    reg = pInfo->uClipLen & 0x1fffff;

    if(pInfo->uFlag)
        reg |= DECOD_VCLIP_BLOCK_VALID | DECOD_VCLIP_END_OF_STREAM;
    else
        reg |= DECOD_VCLIP_BLOCK_VALID;

    //write to video decoder
    MT_DCR(VID_VCLIP_ADR, pInfo->uClipAdrOff & 0x1fffff);  //at most 2M
    MT_DCR(VID_VCLIP_LEN, reg);
}


INT vid_atom_fast_forward(UINT uSpeed)
{
	VIDEOCMD vc;

    vc.uCmd = DECOD_COM_FF;
    vc.uNum = 1;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if(uSpeed == VID_FF_IPFRAME)
        vc.uPara[0] = 0x8000;
    else
        vc.uPara[0] = 0x0000;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("exec video freeze frame error\n");
        return -1;
    }
	return 0;
}

INT vid_atom_slow_motion(UINT uSpeed)
{
	VIDEOCMD vc;

    vc.uCmd = DECOD_COM_SLOWMO;
    vc.uNum = 1;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    vc.uPara[0] = uSpeed & 0x07;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("exec video freeze frame error\n");
        return -1;
    }
	return 0;
}

/*entering still picture mode, before that the decoder must be in clip mode*/
INT vid_atom_enter_stillp(UINT uStillpAdr)
{
    VIDEOCMD vc;
    UINT32 mask;
    ULONG reg;

    //printk("enter still picture mode\n");

      /*----------------------------------------------------------------------+
      | Freeze last displayed frame.
      +----------------------------------------------------------------------*/
      vid_atom_freeze();
      /*----------------------------------------------------------------------+
      | Start video decode must be on before this section of code will
      | execute correctly.
      +----------------------------------------------------------------------*/
      reg = MF_DCR(VID_CHIP_CTRL);
      MT_DCR(VID_CHIP_CTRL, reg | DECOD_CHIP_CONTROL_VID_CLIP |
           DECOD_CHIP_CONTROL_SVP | DECOD_CHIP_CONTROL_SVD);
           
      /*----------------------------------------------------------------------+
      | Reset frame buffers command.  Blank frame and rate buffers.

      +----------------------------------------------------------------------*/

      vid_atom_reset_ratebuf(VID_RB_NORMAL_PLAY);
      //vid_atom_clear_framebuf();

#if 0
    //reset video decoder
    //printk("reset vdec\n");
    vid_atom_reset_vdec();
    //vid_atom_reset_ratebuf(VID_RB_NORMAL_PLAY);

    //reset rate buffer
    //printk("reset rate buffer\n");
    vc.uCmd = DECOD_COM_RES_VID_BUF;
    vc.uNum = 0;
    vc.uPara[0] = 0;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;
    
    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("execute config command failed\n");
        return -1;
    }

    //start still picture mode
    //printk("move decoder buffer\n");
    vc.uCmd = DECOD_COM_MOV_DEC_BUF;
    vc.uNum = 1;
    vc.uPara[0] = uStillpAdr / 128;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("execute config command failed\n");
        return -1;
    }
#endif	
    //printk("start still picture\n");

    //start still picture mode
    vc.uCmd = DECOD_COM_STILL_P;
    vc.uNum = 1;
    vc.uPara[0] = 0x0000;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("execute config command failed\n");
        return -1;
    }

    //printk("enter stillp mode ok\n");

    //enable sequence end interrupt
    mask = vid_atom_get_irq_mask();
    vid_atom_set_irq_mask(mask | DECOD_HOST_MASK_SEND);

	return 0;
}



INT vid_atom_exit_stillp(void)
{
    VIDEOCMD vc;
    UINT32 mask;    

    //printk("exit still picture mode\n");

    //disable sequence end interrupt
    mask = vid_atom_get_irq_mask();
    vid_atom_set_irq_mask(mask & (~DECOD_HOST_MASK_SEND));

   /*-------------------------------------------------------------------------+
   | Unless block valid and end of stream is not signalled here the transition
   | to "live" vodeo will be very choppy.
   +-------------------------------------------------------------------------*/
   MT_DCR(VID_VCLIP_LEN, DECOD_VCLIP_BLOCK_VALID|DECOD_VCLIP_END_OF_STREAM);
   
   /*-------------------------------------------------------------------------+
   | Issue freaze frame command.
   +-------------------------------------------------------------------------*/
   /*vid_atom_reset_ratebuf(VID_RB_NORMAL_PLAY);

    vc.uCmd = DECOD_COM_FFRAME;
    vc.uNum = 0;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("exec video freeze frame error\n");
        return -1;
    }*/

   /*-------------------------------------------------------------------------+
   | Turn off clip mode.  Indicate that service picture mode is off.
   +-------------------------------------------------------------------------*/
    vid_atom_close_clip();

#if 0
    /*-------------------------------------------------------------------------+
    | Load configuration parameters and execute configuration command.
    +-------------------------------------------------------------------------*/
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
#endif 

    //Normal play 
    vc.uCmd = DECOD_COM_PLAY;
    vc.uNum = 0;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("exec video play error\n");
        return -1;
    }
    return 0;
}


ULONG vid_atom_get_stillp_buf(int color)
{
    int flag;

    flag = (*(unsigned short*)(_videoDecoder.user.ulLogicalAddr + 0x108));
    
    flag = flag & 0x1;

    if(flag)
    {
        //frame 1
        if(color == 0)
        {
        //LUM
              return  _videoDecoder.framebuf.ulLogicalAddr + buf1_lum;
        }
        else
        {
        //CHR
              return _videoDecoder.framebuf.ulLogicalAddr + buf1_chr;   
        }
    }
    else
    {
        if(color == 0)
        {
        //LUM
            return _videoDecoder.framebuf.ulLogicalAddr + buf0_lum;
        }
        else
        {
        //CHR
            return _videoDecoder.framebuf.ulLogicalAddr + buf0_chr;
        }
    }
}
