/*
  test_pvr_nb [mode={line|word}]

  If mode equals line then the demux pvr is configured for line mode which DMA's in 32
  byte bursts. If mode equals word then the demux is configured for word mode which
  DMA's in 4 ytes bursts. Default mode is line mode.

  This test program will ask for an input filename, an audio pid, a video pid and the
  video mode (NTSC or PAL). It will read the file into memory and DMA the data to the
  demux. It will also setup the audio and video channels to display the specified audio
  and video data.

  NOTE: audio/video synchronization is not enabled for this test.

*/
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
|       COPYRIGHT   I B M   CORPORATION 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sched.h>
#include <sys/poll.h>
#include <aud/aud_inf.h>
#include <vid/vid_inf.h>
#include <xp/xp_osd_user.h>
#include <xp/pvr_osd.h>
#include "mysem.c"

int fd_pvr=0;                  //handle of PVR device
char stream_filename[256];
char inps[256];

int pvrsem_id_E = 0;
int pvrsem_id_F = 0;
#define nbufs 4
#define BUFFER_SIZE     1024*128

struct {
         int len;
         unsigned char *data;
       } buf[nbufs];


static	pthread_t thread;
static	pthread_attr_t attr;
static	struct sched_param sched;
static	pthread_t tid_pvrsend;
static	pthread_t tid_pvrwrite;

static int fd_data = 0;
static int pvrmode = PLAY_LINE;

static int pvrsend(char *);
static int pvrwrite(void *);
static int start_pes_filter(int fd, unsigned short pid, int type);

int main(int argc, char **argv)
{
  int fda=0;                    //handle of audio decoder
  int fdv=0;                    //handle of video decoder
  int fd_v=0;                   //handle of video demux filter
  int fd_a=0;                   //handle of audio demux filter
  int i;
  int vpid;                     //video pid
  int apid;                     //audio pid
  int vidmode;                  //video mode (PAL or NTSC)
  int dummy;

  // parse input parameters

  for(i=0;i<argc;i++)
  {
    if(strncasecmp(argv[i],"mode=line",9) == 0)
      pvrmode=PLAY_LINE;
    else if(strncasecmp(argv[i],"mode=word",9) == 0)
      pvrmode=PLAY_WORD;
  }

  write(0,"Enter stream file name: ",24);
  read(0,inps,sizeof(inps));
  sscanf(inps,"%s",stream_filename);
  if ((fd_data = open(stream_filename, O_RDONLY)) < 0)
  {
    printf("Error opening file %s\n",stream_filename);
    return -1;
  }

  write(0,"Enter video pid(hex): ",22);
  read(0,inps,sizeof(inps));
  sscanf(inps,"%x",&vpid);

  write(0,"Enter audio pid(hex): ",22);
  read(0,inps,sizeof(inps));
  sscanf(inps,"%x",&apid);

  write(0,"PAL or NTSC ? p|n: ",19);
  read(0,inps,sizeof(inps));
  if(inps[0] == 'p')
    vidmode = DENC_MODE_PAL;
  else
    vidmode = DENC_MODE_NTSC;

  //open video decoder
  if ((fdv = open("/dev/vdec_dev", O_RDWR)) < 0)
  {
    printf("Open Video Fail\n");
    return -1;
  }

  //set video mode
  if(ioctl( fdv, MPEG_VID_SET_DISPFMT, vidmode) < 0)
    printf("\nMPEG_VID_SET_DISPFMT failed\n");

  //Set video source = demux
  if( ioctl( fdv, MPEG_VID_SELECT_SOURCE, 0) < 0)
  {
    printf("video clip mode init error\n");
    return -1;
  }

  //initially blank video
  ioctl(fdv, MPEG_VID_SET_BLANK, 0);

  //open audio decoder
  if ((fda = open("/dev/adec_dev", O_RDWR)) < 0)
  {
    printf("Open Audio Fail\n");
    return -1;
  }

  //Set audio source = demux
  if( ioctl( fda, MPEG_AUD_SELECT_SOURCE, 0) < 0)
  {
    printf("audio clip mode init error\n");
    return -1;
  }
  
  //set audio decode type to pes
  if( ioctl( fda, MPEG_AUD_SET_STREAM_TYPE, AUD_STREAM_TYPE_PES) < 0)
  {
    printf("select audio format error\n");
    return -1;
  }

  //open demux filter for video
  if ((fd_v = open("/dev/demuxapi0", O_RDONLY)) < 0)
  {
    printf("Open video demux channel failed\n");
    return -1;
  }
  
  //open demux filter for audio
  if ((fd_a = open("/dev/demuxapi0", O_RDONLY)) < 0)		//open a audio PES filter
  {
    printf("Error open demuxapi\n");
    return -1;
  }

  //start video decoder
  ioctl(fdv, MPEG_VID_PLAY, 0);
  
  //start audio decoder
  ioctl(fda, MPEG_AUD_PLAY, 0);

  //disable video sync
  ioctl(fdv, MPEG_VID_SYNC_ON, 0);

  //disable audio sync
  ioctl(fdv, MPEG_AUD_SYNC_ON, 0);

  //open pvr device
  if ((fd_pvr = open("/dev/pvr", O_RDWR)) < 0)
  {
    printf("Error open PVR\n");
    return -1;
  }

  //set pvr mode (WORD or LINE)
  if ((ioctl(fd_pvr,PVR_SET_MODE ,pvrmode)) < 0)
  {
    printf("Error setting PVR mode\n");
    return -1;
  }

  //allocate memory buffers for data
  for(i = 0; i < nbufs; i++)
  {
    PVR_ALLOCATE_BUFFER_PARAM param;

    param.len = buf[i].len = BUFFER_SIZE;
    if(0 !=ioctl(fd_pvr, PVR_ALLOCATE_BUFFER, &param))
    {
      printf("PVR_ALLOCATE_BUFFER failed\n");
      return(-1);
    }
    buf[i].data = param.vaddr;
  }

  //create the empty buffer semaphore
  pvrsem_id_E = CreateSemaphore(nbufs);
  if(pvrsem_id_E == -1)
  {
    printf("CreateSemaphore(2) error\n");
    return(-1);
  }

  //create the full buffer semaphore
  pvrsem_id_F = CreateSemaphore(0);
  if(pvrsem_id_F == -1)
  {
    printf("CreateSemaphore(0) error\n");
    return(-1);
  }

  //start the demux video filter
  if(start_pes_filter(fd_v,vpid,DMX_PES_VIDEO))
  {
    printf("start_video failed\n");
    return -1;
  }
  
  //start the demux audio filter
  if(start_pes_filter(fd_a,apid,DMX_PES_AUDIO))
  {
    printf("start_video failed\n");
    return -1;
  }

  //create a separate thread for reading data from the file into the empty buffers.
  pthread_attr_init(&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_RR);
  sched.sched_priority=0;
  pthread_attr_setschedparam(&attr, &sched);
  pthread_create(&tid_pvrsend,(void *)&attr,(void *)pvrsend,NULL);

  //create a separate thread for DMA'ing full buffers to the demux
  pthread_create(&tid_pvrwrite,(void *)&attr,(void *)pvrwrite,NULL);

  while(1)
  {

    write(0,"PRESS ENTER TO QUIT ",20);
    read(0,inps,20);


    pthread_cancel(tid_pvrsend);
    pthread_join(tid_pvrsend,NULL);

    pthread_cancel(tid_pvrwrite);
    pthread_join(tid_pvrwrite,NULL);

    break;
  }
  ioctl(fd_v,DEMUX_STOP,0);
  ioctl(fd_a,DEMUX_STOP,0);
exit:
  if(fdv)
    close(fdv);
  if(fda)
    close(fda);
  if(fd_v)
    close(fd_v);
  if(fd_a)
    close(fd_a);

  if(fd_data)
    close(fd_data);
  return 0;
}

