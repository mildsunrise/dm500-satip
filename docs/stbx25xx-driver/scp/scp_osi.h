/*---------------------------------------------------------------------------+
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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Mark Detrick
| Component: scp
| File:      scp_osi.h
| Purpose:   OS independant functions for serial control port
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 09/19/2003  MSD               Created.
+----------------------------------------------------------------------------*/

#ifndef _scp_osi_h_
#define _scp_osi_h_

int scp_osi_display_regs();
int scp_osi_get_cmd(unsigned long *p_value);
int scp_osi_set_cmd(unsigned long value);
int scp_osi_get_reverse_data(unsigned long *p_value);
int scp_osi_set_reverse_data(unsigned long value);
int scp_osi_get_clock_invert(unsigned long *p_value);
int scp_osi_set_clock_invert(unsigned long value);
int scp_osi_get_loopback(unsigned long *p_value);
int scp_osi_set_loopback(unsigned long value);

#endif
