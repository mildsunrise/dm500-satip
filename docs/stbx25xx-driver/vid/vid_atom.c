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
| Author:  Ling shao
| File:    vid_atom.c
| Purpose: video decoder atom layer.
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01     create
| 25-Jul-03     Added code to allow for more flexible memory map
+----------------------------------------------------------------------------*/

#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/module.h>       /* Specifically, a module */
#include <linux/version.h>

#include <os/os-io.h>
#include <os/drv_debug.h>
#include <os/os-sync.h>
#include <hw/hardware.h>
#include "vid_atom_local.h"
#include "vid_atom_hw.h"
#include "vid_uc.h"
#include "vid_osi.h"
#include <hw/physical-mem.h>
#include <os/pm-alloc.h>

#ifdef __DRV_FOR_VULCAN__
/*
#define OS_CORE_DRIVER_NAME   "STBx25xx OS-Core"
#ifdef MODULE
MODULE_DESCRIPTION("OS Core support driver for IBM STBx25xx Drivers");
#endif
*/
#define DCR_VID_MEMSEG0     0x175
#define DCR_VID_MEMSEG1     0x176
#define DCR_VID_MEMSEG2     0x177
#define DCR_VID_MEMSEG3     0x178


#elif defined(__DRV_FOR_PALLAS__)
/*
#define OS_CORE_DRIVER_NAME   "STB04xxx OS-Core"
#ifdef MODULE
MODULE_DESCRIPTION("OS Core support driver for IBM STB04xxx Drivers");
#endif
*/
#define DCR_VID_MEMSEG0     0x175
#define DCR_VID_MEMSEG1     0x176
#define DCR_VID_MEMSEG2     0x177
#define DCR_VID_MEMSEG3     0x178

#elif defined(__DRV_FOR_VESTA__)
/*
#define OS_CORE_DRIVER_NAME   "STB03xxx OS-Core"
#ifdef MODULE
MODULE_DESCRIPTION("OS Core support driver for IBM STB04xxx Drivers");
#endif
*/
#define DCR_VID_MEMSEG0     0x175
#define DCR_VID_MEMSEG1     0x176
#define DCR_VID_MEMSEG2     0x177
#define DCR_VID_MEMSEG3     0x178

#else   // why


#error "Unsupported architecture, please specify it in 'include/hw/hardware.h'"

#endif

#define  DEF_VID_IRQ_MASK   DECOD_HOST_MASK_BLOCK_READ | DECOD_HOST_MASK_CHAN_CHAN | DECOD_HOST_MASK_SERROR | \
                            DECOD_HOST_MASK_PS_STATUS  | DECOD_HOST_MASK_IPDC
		   
extern vidsfm_t vid_sfm;
extern UINT    _fmt;

static int vid_mask_save=0;

int      _initial_cc;
volatile int __FF_PENDING = 0;
volatile int __FF_COMPLETE = 0;
volatile int __PS_SF_COMPLETE;

ULONG guFBVideoOffset;   //this is set to the start of the frame buffer memory within the logical video memory.
ULONG guUserVideoOffset; //this is set to the start of the user data memory within the logical video memory.
ULONG guVBI0VideoOffset; //this is set to the start of the vbi0 data memory within the logical video memory.
ULONG guVBI1VideoOffset; //this is set to the start of the vbi1 data memory within the logical video memory.
ULONG guRBVideoOffset;   //this is set to the start of the rate buffer data memory within the logical video memory.
ULONG guClipVideoOffset; //this is set to the start of the clip buffer data memory within the logical video memory.
ULONG guGraphicsVideoOffset;

EXPORT_SYMBOL_NOVERS(vid_atom_set_irq_mask);
EXPORT_SYMBOL_NOVERS(vid_atom_get_irq_mask);
EXPORT_SYMBOL_NOVERS(guGraphicsVideoOffset);


