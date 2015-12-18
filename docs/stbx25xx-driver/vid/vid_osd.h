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
| File:   vid_osd.h
| Purpose: video decoder osd layer for PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01             create                                                                                          SL
+----------------------------------------------------------------------------*/
#ifndef PALLAS_VID_OSD_H
#define PALLAS_VID_OSD_H

#include <linux/fs.h>
#include <os/os-types.h>
#include "clip/clip.h"
#include "vid_osi.h"



/*divide a large data block into a group of data segments and write
  them to clip info queue sequentially*/
ULONG     vid_osd_write(struct file *file, void *src, ULONG ulLen);
INT       vid_osd_get_buf_nowait(CLIPINFO *info);
INT       vid_osd_get_buf_wait(CLIPINFO *info);
INT       vid_osd_clip_write(CLIPINFO *info);
INT       vid_osd_cc_complete();
INT       vid_osd_cc_done();
INT       vid_osd_init_clip();
void      vid_osd_close_clip();
UINT      vid_osd_get_clip_mem_size();
INT       vid_osd_init_stillp();
void      vid_osd_close_stillp();
void*     vid_osd_get_stillp_buf();
INT       vid_osd_map_vbi_laddr(ULONG *vbi0_ula, ULONG *vbi1_ula);
void      vid_osd_unmap_vbi_laddr();
INT       vid_osd_end_stream();
INT       vid_osd_reset_frame_buffer();
INT       vid_osd_init();
void      vid_osd_close();

#endif
