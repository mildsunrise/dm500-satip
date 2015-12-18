/*----------------------------------------------------------------------------+
|     This source code has been made available to you by IBM on an AS-IS
|     basis.  Anyone receiving this source is licensed under IBM
|     copyrights to use it in any way he or she deems fit, including
|     copying it, modifying it, compiling it, and redistributing it either
|     with or without modifications.  No license under IBM patents or
|     patent applications is to be implied by the copyright license.
|
|     Any user of this software should understand that IBM cannot provide
|     technical support for this software and will not be responsible for
|     any consequences resulting from the use of this software.
|
|     Any person who transfers this source code or any derivative work
|     must include the IBM copyright notice, this paragraph, and the
|     preceding two paragraphs in the transferred software.
|
|       IBM CONFIDENTIAL
|       STB025XX VXWORKS EVALUATION KIT SOFTWARE
|       (C) COPYRIGHT IBM CORPORATION 2003
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author    :  Katsuyuki Sugita
| Component :  ttx
| File      :  ttx.h
| Purpose   :  Teletext Driver include file
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 22-Sep-03  TJC  Modified
+----------------------------------------------------------------------------*/
#ifndef _TTX_H_
#define _TTX_H_

/*----------------------------------------------------------------------------+
| Defines
+----------------------------------------------------------------------------*/
#define SUCCESS                  0
#define E_TTX_NOT_INITIALIZED   -1
#define E_TTX_NOMEM             -2
#define E_TTX_BAD_LENGTH        -3
#define E_TTX_BAD_DISP_MODE     -4
#define E_TTX_LESS_VBI_COUNT    -5
#define E_TTX_VDC_INT_NOTIFY    -6
#define E_TTX_BAD_VBI_ADDR      -7

/*----------------------------------------------------------------------------+
| Function Prototypes
+----------------------------------------------------------------------------*/
int  ttx_initialize();
int  ttx_terminate();
int  ttx_write(void *buffer, size_t length);
int  ttx_insert_pid(unsigned short pid);
int  ttx_delete_pid(void);
void ttx_stats_get(unsigned long);
void ttx_stats_clear(void);

#endif
