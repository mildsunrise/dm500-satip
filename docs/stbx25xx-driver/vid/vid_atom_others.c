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
| File:   vid_others.c
| Purpose: video decoder atom layer others PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
| 25-Jul-03     added code to support flexible frame buffer base addresses
+----------------------------------------------------------------------------*/
#include <os/os-generic.h>
#include <os/os-io.h>
#include <os/drv_debug.h>
#include <os/os-sync.h>
#include <hw/hardware.h>
#include "vid_atom_hw.h"
#include "vid_atom_local.h"
#include "vid_osi.h"


unsigned long buf0_lum, buf0_chr, buf1_lum, buf1_chr, buf2_lum, buf2_chr;
unsigned long buf0_lum_size, buf0_chr_size, buf1_lum_size, buf1_chr_size,
              buf2_lum_size, buf2_chr_size;

extern VDEC _videoDecoder;


inline ULONG vid_atom_make_memseg(MEM_SEG *pHigh, MEM_SEG *pLow)
{
    unsigned long reg;

    reg = ((pHigh->uSegSize & 0x00000007) << 28) |
          ((pHigh->uAddr & 0xFFF00000) >> 4)     |
          ((pLow->uSegSize & 0x00000007) << 12)  |
          ((pLow->uAddr & 0xFFF00000) >> 20);
    return reg;
}

/*-----------------------------------------------------------
| Microcode load and verify
+------------------------------------------------------------*/
INT vid_atom_load_microcode(USHORT *pCode, INT nCount)
{
    unsigned long reg;
    int rc;
    int i;

    PDEBUG("Loading video code\n");
    /*-------------------------------------------------------------------------+
    | Stop video processor.  Enable instruction store writes.
    +-------------------------------------------------------------------------*/
    reg =
        MF_DCR(VID_CHIP_CTRL) &
        (~(DECOD_CHIP_CONTROL_SVP | DECOD_CHIP_CONTROL_SVD));
    MT_DCR(VID_CHIP_CTRL, reg);
    MT_DCR(VID_WRT_PROT, DECOD_WR_PROT_DISABLE);
    /*-------------------------------------------------------------------------+
    | Initialize control store address.
    +-------------------------------------------------------------------------*/
    MT_DCR(VID_PROC_IADDR, 0x8000);
    /*-------------------------------------------------------------------------+
    | Load microcode.
    +-------------------------------------------------------------------------*/
    for (i = 0; i < nCount; i++)
    {
        MT_DCR(VID_PROC_IDATA, pCode[i]);
    }

    /*-------------------------------------------------------------------------+
    | Verify microcode load.
    +-------------------------------------------------------------------------*/
    rc = vid_atom_verify_microcode(pCode, nCount);

    return (rc);
}

/*-------------------------------------------------------------------------+
| Verify microcode load.
+-------------------------------------------------------------------------*/
INT vid_atom_verify_microcode(USHORT *pCode, INT nCount)
{
    int i;

    PDEBUG("Verifying video code\n");

    MT_DCR(VID_WRT_PROT, DECOD_WR_PROT_DISABLE);

    for (i = 0; i < nCount; i++)
    {
        MT_DCR(VID_PROC_IADDR, i);

        if (MF_DCR(VID_PROC_IDATA) != pCode[i])
            return -1;
    }

    MT_DCR(VID_PROC_IADDR, 0);
    MT_DCR(VID_WRT_PROT, DECOD_WR_PROT_ENABLE);
    return (0);
}


/*WARNING: This function may result in re-shedule*/
INT vid_atom_wait_cmd_done(UINT uRetry)
{
    unsigned long cmd_reg;

    while (1)
    {
        cmd_reg = MF_DCR(VID_CMD_STAT);

        if ((cmd_reg & DECOD_COMD_STAT_PENDING) == 0)
        {
            return (0);
        }
        else
        {            
            if (--uRetry == 0)
            {
                return (-1);
            }
            os_sleep(1);
            //PDEBUG("retry = %d\n", uRetry);
        }
    }
}

void vid_atom_set_cmd(VIDEOCMD *pCmd)
{
    int i;

    for (i = 0; i < pCmd->uNum; i++)
    {
        MT_DCR(VID_CMD_ADDR, i);
        MT_DCR(VID_CMD_DATA, pCmd->uPara[i]);
    }
}

