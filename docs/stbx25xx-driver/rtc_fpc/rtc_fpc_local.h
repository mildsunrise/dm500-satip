//vulcan/drv/rtc_fpc/rtc_fpc_local.h
/*----------------------------------------------------------------------------+
|
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

#ifndef RTC_FPC_LOCAL_H
#define RTC_FPC_LOCAL_H

#define IRQ_RTCFPC                   0

//Define the DCRs for the RTC/Front Panel Controller.
#define DCR_RTC_FPC_CNTL             0x310
#define DCR_RTC_FPC_INT              0x311
#define DCR_RTC_FPC_TIME             0x312
#define DCR_RTC_FPC_ALRM             0x313

#define DCR_RTC_FPC_D1               0x314
#define DCR_RTC_FPC_D2               0x315
#define DCR_RTC_FPC_D3               0x316
#define DCR_RTC_FPC_D4               0x317
#define DCR_RTC_FPC_D5               0x318

#define DCR_RTC_FPC_FCNTL            0x319
#define DCR_RTC_FPC_BRT              0x31A

#endif

