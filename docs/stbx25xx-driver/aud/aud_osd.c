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
| File:   aud_osd.c
| Purpose: audio driver osd layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#include <linux/mm.h>           /* for verify_area */
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>        /* for get_user, copy_from_user */
#include <linux/string.h>

#include <os/drv_debug.h>
#include <hw/physical-mem.h>
#include "aud_osi.h"
#include "aud_osd.h"


INT   aud_osd_end_stream(AUD_WRITE_STRU *pDev)
{
    PDEBUG("end stream\n");
    pDev->uEndOfStream = 1;
    return 0;
}


UINT aud_osd_buf_fill(AUD_WRITE_STRU *pDev, CLIPINFO *pClipInfo, void *src, UINT uLen)
{
    void *des;
    
    des = (void *) (pDev->ulBufLogAdr + pClipInfo->ulBufAdrOff);
    PDEBUG("log des = %p, len = 0x%8.8x\n", des, uLen);
        
    //copy data from use space to kernel space
    if(copy_from_user(des, src, uLen) < 0)
    {
        PDEBUG("copy from user error\n");
        return 0;
    }
    pClipInfo->uClipLen = uLen;
    if(clipdev_write_ex(pDev->clipdev, pClipInfo) != 0)
        return 0;
    else
        return uLen;
}


ULONG aud_osd_write(AUD_WRITE_STRU *pDev, struct file *file, void *src, ULONG ulLen)
{
    ULONG cur_len = 0, offset = 0;
    UINT  ret;
    CLIPINFO  info;

    PDEBUG("buf fill length = %ld", ulLen);

    if(pDev->clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }

    while(ulLen)
    {
        if (file->f_flags & O_NONBLOCK)
        {
            if(clipdev_get_buf_nowait(pDev->clipdev, &info) != 0)
            {
                PDEBUG("no more empty buffer\n");
                return offset;
            }
        }
        else
        {
            if(clipdev_get_buf_wait(pDev->clipdev, &info) != 0)
            {
                PDEBUG("no more empty buffer\n");
                return offset;
            }
        }

        if(ulLen > info.ulBufLen)
            cur_len = info.ulBufLen;
        else
        {
            cur_len = ulLen;
            if(pDev->uEndOfStream)
            {
                info.uFlag = 1;
                pDev->uEndOfStream = 0;
            }
        }
        if((ret = aud_osd_buf_fill(pDev, &info, src + offset, cur_len)) != cur_len)
        {
            PDEBUG("buffer fill error\n");
            return offset;
        }
        offset += ret;
        ulLen -= ret;
    }
    return offset;
}


INT  aud_osd_get_buf_nowait(AUD_WRITE_STRU *pDev, CLIPINFO *info)
{
    CLIPINFO infok;

    if(pDev->clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }
    if(clipdev_get_buf_nowait(pDev->clipdev, &infok) != 0)
    {
        PDEBUG("get buf no wait error\n");
        return -1;
    }
    /*put_user(infok.ulBufAdrOff, &(info->ulBufAdrOff));
    put_user(infok.ulBufLen, &(info->ulBufLen));
    put_user(infok.uClipAdrOff, &(info->uClipAdrOff));
    put_user(infok.uClipLen, &(info->uClipLen));
    put_user(infok.uFlag, &(info->uFlag));
    put_user(infok.pOtherInfo, &(info->pOtherInfo));*/
    copy_to_user(info, &infok, sizeof(CLIPINFO));
    return 0;
}

    
INT  aud_osd_get_buf_wait(AUD_WRITE_STRU *pDev, CLIPINFO *info)
{
    CLIPINFO infok;

    if(pDev->clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }
    if(clipdev_get_buf_wait(pDev->clipdev, &infok) != 0)
    {
        PDEBUG("get buf wait error\n");
        return -1;
    }
    /*put_user(infok.ulBufAdrOff, &(info->ulBufAdrOff));
    put_user(infok.ulBufLen, &(info->ulBufLen));
    put_user(infok.uClipAdrOff, &(info->uClipAdrOff));
    put_user(infok.uClipLen, &(info->uClipLen));
    put_user(infok.uFlag, &(info->uFlag));
    put_user(infok.pOtherInfo, &(info->pOtherInfo));*/
    copy_to_user(info, &infok, sizeof(CLIPINFO));
    return 0;
}

INT aud_osd_clip_write(AUD_WRITE_STRU *pDev, CLIPINFO *info)
{
    CLIPINFO infok;

    if(pDev->clipdev == NULL)
    {
        PDEBUG("No clip device\n");
        return 0;
    }
    /*if(clipdev_get_buf_wait(clipdev, &infok) != 0)
    {
        PDEBUG("get buf wait error\n");
        return -1;
    }*/
    /*get_user(infok.ulBufAdrOff, &(info->ulBufAdrOff));
    get_user(infok.ulBufLen, &(info->ulBufLen));
    get_user(infok.uClipAdrOff, &(info->uClipAdrOff));
    get_user(infok.uClipLen, &(info->uClipLen));
    get_user(infok.uFlag, &(info->uFlag));
    get_user(infok.pOtherInfo, &(info->pOtherInfo));*/
    copy_from_user(&infok, info, sizeof(CLIPINFO));
    
    if(clipdev_write_ex(pDev->clipdev, &infok) != 0)
    {
        PDEBUG("clip write error\n");
        return -1;
    }
    return 0;
}

INT aud_osd_init_clip(AUD_WRITE_STRU *pDev)
{
    INT ret;
    pDev->ulBufLogAdr = (unsigned long)ioremap(pDev->ulBufPhyStart, 
                                               pDev->ulBufSize);
    if(pDev->ulBufLogAdr == 0)
    {
        PDEBUG("ioremap error\n");
        return -1;
    }
    PDEBUG("ioremap OK base = 0x%8.8lx\n", pDev->ulBufLogAdr);
    ret = pDev->aud_osi_clip_init(pDev->ulBufPhyStart, pDev->ulBufSize);
    if(ret == 0)
    {
        pDev->clipdev = pDev->aud_osi_get_dev();
    }
    return ret;
}   


void aud_osd_close_clip(AUD_WRITE_STRU *pDev)
{
    pDev->aud_osi_clip_close();
    
    if(pDev->ulBufLogAdr)
        iounmap((void*)pDev->ulBufLogAdr);
}

INT aud_osd_mmap(AUD_WRITE_STRU *pDev, struct vm_area_struct *vma)
{
    int ret = -EINVAL;
    unsigned long size;

    if(vma->vm_flags & VM_WRITE)
    {
        if(vma->vm_pgoff != 0)
            return ret;
        size = vma->vm_end - vma->vm_start;
        PDEBUG("mmap size = 0x%8.8lx\n", size);
        if(size > MPEG_A_CLIP_BUF_LEN)
            return ret;
        ret = -EAGAIN;
	    vma->vm_page_prot.pgprot |= _PAGE_NO_CACHE;	// map without caching
        if(remap_page_range(vma->vm_start, pDev->ulBufPhyStart, 
                            size, vma->vm_page_prot))
            return -EINVAL;

        pDev->uMapped = 1;
    }
    ret = 0;
    return ret;
}