static void __vid_init_segment_reg(int i, UINT32 size, UINT32 addr)
{
    UINT32  uVal=0;
    PDEBUG("__vid_init_segment_reg: segment = 0x%8.8x size =0x%8.8x address = 0x%8.8x \n",i,size,addr);
    switch(i&3)
    {
    case 0:
        uVal = MF_DCR(DCR_VID_MEMSEG0);
        break;
    case 1:
        uVal = MF_DCR(DCR_VID_MEMSEG1);
        break;
    case 2:
        uVal = MF_DCR(DCR_VID_MEMSEG2);
        break;
    case 3:
        uVal = MF_DCR(DCR_VID_MEMSEG3);
        break;
    }
    if(i&4)     // 0:15
    {
       uVal = (uVal & 0x0000ffff) | (size << 28) | ((addr&0xfff00000) >> 4);
    }
    else    // 16:31
    {
       uVal = (uVal & 0xffff0000) | (size << 12) | ((addr&0xfff00000) >> 20);
    }

    switch(i&3)
    {
    case 0:
        PDEBUG("\nDCR_VID_MEMSEG0 is being set to 0x%8.8x\n",uVal);
        MT_DCR(DCR_VID_MEMSEG0, uVal);
        break;
    case 1:
        PDEBUG("\nDCR_VID_MEMSEG1 is being set to 0x%8.8x\n",uVal);
        MT_DCR(DCR_VID_MEMSEG1, uVal);
        break;
    case 2:
        PDEBUG("\nDCR_VID_MEMSEG2 is being set to 0x%8.8x\n",uVal);
        MT_DCR(DCR_VID_MEMSEG2, uVal);
        break;
    case 3:
        PDEBUG("\nDCR_VID_MEMSEG3 is being set to 0x%8.8x\n",uVal);
        MT_DCR(DCR_VID_MEMSEG3, uVal);
        break;
    }
}
//initialize screen display
INT vid_atom_init(UINT uDispDelay)
{
   unsigned long        addr;
   unsigned long        segreg;
   unsigned long        segsize;
   unsigned long        segnum;
   unsigned long        segaddr;
   int                  i;
   unsigned long        ulTempAddr;
   int                  iTempSize,iIncrementSize;
   int                  videoMemSize=0;

    /*-------------------------------------------------------------------------+
    | Reset video chip
    +--------------------------------------------------------------------------*/
    //reset
    /*reg = MF_DCR(CICVCR);
    reg |= 0x00000001;
    MT_DCR(CICVCR, reg);

    os_delay(1);
    reg &= 0xfffffffe;
    MT_DCR(CICVCR, reg);

    MT_DCR(VID_CHIP_MODE, 0);*/

    /*-------------------------------------------------------------------------+
    | Stop the decoder.
    +-------------------------------------------------------------------------*/
    MT_DCR(VID_CHIP_CTRL, 0);


   /*-------------------------------------------------------------------------+
   | Set up the decoder logical memory space as follows:
   |
   | Memory segment 0 is reserved for the video frame buffers. Physical memory
   | for this segment is defined by __STB_V_FB_MEM_BASE_ADDR which must located
   | on a 1 MB boundry .Segment 0 size is defined by __STB_V_FB_MEM_SIZE however
   | it is fixed at 2 MB.
   |
   | Memory segment 1 contains the video user data buffer, vbi buffer and rate
   | buffer. Physical memory for this segment is defined by __STB_V_MEM_BASE_ADDR
   | which must be located on a 128 byte boundry.   Segment 1 size is defined by
   | __STB_V_MEM_SIZE.
   |
   | Memory segment 2 is reserved for the Graphics plane OSD buffers. Physical
   | memory for this segment is defined by __STB_GRAPHICS_MEM_BASE_ADDR which
   | must be located on 128 byte boundry. Segment 2 size is determined by
   | __STB_GRAPHICS_MEM_SIZE.
   |
   |
   +-------------------------------------------------------------------------*/
   PDEBUG("__STB_V_FB_MEM_BASE_ADDR = 0x%8.8x\n",__STB_V_FB_MEM_BASE_ADDR);
   if(VID_FRAME_MEM_SIZE!=0x00200000)
   {
      printk("__STB_V_FB_MEM_SIZE is not correct\n");
      return -1;
   }

   segnum = 0;
   segaddr = 0;
   guFBVideoOffset = segaddr;
   segsize = 1;                   /* 2 MB */
   addr = VID_FRAME_MEM_BASE;     /* frame buffer physical address */
   videoMemSize+=1<<segsize;

   if((addr%0x00100000)!= 0) {
     printk("ERROR: __STB_V_FB_MEM_BASE_ADDR must start on 1 MB boundary\n");
     return -1;
   }
   segreg=(segsize<<12)|((addr&0xFFF00000)>>20);
   PDEBUG("\nDCR_VID_MEMSEG0 is being set to 0x%8.8x for frame buffer. segment size = %dx\n",segreg, segsize);

   __vid_init_segment_reg(segnum, segsize,addr);

   segnum++;
   segaddr += 0x00100000<<segsize;

   PDEBUG("__STB_V_MEM_BASE_ADDR = 0x%8.8x\n",__STB_V_MEM_BASE_ADDR);
   addr = VID_MEM_BASE;
   if((addr%DECOD_MEM_ALIGN)!= 0) {
     printk("ERROR: __STB_V_MEM_BASE_ADDR  must start on 128 byte boundary\n");
     return -1;
   }

   guUserVideoOffset = segaddr + (addr&0x000FFFFF);
   PDEBUG("guUserVideoOffset = 0x%08x\n", guUserVideoOffset);

   guVBI0VideoOffset = guUserVideoOffset+VID_USER_MEM_SIZE;
   PDEBUG("guVBI0VideoOffset = 0x%08x\n", guVBI0VideoOffset);

   guVBI1VideoOffset = guVBI0VideoOffset+VID_VBI0_MEM_SIZE;
   PDEBUG("guVBI1VideoOffset = 0x%08x\n", guVBI1VideoOffset);

   guRBVideoOffset = guVBI1VideoOffset+VID_VBI1_MEM_SIZE;
   PDEBUG("guRBVideoOffset = 0x%08x\n", guRBVideoOffset);

/* clip buffer start and end must be on 4k boundries */
   guClipVideoOffset = ((guRBVideoOffset + 0x1000-1)/0x1000)*0x1000;
   PDEBUG("guClipVideoOffset = 0x%08x\n", guClipVideoOffset);

   i = ((addr&0x000FFFFF)+ VID_MEM_SIZE-1)>>20;
   PDEBUG("VID_MEM_SIZE = 0x%8.8x\n",VID_MEM_SIZE);
   PDEBUG("i = 0x%8.8x\n",i);

   for(segsize = 0; i != 0; segsize++) {
       i = i>>1;
   }
   if(segsize>4) {
      printk("ERROR: VID_MEM_SIZE must be less than 16MB\n");
     return -1;
   }
   videoMemSize+=1<<segsize;
   segreg=(segsize<<12)|((addr&0xFFF00000)>>20);
   PDEBUG("\nDCR_VID_MEMSEG1 is being set to 0x%8.8x for video buffers. segment size = %d\n",segreg, segsize);
   __vid_init_segment_reg(segnum, segsize,addr);

   PDEBUG("__STB_GRAPHICS_MEM_BASE_ADDR = 0x%8.8x __STB_GRAPHICS_MEM_SIZE = 0x%8.8x\n", __STB_GRAPHICS_MEM_BASE_ADDR,__STB_GRAPHICS_MEM_SIZE);

   if(__STB_GRAPHICS_MEM_SIZE!=0)
   {

      if(__STB_GRAPHICS_MEM_SIZE > 0x03700000) {
         printk("ERROR: __STB_GRAPHICS_MEM_SIZE  must be 55 Megabytes or less\n");
        return -1;
      }

     ulTempAddr = __STB_GRAPHICS_MEM_BASE_ADDR;
     iTempSize = __STB_GRAPHICS_MEM_SIZE;
     if((ulTempAddr%DECOD_MEM_ALIGN)!= 0) {
        printk("ERROR: __STB_GRAPHICS_MEM_BASE_ADDR  must start on 128 byte boundary\n");
        return -1;
      }
     PDEBUG("segaddr = 0x%8.8x segsize= 0x%8.8x    0x00100000<<segsize = 0x%8.8x \n",segaddr,segsize,(0x00100000<<segsize));
     guGraphicsVideoOffset = (segaddr + (0x00100000<<segsize))+(ulTempAddr&0x000fffff);
     PDEBUG("guGraphicsVideoOffset = 0x%8.8x\n",guGraphicsVideoOffset);

      while((iTempSize > 0) && (segnum < 7))
      {
         segnum++;
         segaddr += 0x00100000<<segsize;
         addr = ulTempAddr;
         PDEBUG("segnum = %d, segaddr = 0x%8.8x, addr = 0x%8.8x\n",segnum,segaddr,addr);

         if(iTempSize > 0x01000000)
            iIncrementSize = 0x01000000;
         else
            iIncrementSize = iTempSize;

         PDEBUG("iTempSize = 0x%8.8x\n",iTempSize);
         PDEBUG("iIncrementSize = 0x%8.8x\n",iIncrementSize);

         i = ((addr & 0x000FFFFF)+iIncrementSize-1)>>20;
         PDEBUG("i = 0x%8.8x\n",i);

         for(segsize = 0; i != 0; segsize++) {
            i = i>>1;
         }

         if(segsize>4) {  //max segment size is 4 (16MB)
            segsize=4;
         }
         videoMemSize+=1<<segsize;   
         PDEBUG("segsize = %d, increment size = 0x%8.8x\n",segsize,iIncrementSize);
         segreg=(segsize<<12)|((addr&0xFFF00000)>>20);
         PDEBUG("\nsegnum %d is being set to 0x%8.8x for graphics plane. segment size = %d\n",segnum, segreg, segsize);
          __vid_init_segment_reg(segnum, segsize,addr);

         iTempSize-=iIncrementSize;
         ulTempAddr+=iIncrementSize;
      }

      if(iTempSize > 0)
         printk("vid_atom_init: unable to allocate all the memory in the video segments\n");

    }
   else { // graphics memory come from the allocated physcial memory
     PDEBUG("__STB_ALLOC_MEM_BASE_ADDR = 0x%8.8x\n", ulTempAddr);

     if(__STB_ALLOC_MEM_SIZE > 0x03700000) {
         printk("ERROR: __STB_ALLOC_MEM_SIZE  must be 55 Megabytes or less\n");
        return -1;
      }

      ulTempAddr = __STB_ALLOC_MEM_BASE_ADDR;
      iTempSize = __STB_ALLOC_MEM_SIZE;
      if((ulTempAddr%DECOD_MEM_ALIGN)!= 0) {
        printk("ERROR: __STB_ALLOC_MEM_BASE_ADDR  must start on 128 byte boundary\n");
        return -1;
      }
      PDEBUG("segaddr = 0x%8.8x segsize= 0x%8.8x\n",segaddr,segsize);
      guGraphicsVideoOffset = (segaddr + (0x00100000<<segsize))+(ulTempAddr&0x000fffff);
      PDEBUG("guGraphicsVideoOffset = 0x%8.8x\n",guGraphicsVideoOffset);

    
      while((iTempSize > 0) && (segnum < 7))
      {
         segnum++;
         segaddr += 0x00100000<<segsize;
         addr = ulTempAddr;
         PDEBUG("segnum = %d, segaddr = 0x%8.8x, addr = 0x%8.8x\n",segnum,segaddr,addr);

         if(iTempSize > 0x01000000)
            iIncrementSize = 0x01000000;
         else
            iIncrementSize = iTempSize;

         PDEBUG("iTempSize = 0x%8.8x\n",iTempSize);
         PDEBUG("iIncrementSize = 0x%8.8x\n",iIncrementSize);
         PDEBUG("addr & 0x000FFFFF = 0x%8.8x\n",(addr & 0x000FFFFF));

         i = ((addr & 0x000FFFFF)+iIncrementSize-1)>>20;
         PDEBUG("i = 0x%8.8x\n",i);

         for(segsize = 0; i != 0; segsize++) {
            i = i>>1;
         }

         if(segsize>4){ //max segment siz is 4 (16MB)
            segsize=4;
         }
         videoMemSize+=1<<segsize;   

         PDEBUG("segsize = %d, increment size = 0x%8.8x\n",segsize,iIncrementSize);
         segreg=(segsize<<12)|((addr&0xFFF00000)>>20);
         PDEBUG("\nsegnum %d is being set to 0x%8.8x for graphics plane. segment size = %d\n",segnum, segreg, segsize);
         __vid_init_segment_reg(segnum, segsize,addr);

         iTempSize-=iIncrementSize;
         ulTempAddr+=iIncrementSize;
      }

      if(iTempSize > 0)
         printk("vid_atom_init: unable to allocate all the memory in the video segments\n");
   }
    /*-------------------------------------------------------------------------+
    | Set display control for errta
    +--------------------------------------------------------------------------*/
    //reg = MF_DCR(VID_DISP_CNTL);
    //MT_DCR(VID_DISP_CNTL, reg | 0x000000C0);

    /*-------------------------------------------------------------------------+
    | Define top and left borders, and display delay.  Set top border in the
    | letterbox mode (number of lines from the top of the display).
    +-------------------------------------------------------------------------*/
    MT_DCR(VID_DISP_BOR, 0);
    MT_DCR(VID_LBOX, 0);
    /*-------------------------------------------------------------------------+
    | Set display mode.  Right now the external and internal DENC both use the
    | leading sync mode.
    +-------------------------------------------------------------------------*/
    MT_DCR(VID_DISP_MODE,
           DECOD_DISP_MODE_BK_BLACK |
           DECOD_DISP_MODE_TRANS_POL_LOW |
           DECOD_DISP_MODE_SYNC_LEADING);
    /*-------------------------------------------------------------------------+
    | Set PTS control.  PTS filter set to 728ms (90Khz clock), or 621ms (27Mhz
    | clock (32* 22.76ms, or 32* 19.42ms).
    +-------------------------------------------------------------------------*/
    MT_DCR(VID_PTS_CTRL, 32);
    /*-------------------------------------------------------------------------+
    | Set display delay for denc
    +-------------------------------------------------------------------------*/
    MT_DCR(VID_DISP_DLY, uDispDelay);

    return 0;
}


