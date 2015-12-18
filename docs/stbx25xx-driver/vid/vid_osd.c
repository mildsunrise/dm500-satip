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
| File:   vid_osd.c
| Purpose: video decoder osd layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01             create                                                                                          SL
+----------------------------------------------------------------------------*/
#include <linux/kernel.h>       /* We're doing kernel work */
#include <os/os-sync.h>

#include <linux/mm.h>           /* for verify_area */
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>        /* for get_user, copy_from_user */
#include <linux/module.h>       /* Specifically, a module */
#include <linux/version.h>
#include <linux/string.h>

#include <os/drv_debug.h>
#include <os/os-io.h>
#include <hw/physical-mem.h>
#include <hw/hardware.h>
#include "vid_atom_hw.h"
#include "vid_osi.h"
#include "vid_osd.h"



VDEC _videoDecoder;
UINT _uStreamEnd = 0;

static DECLARE_WAIT_QUEUE_HEAD(vid_cc_WaitQ);
volatile int __CC_PENDING = 0;
volatile int __CC_COMPLETE = 0;

EXPORT_SYMBOL_NOVERS(vid_osd_map_vbi_laddr);
EXPORT_SYMBOL_NOVERS(vid_osd_unmap_vbi_laddr);


INT   vid_osd_init()
{
    /*-------------------------------------------------------------------------+
    |  Frame Buffer Memory
    +-------------------------------------------------------------------------*/
    _videoDecoder.framebuf.uAddr = VID_FRAME_MEM_BASE;
    _videoDecoder.framebuf.uLen = VID_FRAME_MEM_SIZE;
    _videoDecoder.framebuf.ulLogicalAddr = (unsigned long)ioremap(VID_FRAME_MEM_BASE, VID_FRAME_MEM_SIZE);
    if(_videoDecoder.framebuf.ulLogicalAddr == 0)
    {
        PDEBUG("ioremap frame buffer base error\n");
        return -1;
    }
    PDEBUG("ioremap frame buffer base: physical = 0x%8.8lx logical = 0x%8.8lx size = 0x%8.8lx \n",  _videoDecoder.framebuf.uAddr,_videoDecoder.framebuf.ulLogicalAddr, _videoDecoder.framebuf.uLen);

    /*-------------------------------------------------------------------------+
    |  Video User Memory
    +-------------------------------------------------------------------------*/
    _videoDecoder.user.uAddr = VID_USER_MEM_BASE;
    _videoDecoder.user.uLen = VID_USER_MEM_SIZE;
    _videoDecoder.user.ulLogicalAddr = (unsigned long)ioremap(VID_USER_MEM_BASE, VID_USER_MEM_SIZE);
    if(_videoDecoder.user.ulLogicalAddr == 0)
    {
        PDEBUG("ioremap user data base error\n");
        iounmap((void*)_videoDecoder.framebuf.ulLogicalAddr);
        return -1;
    }
    PDEBUG("ioremap user data base: physical = 0x%8.8lx logical = 0x%8.8lx size = 0x%8.8lx \n", _videoDecoder.user.uAddr, _videoDecoder.user.ulLogicalAddr, _videoDecoder.user.uLen);

    /*-------------------------------------------------------------------------+
    |  Video Rate Buffers
    +-------------------------------------------------------------------------*/
    _videoDecoder.ratebuf.uAddr = VID_RB_MEM_BASE;
    _videoDecoder.ratebuf.uLen = VID_RB_MEM_SIZE;
    _videoDecoder.ratebuf.ulLogicalAddr = (unsigned long)ioremap(VID_RB_MEM_BASE, VID_RB_MEM_SIZE);
    if( _videoDecoder.ratebuf.ulLogicalAddr == 0)
    {
        PDEBUG("ioremap rate buffer base error\n");
        iounmap((void*)_videoDecoder.framebuf.ulLogicalAddr);
        iounmap((void*)_videoDecoder.user.ulLogicalAddr);
        return -1;
    }
    PDEBUG("ioremap rate buffer base: physical = 0x%8.8lx logical = 0x%8.8lx size = 0x%8.8lx \n", _videoDecoder.ratebuf.uAddr,  _videoDecoder.ratebuf.ulLogicalAddr, _videoDecoder.ratebuf.uLen);

    /*-------------------------------------------------------------------------+
    |  Clip Buffer Memory
    +-------------------------------------------------------------------------*/
   //the clip buffer uses the ratebuffer memoey area however it must be 4k byte aligned
    _videoDecoder.clipbuf.uAddr =  ((_videoDecoder.ratebuf.uAddr + 0x1000-1)/0x1000)*0x1000;
    PDEBUG("vid_osd_init: _videoDecoder.clipbuf.uAddr = 0x%8.8x\n", _videoDecoder.clipbuf.uAddr);
    _videoDecoder.clipbuf.ulLogicalAddr =  _videoDecoder.ratebuf.ulLogicalAddr + (_videoDecoder.clipbuf.uAddr - _videoDecoder.ratebuf.uAddr);
    PDEBUG("vid_osd_init: _videoDecoder.clipbuf.ulLogicalAddr = 0x%8.8x\n", _videoDecoder.clipbuf.ulLogicalAddr);

    // make sure the clipbuf size is 4k aligned
     _videoDecoder.clipbuf.uLen = ((_videoDecoder.ratebuf.uAddr + _videoDecoder.ratebuf.uLen) -_videoDecoder.clipbuf.uAddr) & 0xfffff000;
//       _videoDecoder.clipbuf.uLen = 0x00100000;

   PDEBUG("vid_osd_init: _videoDecoder.clipbuf.uLen = 0x%8.8x\n", _videoDecoder.clipbuf.uLen);

   return vid_osi_init();

}

