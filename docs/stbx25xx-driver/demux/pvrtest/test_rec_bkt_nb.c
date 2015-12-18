/*
     test_rec_bkt [/dmxid=n] [/dmxci=n] [/bti=n] [/qsize=n]

       dmxid        0, 1 or 2
                    default=0

       dmxci        0-CI0  1-CI1 
                    default=0

       bti          number of 256 byte blocks for threshold interrupt
                    default=128

       qsize        filter buffer size in bytes. Must be multiple of 4096.
                    default=65536.

     Record audio/video transport packets from the input transport stream to a 
     file using bucket queue 1. The program will ask for the audio/vide pids to
     be recorded. The audio and video transport packets are written to the output
     file bkt_temp.

     Bucket queue 1 is set up to receive the audio/video transport packets. The
     audio and video filters are set up to write their packets into the bucket
     queue. The bucket queue is configured to invoke the signal handler when 
     bti*256 bytes have been written to the queue. The signal handler will then 
     call the driver to get the read/write queue offsets. Using the queue pointer
     obtained from calling mmap and the read/write offsets the data is then
     written to the output file. Then the driver is called to update the read
     pointer. The signal handler will be called again after another bti*256 bytes
     have been written to the queue.
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
+----------------------------------------------------------------------------*/
/****************************************************************************/
/*                                                                          */
/*    DESCRIPTION :  test audio and video recording using the bucket queue
/****************************************************************************/
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/poll.h>
#include <errno.h>

#include <aud/aud_inf.h>
#include <vid/vid_inf.h>

#include <xp/xp_osd_user.h>
//#include "test_rec.h"

static unsigned dmxid=0;
static unsigned dmxci=0;
static unsigned bti=128;
static unsigned qsize = 256*256;

static int recording=1;
static unsigned char inps[256];
static unsigned char dmx_dev[] = "/dev/demuxapi_   ";
static unsigned char ts_filename[16];
static unsigned char *pbktq = 0;
static int fd_bkt=0;
static int filesize = 0;
static int tsfile;
static int vpid;
static int apid;

static int start_filter(int fd, int pid, PesType ptype);
static int _ts_rec(unsigned dmxid, unsigned source);
static void sgh(int s);
static void pat_sgh(int s);


/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int main(int argc, char **argv)
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
    else if(0 == strncasecmp(argv[i],"/bti=",5))
    {
      if(1 != sscanf(argv[i]+5,"%d",&bti))
      {
        printf("Invalid bti\n");
        return(-1);
      }
    }
    else if(0 == strncasecmp(argv[i],"/qsize=",7))
    {
      if(1 != sscanf(argv[i]+7,"%d",&qsize))
      {
        printf("Invalid qsize\n");
        return(-1);
      }
      if(qsize == 0 || (qsize%4096))
      {
        printf("Invalid qsize - must be multiple of 4096\n");
        return(-1);
      }
    }
    else 
      goto help;
  }
  if(qsize < (bti*256))
  {
    printf("BTI >= qsize ?\n");
    return(-1);
  }
  write(0,"Enter video pid(in hex): ",25);
  read(0,inps,sizeof(inps));
  sscanf(inps,"%x",&vpid);
  write(0,"Enter audio pid(in hex): ",25);
  read(0,inps,sizeof(inps));
  sscanf(inps,"%x",&apid);
  _ts_rec(dmxid,dmxci);
  return(0);

help:
  printf("rec_bkt [/dmxid=n] [/dmxci=n] [/bti=n] [/qsize=n]\n");
  return(0);
}

