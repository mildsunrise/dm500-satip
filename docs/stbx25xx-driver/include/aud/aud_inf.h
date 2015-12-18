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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| File:   aud_inf.h
| Purpose: audio driver interface layer PALLAS
| Changes:
| Date:     Comment:
| -----     --------
| 15-Oct-01 create
| 17-Jun-03 Added IOCTL definition for retrieving the current audio status
| 25-Jun-03 Added support for DTS/LPCM coreload (DTS minor=4, LPCM minor=5)
+----------------------------------------------------------------------------*/
#ifndef PALLAS_AUD_INF_H
#define PALLAS_AUD_INF_H

#include "clip/clipinfo.h"
#include "aud/aud_types.h"

#define MAJOR_NUM_ADEC	            202
#define DEVICE_NAME_ADEC            "adec_dev"

#define DRV_MINOR_AUD_MPEG          0
#define DRV_MINOR_AUD_AC3           1
#define DRV_MINOR_AUD_PCM           2
#define DRV_MINOR_AUD_MIXER         3
#define DRV_MINOR_AUD_DTS           4
#define DRV_MINOR_AUD_LPCM          5

#define MPEG_AUD_STOP               _IOW('a',1,int)
#define MPEG_AUD_PLAY               _IOW('a',2,int)
//#define MPEG_AUD_CONTINUE           _IOW('a',4,int)
#define MPEG_AUD_SELECT_SOURCE      _IOW('a',5,int)
#define MPEG_AUD_SET_MUTE           _IOW('a',6,int)
#define MPEG_AUD_SELECT_CHANNEL     _IOW('a',9,audChannel_t)
#define MPEG_AUD_GET_STATUS         _IOR('a',10, audioStatus *)
//#define MPEG_AUD_SET_MODE           _IOW('a', 11,audMode_t )
#define MPEG_AUD_SET_STREAM_TYPE    _IOW('a', 15, audStream_t)
#define MPEG_AUD_SET_PCM_FORMAT     _IOW('a', 16, AUD_PCM_FORMAT_CONFIG*)  // YYD, add for PCM support

//#define MPEG_AUD_GEN_TONE         _IOW('a', 12, BEEPTONE*)

/*volume group*/
#define MPEG_AUD_SET_VOL            _IOW('a', 13, AUDVOL*)
#define MPEG_AUD_GET_VOL            _IOR('a', 14, AUDVOL*)

/*clip group*/
#define MPEG_AUD_GET_BUF_NOWAIT     _IOR('a', 17, CLIPINFO* )
#define MPEG_AUD_GET_BUF_WAIT       _IOR('a', 18, CLIPINFO* )
#define MPEG_AUD_CLIP_WRITE         _IOW('a', 19, CLIPINFO* )
#define MPEG_AUD_GET_BUF_SIZE       _IOW('a', 20, unsigned long *)
#define MPEG_AUD_END_OF_STREAM      _IOW('a', 25, int)
#define MPEG_AUD_RESET_CLIPBUF      _IOW('a', 26, int)
#define MPEG_AUD_CLIP_FLUSH         _IOW('a', 27, int)

/*synchronization group*/
#define MPEG_AUD_GET_SYNC_INFO      _IOR('a', 21, SYNCINFO *)
#define MPEG_AUD_SET_SYNC_STC       _IOW('a', 22, STC_T *)
#define MPEG_AUD_SYNC_ON            _IOW('a', 23, int )
#define MPEG_AUD_SYNC_OFF           _IOW('a', 24, int )


#endif