void   vid_osd_close()
{
    vid_osi_close();
    if(_videoDecoder.framebuf.ulLogicalAddr)
    {
       iounmap((void*)_videoDecoder.framebuf.ulLogicalAddr);
       _videoDecoder.framebuf.ulLogicalAddr = 0;
    }
    if(_videoDecoder.user.ulLogicalAddr)
    {
       iounmap((void*)_videoDecoder.user.ulLogicalAddr);
       _videoDecoder.user.ulLogicalAddr = 0;
    }

    if(_videoDecoder.ratebuf.ulLogicalAddr)
    {
       iounmap((void*)_videoDecoder.ratebuf.ulLogicalAddr);
       _videoDecoder.ratebuf.ulLogicalAddr = 0;
    }
}


INT   vid_osd_end_stream()
{
    PDEBUG("end stream\n");
    _uStreamEnd = 1;
    return 0;
}


UINT vid_osd_buf_fill(CLIPDEV_T clipdev, CLIPINFO *pClipInfo, void *src, UINT uLen)
{
    void *des;

    des = (void *) (_videoDecoder.clipbuf.ulLogicalAddr + pClipInfo->ulBufAdrOff);
    PDEBUG("log des = %p, len = 0x%8.8x\n", des, uLen);

    //copy data from use space to kernel space
    if(copy_from_user(des, src, uLen) < 0)
    {
        PDEBUG("AUD: access verify bad\n");
        return 0;
    }
    pClipInfo->uClipLen = uLen;
    if(clipdev_write_ex(clipdev, pClipInfo) != 0)
        return 0;
    else
        return uLen;
}


