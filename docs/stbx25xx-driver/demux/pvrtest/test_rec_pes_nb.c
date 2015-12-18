/*
     test_rec_ps /dmxid=n /dmxci=n /vid_bti=n /aud_bti=n /vid_qsize=n /aud_qsize=n

       dmxid        0, 1 or 2
                    default=0

       dmxci        0-CI0  1-CI1
                    default=0

       vid_bti      number of 256 byte blocks for threshold interrupt
                    default=128

       aud_bti      number of 256 byte blocks for threshold interrupt
                    default=4

       vid_qsize    filter buffer size in bytes. Must be multiple of 4096.
                    default=65536.

       aud_qsize    filter buffer size in bytes. Must be multiple of 4096.
                    default=4096.

     Record audio/video PES packets from input transport stream to file.
     The audio PES data is written to aud_temp.pes and the video PES data 
     is written to vid_temp.pes .

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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+---------------------------------------------------------------------------*/
/****************************************************************************/
/*                                                                          */
/*    DESCRIPTION :  A demo to record MPEG PES stream to HD                 */
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
#include <pthread.h>
#include <sched.h>

#include <xp/xp_osd_user.h>
#include "os/os-types.h"

static  pthread_t thread;
static  pthread_attr_t attr;
static  struct sched_param sched;

typedef struct {
                 pid_t pid;
                 int fd;
                 char *xpdev;
                 int fd_data;
                 char *filename;
                 unsigned short xpid;
                 int qsize;
                 int bti;
                 unsigned char *pbuf;
		 unsigned count;
               } rec_s;


static char inps[128];
static char xpdev[80];
static int recording=1;

static unsigned dmxid=0;
static unsigned dmxci=0;
static char filename_aud[] = "aud_temp.pes";
static char filename_vid[] = "vid_temp.pes";
static rec_s aud_rec = {0,0,xpdev,0,filename_aud,0,0x1000,4,NULL,0};
static rec_s vid_rec = {0,0,xpdev,0,filename_vid,0,0x10000,128,NULL,0};

static void sgh(int s);
int rectask(rec_s *rec);

/******************************************************************************/
/******************************************************************************/
/* Main                                                                       */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
int main(int argc, char * argv[])
{
  int i;


  for(i = 1; i < argc; i++)
  {
    if(0 == strncasecmp(argv[i],"/dmxid=",7))
    {
      if(1 != sscanf(argv[i]+7,"%d",&dmxid))
      {
        printf("Invalid dmxid\n");
        return(-1);
      }
      if(dmxid > 2)
      {
        printf("Invalid dmxid\n");
        return(-1);
      }
    }
    else if(0 == strncasecmp(argv[i],"/dmxci=",7))
    {
      if(1 != sscanf(argv[i]+7,"%d",&dmxci))
      {
        printf("Invalid dmxci\n");
        return(-1);
      }
      if(dmxci > 1)
      {
        printf("Invalid dmx source selected\n");
        return(-1);
      }
    }
    else if(0 == strncasecmp(argv[i],"/vid_bti=",9))
    {
      if(1 != sscanf(argv[i]+9,"%d",&vid_rec.bti))
      {
        printf("Invalid vid_bti\n");
        return(-1);
      }
    }
    else if(0 == strncasecmp(argv[i],"/aud_bti=",9))
    {
      if(1 != sscanf(argv[i]+9,"%d",&aud_rec.bti))
      {
        printf("Invalid aud_bti\n");
        return(-1);
      }
    }
    else if(0 == strncasecmp(argv[i],"/vid_qsize=",11))
    {
      if(1 != sscanf(argv[i]+11,"%d",&vid_rec.qsize))
      {
        printf("Invalid vid_qsize\n");
        return(-1);
      }
      if(vid_rec.qsize == 0 || (vid_rec.qsize%4096))
      {
        printf("Invalid vid_qsize - must be multiple of 4096\n");
        return(-1);
      }
    }
    else if(0 == strncasecmp(argv[i],"/aud_qsize=",11))
    {
      if(1 != sscanf(argv[i]+11,"%d",&aud_rec.qsize))
      {
        printf("Invalid aud_qsize\n");
        return(-1);
      }
      if(aud_rec.qsize == 0 || (aud_rec.qsize%4096))
      {
        printf("Invalid aud_qsize - must be multiple of 4096\n");
        return(-1);
      }
    }
    else if(0 == strncasecmp(argv[i],"/h",2))
      goto help;
  }
  if(vid_rec.qsize < (vid_rec.bti*256))
  {
    printf("vid_bti >= vid_qsize ?\n");
    return(-1);
  }
  if(aud_rec.qsize < (aud_rec.bti*256))
  {
    printf("aud_bti >= aud_qsize ?\n");
    return(-1);
  }

  printf("Audio output file = %s\n",filename_aud);
  printf("Video output file = %s\n",filename_vid);

  sprintf(xpdev,"%s%c","/dev/demuxapi",'0'+dmxid);

  write(0,"Enter video pid (in hex): ",26);
  read(0,inps,sizeof(inps));
  sscanf(inps,"%hx",&vid_rec.xpid);

  write(0,"Enter audio pid (in hex): ",26);
  read(0,inps,sizeof(inps));
  sscanf(inps,"%hx",&aud_rec.xpid);

  pthread_attr_init(&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_RR);
  sched.sched_priority=0;
  pthread_attr_setschedparam(&attr, &sched);
  pthread_create(&thread,(void *)&attr,(void *)rectask,&aud_rec);
  pthread_create(&thread,(void *)&attr,(void *)rectask,&vid_rec);

  while(1)
  {

    write(0,"PRESS ENTER TO QUIT \n",20);
    read(0,inps,20);
    break;
  }

  if(aud_rec.fd)
  {
    if (ioctl(aud_rec.fd, DEMUX_STOP) < 0)
      printf("Error Stopping audio Filter\n");
    close(aud_rec.fd);
  }

  if(vid_rec.fd)
  {
    if (ioctl(vid_rec.fd, DEMUX_STOP) < 0)
      printf("Error Stopping video Filter\n");
    close(vid_rec.fd);
  }

  if(aud_rec.pid)
  {
    kill(aud_rec.pid, SIGTERM);
  }

  if(vid_rec.pid)
  {
    kill(vid_rec.pid, SIGTERM);
  }

  printf("%d bytes written to %s\n",aud_rec.count,filename_aud);
  printf("%d bytes written to %s\n",vid_rec.count,filename_vid);
exit:
  if(aud_rec.fd_data)
    close(aud_rec.fd_data);
  if(vid_rec.fd_data)
    close(vid_rec.fd_data);
  return 0;

help:
  printf("test_rec_pes /dmxid=n /dmxci=n /vid_bti=n /aud_bti=n /vid_qsize=n /aud_qsize=n\n");
  return 0;
}

