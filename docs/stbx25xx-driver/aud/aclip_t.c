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
| Author: Ling shao
| File:   aud_t.c
| Purpose: audio driver test PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#include <sys/ioctl.h>
#include <stdio.h>

#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/poll.h>

#include <aud/aud_inf.h>


#include <string.h>

#include <sys/mman.h>
#include "od_bachs.h"

#define AUD_MAP_LEN         3*(32*1024)     
static unsigned long aud_app_write(int fd, void* base, void *src, unsigned long ulLen);
static int fplay(void *dummy);

unsigned char tbuf[32];
unsigned char data_buf[65536];
unsigned char method = 1;
int fd_data = -1;
int fda = -1;
void* base = NULL;
int exitflag=0;
int vexitflag=0;
pthread_t thread;
pthread_attr_t attr;
struct sched_param sched;

int main(int argc, char **argv)
{

  char c;
  int n;
  CLIPINFO info;

  /* init the audio to mpeg*/

  printf("open audio MPEG\n");
  if((fda = open("/dev/aud_mpeg",O_RDWR)) <0)
  {
    printf("Open clip_dev fail\n");
    goto exit;
  }
  ioctl(fda, MPEG_AUD_SELECT_SOURCE, 1);
  ioctl(fda, MPEG_AUD_SET_STREAM_TYPE, AUD_STREAM_TYPE_ES);
  ioctl(fda, MPEG_AUD_PLAY, 0);
  ioctl(fda, MPEG_AUD_SET_MUTE, 0);

  for(n = 1; n < argc; n++)
  {
    if(argv[n][0] == '-')
    {
      if(argv[n][1] == 'h' || argv[n][1] == 'H')
      {
        printf("\nhelp: aud_t [1|2] [filename]\n");
        printf("1- write, 2- mmap write\n");
        printf("filename - mpeg audio file\n");
        goto exit;
      }
      else
      {
        printf("Invalid option\n");
        goto exit;
      }
      continue;
    }

    if(strcmp(argv[n], "1") == 0)
      method = 1;
    else if(strcmp(argv[n],"2") == 0)
    {
      method = 2;
      base = mmap(NULL, AUD_MAP_LEN, PROT_WRITE, MAP_SHARED, fda, 0);
      if(base == NULL)
      {
        printf("mmap failed\n");
        goto exit;
      }
      printf("mmap OK = 0x%8.8x\n",(unsigned long)base);
    }
    else 
    {  
      if(fd_data > 0)
      {
        printf("Invalid argument\n");
        goto exit;
      }

      if((fd_data = open(argv[n], O_RDONLY)) < 0)
      {
        printf("File %s not found\n",argv[n]);
        goto exit;
      }
    }
  }
  
  if(fd_data > 0)
  {
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    sched.sched_priority=0;
    pthread_attr_setschedparam(&attr, &sched);
    pthread_create(&thread,(void *)&attr,(void *)fplay,(void *)NULL);
  }
  else 
  {
    if(method == 1)
    {
      if(write(fda,a_clip_data,a_clip_len)!= a_clip_len)
      {
        printf("write clip data error\n");
        goto exit;
      }
    }
    else 
    {
      aud_app_write(fda, base, (void*)a_clip_data, a_clip_len);
    }
  }
  printf("playing...\n");
  printf("press any key to exit\n");
  read(0,tbuf,20);

exit:
  if(fda > 0)
    close(fda);
  if(fd_data > 0)
    close(fd_data);
  if(base != NULL)
  {
    if(munmap(base, AUD_MAP_LEN) < 0)
      printf("unmap error\n");
  }
  return (0);
}

static unsigned long aud_app_write(int fd, void* base, void *src, unsigned long ulLen)
{
    unsigned long cur_len = 0, offset = 0;
    unsigned int  ret;

    CLIPINFO  info;

    while(ulLen)
    {
        if( ioctl(fd, MPEG_AUD_GET_BUF_WAIT, &info) < 0)
        {
            printf("get empty info error\n");
            return offset;
        }
        if(ulLen > info.ulBufLen)
            cur_len = info.ulBufLen;
        else
        {
            cur_len = ulLen;
        }

        memcpy(base + info.ulBufAdrOff, src + offset, cur_len);

        info.uClipAdrOff = info.ulBufAdrOff;
        info.uClipLen = cur_len;
        
        if(ioctl(fd, MPEG_AUD_CLIP_WRITE, &info) < 0)
        {
            printf("clip write error\n");
            return offset;
        }
        offset += cur_len;
        ulLen -= cur_len;
    }
    return offset;
}

static int fplay(void *dummy)
{
    int rc;
    int count;

    dummy = 0;
    while(vexitflag==0)
    {
      rc = read(fd_data,data_buf, sizeof(data_buf));
      if(rc <= 0)
      {
        printf("read file = %d\n", rc);
        break;
      }
      count = rc;


      if(method == 1)
      {
        if(write(fda,data_buf,count)!= count)
        {
          printf("write clip data error\n");
          break;
        }
      }
      else 
      {
        if(aud_app_write(fda, base, data_buf, count) != count)
        {
          printf("write clip data error\n");
          break;
        }
      }
    }
    exitflag = 1;
    printf("fplay: exit\n");
    return(0);
}