/*----------------------------------------------------------------------------+
| Config_video_modes.
+----------------------------------------------------------------------------*/
INT vid_atom_config_vdec(VDEC_CON *pVdec)
{
    unsigned long reg;
    VIDEOCMD vc;

    /*-------------------------------------------------------------------------+
    | Load video microcode.  This is required for the rest of the
    | initialization.
    +-------------------------------------------------------------------------*/
    if( vid_atom_load_microcode((USHORT*)vid_ucode, vid_ucode_len/2) != 0)
    {
        PDEBUG("load microcode failed\n");
        return (-1);
    }
    PDEBUG("load microcode OK\n");
    /*-------------------------------------------------------------------------+
    | Start video processor
    |
    +-------------------------------------------------------------------------*/
    reg = DECOD_CHIP_CONTROL_SVP | DECOD_CHIP_CONTROL_DIS_SYNC;
#ifdef VID_BLANK_ENABLE
    reg |= DECOD_CHIP_CONTROL_BLANK_VID;
#endif
    MT_DCR(VID_CHIP_CTRL, reg);

    /*-------------------------------------------------------------------------
    | Set user data base
    +--------------------------------------------------------------------------*/
    pVdec->user.ulVideoLogicalAddr=guUserVideoOffset;
//    MT_DCR(VID_USERDATA_BASE, pVdec->user.uAddr / DECOD_MEM_ALIGN);
    MT_DCR(VID_USERDATA_BASE,pVdec->user.ulVideoLogicalAddr / DECOD_MEM_ALIGN);
    PDEBUG("user data video logical addr = 0x%08x setting user base = 0x%08x\n", pVdec->user.ulVideoLogicalAddr, pVdec->user.ulVideoLogicalAddr / DECOD_MEM_ALIGN);

    /*-------------------------------------------------------------------------
    | Set VBI0 and VBI1 data base
    +--------------------------------------------------------------------------*/
    if (VID_VBI_LINES > 0) {
       reg  = (((guVBI0VideoOffset / DECOD_MEM_ALIGN)<<16) & 0x7FFF0000);
       reg |= ((guVBI1VideoOffset  / DECOD_MEM_ALIGN)      & 0x00007FFF);
       MT_DCR(VID_VBI_BASE, reg);

       PDEBUG("Setting VBI0 base = 0x%08x\n", guVBI0VideoOffset / DECOD_MEM_ALIGN);
       PDEBUG("Setting VBI1 base = 0x%08x\n", guVBI1VideoOffset / DECOD_MEM_ALIGN);
       PDEBUG("Setting number VBI lines = 0x%08x\n", VID_VBI_LINES);
    }

    /*-------------------------------------------------------------------------
    | Set rate buffer base and size
    +--------------------------------------------------------------------------*/
    pVdec->ratebuf.ulVideoLogicalAddr=guRBVideoOffset;
//    MT_DCR(VID_RB_BASE, pVdec->ratebuf.uAddr / DECOD_MEM_ALIGN);
    MT_DCR(VID_RB_BASE, pVdec->ratebuf.ulVideoLogicalAddr / DECOD_MEM_ALIGN);
    MT_DCR(VID_RB_SIZE, pVdec->ratebuf.uLen / DECOD_RB_ALIGN);
    PDEBUG("ratebuf video logical base addr = 0x%08x setting rate base reg to 0x%08x\n",   pVdec->ratebuf.ulVideoLogicalAddr,  pVdec->ratebuf.ulVideoLogicalAddr / DECOD_MEM_ALIGN);
    PDEBUG("ratebuf size =0x%08x setting  rate size reg to 0x%08x\n", pVdec->ratebuf.uLen, pVdec->ratebuf.uLen / DECOD_RB_ALIGN);

    /*-------------------------------------------------------------------------
    | Set frame buffer base
    +--------------------------------------------------------------------------*/
    if(_fmt == DENC_MODE_NTSC)
    {
       pVdec->framebuf.ulVideoLogicalAddr=guFBVideoOffset+0x21d00; // use 0x21d00 for NTSC on Vulcan
    }
    else
    {
       pVdec->framebuf.ulVideoLogicalAddr=guFBVideoOffset+0x16900; // use 0x16900 for PAL on Vulcan
    }

    reg = MF_DCR(VID_FRAME_BUF) & (~0x00007FFF);
//    reg |= ((pVdec->framebuf.uAddr / DECOD_MEM_ALIGN) & 0x00007FFF);
    reg |= ((pVdec->framebuf.ulVideoLogicalAddr / DECOD_MEM_ALIGN) & 0x00007FFF);
    MT_DCR(VID_FRAME_BUF, reg);
    PDEBUG("framebuf.uAddr = 0x%08x\n", pVdec->framebuf.uAddr);
    PDEBUG("framebuf.uLen = 0x%08x\n", pVdec->framebuf.uLen);
    PDEBUG("framebuf.ulLogicalAddr = 0x%08x\n", pVdec->framebuf.ulLogicalAddr);
    PDEBUG("framebuf.ulVideoLogicalAddr = 0x%08x\n", pVdec->framebuf.ulVideoLogicalAddr);



    PDEBUG("framebuf video logical base addr = 0x%08x setting framebuf base reg to 0x%08x\n", pVdec->framebuf.ulVideoLogicalAddr,pVdec->framebuf.ulVideoLogicalAddr / DECOD_MEM_ALIGN);

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

#ifdef __DRV_FOR_PALLAS__
    /*-------------------------------------------------------------------------+
    | Execute 'set display regs' command
    +-------------------------------------------------------------------------*/
    vc.uCmd =0x20;
    vc.uNum = 0;
    vc.uPara[0] = 0;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("set display regs command failed\n");
        return -1;
    }
