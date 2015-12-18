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
/*    DESCRIPTION :  test audio and video PES recoding
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*  Author    :  Lin Guo Hui                            */
/*  File      :                                                             */
/*  Purpose   :  Test application                                       */
/*  Changes   :                                                             */
/*  Date         Comments                                                   */
/*  ----------------------------------------------------------------------  */
/*  25-Jun-2001    Created                                                  */
/*  12-Jul-2001    Add Network ID recognition, When Filter PMT, add the     */
/*         Program Number in the filter parameters.                 */
/*  10-Oct-2001    update for pallas                                        */
/*  25-Oct-2001    seperate TV function to tv_function.h                    */
/****************************************************************************/
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>

#include <aud/aud_inf.h>
#include <vid/vid_inf.h>

#include <xp/xp_osd_user.h>
#include <app/tv_function.h>

int main()
{
  int fd_pat=0;
  int fd_pmt=0;
  int fd_a=0;
  int fd_v=0;
  int fd_pcr=0;
  int fda=0;
  int fdv=0;

  int len = 0;
  int i;
  int ProgramCount = 0;
  int length;

  int Pnumber=0;

  int afile, vfile;
  int alen,vlen;
  unsigned char  audio_buffer[65536];
  unsigned char* video_buffer =audio_buffer;
  char key;

  if ((afile = open("audio_tmp", O_CREAT|O_TRUNC|O_RDWR)) < 0)
  {
    printf("Creat audio file Fail\n");
    goto exit;
  }

  if ((vfile = open("video_tmp", O_CREAT|O_TRUNC|O_RDWR)) < 0)
  {
    printf("Creat video file Fail\n");
    goto exit;
  }

  if ((fdv = open("/dev/vdec_dev", O_RDWR)) < 0)
  {
    printf("Open Video Fail\n");
    goto exit;
  }

  if ((fda = open("/dev/adec_dev", O_RDWR)) < 0)
  {
    printf("Open audio fail\n");
    goto exit;
  }


  if ((fd_pat = open("/dev/demuxapi0", O_RDONLY)) < 0)
  {
    printf("Error open demuxapi\n");
    goto exit;
  }

  if ((fd_pmt = open("/dev/demuxapi0", O_RDONLY)) < 0)
  {
    printf("Error open demuxapi\n");
    goto exit;
  }

  if ((fd_a = open("/dev/demuxapi0", O_RDONLY|O_NONBLOCK)) < 0)
  {
    printf("Error open demuxapi\n");
    goto exit;
  }

  if ((fd_v = open("/dev/demuxapi0", O_RDONLY|O_NONBLOCK)) < 0)
  {
    printf("Error open demuxapi\n");
    goto exit;
  }


  if ((fd_pcr = open("/dev/demuxapi0", O_RDONLY|O_NONBLOCK)) < 0)
  {
    printf("Error open demuxapi\n");
    goto exit;
  }


  if( ioctl( fdv, MPEG_VID_SELECT_SOURCE, 0) < 0)
  {
    printf("video clip mode init error\n");
    goto exit;
  }

  if( ioctl( fda, MPEG_AUD_SELECT_SOURCE, 0) < 0)
  {
    printf("audio clip mode init error\n");
    goto exit;
  }

  if( ioctl( fda, MPEG_AUD_SET_STREAM_TYPE, AUD_STREAM_TYPE_PES) < 0)
  {
    printf("select audio format error\n");
    goto exit;
  }

  ioctl(fda, MPEG_AUD_PLAY, 0);
  ioctl(fdv, MPEG_VID_PLAY, 0);
  ProgramCount = get_program_count(fd_pat);

  printf("Please choose a program to record (0-n)\n");
  Pnumber = getchar()-'0';

  if(Pnumber<0 || Pnumber>=ProgramCount)
  {
    printf("Invalid program number, please rechoose...\n");
    Pnumber = getchar()-'0';
  }

  printf("Pnumber = %d\n",Pnumber);

  if(0 > fcntl(0,F_SETFL,O_NONBLOCK))         //change the standard input to Non-block mode
  {
    printf("error change stdin mode\n");
    goto exit;
  }


  if(ProgramCount >0)
  {
    if(start_program(fd_pmt,fd_a,fd_v,fd_pcr,Pnumber,PLAY_TO_MEMORY))
      goto exit;

    while(1)
    {
      alen = read(fd_a,audio_buffer,65536);
      if (alen>0)
      {
//        printf("alen = %d\n",alen);
//        printf("PES packet = %d\n",(audio_buffer[4]*256 + audio_buffer[5] + 6));
        write(afile,audio_buffer, alen);
      }

      vlen = read(fd_v,video_buffer,65536);
      if (vlen>0)
      {
        write(vfile,video_buffer, vlen);
//        printf("vlen = %d\n", vlen);
      }

      if((read(0,&key,1) > 0) && key == 'q')
        break;
    }
  }

exit:
  if(fd_a)
    close(fd_a);
  if(fd_v)
    close(fd_v);
  if(fd_pmt)
    close(fd_pmt);
  if(fd_pcr)
    close(fd_pcr);
  if(fd_pat)
    close(fd_pat);
  if(afile)
    close(afile);
  if(vfile)
    close(vfile);

  return 0;
}