ULONG vid_osd_write(struct file *file, void *src, ULONG ulLen)
{
    ULONG cur_len = 0, offset = 0;
    UINT  ret;

    CLIPDEV_T clipdev;
    CLIPINFO  info;

    PDEBUG("buf fill length = %ld", ulLen);

    //get empty clip segment from clip device
    clipdev = vid_osi_get_clipdev();
    if(clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }

    while(ulLen)
    {
        if (file->f_flags & O_NONBLOCK)
        {
            if(clipdev_get_buf_nowait(clipdev, &info) != 0)
            {
                PDEBUG("no more empty buffer\n");
                return offset;
            }
        }
        else
        {
            if(clipdev_get_buf_wait(clipdev, &info) != 0)
            {
                PDEBUG("no more empty buffer\n");
                return offset;
            }
        }

        info.uFlag = 0;
        if(ulLen > info.ulBufLen)
        {
            cur_len = info.ulBufLen;
        }
        else
        {
            cur_len = ulLen;
            if(_uStreamEnd)
            {
                info.uFlag = 1;
                _uStreamEnd = 0;
            }
        }
        if((ret = vid_osd_buf_fill(clipdev, &info, src + offset, cur_len)) != cur_len)
        {
            PDEBUG("buffer fill error\n");
            return offset;
        }
        offset += ret;
        ulLen -= ret;
    }
    return offset;
}


INT  vid_osd_get_buf_nowait(CLIPINFO *info)
{
    CLIPINFO infok;
    CLIPDEV_T clipdev;

    //get empty clip segment from clip device
    clipdev = vid_osi_get_clipdev();
    if(clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }
    if(clipdev_get_buf_nowait(clipdev, &infok) != 0)
    {
        PDEBUG("get buf no wait error\n");
        return -1;
    }
    put_user(infok.ulBufAdrOff, &(info->ulBufAdrOff));
    put_user(infok.ulBufLen, &(info->ulBufLen));
    put_user(infok.uClipAdrOff, &(info->uClipAdrOff));
    put_user(infok.uClipLen, &(info->uClipLen));
    put_user(infok.uFlag, &(info->uFlag));
    put_user(infok.pOtherInfo, &(info->pOtherInfo));
    return 0;
}


INT  vid_osd_get_buf_wait(CLIPINFO *info)
{
    CLIPINFO infok;
    CLIPDEV_T clipdev;

    //get empty clip segment from clip device
    clipdev = vid_osi_get_clipdev();
    if(clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }
    if(clipdev_get_buf_wait(clipdev, &infok) != 0)
    {
        PDEBUG("get buf wait error\n");
        return -1;
    }
    put_user(infok.ulBufAdrOff, &(info->ulBufAdrOff));
    put_user(infok.ulBufLen, &(info->ulBufLen));
    put_user(infok.uClipAdrOff, &(info->uClipAdrOff));
    put_user(infok.uClipLen, &(info->uClipLen));
    put_user(infok.uFlag, &(info->uFlag));
    put_user(infok.pOtherInfo, &(info->pOtherInfo));
    return 0;
}

INT vid_osd_clip_write(CLIPINFO *info)
{
    CLIPINFO infok;
    CLIPDEV_T clipdev;

    //get empty clip segment from clip device
    clipdev = vid_osi_get_clipdev();
    if(clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }
    /*if(clipdev_get_buf_wait(clipdev, &infok) != 0)
    {
        PDEBUG("get buf wait error\n");
        return -1;
    }*/
    get_user(infok.ulBufAdrOff, &(info->ulBufAdrOff));
    get_user(infok.ulBufLen, &(info->ulBufLen));
    get_user(infok.uClipAdrOff, &(info->uClipAdrOff));
    get_user(infok.uClipLen, &(info->uClipLen));
    get_user(infok.uFlag, &(info->uFlag));
    get_user(infok.pOtherInfo, &(info->pOtherInfo));

    if(clipdev_write_ex(clipdev, &infok) != 0)
    {
        PDEBUG("clip write error\n");
        return -1;
    }
    return 0;
}

INT vid_osd_cc_complete()
{
    long timeout = 5*HZ;

    __CC_COMPLETE = 0;
    __CC_PENDING = 1;

    vid_atom_set_irq_mask(vid_atom_get_irq_mask() | DECOD_HOST_MASK_PSTART);
    while(__CC_COMPLETE == 0)
    {
      timeout = interruptible_sleep_on_timeout(&vid_cc_WaitQ,timeout);
      if(timeout == 0)
      {
        printk("vid_osd_cc_complete timed out\n");
        break;
      }
    }
    __CC_PENDING = 0;
    __CC_COMPLETE = 0;
    return(0);
}