#endif
     _initial_cc = 1;
    PDEBUG("config video mode OK\n");
    return (0);
}


/*-------------------------------------------------------------------
| MPEG video play
+-------------------------------------------------------------------*/
INT vid_atom_play()
{
    unsigned long reg;
    VIDEOCMD vc;

    // normal play

    reg = MF_DCR(VID_CHIP_CTRL);
#ifdef VID_BLANK_ENABLE
    reg |= DECOD_CHIP_CONTROL_BLANK_VID;
#endif

    vc.uCmd = DECOD_COM_PLAY;
    vc.uNum = 0;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("exec video play error\n");
        return -1;
    }

    reg |= DECOD_CHIP_CONTROL_SVP | DECOD_CHIP_CONTROL_SVD;
    MT_DCR(VID_CHIP_CTRL, reg);

//sync problem
/*    if (get_vid_sync() != 0)
        DEMUX_PCR_CALLBACK(XP_CALLBACK_RUN);
*/
    //vid_atom_enable_sync();
    _initial_cc = 0;
    return (0);
}


void vid_atom_stop()
{
    unsigned long reg;

    vid_atom_disable_sync();

    reg = MF_DCR(VID_CHIP_CTRL) & (~(DECOD_CHIP_CONTROL_SVD)) ;
    MT_DCR(VID_CHIP_CTRL, reg);
}


