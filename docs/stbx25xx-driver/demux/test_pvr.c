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
+---------------------------------------------------------------------------*/
/****************************************************************************/
/*                                                                          */
/*    DESCRIPTION :  A demo to test PVR play back, This application can     */
/*    be a streaming source of the application test_av                      */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*  Author    :  Lin Guo Hui                                                */
/*  File      :                                                             */
/*  Purpose   :  Test application                                           */
/*  Changes   :                                                             */
/*  Date         Comments                                                   */
/*  ----------------------------------------------------------------------  */
/*  10-Oct-01  Created LGH                                                  */
/****************************************************************************/
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>

#include <xp/xp_osd_user.h>

#define BUFFER_SIZE     1024*30

int main(int argc, char * argv[])
{
  int fd;             //handle of PVR device
  int fd_data;

  unsigned char data[BUFFER_SIZE];

  int len = 0;
  int i;

  int flag;
  int length;
  char buf[20];
  struct stat filestat;
  int smallfile = 0;

  char filename[80];

  if(argc <2)
  {
    printf("Please using command filename\n");
    return -1;
  }

  strcpy(filename,argv[1]);     //get the TS data file name

  if(0 != stat(filename,&filestat))
  {
    printf("Cant stat file %s\n",filename);
    return -1;
  }

  flag = fcntl(0,F_GETFL);      //change the standard INPUT device(0) to
  flag |= O_NONBLOCK;               //be non-block
  fcntl(0,F_SETFL,flag);


  if ((fd = open("/dev/pvr", O_RDWR)) < 0)
  {
    printf("Error open PVR\n");
    return -1;
  }

  if ((fd_data = open(filename, O_RDONLY)) < 0)
  {
    printf("Error open data file\n");
    close(fd);
    return -1;
  }

  while(1)
  {
    //if user press 'q', break the app
    length = read(0,buf,20);
    if(length > 0 && buf[0] == 'q')
      break;
    if(smallfile == 0)
    {
      len = read(fd_data, data, BUFFER_SIZE);
      if(len == filestat.st_size)
        smallfile = 1;
    }

    if(len > 0)
    {
//      printf("len = %d\n",len);
      write(fd, data, len);     //write data to PVR device
    }
    else                            //if end of file, restart
    {
      lseek(fd_data,0,SEEK_SET);
    }
  }

  close(fd);
  close(fd_data);
  return 0;
}
