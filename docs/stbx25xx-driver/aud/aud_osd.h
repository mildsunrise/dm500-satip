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
| File:   aud_osd.h
| Purpose: audio driver osd layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#ifndef PALLAS_AUD_OSD_H
#define PALLAS_AUD_OSD_H

#include <linux/fs.h>
#include <os/os-types.h>
#include "clip/clip.h"

typedef struct tagAudWriteStruct
{
    ULONG       ulBufPhyStart;
    ULONG       ulBufSize;
    CLIPDEV_T   clipdev;
    int         (*aud_osi_clip_init)(ULONG, ULONG);
    void        (*aud_osi_clip_close)();
    CLIPDEV_T   (*aud_osi_get_dev)();
    ULONG       ulBufLogAdr;
    UINT        uEndOfStream;
    UINT        uMapped;
}AUD_WRITE_STRU;


/*divide a large data block into a group of data segments and write
  them to clip info queue sequentially*/
ULONG     aud_osd_write(AUD_WRITE_STRU *pDev, struct file *file, void *src, ULONG ulLen);
INT       aud_osd_get_buf_nowait(AUD_WRITE_STRU *pDev, CLIPINFO *info);
INT       aud_osd_get_buf_wait(AUD_WRITE_STRU *pDev, CLIPINFO *info);
INT       aud_osd_clip_write(AUD_WRITE_STRU *pDev, CLIPINFO *info);
INT       aud_osd_init_clip(AUD_WRITE_STRU *pDev);
void      aud_osd_close_clip(AUD_WRITE_STRU *pDev);
INT       aud_osd_end_stream(AUD_WRITE_STRU *pDev);
INT       aud_osd_mmap(AUD_WRITE_STRU *pDev, struct vm_area_struct *vma);

#endif
