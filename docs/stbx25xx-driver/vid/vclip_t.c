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
| File:   vid_t.c
| Purpose: video decoder test PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/mman.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#include <vid/vid_inf.h>
#include "od_vpiz2.h"
//#include "stillp_data.h"

#define VID_MAP_LEN     0x000100000 /*1M*/       
static unsigned char data_buf[VID_MAP_LEN];
static int fdv, fd_data;
static int verbose = 0;

static unsigned long vid_app_write(int fd, void* base, void *src, 
                            unsigned long ulLen, int eos);
static int write_stillp(char* data, int size);
static void help(void);

int main(int argc, char **argv)
{
  int rc;
	
  void* base;
  CLIPINFO info;
  char filename[80];
  unsigned long size;
  int argn;
  
  if(argc < 2)
  {
    help();
    return(-1);
  }

  if(argv[1][0] == '-')
  {
    if(argv[1][1] == 'v')
      verbose = 1;
    argc--;
    argn = 2;
  }
  else
    argn = 1;

 /* init the video*/
  if(verbose)printf("open video\n");
  if((fdv = open("/dev/vid",O_RDWR)) <0)
  {
    printf("Open clip_dev fail\n");
    return(-1);
  }

  if( argc > 2 && argv[argn][0] == '1')
  {
    if((fd_data = open(argv[argn+1],O_RDONLY)) <0)
    {
      printf("Open %s error\n", argv[argn+1]);
      goto exit;
    }
    if(verbose)printf("open data file OK\n");

    ioctl(fdv, MPEG_VID_GET_BUF_SIZE, &size);
    if(verbose)printf("video clip buffer size = 0x%8.8lx\n", size);
    base = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fdv, 0);
    if(base == NULL || base == (void *)0xffffffff)
    {
      printf("mmap failed\n");
      close(fd_data);
      goto exit;
    }
    if(verbose)printf("mmap OK = 0x%8.8x\n",(unsigned long)base);


    ioctl(fdv, MPEG_VID_SELECT_SOURCE, 1);
    ioctl(fdv, MPEG_VID_PLAY, 0);
    while(1)
    {
      rc = read(fd_data,data_buf, 0x10000);
      if(verbose)printf("read file = %d\n", rc);
      if(rc <= 0)
        break; 
      vid_app_write(fdv, base, data_buf, rc, 0);
    }
    if(munmap(base, VID_MAP_LEN) < 0)
      printf("unmap error\n");
    close(fd_data);
  }
  else if( argc > 2 && argv[argn][0] == '2' )
  {
    if((fd_data = open(argv[argn+1],O_RDONLY)) <0)
    {
      printf("Open %s file error\n", argv[argn+1]);
      goto exit;
    }
    if(verbose)printf("open data file OK\n");
            
    ioctl(fdv, MPEG_VID_SELECT_SOURCE, 1);
    ioctl(fdv, MPEG_VID_PLAY, 0);

    while(1)
    {
      rc = read(fd_data,data_buf, 0x10000);
      if(verbose)printf("read file = %d\n", rc);
      if(rc <= 0)
        break;
      if( write(fdv, data_buf, rc) != rc)
      {
	printf("write data error\n");
      }
    }
    close(fd_data);
  }

  else if( argc > 1 && argv[argn][0] == '3')
  {
    ioctl(fdv, MPEG_VID_SELECT_SOURCE, 1);
    ioctl(fdv, MPEG_VID_PLAY, 0);
    //ioctl(fdv, MPEG_VID_FREEZE, 0);
    //ioctl(fdv, MPEG_VID_SET_BLANK, 0);
    ioctl(fdv, MPEG_VID_START_STILLP, 0);
    {
      if(ioctl(fdv, MPEG_VID_STILLP_READY, 0) == 0)
      {
        BUFINFO binf;
        char padding[256];

        if(verbose)printf("stillp ready and write begin\n");
        if(argc > 2)
        {
          struct stat buf;
          
          if(0 != stat(argv[argn+1],&buf))
          {
            printf("Cant find file %s\n",argv[argn+1]);
            goto exit;
          }
          if(buf.st_size > sizeof(data_buf))
          {
            printf("File too large. Must be <= %d\n",sizeof(data_buf));      
	    goto exit;
          }
          if((fd_data = open(argv[argn+1],O_RDONLY)) <0)
          {
            printf("Open %s error\n", argv[argn+1]);
            goto exit;
          }
          rc = read(fd_data,data_buf, sizeof(data_buf));
          close(fd_data);
          if(rc <= 0)
          {
            printf("Error reading file\n");
            goto exit;
          }
          binf.ulStartAdrOff = (unsigned long)data_buf;
          binf.ulLen = rc;
      
        }
        else
        {
          binf.ulStartAdrOff = (unsigned long)v4_clip_data;
          binf.ulLen = v4_clip_len;
        }
        ioctl(fdv, MPEG_VID_STILLP_WRITE, &binf);

        ioctl(fdv, MPEG_VID_END_OF_STREAM, 0);

	//padding 128 bytes
        memset(padding, 0, 128);
        binf.ulStartAdrOff = (unsigned long)padding;
        binf.ulLen = 128;

        ioctl(fdv, MPEG_VID_STILLP_WRITE, &binf);

        //wait for sequence end
	if(verbose)printf("wait for sequence end\n");
        while(1)
        {
          if(ioctl(fdv, MPEG_VID_STILLP_READY, 0) == 0)
            break;
        }
      }
    }

    ioctl(fdv, MPEG_VID_SET_BLANK, 0);
    ioctl(fdv, MPEG_VID_STOP_STILLP, 0);
    sleep(5);
  }
  else 
  {
    printf("Invalid parameter\n");
    help();
    goto exit;
  }

  if(verbose) printf("closing video device\n");