INT vid_osd_cc_done()
{
    wake_up_interruptible(&vid_cc_WaitQ);
    return(0);
}

INT vid_osd_init_clip()
{
    return vid_osi_init_clip(_videoDecoder.clipbuf.uAddr, _videoDecoder.clipbuf.uLen);
}

INT vid_osd_init_stillp()
{
   return vid_osi_init_stillp(VID_RB_MEM_BASE, vid_osd_get_clip_mem_size());
}

UINT vid_osd_get_clip_mem_size()
{
    return _videoDecoder.clipbuf.uLen;
}


/*-----------------------------------------------------------------------------+
|  vid_osd_map_vbi_laddr
+-----------------------------------------------------------------------------*/
INT vid_osd_map_vbi_laddr(ULONG *vbi0_ulLogicalAddr,
                          ULONG *vbi1_ulLogicalAddr)
{
    /*-------------------------------------------------------------------------+
    |  Initialization
    +-------------------------------------------------------------------------*/
    *vbi0_ulLogicalAddr = 0;
    *vbi1_ulLogicalAddr = 0;

    /*-------------------------------------------------------------------------+
    |  Map VBI0 Buffer
    +-------------------------------------------------------------------------*/
    _videoDecoder.vbi0.uAddr = VID_VBI0_MEM_BASE;
    _videoDecoder.vbi0.uLen  = VID_VBI0_MEM_SIZE;
    _videoDecoder.vbi0.ulLogicalAddr = (unsigned long)ioremap(VID_VBI0_MEM_BASE, VID_VBI0_MEM_SIZE);
    if(_videoDecoder.vbi0.ulLogicalAddr == 0)
    {
        PDEBUG("ioremap VBI0 base error\n");
        return(-1);
    }
    PDEBUG("ioremap VBI0 data base: physical = 0x%8.8lx logical = 0x%8.8lx size = 0x%8.8lx \n",
           _videoDecoder.vbi0.uAddr, _videoDecoder.vbi0.ulLogicalAddr,_videoDecoder.vbi0.uLen);

    /*-------------------------------------------------------------------------+
    |  Map VBI1 Buffer
    +-------------------------------------------------------------------------*/
    _videoDecoder.vbi1.uAddr = VID_VBI1_MEM_BASE;
    _videoDecoder.vbi1.uLen  = VID_VBI1_MEM_SIZE;
    _videoDecoder.vbi1.ulLogicalAddr = (unsigned long)ioremap(VID_VBI1_MEM_BASE, VID_VBI1_MEM_SIZE);
    if(_videoDecoder.vbi1.ulLogicalAddr == 0)
    {
        PDEBUG("ioremap VBI1 base error\n");
        iounmap((void*)_videoDecoder.vbi0.ulLogicalAddr);
        return(-1);
    }
    PDEBUG("ioremap VBI1 data base: physical = 0x%8.8lx logical = 0x%8.8lx size = 0x%8.8lx \n",
           _videoDecoder.vbi1.uAddr, _videoDecoder.vbi1.ulLogicalAddr,_videoDecoder.vbi1.uLen);

    *vbi0_ulLogicalAddr = _videoDecoder.vbi0.ulLogicalAddr;
    *vbi1_ulLogicalAddr = _videoDecoder.vbi1.ulLogicalAddr;
    return(0);
}

/*-----------------------------------------------------------------------------+
|  vid_osi_unmap_vbi_laddr
+-----------------------------------------------------------------------------*/
void vid_osd_unmap_vbi_laddr()

{
    if(_videoDecoder.vbi0.ulLogicalAddr)
    {
       iounmap((void*)_videoDecoder.vbi0.ulLogicalAddr);
       _videoDecoder.vbi0.ulLogicalAddr = 0;
    }

    if(_videoDecoder.vbi1.ulLogicalAddr)
    {
       iounmap((void*)_videoDecoder.vbi1.ulLogicalAddr);
       _videoDecoder.vbi1.ulLogicalAddr = 0;
    }
}
