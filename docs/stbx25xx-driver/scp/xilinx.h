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
| File:      xilinx.h
| Purpose:   xilinx functions for serial control port
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 09/19/2003  MSD               Created.
+----------------------------------------------------------------------------*/

#ifndef _xilinx_h_
#define _xilinx_h_

/*----------------------------------------------------------------------------+
| SCP_CHIP_SELECT().
+----------------------------------------------------------------------------*/
void xilinx_sflash_chip_select();

/*----------------------------------------------------------------------------+
| SCP_CHIP_DESELECT().
+----------------------------------------------------------------------------*/
void xilinx_sflash_chip_deselect();
 
/*----------------------------------------------------------------------------+
| ENABLE_SCP_BUS().
+----------------------------------------------------------------------------*/
void xilinx_enable_scp_bus();

/*----------------------------------------------------------------------------+
| DISABLE_SCP_BUS().
+----------------------------------------------------------------------------*/
void xilinx_disable_scp_bus();

/*----------------------------------------------------------------------------+
| Write Protect
+----------------------------------------------------------------------------*/
void xilinx_sflash_wp_on();

/*----------------------------------------------------------------------------+
| Reset Serial Flash().
+----------------------------------------------------------------------------*/
void xilinx_sflash_wp_off();

/*----------------------------------------------------------------------------+
| Reset Serial Flash().
+----------------------------------------------------------------------------*/
void xilinx_sflash_reset_on();

/*----------------------------------------------------------------------------+
| Reset Serial Flash().
+----------------------------------------------------------------------------*/
void xilinx_sflash_reset_off();









#endif
