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

#include <stdio.h>
#include <linux/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>

main()
{
  struct pollfd ufds;
  int fd, ret;
  fd = open("/dev/rawir", O_RDONLY|O_NONBLOCK);
  if(fd < 0) {
    printf("What's wrong with the device driver /dev/rawir ?\n");
  }
  
  
  ufds.fd = fd;
  ufds.events = POLLIN;
  
  printf("Press power off to quit test, other keys to test .\n"); fflush(stdout);
  while(1) {
   ret = poll(&ufds, 1, 5000); 
   if (ret){
     printf("poll down\n"); fflush(stdout);
     if (ufds.revents & POLLIN){ /* rawir*/
       short int cmd;
       int cnt = read(fd, (void*)&cmd, 2);
       if(cnt <= 0)  {
         printf("Poll returned without key!\n");  fflush(stdout);
       }
       else
       {
         printf("KeyCode = 0x%04x\n", cmd); fflush(stdout);
         if(cmd == 266 || cmd == 261) break;
       }
     }
   }
   else {
     printf("poll timeout\n"); fflush(stdout);
   }
  }
  close(fd);
}