/******************************************************************************/
/******************************************************************************/
/* Separate process for each channel to be recorded so that the signal handler*/
/* can ID which channel needs service by unique process IDs.                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
int rectask(rec_s *rec)
{
  struct sigaction sa;
  int oflags;
  Pes_para PesPara;

  rec->pid = getpid();

  if ((rec->fd = open(rec->xpdev, O_RDONLY|O_NONBLOCK)) < 0)
  {
    printf("Error open %s\n",rec->xpdev);
    return -1;
  }

  if ((rec->fd_data = open(rec->filename,O_CREAT|O_TRUNC|O_RDWR)) < 0)
  {
    printf("Error open %s\n",rec->filename);
    return -1;
  }

  sa.sa_handler = sgh;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGIO, &sa, 0) == -1)
  {
    printf("Could not install signal handler\n");
    return(-1);
  }

  fcntl(rec->fd, F_SETOWN, rec->pid);
  oflags = fcntl(rec->fd, F_GETFL);
  fcntl(rec->fd, F_SETFL, oflags | FASYNC);


  if(ioctl(rec->fd, DEMUX_FILTER_SET_FLAGS, FILTER_FLAG_NONBUFFERED) < 0)
  {
    printf("DEMUX_FILTER_SET_FLAGS failed\n");
    return(-1);
  }


  if(ioctl(rec->fd, DEMUX_SET_BUFFER_SIZE, rec->qsize))
  {
    printf("Error set buffer size\n");
    return(-1);
  }

  PesPara.pid = rec->xpid;
  PesPara.output = OUT_MEMORY;
  PesPara.unloader.threshold = rec->bti;
  PesPara.unloader.unloader_type = UNLOADER_TYPE_PAYLOAD;
  PesPara.pesType = DMX_PES_OTHER;

  if (ioctl(rec->fd, DEMUX_FILTER_PES_SET,&PesPara) < 0)
  {
    printf("Error set PES Filter\n");
    return(-1);
  }

  rec->pbuf = mmap(NULL, rec->qsize, PROT_READ, MAP_SHARED | MAP_LOCKED, rec->fd, 0);
  if(rec->pbuf == NULL || rec->pbuf == (void *)-1)
  {
    printf("Error %d mapping queue\n",errno);
    return(-1);
  }

  if (ioctl(rec->fd, DEMUX_START) < 0)
  {
    printf("Error Start Filter\n");
    return(-1);
  }

  while(1)
    sleep(5);

  return(0);
}

/******************************************************************************/
/******************************************************************************/
/* Signal Handler                                                             */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
static void sgh(int s)
{
  pid_t pid;
  int tslen;
  int rc;
  queue_para qpara;
  rec_s *rec;

  pid = getpid();
  if(pid == aud_rec.pid)
    rec = &aud_rec;
  else if(pid == vid_rec.pid)
    rec = &vid_rec;
  else
  {
    printf("Invalid signal received\n");
    return;
  }

  rc = ioctl(rec->fd, DEMUX_FILTER_GET_QUEUE,&qpara);
  if(rc != 0)
  {
    printf("Error %d from DEMUX_FILTER_GET_QUEUE\n");
    printf("readptr = %x writeptr = %x\n",qpara.readptr,qpara.writeptr);
    return;
  }

  if(recording)
  {
    if(qpara.writeptr < qpara.readptr)
    {
      tslen = rec->qsize - qpara.readptr;

      rc = write(rec->fd_data, rec->pbuf+qpara.readptr, tslen);
      if(rc != tslen)
      {
        printf("Error writing to file\n");
        recording = 0;
        return;
      }
      rec->count += tslen;
      tslen = qpara.writeptr;
      if(tslen)
      {

        rc = write(rec->fd_data, rec->pbuf, tslen);
        if(rc != tslen)
        {
          printf("Error writing to file\n");
          recording = 0;
          return;
        }
        rec->count += tslen;
      }
    }
    else
    {
      tslen = qpara.writeptr - qpara.readptr;

      rc = write(rec->fd_data, rec->pbuf+qpara.readptr, tslen);
      if(rc != tslen)
      {
        printf("Error writing to file\n");
        recording = 0;
        return;
      }
      rec->count += tslen;
    }
  }
  rc = ioctl(rec->fd, DEMUX_FILTER_SET_READPTR, qpara.writeptr);
  if(rc != 0)
    printf("%d = DEMUX_FILTER_SET_READPTR\n",rc);

  return;

}