/*WARNING: This function may result in re-schedule*/
INT vid_atom_ps_wait()
{
    int i = 0;

    while (1)
    {
        if (++i >= 20)
        {
            PDEBUG("PAUSE wait overtime\n");
            return (-1);
        }

        if (__PS_SF_COMPLETE != 0)
            return 0;

        os_sleep(1);
    }
    return (0);
}


INT vid_atom_pause()
{
    VIDEOCMD vc;

    PDEBUG("video paused\n");
    vid_atom_disable_sync();

    // set display to first field only
    vid_atom_set_sfm(VID_SFM_FIRST_ONLY);

    __PS_SF_COMPLETE = 0;
    vc.uCmd = DECOD_COM_PAUSE;
    vc.uNum = 0;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("exec video freeze frame error\n");
        return -1;
    }

    if(vid_atom_ps_wait() != 0)
        return -1;

    return 0;
}

/*WARNING: This function may result in re-schedule*/
INT vid_atom_ff_wait()
{
    int i = 0;
    int rc = 0;

    __FF_COMPLETE = 0;
    __FF_PENDING = 1;

    vid_atom_set_irq_mask(vid_atom_get_irq_mask() | DECOD_HOST_MASK_PSTART);
    while (1)
    {
        if (++i >= 20)
        {
            PDEBUG("FFRAME wait overtime\n");
            rc = -1;
            break;
        }

        if (__FF_COMPLETE != 0)
            break;

        os_sleep(1);
    }
    __FF_PENDING = 0;
    __FF_COMPLETE = 0;
    return (rc);
}