INT vid_atom_exec_cmd(VIDEOCMD *pCmd)
{
    PDEBUG("exec cmd = %d\n", pCmd->uCmd);

    //PDEBUG("wait last cmd...\n");
    if (vid_atom_wait_cmd_done(pCmd->uRetry) != 0)
    {
        PDEBUG("OVERTIME\n");
        return (-1);
    }
    //PDEBUG("DONE\n");

    //check if this command has parameters
    if (pCmd->uNum != 0)
        vid_atom_set_cmd(pCmd);

    if (pCmd->uChained != 0)
    {
        MT_DCR(VID_CMD, pCmd->uCmd | DECOD_COMD_CHAIN);
    }
    else
    {
        MT_DCR(VID_CMD, pCmd->uCmd);
    }

    //activate command
    MT_DCR(VID_CMD_STAT, DECOD_COMD_STAT_PENDING);

    //PDEBUG("wait THIS cmd...\n");
    if (vid_atom_wait_cmd_done(pCmd->uRetry) != 0)
    {
        /*----------------------------------------------------------------------+
        | Time-out.  Force microcode to accept our command.  This is done
        | by writing 0x8200 to the instruction address register.  The
        | only command that can be accepted this way is reset video buffer
        | command.  Command chaining is set so that we can issue the original
        | command.
        +----------------------------------------------------------------------*/

        if ((pCmd->uCmd != DECOD_COM_RES_VID_BUF)
            || (pCmd->uChained != 0))
        {
            MT_DCR(VID_CMD, DECOD_COM_RES_VID_BUF | DECOD_COMD_CHAIN);
        }
        //PDEBUG("OVERTIME\n");
        //printk("soft reset\n");

        MT_DCR(VID_PROC_IADDR, 0x00008200);

        if (vid_atom_wait_cmd_done(pCmd->uRetry) != 0)
        {
            PDEBUG("OVERTIME\n");
            return (-1);
        }
        //PDEBUG("DONE\n");
        PDEBUG("Retry THIS cmd...\n");
        if (pCmd->uCmd != DECOD_COM_RES_VID_BUF)
        {
            if (pCmd->uChained != 0)
            {
                MT_DCR(VID_CMD, pCmd->uCmd | DECOD_COMD_CHAIN);
            }
            else
            {
                MT_DCR(VID_CMD, pCmd->uCmd);
            }

            MT_DCR(VID_CMD_STAT, DECOD_COMD_STAT_PENDING);

            if (vid_atom_wait_cmd_done(pCmd->uRetry) != 0)
            {
                PDEBUG("OVERTIME\n");
                return (-1);
            }
        }
    }
    PDEBUG("DONE\n");
    return (0);
}

void vid_atom_reset_vdec()
{
	PDEBUG("soft reset video decoder\n");

	MT_DCR(VID_CMD_STAT, 0);
    MT_DCR(VID_PROC_IADDR, 0x00008200);
}

INT vid_atom_reset_ratebuf(int mode)
{
	    VIDEOCMD vc;

		PDEBUG("reset rate buffer\n");
		vid_atom_reset_vdec();

        vc.uCmd = DECOD_COM_RES_VID_BUF;
        vc.uNum = 1;
//        vc.uPara[0] = 0;
//lingh added for PVR demo
		vc.uPara[0] = mode;
        vc.uChained = 0;
        vc.uRetry = DECOD_TIMEOUT;
		/*switch(mode)
		{
			case VID_RB_NORMAL_PLAY:	
				vc.uPara[0] = 0x8000;
				break;
			case VID_RB_SFM:
				vc.uPara[0] = 0x8010;
				break;
			case VID_RB_OTHER:
				vc.uPara[0] = 0xa000;
				break;
			default:
				vc.uPara[0] = 0x8000;
				break;
		}*/
        if (vid_atom_exec_cmd(&vc) != 0)
        {
            printk("execute config command failed\n");
            return -1;
        }

//        _OS_MEMSET((void*)GET_RB_BASE(_ulVidLogAdr), 0x0, VID_RB_MEM_SIZE);
		return 0;
}

INT vid_atom_reset_ratebuf_freeze()
{
  VIDEOCMD vc;
  PDEBUG("reset rate buffer\n");
  
  vid_atom_reset_vdec();
  vc.uCmd = DECOD_COM_RES_VID_BUF;
  vc.uNum = 1;
  vc.uPara[0] = 0;
  vc.uChained = 1;
  vc.uRetry = DECOD_TIMEOUT;
  if (vid_atom_exec_cmd(&vc) != 0)
  {
    printk("execute RES_VID_BUF command failed\n");
    return -1;
  }

  vc.uCmd = DECOD_COM_FFRAME;
  vc.uNum = 0;
  vc.uChained = 0;
  vc.uRetry = DECOD_TIMEOUT;
  if (vid_atom_exec_cmd(&vc) != 0)
  {
    PDEBUG("exec video freeze frame error\n");
    return -1;
  }

  return 0;
}

void vid_atom_clear_rb()
{
    _OS_MEMSET((void*)_videoDecoder.ratebuf.ulLogicalAddr, 0x0, VID_RB_MEM_SIZE);
}

