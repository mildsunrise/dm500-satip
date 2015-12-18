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
| File:      sci_inf.h
| Purpose:   Smart Card Interface device driver PUBLIC API header file.
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/26/2001  Zongwei Liu       Initial check-in
| 09/26/2001  zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
+----------------------------------------------------------------------------*/

#ifndef _sci_inf_h_
#define _sci_inf_h_

#include "sci/sci_global.h"

/*#define SCI_DEBUG_PRINT*/
/*#define SCI_DEBUG_TRACE*/

/* constants */
#define DEVICE_NAME                 "sci_dev"

/* ioctl cmd table */
#define SCI_IOW_MAGIC			    's'
#define IOCTL_SET_RESET			    _IOW(SCI_IOW_MAGIC, 1,  ULONG)
#define IOCTL_SET_MODES			    _IOW(SCI_IOW_MAGIC, 2,  SCI_MODES)
#define IOCTL_GET_MODES			    _IOW(SCI_IOW_MAGIC, 3,  SCI_MODES)
#define IOCTL_SET_PARAMETERS		_IOW(SCI_IOW_MAGIC, 4,  SCI_PARAMETERS)
#define IOCTL_GET_PARAMETERS		_IOW(SCI_IOW_MAGIC, 5,  SCI_PARAMETERS)
#define IOCTL_SET_CLOCK_START		_IOW(SCI_IOW_MAGIC, 6,  ULONG)
#define IOCTL_SET_CLOCK_STOP		_IOW(SCI_IOW_MAGIC, 7,  ULONG)
#define IOCTL_GET_IS_CARD_PRESENT	_IOW(SCI_IOW_MAGIC, 8,  ULONG)
#define IOCTL_GET_IS_CARD_ACTIVATED	_IOW(SCI_IOW_MAGIC, 9,  ULONG)
#define IOCTL_SET_DEACTIVATE		_IOW(SCI_IOW_MAGIC, 10, ULONG)
#define IOCTL_SET_ATR_READY		    _IOW(SCI_IOW_MAGIC, 11, ULONG)
#define IOCTL_GET_ATR_STATUS		_IOW(SCI_IOW_MAGIC, 12, ULONG)
#define IOCTL_DUMP_REGS			    _IOW(SCI_IOW_MAGIC, 20, ULONG)

/* MAJOR NUM OF DEVICE DRVIER */
#define MAJOR_NUM			169

#endif /* _sci_inf_h_ */
