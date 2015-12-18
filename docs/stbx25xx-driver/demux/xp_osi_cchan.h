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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author    :  Tony Cerreto
| Component :  xp
| File      :  xp_cchan.h
| Purpose   :  XP Change Channel declarations and prototypes
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 04-Jun-01  TJC  Created
| 30-Sep-01  LGH  ported to Linux
+----------------------------------------------------------------------------*/
#ifndef XP_CCHAN_H
#define XP_CCHAN_H

/*----------------------------------------------------------------------------+
|  Local Prototype Definitions
+----------------------------------------------------------------------------*/
void  xp_osi_cchan_auto (
      GLOBAL_RESOURCES *pGlobal,
      USHORT uwApid,
      USHORT uwVpid,
      USHORT uwPpid);

void  xp_osi_video_cchan_auto (
      GLOBAL_RESOURCES *pGlobal,
      USHORT uwVpid);

void  xp_osi_audio_cchan_auto (
      GLOBAL_RESOURCES *pGlobal,
      USHORT uwApid);

void  xp_osi_pcr_cchan_auto (
      GLOBAL_RESOURCES *pGlobal,
      USHORT uwPpid);

SHORT xp_osi_cchan_init(GLOBAL_RESOURCES *pGlobal0);

int   xp_osi_cchan_start_audio(USHORT uwPid);

void  xp_osi_cchan_start_pcr(USHORT uwPid);

int   xp_osi_cchan_start_video(USHORT uwPid);

int   xp_osi_cchan_stop_audio(void);

void  xp_osi_cchan_stop_pcr(void);

int   xp_osi_cchan_stop_video(void);

#endif
