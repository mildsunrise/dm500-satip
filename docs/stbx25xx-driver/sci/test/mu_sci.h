/*-----------------------------------------------------------------------------+
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
|       COPYRIGHT   I B M   CORPORATION 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------+
| Author:    Mike Lepore
| Component: sci
| File:      mu_sci.h
| Purpose:   Smart Card menu application header file.
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  zongwei Liu       Port to pallas
+----------------------------------------------------------------------------*/

#ifndef mu_sci_h_
#define mu_sci_h_

SCI_ERROR sci_test(unsigned long sci_id, char *cmd);
SCI_ERROR select_file(unsigned long sci_id);
SCI_ERROR read_record(unsigned long sci_id);
SCI_ERROR update_record(unsigned long sci_id);
SCI_ERROR loop(unsigned long sci_id);
void print_error(char *fn_name, int error_code);

void pps(unsigned long sci_id);
void get_ATR_parms(unsigned long sci_id);
void get_parms(unsigned long sci_id);
void set_parms(unsigned long sci_id);
void get_ifsd(unsigned long sci_id);
void set_ifsd(unsigned long sci_id);
void get_modes(unsigned long sci_id);
void set_modes(unsigned long sci_id);

#endif /* _mu_sci_h_ */
