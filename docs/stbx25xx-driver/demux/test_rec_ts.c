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
/*    DESCRIPTION :  A demo to record MPEG TS stream to HD                  */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*  Author    :  Lin Guo Hui                                                */
/*  File      :                                                             */
/*  Purpose   :  Test application                                       */
/*  Changes   :                                                             */
/*  Date         Comments                                                   */
/*  ----------------------------------------------------------------------  */
/*  10-Oct-01    Created                                                    */
/****************************************************************************/
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>

#include <xp/xp_osd_user.h>

int main(int argc, char * argv[])
{

  int fd;
  int fd_data;

  unsigned char data[640000];

  int len = 0;
  int i;

  int flag;
  int length;
  char buf[20];
  char filename[80];

  int device= 0;

  if(argc<2 || argc>3)
  {
    printf("Command Syntax:\n");
    printf("  test_rec_ts <filename> <DeviceIndex> where:\n");
    printf("  DeviceIndex: 0 ---------- XP0\n");
    printf("               1 ---------- XP1\n");
    printf("               2 ---------- XP2\n");
    return -1;
  }

  flag = fcntl(0,F_GETFL);
  flag |= O_NONBLOCK;
  fcntl(0,F_SETFL,flag);

  strcpy(filename,argv[1]);

  if(argc == 3)
  {
    device = atoi(argv[2]);             //0: XP0 2: XP1 3: XP2
  }


  switch(device)
  {
    case 0:
        if ((fd = open("/dev/demuxapi0", O_RDONLY)) < 0)
        {
          printf("Error open demuxapi0\n");
          return -1;
        }
        break;
    case 1:
        if ((fd = open("/dev/demuxapi1", O_RDONLY)) < 0)
        {
          printf("Error open demuxapi1\n");
          return -1;
        }
        break;
    case 2:
        if ((fd = open("/dev/demuxapi2", O_RDONLY)) < 0)
        {
          printf("Error open demuxapi2\n");
          return -1;
        }
        break;
    default:
        printf("Error: invalid device index\n");
        return -1;
  }


  printf("after open, filename = %s\n", filename);

  if ((fd_data = open(filename,O_CREAT|O_TRUNC|O_RDWR)) < 0)
  {
    printf("Error open demuxapi\n");
    close(fd);
    return -1;
  }

  if(ioctl(fd, DEMUX_SET_BUFFER_SIZE, 640000))
    printf("Error set buffer size\n");

  if (ioctl(fd, DEMUX_FILTER_TS_SET) < 0)
    printf("Error set Sec Filter\n");

  if (ioctl(fd, DEMUX_START) < 0)
    printf("Error Start Filter\n");

  while(1)
  {
    length = read(0,buf,20);
    if(length > 0 && buf[0] == 'q')
      break;
    len = read(fd, data, 640000);
    if(len > 0)
    {
//      printf("len = %d\n",len);
      write(fd_data, data, len);
    }
  }

  if (ioctl(fd, DEMUX_STOP) < 0)
    printf("Error Stop Filter\n");

  close(fd);
  close(fd_data);

  return 0;
}



