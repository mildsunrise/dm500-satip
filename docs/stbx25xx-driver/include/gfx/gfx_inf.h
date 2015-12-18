//vulcan/drv/include/gfx/gfx_inf.h
/*----------------------------------------------------------------------------+
|
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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
//
//Comment: 
//  Linux device driver interface of GFX controls 
//Revision Log:   
//  Sept/26/2001                          Created by YYD
//  Oct/21/2002                           Define 3 new IOCTL codes to facilitate
//                                        real-time video playback on STB chips
//                                        without 2D video acceleration.

#ifndef  _DRV_INCLUDE_GFX_GFX_INF_H_INC_
#define  _DRV_INCLUDE_GFX_GFX_INF_H_INC_

#include <linux/ioctl.h>

//#include "gfx/gfx_surface.h"  // should not be required
#include "gfx/gfx_common.h"
#include "gfx/gfx_inf_struct.h"

#define GFX_DEV_MAJOR   (0xfb)  // device major 251

#define GFX_DEV_NAME    "/dev/stbgfx"

#define GFX_IOC_MAGIC   (0xfb)  //  magic number to identify me

#define GFX_MAX_ALLOWED_SURFACES    (256)       // we may not have such lots of memory

// gfx surface calls

#define IOC_GFX_CREATE_SURFACE				_IOWR(GFX_IOC_MAGIC, 1,  GFX_CREATE_SURFACE_PARM_T*)

#define IOC_GFX_DESTROY_SURFACE				_IOW (GFX_IOC_MAGIC, 2,  int)


#define IOC_GFX_LOCK_SURFACE				_IOWR(GFX_IOC_MAGIC, 3,  GFX_SURFACE_INFO_T*)

#define IOC_GFX_UNLOCK_SURFACE				_IOW (GFX_IOC_MAGIC, 4,  int)

#define IOC_GFX_GET_SURFACE_PALETTE  		_IOWR(GFX_IOC_MAGIC, 5,  GFX_SURFACE_ACCESS_PALETTE_PARM_T*)

#define IOC_GFX_SET_SURFACE_PALETTE  		_IOW (GFX_IOC_MAGIC, 6,  GFX_SURFACE_ACCESS_PALETTE_PARM_T*)

#define IOC_GFX_GET_SHARED_SURFACE  		_IO  (GFX_IOC_MAGIC, 7)

#define IOC_GFX_SET_SHARED_SURFACE  		_IOW (GFX_IOC_MAGIC, 8,  int)

#define IOC_GFX_SET_CURSOR_ATTRIBUTE        _IOW (GFX_IOC_MAGIC, 9, GFX_CURSOR_ATTRUBUTE_PARM_T*)


// surface display calls
#define IOC_GFX_ATTACH_SURFACE				_IOW (GFX_IOC_MAGIC, 11, GFX_SURFACE_VDEV_PARM_T*)

#define IOC_GFX_DETACH_SURFACE				_IOW (GFX_IOC_MAGIC, 12, GFX_SURFACE_VDEV_PARM_T*)

#define IOC_GFX_GET_SURFACE_DISP_PARM       _IOWR(GFX_IOC_MAGIC, 13, GFX_SURFACE_DISPLAY_T*)

#define IOC_GFX_SET_SURFACE_DISP_PARM       _IOW (GFX_IOC_MAGIC, 14, GFX_SURFACE_DISPLAY_T*)

#define IOC_GFX_MOVE_CURSOR                 _IOW (GFX_IOC_MAGIC, 15, GFX_COORDINATE_T*)

#define IOC_GFX_REPORT_CURSOR               _IOW (GFX_IOC_MAGIC, 16, GFX_COORDINATE_T*)

#define IOC_GFX_GET_SCREEN_INFO				_IOR (GFX_IOC_MAGIC, 17, GFX_SCREEN_INFO_T*)

#define IOC_GFX_SET_SCREEN_INFO				_IOW (GFX_IOC_MAGIC, 18,  GFX_SCREEN_INFO_T*)

#define IOC_GFX_SWAP_SURFACE				_IOWR (GFX_IOC_MAGIC, 19, GFX_2SURFACE_VDEV_PARM_T*)


// display control calls    
#define IOC_GFX_SET_DISPLAY_CONTROL         _IOW (GFX_IOC_MAGIC, 21, GFX_DISPLAY_CONTROL_PARM_T*)

#define IOC_GFX_GET_DISPLAY_CONTROL         _IOWR(GFX_IOC_MAGIC, 22, GFX_DISPLAY_CONTROL_PARM_T*)

#define IOC_GFX_SET_VISUAL_DEVICE_CONTROL   _IOW (GFX_IOC_MAGIC, 23, GFX_VISUAL_DEVICE_CONTROL_PARM_T*)

#define IOC_GFX_GET_VISUAL_DEVICE_CONTROL   _IOWR(GFX_IOC_MAGIC, 24, GFX_VISUAL_DEVICE_CONTROL_PARM_T*)


// 2d accelerated calls

#define IOC_GFX_BITBLT                      _IOW (GFX_IOC_MAGIC, 51, GFX_BITBLT_PARM_T*)

#define IOC_GFX_ADV_BITBLT                  _IOW (GFX_IOC_MAGIC, 52, GFX_ADV_BITBLT_PARM_T*)

#define IOC_GFX_FILLBLT                     _IOW (GFX_IOC_MAGIC, 53, GFX_FILLBLT_PARM_T*)

#define IOC_GFX_ADV_FILLBLT                 _IOW (GFX_IOC_MAGIC, 54, GFX_ADV_FILLBLT_PARM_T*)

#define IOC_GFX_BLEND                       _IOW (GFX_IOC_MAGIC, 55, GFX_BLEND_PARM_T*)

#define IOC_GFX_ADV_BLEND                   _IOW (GFX_IOC_MAGIC, 56, GFX_ADV_BLEND_PARM_T*)

#define IOC_GFX_COLORKEY                    _IOW (GFX_IOC_MAGIC, 57, GFX_COLORKEY_PARM_T*)

#define IOC_GFX_RESIZE                      _IOW (GFX_IOC_MAGIC, 58, GFX_RESIZE_PARM_T*)

// newly added
#define IOC_GFX_WAIT_FOR_COMPLETE           _IOW (GFX_IOC_MAGIC, 60, int)

#define IOC_GFX_RESET_ENGINE                _IO  (GFX_IOC_MAGIC, 61)

#define _GFX_ENGINE_ASYNC_MODE              0
#define _GFX_ENGINE_SYNC_MODE               1

#define IOC_GFX_SET_ENGINE_MODE             _IOW (GFX_IOC_MAGIC, 62, int)

#define IOC_GFX_GET_ENGINE_MODE             _IO  (GFX_IOC_MAGIC, 63)

#define IOC_GFX_GET_SURFACE_INFO            _IOWR(GFX_IOC_MAGIC, 64,  GFX_SURFACE_INFO_T*)  // newly added

#define IOC_GFX_SET_SURFACE_CLIP_RECT       _IOWR(GFX_IOC_MAGIC, 65, GFX_SET_CLIP_PARM_T*)

// these two is used to manage the relationship of surface and opened file handles
// hooked surfaces will be destroyed when file handle is closed
#define IOC_GFX_HOOK_SURFACE                _IOW (GFX_IOC_MAGIC, 66, int)
#define IOC_GFX_UNHOOK_SURFACE              _IOW (GFX_IOC_MAGIC, 67, int)

#define IOC_GFX_GET_SUBPLANE_PSEUDO_SURFACE _IOWR(GFX_IOC_MAGIC, 68, GFX_GET_SUBPLANE_PSEUDO_SURFACE_PARM_T*)

// "block transfer" using DMA controller
#define IOC_GFX_DMABLT                      _IOW (GFX_IOC_MAGIC, 70, GFX_DMABLT_PARM_T*)
#define IOC_GFX_DMABLT_WAIT                 _IO (GFX_IOC_MAGIC, 71)

// physical memory allocator; use with cached mmap() to allow user space application to set up memory
// directly usable by graphics hardware or DMA controller
#define IOC_GFX_PMALLOC                     _IOWR (GFX_IOC_MAGIC, 72, GFX_PMALLOC_PARM_T*)

#endif //  _DRV_INCLUDE_GFX_GFX_INF_H_INC_
