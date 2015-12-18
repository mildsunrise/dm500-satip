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
|       COPYRIGHT   I B M   CORPORATION 1999
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Tony J. Cerreto
| Component: XP
| File:      xp_pcr.h
| Purpose:   Common definitions for xp_pcr.c functions.
|
| Changes:
| Date:      Author  Comment:
| ---------  ------  --------
| 19-Dec-99  TJC     Created.
| 30-Sep-01  LGH     Ported to Linux
+----------------------------------------------------------------------------*/
#ifndef _xp_pcr_h_
#define _xp_pcr_h_



/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/
void  xp_osi_pcr_delay(unsigned long  delay);

void  xp_osi_pcr_sync();

void  xp_osi_pcr_sync_start();

void  xp_osi_pcr_sync_stop();

int xp_osi_pcr_init(GLOBAL_RESOURCES *pGlobal0);

#endif
