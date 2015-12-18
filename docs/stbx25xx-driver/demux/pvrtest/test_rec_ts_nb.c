/*
     test_rec_ts [/dmxid=n]

     dmxid     - 0 - demux 0  [default]
                 1 - demux 1
                 2 - demux 2

     This program will record the entire transport stream from the selected
     demux to the file ts_temp. The TS filter will be set to non-buffered
     mode.
*/
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
+---------------------------------------------------------------------------*/
/****************************************************************************/
/*                                                                          */
/*    DESCRIPTION :  A demo to record MPEG TS stream to HD                  */
/****************************************************************************/

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fcntl.h>
#include <time.h>
#include <sys/poll.h>
#include <errno.h>
#include <signal.h>

#include <xp/xp_osd_user.h>
#include "os/os-types.h"

static unsigned char *pbuf=NULL;
static unsigned qsize=0x100000;
static int fd=0;
static int fd_data=0;
static char inps[128];
static unsigned filesize = 0;
static unsigned long recording = 1;
static void sgh(int s);

int main(int argc, char * argv[])
{
  int rc;
  queue_para qpara;
  int len = 0;
  char buf[20];
  char filename[80];
  struct sigaction sa;
  int oflags;
  int i;

  unsigned device= 0;

  for(i = 1; i < argc; i++)
  {
    if(0 == strncasecmp(argv[i],"/dmxid=",7))
    {
      if(1 != sscanf(argv[i]+7,"%d",&device))
      {
        printf("Invalid dmxid\n");
        return(-1);
      }
      if(device > 2)
      {
        printf("Invalid dmxid\n");
        return(-1);
      }
    }
    else
      goto help;
  }

// open demux device

  switch(device)
  {
    case 0:
        if ((fd = open("/dev/demuxapi0", O_RDONLY|O_NONBLOCK)) < 0)
        {
          printf("Error open demuxapi0\n");
          return -1;
        }
        break;
    case 1:
        if ((fd = open("/dev/demuxapi1", O_RDONLY|O_NONBLOCK)) < 0)
        {
          printf("Error open demuxapi1\n");
          return -1;
        }
        break;
    case 2:
        if ((fd = open("/dev/demuxapi2", O_RDONLY|O_NONBLOCK)) < 0)
        {
          printf("Error open demuxapi2\n");
          return -1;
        }
        break;
    default:
        printf("Error: invalid device index\n");
        return -1;
  }

// create output file

  sprintf(filename,"ts_temp");
  
  if ((fd_data = open(filename,O_CREAT|O_TRUNC|O_RDWR)) < 0)
  {
    printf("Error creating output file - %s\n",filename);
    goto exit;
  }

  printf("Output filename = %s\n",filename);

// Install signal handler to be called when data is available

  sa.sa_handler = sgh;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGIO, &sa, 0) == -1)
  {
    printf("Could not install signal handler\n");
    return(-1);
  }

// Set ownership of demux device for asynchronous notification

  fcntl(fd, F_SETOWN, getpid());
  oflags = fcntl(fd, F_GETFL);

// Set asynchronous mode

  fcntl(fd, F_SETFL, oflags | FASYNC);

// set demux device in non-buffered mode

  if(ioctl(fd, DEMUX_FILTER_SET_FLAGS, FILTER_FLAG_NONBUFFERED) < 0)
  {
    printf("DEMUX_FILTER_SET_FLAGS failed\n");
    goto exit;
  }

// Set demux buffer size

  if(ioctl(fd, DEMUX_SET_BUFFER_SIZE, qsize))
  {
    printf("Error set buffer size\n");
    goto exit;
  }

// Set the demux filter type to TS
// BTI fixed for this filter type at 255

  if (ioctl(fd, DEMUX_FILTER_TS_SET) < 0)
  {
    printf("Error set TS Filter\n");
    goto exit;
  }

// mmap the demux filter into application address space

  pbuf = mmap(NULL, qsize, PROT_READ, MAP_SHARED+MAP_LOCKED, fd, 0);
  if(pbuf == NULL || pbuf == (void *)-1)
  {
    printf("Error %d mapping queue\n",errno);
    goto exit;
  }

// start the demux TS filter

  if (ioctl(fd, DEMUX_START) < 0)
  {
    printf("Error Start Filter\n");
    goto exit;
  }

  while(recording)
  {
    write(0,"PRESS ENTER TO QUIT:",20);
    len = read(0,buf,20);
    recording = 0;
    break;
  }

// stop the demux filter

  if (ioctl(fd, DEMUX_STOP) < 0)
    printf("Error Stop Filter\n");

  printf("%d bytes written to %s\n",filesize,filename);
exit:
  if(fd)
    close(fd);
  if(fd_data)
    close(fd_data);
  return 0;
  
help:
  printf("command error:\n");
  printf("    test_rec_ts_nb /dmxid=n\n");
  printf("                          n = 0 ---------- XP0 (default)\n");
  printf("                              1 ---------- XP1\n");
  printf("                              2 ---------- XP2\n");
  return 0;
}

static void sgh(int s)
{
  int tslen;
  int rc;
  queue_para qpara;
  unsigned long n;

  // Demux data is available, get the read/write queue offsets

  rc = ioctl(fd, DEMUX_FILTER_GET_QUEUE,&qpara);
  if(rc != 0)
  {
    printf("Error %d from DEMUX_FILTER_GET_QUEUE\n");
    printf("readptr = %x writeptr = %x\n",qpara.readptr,qpara.writeptr);
    return;
  }


  if(recording != 0)
  {

    if(qpara.writeptr < qpara.readptr)
    {
    // if data wraps the end of the queue
    // write from current read offset to end of queue

      tslen = qsize - qpara.readptr;
      rc = write(fd_data, pbuf+qpara.readptr, tslen);
      if(rc != tslen)
      {
        printf("Error writing to file\n");
        recording = 0;
        return;
      }
      else
        filesize += tslen;
      tslen = qpara.writeptr;
      if(tslen)
      {
        // write from beginning of queue to current write offset

        rc = write(fd_data, pbuf, qpara.writeptr);
        if(rc != tslen)
        {
          printf("Error writing to file\n");
          recording = 0;
          return;
        }
        else
          filesize += tslen;
      }
    }
    else
    {
    // data does not wrap end of queue
    // write data from current read offset to current write offset

      tslen = qpara.writeptr - qpara.readptr;
      rc = write(fd_data, pbuf+qpara.readptr, tslen);
      if(rc != tslen)
      {
        printf("Error writing to file\n");
        recording = 0;
        return;
      }
      else
        filesize += rc;
    }
  }

  // update current read offset

  rc = ioctl(fd, DEMUX_FILTER_SET_READPTR, qpara.writeptr);
  if(rc != 0)
    printf("%d = DEMUX_FILTER_SET_READPTR\n",rc);

  return;

}