int pvrsend(char *p)
{
  int len = 0;
  int i;
  int flag;

  //enable the thread to be cancelled
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  //initial empty buffer index = 0
  i = 0;
  while(1)
  {
    //wait for an empty buffer
    SemWait(pvrsem_id_E);
    //fill empty buffer from the file
    buf[i].len = read(fd_data, buf[i].data, BUFFER_SIZE);

    if(buf[i].len > 0)
    {
      //post the full buffer to pvrwrite
      SemPost(pvrsem_id_F);
      i=i+1;
      if(i >= nbufs)
        i = 0;
    }
    else
    {
      //wrap from end of file to beginning
      lseek(fd_data,0,SEEK_SET);
      SemPost(pvrsem_id_E);
    }
  }

  return 0;
}

int pvrwrite(void *p)
{
  int i;
  int len;
  PVR_WRITE_BUFFER_PARAM wp;

  //enable the thread to be cancelled
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  //initial full buffer index = 0
  i = 0;
  while(1)
  {
    //wait for a full buffer
    SemWait(pvrsem_id_F);
    wp.vaddr = buf[i].data;
    wp.len = buf[i].len;
 
    //call PVR device to DMA buffer into demux
    len=ioctl(fd_pvr,PVR_WRITE_BUFFER,&wp);

    //post the now empty buffer
    SemPost(pvrsem_id_E);
    i = i+1;
    if(i >= nbufs)
      i = 0;
  }

  return 0;
}


/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int start_pes_filter(int fd, unsigned short pid, int type)
{
  Pes_para PesPara;

  PesPara.pid = pid;
  PesPara.output = OUT_DECODER;
  PesPara.pesType = type;

  if (ioctl(fd, DEMUX_FILTER_PES_SET, &PesPara) < 0)
  {
    printf("Error set PES Filter\n");
    return(-1);
  }

  if (ioctl(fd, DEMUX_START) < 0)
  {
    printf("Error Start PES Filter\n");
    return(-1);
  }

  return(0);
}