//video freeze only freeze screen but decoder keep on running
INT vid_atom_freeze()
{
    VIDEOCMD vc;

    vid_atom_disable_sync();

    // set display to first field only
    vid_atom_set_sfm(VID_SFM_FIRST_ONLY); 

    vc.uCmd = DECOD_COM_FFRAME;
    vc.uNum = 1;
    vc.uPara[0] = 0;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        PDEBUG("exec video freeze frame error\n");
        return -1;
    }

    if( vid_atom_ff_wait() != 0 )
    {
        return -1;
    }

    return 0;
}


void vid_atom_close()
{
    unsigned long reg;

    reg =
        MF_DCR(VID_CHIP_CTRL) &
        (~
         (DECOD_CHIP_CONTROL_SVP | 
          DECOD_CHIP_CONTROL_SVD |
          DECOD_CHIP_CONTROL_BLANK_VID));

    MT_DCR(VID_CHIP_CTRL, reg);
}


void vid_atom_set_dispmode(UINT uMode)
{
    unsigned long reg;

    reg = MF_DCR(VID_DISP_MODE) & 
          (~(
            DECOD_DISP_MODE_MASK|
            DECOD_DISP_MODE_16_9_MONITOR));
    //NTSC or PAL
    /*if(uMode & VID_MODE_PAL)
        reg |= DECOD_DISP_MODE_PAL_MODE;*/
    //4:3 or 16:9
    if(uMode & VID_MODE_16_9)
        reg |= DECOD_DISP_MODE_16_9_MONITOR;
    //display mode
    reg |= (uMode & 0x0f) << 1;
    MT_DCR(VID_DISP_MODE, reg);
    PDEBUG("set disp mode = 0x%lx\n", reg);
}

void vid_atom_set_dispfmt(UINT uPal)
{
    unsigned long reg;

    reg = MF_DCR(VID_DISP_MODE) & (~DECOD_DISP_MODE_PAL_MODE);
    if(uPal)
    {
        MT_DCR(VID_DISP_MODE, reg | DECOD_DISP_MODE_PAL_MODE);
        //get frame info in PAL mode
        vid_atom_set_fb_adr(1);
    }
    else
    {
        MT_DCR(VID_DISP_MODE, reg);
        //get frame info in NTSC mode
        vid_atom_set_fb_adr(0);
    }
    PDEBUG("set dispfmt = 0x%lx\n", reg);
}

