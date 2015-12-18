//vulcan/drv/include/rtc_fpc/rtc_fpc.h
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

#ifndef RTC_FPC_H
#define RTC_FPC_H


//Declare the driver related entities. 
typedef struct _STB_RTC_TIME {
  unsigned char secs;
  unsigned char mins;
  unsigned char hours;
  unsigned char am_pm;
  unsigned char days;
} STB_RTC_TIME, *PSTB_RTC_TIME;


#define RTC_FPC_DEV_MAJOR             0xf0

#define RTC_FPC_DEV_NAME              "stbrtc"

#define RTC_FPC_DEV_MAGIC             0xf0 

#define IOC_RTC_FPC_GET_TIME         _IOR(RTC_FPC_DEV_MAGIC, 1, STB_RTC_TIME *)
#define IOC_RTC_FPC_SET_TIME         _IOW(RTC_FPC_DEV_MAGIC, 2, STB_RTC_TIME *)

#define IOC_RTC_FPC_GET_ALRM_TIME    _IOR(RTC_FPC_DEV_MAGIC, 3, STB_RTC_TIME *)
#define IOC_RTC_FPC_SET_ALRM_TIME    _IOW(RTC_FPC_DEV_MAGIC, 4, STB_RTC_TIME *)

#define IOC_RTC_FPC_SET_FP_DATA      _IOW(RTC_FPC_DEV_MAGIC, 5, unsigned char *)

#define IOC_RTC_FPC_ENA_ALMINT       _IO(RTC_FPC_DEV_MAGIC, 6)
#define IOC_RTC_FPC_DIS_ALMINT       _IO(RTC_FPC_DEV_MAGIC, 7)

#define IOC_RTC_FPC_ENA_UPEINT       _IO(RTC_FPC_DEV_MAGIC, 8)
#define IOC_RTC_FPC_DIS_UPEINT       _IO(RTC_FPC_DEV_MAGIC, 9)

#define IOC_RTC_FPC_ENA_DISPDAT      _IO(RTC_FPC_DEV_MAGIC, 10)
#define IOC_RTC_FPC_ENA_DISPRTC      _IO(RTC_FPC_DEV_MAGIC, 11)

#define IOC_RTC_FPC_SET_24TOFM       _IO(RTC_FPC_DEV_MAGIC, 12)
#define IOC_RTC_FPC_SET_12TOFM       _IO(RTC_FPC_DEV_MAGIC, 13)

#define IOC_RTC_FPC_WAIT_FOR_ALRM    _IO(RTC_FPC_DEV_MAGIC, 14)

#endif

