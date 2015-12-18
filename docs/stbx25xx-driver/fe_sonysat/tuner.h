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
|       COPYRIGHT   I B M   CORPORATION 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/

#ifndef TUNER_H
#define TUNER_H

#define TUNER_DEV_MAJOR               250
#define TUNER_DEV_NAME                "fe_sonysat"
#define TUNER_DEV_MAGIC               250 

#define SONY_TUNER_ADDR (0x10>>1)

/*** Tuner IOCTL parameters ***/

typedef struct {
                 unsigned char lnb_power;
                 unsigned char disecq_ctrl;
                 unsigned long symbolrate;
                 long symbolrateband;
                 unsigned long symbolrateparm;
                 unsigned long freq;
                 unsigned char rf_val[4];
               } IOCTL_TUNE_PARAMETER;

typedef struct {
                 unsigned char lnb_power;
                 unsigned char disecq_ctrl;
                 unsigned long symbolrate;
                 unsigned long freq;
                 unsigned char tslock;
                 unsigned char code_rate_n;    // code rate = num/div
                 unsigned char code_rate_d;
               } IOCTL_STATUS_PARAMETER;

/*** Tuner IOCTL interface ***/

#define TUNER_IOCTL_TUNE              _IOW(TUNER_DEV_MAGIC, 1,  ULONG)
#define TUNER_IOCTL_STATUS            _IOW(TUNER_DEV_MAGIC, 2,  ULONG)

/*** LNB control    ***/

#define FE_LNB_NO_POWER                  	0
#define FE_LNB_POWER_VERTICAL            	1
#define FE_LNB_POWER_HORIZONTAL          	2

/*** Disecq control ***/

#define FE_DISECQ_22K_OFF  			0
#define FE_DISECQ_22K_ON   			1



#endif