void vid_atom_set_sfm(vidsfm_t sfm)
{
  unsigned reg;

  if(vid_sfm != VID_SFM_NORMAL)  /* if single field mode locked by the user */
    return;              /* ignore the request                      */
    
  reg = MF_DCR(VID_DISP_MODE) & ~DECOD_DISP_MODE_SFM_MASK;
  switch(sfm)
  {
    case VID_SFM_NORMAL:
        reg |= DECOD_DISP_MODE_NORM_DISP;
        break;
    case VID_SFM_BOTTOM_ONLY:
        reg |= DECOD_DISP_MODE_BOTTOM_ONLY;
        break;
    case VID_SFM_TOP_ONLY:
        reg |= DECOD_DISP_MODE_TOP_ONLY;
        break;
    case VID_SFM_FIRST_ONLY:
        reg |= DECOD_DISP_MODE_FIRST_ONLY;
        break;
  }

  MT_DCR(VID_DISP_MODE, reg);
  return;
}

void vid_atom_set_dispborder(UINT uLeft, UINT uTop)
{
    unsigned long reg;    

    PDEBUG("set disp border left = %d, top = %d\n", uLeft, uTop);

    uLeft &= 0x03ff;
    uTop  &= 0x03ff;
    reg = MF_DCR(VID_DISP_BOR);
    reg = reg | uLeft << 16 | uTop;
    MT_DCR(VID_DISP_BOR, reg);
}


void vid_atom_set_scalepos(RECT *prectSrc, RECT *prectDes)
{
    int x, y;

    PDEBUG("src hori off = 0x%8.8x, src hori size = 0x%8.8x\n", prectSrc->hori_off, prectSrc->hori_size);
    PDEBUG("src veri off = 0x%8.8x, src veri size = 0x%8.8x\n", prectSrc->vert_off, prectSrc->vert_size);
    PDEBUG("des hori off = 0x%8.8x, des hori size = 0x%8.8x\n", prectDes->hori_off, prectDes->hori_size);
    PDEBUG("des veri off = 0x%8.8x, des veri size = 0x%8.8x\n", prectDes->vert_off, prectDes->vert_size);

#ifdef __DRV_FOR_PALLAS__
    //CROP OFFSET
    //10bits horizontal offset to the center of the frame
    x =  (prectSrc->hori_off) & 0x03ff;     
    //10bits vertical offset to the center of the frame
    y =  (prectSrc->vert_off) & 0x03ff;     
    MT_DCR(VID_CROP_OFFSET, (x << 16) | y);

    //CROP SIZE
    x = (prectSrc->hori_size) & 0x03ff;
    y = (prectSrc->vert_size) & 0x03ff;
    MT_DCR(VID_CROP_SIZE, (x << 16) | y);
    
    //SCALE BORDER
    x = (prectDes->hori_off) & 0x03ff;     
    //10bits vertical offset to the center of the frame
    y = (prectDes->vert_off) & 0x03ff;     
    MT_DCR(VID_SCALE_BORDER, (x << 16) | y);
  
    //SCALE SIZE
    x = (prectDes->hori_size) & 0x03ff;     
    //10bits vertical offset to the center of the frame
    y = (prectDes->vert_size) & 0x03ff;     
    MT_DCR(VID_SCALE_SIZE, (x << 16) | y);
#else

	x = (prectDes->hori_off) & 0xff;
	y = (prectDes->vert_off) & 0xff;
	MT_DCR(VID_SMALL_BORDER, (x << 16) | y);
#endif
	return;
}


void vid_atom_scale_on(UINT uFlag)
{
#ifdef __DRV_FOR_PALLAS__
	unsigned long reg;
    reg = MF_DCR(VID_CROP_SIZE) & (~DECOD_CROP_SIZE_SS);

    if(uFlag)
    {
        reg |= DECOD_CROP_SIZE_SS;  //original size
    }
    MT_DCR(VID_CROP_SIZE, reg);

    reg = MF_DCR(VID_DISP_MODE) & (~DECOD_DISP_MODE_MASK);
    reg |= ( VID_MODE_SCALE << 1);
    MT_DCR(VID_DISP_MODE, reg);
#endif 
	return;
}

void vid_atom_scale_off()
{
    unsigned long reg;
    
    reg = MF_DCR(VID_DISP_MODE) & (~DECOD_DISP_MODE_MASK);
    MT_DCR(VID_DISP_MODE, reg);
}


UINT32  vid_atom_get_irq_mask()
{
    return MF_DCR(VID_MASK);
}

void vid_atom_set_irq_mask(UINT32 uMask)
{
    UINT32 flags;

    flags = os_enter_critical_section();
    vid_mask_save = uMask & DECOD_HOST_MASK_VBI_START;
    MT_DCR(VID_MASK, uMask);
    os_leave_critical_section(flags);
}

void vid_atom_init_irq_mask()
{
    UINT32 flags;
    
    flags = os_enter_critical_section();
    MT_DCR(VID_MASK, DEF_VID_IRQ_MASK | vid_mask_save);
    os_leave_critical_section(flags);
}


