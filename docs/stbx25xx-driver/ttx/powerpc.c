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
| Author    :  Tony J. Cerreto
| Component :  TTX
| File      :  powerpc.c
| Purpose   :  Teletext Driver PowerPC Stub functions.
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 22-Sep-03  TJC  Created
+----------------------------------------------------------------------------*/
#include <linux/config.h>
#include <linux/version.h>
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#define  __NO_VERSION__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <os/os-sync.h>
#include <stdio.h>
#include "powerpc.h"

/*----------------------------------------------------------------------------+
| VxWorks Equivalent Functions in Linux
+----------------------------------------------------------------------------*/
unsigned long beginCriticalSection(void)
{
  return(os_enter_critical_section());
}

void endCriticalSection(unsigned long crit)
{
  os_leave_critical_section(crit);
  return;
}

unsigned long xp0_dcr_read(unsigned long address)
{
  powerpcMtxpt0_lr(address);
  return(powerpcMfxpt0_dt());
}
