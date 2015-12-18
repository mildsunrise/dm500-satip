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
| File:   vid_inf.h
| Purpose: video decoder interface layer for PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#ifndef PALLAS_VID_INF_H
#define PALLAS_VID_INF_H

#include <linux/ioctl.h>
#include "clip/clipinfo.h"
#include "vid/vid_types.h"

#define MAJOR_NUM_VDEC                203
#define DEVICE_NAME_VDEC              "vdec_dev"
#define MPEG_VID_SOURCE_DEMUX         0


#define MPEG_VID_STOP                 _IOW('v', 21, int)
#define MPEG_VID_PLAY                 _IOW('v', 22, int)
#define MPEG_VID_FREEZE               _IOW('v', 23, int)
#define MPEG_VID_CONTINUE             _IOW('v', 24, int)
#define MPEG_VID_SELECT_SOURCE        _IOW('v', 25, int)
#define MPEG_VID_SET_BLANK            _IOW('v', 26, int)
#define MPEG_VID_PAUSE                _IOW('v', 28, int)
#define MPEG_VID_FASTFORWARD          _IOW('v', 29, int)
#define MPEG_VID_SLOWMOTION           _IOW('v', 30, int)
#define MPEG_VID_GET_BUF_NOWAIT       _IOR('v', 31, CLIPINFO* )
#define MPEG_VID_GET_BUF_WAIT         _IOR('v', 32, CLIPINFO* )
#define MPEG_VID_CLIP_WRITE           _IOW('v', 33, CLIPINFO* )
#define MPEG_VID_END_OF_STREAM        _IOW('v', 34, int)
#define MPEG_VID_GET_BUF_SIZE         _IOR('v', 35, unsigned long*)
#define MPEG_VID_SET_SCALE_POS        _IOW('v', 36, SCALEINFO* )
#define MPEG_VID_SCALE_ON             _IOW('v', 37, int )
#define MPEG_VID_SCALE_OFF            _IOW('v', 38, int )
#define MPEG_VID_GET_SYNC_INFO        _IOR('v', 39, SYNCINFO *)
#define MPEG_VID_SET_SYNC_STC         _IOW('v', 40, STC_T *)
#define MPEG_VID_SET_DISPSIZE         _IOW('v', 41, vidDispSize_t)
#define MPEG_VID_SYNC_ON              _IOW('v', 42, vidSync_t)
#define MPEG_VID_SYNC_OFF             _IOW('v', 43, int )

#define MPEG_VID_GET_V_INFO           _IOR('v', 44, VIDEOINFO *)
#define MPEG_VID_SET_DISPFMT          _IOW('v', 45, vidDispFmt_t)

#define MPEG_VID_START_STILLP         _IOW('v', 46, int)
#define MPEG_VID_STOP_STILLP          _IOW('v', 47, int)
#define MPEG_VID_STILLP_READY         _IOR('v', 48, int)
#define MPEG_VID_STILLP_WRITE         _IOW('v', 49, BUFINFO*)
#define MPEG_VID_STILLP_READ          _IOR('v', 50, READINFO*)

#define MPEG_VID_RESET_CLIPBUF        _IOW('v', 51, int)

//lingh added for pvr demo
#define MPEG_VID_SET_RB_SIZE          _IOW('v', 52, int)
#define MPEG_VID_SINGLE_FRAME         _IOW('v', 53, int)
#define MPEG_VID_SF_READY             _IOW('v', 54, int)
#define MPEG_VID_RESUME_FROM_SF       _IOW('v', 55, int)
#define MPEG_VID_RESET_RB             _IOW('v', 56, int)

#define MPEG_VID_SET_OUTFMT           _IOW('v', 57, vidOutFmt_t)
#define MPEG_VID_SET_DISPMODE         _IOW('v', 58, vidDispMode_t)
#define MPEG_VID_CLIP_CC              _IOW('v', 59, int)
#define MPEG_VID_CC_COMPLETE          _IOW('v', 60, int)
#define MPEG_VID_PLAY_STATUS          _IOR('v', 61, int)
#define MPEG_VID_CLIP_FLUSH           _IOW('v', 62, int)
#define MPEG_VID_SET_SFM              _IOW('v', 63, vidsfm_t)
#define MPEG_VID_ENABLE_DISP_14_9     _IOW('v', 64, int)
#define MPEG_VID_DISABLE_DISP_14_9    _IOW('v', 65, int)



#endif