INT vid_atom_init_tv(AT_MEM *pRateBase, UINT uRateThreshold)
{
    unsigned long reg;

    if(pRateBase != NULL)
    {
        /*-------------------------------------------------------------------------
        | set rate buffer base and size
        +--------------------------------------------------------------------------*/
//        if((pRateBase->uAddr % DECOD_MEM_ALIGN) != 0)
        if((pRateBase->ulVideoLogicalAddr % DECOD_MEM_ALIGN) != 0)
            MT_DCR(VID_RB_BASE, pRateBase->uAddr / DECOD_MEM_ALIGN);
        if((pRateBase->uLen % DECOD_RB_ALIGN) != 0)
            MT_DCR(VID_RB_SIZE, pRateBase->uLen / DECOD_RB_ALIGN);
    }
    //rate buffer threshold to be done

    PDEBUG("start tv mode\n");

    reg = MF_DCR(VID_CHIP_CTRL);
    reg = reg & (~DECOD_CHIP_CONTROL_VID_CLIP);
    MT_DCR(VID_CHIP_CTRL, reg);

    return 0;
}


/*---------------------------------------------------------------------------+
| GET microcode version
+----------------------------------------------------------------------------*/
INT vid_atom_get_microcode_ver(ULONG *pVer)
{
   int   rc = 0;

   if (vid_ucode_len==1) {
     rc = -1;
   } else {
     *pVer = (ULONG)(vid_ucode[0]);
   }
   return(rc);
}


void vid_atom_blank()
{
    unsigned long reg;

    PDEBUG("video plane disable\n");
    reg = MF_DCR(VID_CHIP_CTRL);
    MT_DCR(VID_CHIP_CTRL, reg & (~DECOD_CHIP_CONTROL_BLANK_VID));

    return;
}

void vid_atom_show()
{
    unsigned long reg;

    PDEBUG("video plane enable\n");
    reg = MF_DCR(VID_CHIP_CTRL);
    MT_DCR(VID_CHIP_CTRL, reg | DECOD_CHIP_CONTROL_BLANK_VID);  /* enable vid */
    return;
}





void vid_atom_reg_dump()
{
    PDEBUG("CTRL = 0x%8x\n", MF_DCR(VID_CHIP_CTRL));
    PDEBUG("DISP MODE = 0x%8.8x\n", MF_DCR(VID_DISP_MODE));
    PDEBUG("DISP DLY = 0x%8.8x\n", MF_DCR(VID_DISP_DLY));
    PDEBUG("MEM SEG 0 = 0x%8.8x\n", MF_DCR(VID_SEG0));
    PDEBUG("MEM SEG 1 = 0x%8.8x\n", MF_DCR(VID_SEG1));
    PDEBUG("MEM SEG 2 = 0x%8.8x\n", MF_DCR(VID_SEG2));
    PDEBUG("MEM SEG 3 = 0x%8.8x\n", MF_DCR(VID_SEG3));
    PDEBUG("USER BUF = 0x%8.8x\n", MF_DCR(VID_USERDATA_BASE));
    PDEBUG("FRAME BUF = 0x%8.8x\n", MF_DCR(VID_FRAME_BUF));
    PDEBUG("RB BUF = 0x%8.8x\n", MF_DCR(VID_RB_BASE));
    PDEBUG("RB SIZE = 0x%8.8x\n", MF_DCR(VID_RB_SIZE));
}

//lingh added for PVR demo
void vid_atom_set_rb_size(ULONG value)
{
	MT_DCR(VID_RB_SIZE, value);
}
//lingh added for PVR demo
int vid_atom_single_frame(int mode)
{
    VIDEOCMD vc;
    int i;
    // set display to first field only
    vid_atom_set_sfm(VID_SFM_FIRST_ONLY); 
    
    vc.uCmd = DECOD_COM_SFRAME;
    vc.uNum = 1;
    vc.uPara[0] = mode;
    vc.uChained = 0;
    vc.uRetry = DECOD_TIMEOUT;

    if (vid_atom_exec_cmd(&vc) != 0)
    {
        return -1;
    }

    for(i = 0; i< 256; i++)
    {
        if(vid_atom_svd_has_reset() == 0)
	break;
		
	os_sleep(10);
    }

    if(i >= 256)
    {
	return -1;
    }

    return 0;
}

INT vid_atom_svd_has_reset()
{
	if((MF_DCR(VID_CHIP_CTRL) & 0x01) == 0)
		return 0;

	return -1;
}

int vid_atom_resume_from_sf(ULONG mode)
{
	int ret=0;

	if(mode == 0)
	{
		ret = vid_atom_reset_ratebuf(0xf000);
	}

	MT_DCR(VID_CHIP_CTRL, MF_DCR(VID_CHIP_CTRL) | 0x01);
	return ret;
}

INT vid_atom_force_frame_switch(ULONG initflag)
{
  VIDEOCMD vc;

  vc.uCmd = DECOD_COM_FRAME_SW;
  vc.uNum = 1;
  vc.uChained = 0;
  vc.uRetry = DECOD_TIMEOUT;
  vc.uPara[0] = initflag;

  if (vid_atom_exec_cmd(&vc) != 0)
  {
    return -1;
  }

  return(0);
}
