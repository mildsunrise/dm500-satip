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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
+----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>


#include "rtc_fpc/rtc_fpc.h"


int main(int argc, char **argv)
{
  int fd;
  STB_RTC_TIME  st_app_time;
  unsigned char uc_disp[5];

  printf("open the STBRTC device\n");
  if((fd = open("/dev/stbrtc",O_RDWR)) < 0 ) {
      printf("Open stbrtc fail\n");
      return 0;
  }

  ioctl(fd, IOC_RTC_FPC_GET_TIME, &st_app_time);

  printf("The Real Time Values are :\n");
  
  printf("Days : %d\n", st_app_time.days);
  printf("Hours : %d\n", st_app_time.hours);
  printf("Minutes : %d\n", st_app_time.mins);
  printf("Seconds : %d\n", st_app_time.secs);

  printf("Setting the Alarm 5 Secs higher than the time Read\n");

  st_app_time.secs += 5;
  if(st_app_time.secs > 59){
    st_app_time.secs -= 59;
  }

  ioctl(fd, IOC_RTC_FPC_SET_ALRM_TIME, &st_app_time);
  ioctl(fd, IOC_RTC_FPC_ENA_ALMINT, 0);

  ioctl(fd, IOC_RTC_FPC_GET_ALRM_TIME, &st_app_time);

  printf("Read Alarm Time Values are :\n");
  
  printf("Days : %d\n", st_app_time.days);
  printf("Hours : %d\n", st_app_time.hours);
  printf("Minutes : %d\n", st_app_time.mins);
  printf("Seconds : %d\n", st_app_time.secs);

  close(fd);
       
  return (0);
}

