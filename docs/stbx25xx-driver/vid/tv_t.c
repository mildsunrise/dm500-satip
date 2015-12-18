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
/*    DESCRIPTION :  A demo to test demux, audio and video 
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*  Author    :  Lin Guo Hui												*/
/*  File      :                                                             */
/*  Purpose   :  Test application	                                        */
/*  Changes   :                                                             */
/*  Date         Comments                                                   */
/*  ----------------------------------------------------------------------  */
/*  25-Jun-2001    Created                                                  */
/*	12-Jul-2001	   Add Network ID recognition, When Filter PMT, add the		*/
/*				   Program Number in the filter parameters.					*/
/*                                                                          */
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
#include "vid_t.h"


#define PLAY_TO_MEMORY	0
#define PLAY_TO_DECODE	1

int main()
{
    int fd_pat;				//handle of filter for PAT
    int fd_pmt;				//handle of filter for PMT
    int fd_a;				//handle of filter for audio PES
    int fd_v;				//handle of filter for video PES
    int fd_pcr;				//handle of filter for PCR

    int fda;				//handle of audio decoder
    int fdv;				//handle of video decoder
    int Pnumber = 0;
	
    int Program_Count;	
	
    if ((fd_pat = open("/dev/demuxapi0", O_RDONLY)) < 0)	//open a PAT filter from 
    {														//XP0 device, you can change
        printf("Error open demuxapi\n");					//the device using demuxapi1 or
        return -1;											//demuxapi2 to replace demuxapi0
    }
	
    if ((fd_pmt = open("/dev/demuxapi0", O_RDONLY)) < 0)	//open a PMT filter
    {
        printf("Error open demuxapi\n");
        return -1;
    }
	
    if ((fd_a = open("/dev/demuxapi0", O_RDONLY)) < 0)		//open a audio PES filter
    {
        printf("Error open demuxapi\n");
        return -1;
    }
	
    if ((fd_v = open("/dev/demuxapi0", O_RDONLY)) < 0)		//open a video PES filter
    {
        printf("Error open demuxapi\n");
        return -1;
    }
	
    if ((fd_pcr = open("/dev/demuxapi0", O_RDONLY)) < 0)	//open a PCR filter
    {
        printf("Error open demuxapi\n");
        return -1;
    }
   
    if ((fdv = open("/dev/vdec_dev", O_RDWR)) < 0)		//open video decoder
    {
        printf("Open Video Fail\n");
        return -1;
    }

    if ((fda = open("/dev/adec_dev", O_RDWR)) < 0)		//open audio decoder
    {
        printf("Open audio fail\n");
        return -1;
    }
	
    if( ioctl( fdv, MPEG_VID_SELECT_SOURCE, 0) < 0)	//Set source of the video decoder to be demux
    {
        printf("video clip mode init error\n");
        return -1;
    }
	
    if( ioctl( fda, MPEG_AUD_SELECT_SOURCE, 0) < 0)	//Set source of the video decoder to be demux
    {
        printf("audio clip mode init error\n");
        return -1;
    }
	
    if( ioctl( fda, MPEG_AUD_SET_STREAM_TYPE, AUD_STREAM_TYPE_PES) < 0)
    {
        printf("select audio format error\n");
        return -1;
    }
	
    Program_Count = get_program_count(fd_pat);
    if( Program_Count <= 0)
    {
        printf("Get program number error\n");
        goto close;
    }
    start_program(fd_pmt,fd_a,fd_v,fd_pcr,Pnumber,PLAY_TO_DECODE);
    
    ioctl(fdv, MPEG_VID_PLAY, 0);
    ioctl(fdv, MPEG_VID_SET_BLANK, 0);
    
   while(1)
    {
        char buf[80];
        printf("q-Quit, a-audio test, c-channel, p-play, s-stop, f-freeze, 1-ff, 2-slow, 3-scale");
        scanf("%s", buf);
        if(buf[0] == 'q')
            break;
        switch(buf[0])
        {
            case 'a':
                vid_t_audio(fda);
                break;
            case 'c':
                if(++Pnumber == Program_Count)
                    Pnumber = 0;
                start_program(fd_pmt,fd_a,fd_v,fd_pcr,Pnumber,PLAY_TO_DECODE);
                break;
            case 'p':
                ioctl(fda, MPEG_AUD_SET_MUTE, 0);
                ioctl(fda, MPEG_AUD_PLAY, 0);
                ioctl(fdv, MPEG_VID_PLAY, 0);
                break;
            case 's':
                ioctl(fda, MPEG_AUD_STOP, 0);
                ioctl(fdv, MPEG_VID_STOP, 0);
                break;
            case 'f':
                ioctl(fda, MPEG_AUD_SET_MUTE, 1);
                ioctl(fdv, MPEG_VID_FREEZE, 0);
                break;
            case '1':
                ioctl(fda, MPEG_AUD_SET_MUTE, 1);
                vid_t_fastforward(fdv);
                break;
            case '2':
                ioctl(fda, MPEG_AUD_SET_MUTE, 1);
                vid_t_slowmotion(fdv);
                break;
            case '3':
                vid_t_scale(fdv);
                break;
            default:
                break;
        }
    }

close: close(fda);
       close(fdv);
       close(fd_pcr);
       close(fd_v);
       close(fd_a);
       close(fd_pmt);
       close(fd_pat);
}


    