exit:  close(fdv);
  return (0);
}

static void help(void)
{
  printf("\nhelp: vid_t {-v} {[1|2] filename} | 3 {filename}\n");
  printf("      1-mmap write\n      2-write\n      3-stillp test (uses default if no filename entered)\n\n");
  return;
}

int write_stillp(char* data, int size)
{
  ioctl(fdv, MPEG_VID_START_STILLP, 0);
  {
    if(ioctl(fdv, MPEG_VID_STILLP_READY, 0) == 0)
    {
      BUFINFO binf;
      char padding[256];

      if(verbose)printf("stillp ready and write begin\n");

      /*binf.ulStartAdrOff = (unsigned long)v4_clip_data;
      binf.ulLen = v4_clip_len;*/
      binf.ulStartAdrOff = (unsigned long)data;
      binf.ulLen = size;
      ioctl(fdv, MPEG_VID_STILLP_WRITE, &binf);

      ioctl(fdv, MPEG_VID_END_OF_STREAM, 0);

      //padding 128 bytes
      memset(padding, 0, 128);
      binf.ulStartAdrOff = (unsigned long)padding;
      binf.ulLen = 128;
      ioctl(fdv, MPEG_VID_STILLP_WRITE, &binf);
      //wait for sequence end
      if(verbose)printf("wait for sequence end\n");
      while(1)
      {
        if(ioctl(fdv, MPEG_VID_STILLP_READY, 0) == 0)
          break;
      }
      {
      	VIDEOINFO v_info;
        READINFO ri;
        int img_size;
        int ret;
        int count = 0;
        int i;

       	ioctl(fdv, MPEG_VID_GET_V_INFO, &v_info);

        /*img_size = v_info.h_size * v_info.v_size;
        img_size = 720*576;

       	if((ret = read(fdv, data_buf, img_size)) != img_size)
          printf("image read error\n");*/

        for(i = 0;  i < 576; i++)
        {
          ri.buf = data_buf;
          ri.offset = i*720;
          ri.size = 720;
          ri.color = 0;

          ioctl(fdv, MPEG_VID_STILLP_READ, &ri);

          if(write(fd_data, data_buf, ri.size ) != ri.size )
            printf("write data error\n");
        }

/*      ri.buf = data_buf;
        ri.offset = 0;
        ri.size = 720*576/2;
        ri.color = 1;

        ioctl(fdv, MPEG_VID_STILLP_READ, &ri);

        if(write(fd_data, data_buf, ri.size ) != ri.size )
          printf("write data error\n");
*/

      }
    }
  }
  ioctl(fdv, MPEG_VID_STOP_STILLP, 0);
  return 0;
}

unsigned long vid_app_write(int fd, void* base, void *src, 
                            unsigned long ulLen, int eos)
{
  unsigned long cur_len = 0, offset = 0;
  unsigned int  ret;

  CLIPINFO  info;

  if(verbose)printf("app write = %ld", ulLen);

  while(ulLen)
  {
    if( ioctl(fd, MPEG_VID_GET_BUF_WAIT, &info) < 0)
    {
      printf("get empty info error\n");
      return offset;
    }
    if(ulLen > info.ulBufLen)
      cur_len = info.ulBufLen;
    else
    {
      cur_len = ulLen;
      //end of stream
      if(eos)
        info.uFlag = 1;
      else
        info.uFlag = 0;
    }
    if(verbose)printf("cp to 0x%8.8x, from 0x%8.8x, len = %d\n", base + info.ulBufAdrOff, src + offset, cur_len);

    memcpy(base + info.ulBufAdrOff, src + offset, cur_len);
    //memset(base + info.ulBufAdrOff, 0x80, cur_len);


    if(verbose)printf("memcpy OK\n");

    info.uClipAdrOff = info.ulBufAdrOff;
    info.uClipLen = cur_len;
        

    if(ioctl(fd, MPEG_VID_CLIP_WRITE, &info) < 0)
    {
      printf("clip write error\n");
      return offset;
    }

    offset += cur_len;
    ulLen -= cur_len;
  }
  return offset;
}