/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*                                                                          */
/****************************************************************************/
static int _ts_rec(unsigned dmxid, unsigned dmxci)
{
  int fd_a=0;
  int fd_v=0;
  int len = 0;
  int i;
  int n;
  int Program_Count = 0;
  int length;
  int Pnumber=0;
  int tslen;
  char key;
  int rc = 0;
  bucket_para para;
  struct sigaction sa;
  int oflags;

  if(dmxid > 2)
  {
    printf("Invalid demux id\n");
    return -1;
  }

  switch(dmxci)
  {
    case INPUT_FROM_CHANNEL0:
    case INPUT_FROM_CHANNEL1:
      break;

    default:
      printf("Invalid demux source selected\n");
      return -1;

  }

  sprintf(dmx_dev,"%s%c","/dev/demuxapi",'0'+dmxid);

  // set up video pid filter
  if ((fd_v = open(dmx_dev, O_RDONLY|O_NONBLOCK)) < 0)
  {
    printf("Error open demuxapi\n");
    goto exit;
  }

  // select demux input channel CI0, CI1, 1394 or PVR
  if(ioctl(fd_v, DEMUX_SELECT_SOURCE, dmxci) < 0)
  {
    printf("DEMUX_SELECT_SOURCE failed\n");
    goto exit;
  }

  // set up audio pid filter
  if ((fd_a = open(dmx_dev, O_RDONLY|O_NONBLOCK)) < 0)
  {
    printf("Error open demuxapi\n");
    goto exit;
  }

  // create the output file
  sprintf(ts_filename,"bkt_temp");
  if ((tsfile = open(ts_filename, O_CREAT|O_TRUNC|O_RDWR)) < 0)
  {
    printf("Create %s file Fail\n",ts_filename);
    goto exit;
  }
  printf("Output file = %s\n",ts_filename);


  // open the bucket filter
  if((fd_bkt = open(dmx_dev,O_RDWR|O_NONBLOCK)) < 0)
  {
    printf("Error open demuxapi for bucket queue\n");
    goto exit;
  }

  // set up the signal handler for asynchronous notification
  // that data is available to be written to file.
  sa.sa_handler = sgh;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGIO, &sa, 0) == -1)
  {
    printf("Could not install signal handler\n");
    goto exit;
  }

  // enable asynchronous notification
  fcntl(fd_bkt, F_SETOWN, getpid());
  oflags = fcntl(fd_bkt, F_GETFL);
  fcntl(fd_bkt, F_SETFL, oflags | FASYNC);

  // set non-buffered mode
  if(ioctl(fd_bkt, DEMUX_FILTER_SET_FLAGS, FILTER_FLAG_NONBUFFERED) < 0)
  {
    printf("DEMUX_FILTER_SET_FLAGS failed\n");
    goto exit;
  }

  // set the bucket filter buffer size
  if (ioctl(fd_bkt, DEMUX_SET_BUFFER_SIZE, qsize) < 0)
  {
    printf("Error set video buffer size\n");
    goto exit;
  }


  para.unloader.threshold = bti;
  para.unloader.unloader_type = UNLOADER_TYPE_TRANSPORT;

  // initialize the bucket filter
  if (ioctl(fd_bkt, DEMUX_FILTER_BUCKET_SET, &para) < 0)
  {
    printf("Error set bucket Filter\n");
    goto exit;
  }


  // map the circular bucket queue into user address space
  pbktq = mmap(NULL, qsize, PROT_WRITE, MAP_SHARED|MAP_LOCKED, fd_bkt, 0);
  if(pbktq == NULL || pbktq == (void *)-1)
  {
    printf("Error %d mapping bucket queue\n",errno);
    goto exit;
  }

  //start the bucket filter
  if (ioctl(fd_bkt, DEMUX_START) < 0)
  {
    printf("Error Start bucket Filter\n");
    goto exit;
  }

  // start the video filter
  if(start_filter(fd_v, vpid, DMX_PES_VIDEO))
    goto exit;
    
  // start the audio filter
  if(start_filter(fd_a, apid, DMX_PES_AUDIO))
    goto exit;

  n = 0;
  while(1)
  {
    write(0,"PRESS ENTER TO QUIT ",20);
    read(0,inps,20);
    
    ioctl(fd_bkt, DEMUX_STOP,0);

    break;
  }


exit:
  if(fd_bkt > 0)
    close(fd_bkt);
  if(fd_a > 0)
    close(fd_a);
  if(fd_v > 0)
    close(fd_v);
  if(tsfile > 0)
    close(tsfile);
  printf("%d bytes written to %s\n",filesize,ts_filename);

  return(rc);
}

/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*                                                                          */
/****************************************************************************/
static int start_filter(int fd, int pid, PesType ptype)
{
  Pes_para PesPara;


  // Set filter pid
  PesPara.pid = pid;


  // filter data goes to the bucket queue only
  PesPara.output = OUT_NOTHING;
  PesPara.pesType = DMX_PES_OTHER;
  PesPara.unloader.unloader_type = UNLOADER_TYPE_BUCKET;


  // set the filter 
  if (ioctl(fd, DEMUX_FILTER_PES_SET, &PesPara) < 0)
  {
    printf("Error set Sec Filter\n");
    return(-1);
  }

  // start the filter
  if (ioctl(fd, DEMUX_START) < 0)
  {
    printf("Error Start Filter\n");
    return(-1);
  }

  return(0);
}




/****************************************************************************/
/****************************************************************************/
/* This signal handler will be called when the bucket queue has bti*256     */
/* bytes available in the circular queue.                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
static void sgh(int s)
{
  int tslen;
  int rc;
  queue_para qpara;

  // get the circular queue read/write offsets.
  // data begins at the read offset and ends at the write offset.
  rc = ioctl(fd_bkt, DEMUX_FILTER_GET_QUEUE,&qpara);
  if(rc != 0)
  {
    printf("Error %d from DEMUX_FILTER_GET_QUEUE\n",rc);
    return;
  }
  if(recording != 0)
  {
    // check if the data wraps the end of the queue
    if(qpara.writeptr < qpara.readptr)
    {
      // write the data from the read offset to end of queue
      tslen = qsize - qpara.readptr;
      rc = write(tsfile, pbktq+qpara.readptr, tslen);
      if(rc != tslen)
      {
        printf("Error writing to file\n");
        recording = 0;
        return;
      }
      else
        filesize += tslen;
      // write the data from the begin of queue to the write offset
      tslen = qpara.writeptr;
      if(tslen)
      {
        rc = write(tsfile, pbktq, qpara.writeptr);
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
      // write data from read offset to write offset
      tslen = qpara.writeptr - qpara.readptr;
      rc = write(tsfile, pbktq+qpara.readptr, tslen);
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
  // update the queue read pointer
  rc = ioctl(fd_bkt, DEMUX_FILTER_SET_READPTR, qpara.writeptr);
  if(rc != 0)
    printf("%d = DEMUX_FILTER_SET_READPTR\n",rc);
  return;

}
