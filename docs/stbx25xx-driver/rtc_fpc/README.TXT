General Information of Driver for the RTC and Front Panel Controller on stbx25xx chip.

Module Name:
       stbrtc.o

Files:
       rtc_fpc.c rtc_t.c
       rtc_fpc_local.h
       include/rtc_fpc/rtc_fpc.h

Device Name
       /dev/stbrtc

Node
      mknod /dev/stbrtc c 240 0

      Major No is 240

Application
      rtc_t --> It reads the real time clock, sets the alarm clock and
                reads the alarm clock.

---------------------------------------------------------------------------