void vid_atom_clear_framebuf()
{
   _OS_MEMSET((void*)_videoDecoder.framebuf.ulLogicalAddr + buf0_lum, 0x00, buf0_lum_size);
   _OS_MEMSET((void*)_videoDecoder.framebuf.ulLogicalAddr + buf1_lum, 0x00, buf1_lum_size);
   _OS_MEMSET((void*)_videoDecoder.framebuf.ulLogicalAddr + buf2_lum, 0x00, buf2_lum_size);
   _OS_MEMSET((void*)_videoDecoder.framebuf.ulLogicalAddr + buf0_chr, 0x80, buf0_chr_size);
   _OS_MEMSET((void*)_videoDecoder.framebuf.ulLogicalAddr + buf1_chr, 0x80, buf1_chr_size);
   _OS_MEMSET((void*)_videoDecoder.framebuf.ulLogicalAddr + buf2_chr, 0x80, buf2_chr_size);

}

void vid_atom_set_memseg(MEM_SEG *pMem)
{
    unsigned long reg;

    reg = vid_atom_make_memseg(&(pMem[4]), &(pMem[0]));
    MT_DCR(VID_SEG0, reg);

    reg = vid_atom_make_memseg(&(pMem[5]), &(pMem[1]));
    MT_DCR(VID_SEG1, reg);

    reg = vid_atom_make_memseg(&(pMem[6]), &(pMem[2]));
    MT_DCR(VID_SEG2, reg);

    reg = vid_atom_make_memseg(&(pMem[7]), &(pMem[3]));
    MT_DCR(VID_SEG3, reg);
}


void vid_atom_set_fb_adr(int mode)
{
   ULONG fb_base;


#ifdef __DRV_FOR_PALLAS__
    if(mode == 1)
       fb_base = 0x1780; // use 0x1780 for PAL
    else
       fb_base = 0xcb80; // use 0xcb80 for NTSC


    if(mode == 1)           //pal
    {
        buf2_lum = 0X0 + fb_base;
        buf2_chr = 0X65400 + fb_base;
        buf0_lum = 0X99480 + fb_base;
        buf0_chr = 0XFE880 + fb_base;
        buf1_lum = 0X131280 +fb_base;
        buf1_chr = 0X196680 + fb_base;
        //antiflicker_buf = 0X1C9080 + fb_base;
        buf2_lum_size   = 0X65400;
        buf2_chr_size   = 0X19500;
        buf0_lum_size   = 0X65400;
        buf0_chr_size   = 0X32A00;
        buf1_lum_size   = 0X65400;
        buf1_chr_size   = 0X32A00;
        //antiflicker_buf_size = DECOD_PAL_ANTIFLICKER_SIZE;
   }
   else
   {                                             /* NTSC mode         */
        buf2_lum = 0X0      ;
        buf2_chr = 0X54600  ;
        buf0_lum = 0X9EE80  ;
        buf0_chr = 0XF3480  ;
        buf1_lum = 0X11D780 ;
        buf1_chr = 0X171D80 ;
        //antiflicker_buf = 0X19C080 + fb_base;
        buf2_lum_size   = 0X54600;
        buf2_chr_size   = 0X2A300;
        buf0_lum_size   = 0X54600;
        buf0_chr_size   = 0X2A300;
        buf1_lum_size   = 0X54600;
        buf1_chr_size   = 0X2A300;
        //antiflicker_buf_size =  DECOD_NTSC_ANTIFLICKER_SIZE;
   }
#elif defined(__DRV_FOR_VESTA__) || defined(__DRV_FOR_VULCAN__)
   if(mode == 1)
       fb_base = 0x16900; // use 0x16900 for 4MB PAL
    else
       fb_base = 0x21D00; // use 0x21D00 for NTSC
       
   if(mode == 1)
   {
        buf2_lum = 0x0 + fb_base;
        buf2_chr = 0x65400 + fb_base;
	buf0_lum = 0x84E00 + fb_base;
        buf0_chr = 0xea200 + fb_base;
        buf1_lum = 0x11cc00 + fb_base;
        buf1_chr = 0x182000 + fb_base;
	buf2_lum_size = 0x65400;
        buf1_lum_size = 0x65400;
        buf0_lum_size = 0x65400;
        buf2_chr_size = 0x19E00;
        buf1_chr_size = 0x32a00;
        buf0_chr_size = 0x32a00;
   }
   else
   {
        buf2_lum = 0x0 + fb_base;
        buf2_chr = 0x54600 + fb_base;
        buf0_lum = 0x89d00 + fb_base;
        buf0_chr = 0xde300 + fb_base;
        buf1_lum = 0x108600 + fb_base;
        buf1_chr = 0x15cc00 + fb_base;
        buf2_lum_size = 0x54600;
        buf1_lum_size = 0x54600;
        buf0_lum_size = 0x54600;
        buf2_chr_size = 0x2a300;
        buf1_chr_size = 0x2a300;
        buf0_chr_size = 0x2a300;
   }
#else
#error "platform not supported"
#endif

}


