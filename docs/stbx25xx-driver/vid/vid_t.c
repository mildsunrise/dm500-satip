#include <unistd.h>
#include "vid/vid_inf.h"
#include "aud/aud_inf.h"

int vid_t_dispsize(int fd)
{
    char c;
    printf("\ndisplay size test\n");
    while(1)
    {
        printf("q-quit, 1-16x9, 2-4x3\n");
        c = getchar(); 
        if(c == 'q')
            break;
        if(c == 1)
            ioctl(fd, MPEG_VID_SET_DISPSIZE, VID_DISPSIZE_16x9);
        else if ( c == 2 )
            ioctl(fd, MPEG_VID_SET_DISPSIZE, VID_DISPSIZE_4x3);
    }
}


int vid_t_fastforward(int fd)
{
    char speed;

    printf("fast forward test\n");
    while(1)
    {
        printf("q-quit, 1-I frame only, 2-I & P frame\n");
        speed = getchar();
        if(speed == 'q')
            break;

        if(speed == 0)
        {
            printf("Test: fast forward I frame only\n");
            if( ioctl(fd, MPEG_VID_FASTFORWARD, speed) < 0)
            {
                printf("video fast forward error\n");
                return -1;
            }
        }
        else if(speed == 1)
        {
            printf("Test: fast forward I&P frame\n");
            if( ioctl(fd, MPEG_VID_FASTFORWARD, speed) < 0)
            {
                printf("video fast forward error\n");
                return -1;
            }
        }
    }
    return 0;
}

int vid_t_slowmotion(int fd)
{
    char speed;
    printf("Test: slow motion= %d\n", speed);

    while(1)
    {
        printf("q-quit, 1-I frame only, 2-I & P frame\n");
        speed = getchar();
        if(speed == 'q')
            break;

        if( ioctl(fd, MPEG_VID_SLOWMOTION, speed) < 0)
        {
            printf("video slow motion error\n");
            return -1;
        }
    }
    return 0;
}

int vid_t_get_bufsize(int fd)
{
    unsigned long size;
    printf("Test: get buffer size\n");
    if( ioctl(fd, MPEG_VID_GET_BUF_SIZE, size) < 0)
    {
        printf("video get buffer size error\n");
        return -1;
    }
    printf("buffer size = 0x%8.8x\n", size);
    return 0;
}

int vid_t_input(char *msg, char* value)
{
    printf("%s", msg);
    scanf("%s", value);
    if(value[0] == 'q' || value[0] == 'Q')
    {
        return 1;
    }
    return 0;
}

int vid_t_scale(int fd)
{
    SCALEINFO info;
    char buf[80];
    int mode;
#ifdef __DRV_FOR_PALLAS__
    printf("Test: scale mode test\n");
    while(1)
    {
        if(vid_t_input("\nsrc hori offset:", buf) == 0)
        {
            info.src.hori_off = atoi(buf);
        }
        else
        {
            break;
        }
        if(vid_t_input("\nsrc hori size:", buf) == 0)
        {
            info.src.hori_size = atoi(buf);
        }
        else
        {
            break;
        }
        if(vid_t_input("\nsrc vertical offset:", buf) == 0)
        {
            info.src.vert_off = atoi(buf);
        }
        else
        {
            break;
        }
        if(vid_t_input("\nsrc vertical size:", buf) == 0)
        {
            info.src.vert_size = atoi(buf);
        }
        else
        {
            break;
        }

        if(vid_t_input("\n des hori offset:", buf) == 0)
        {
            info.des.hori_off = atoi(buf);
        }
        else
        {
            break;
        }
        if(vid_t_input("\ndes hori size:", buf) == 0)
        {
            info.des.hori_size = atoi(buf);
        }
        else
        {
            break;
        }
        if(vid_t_input("\ndes vertical offset:", buf) == 0)
        {
            info.des.vert_off = atoi(buf);
        }
        else
        {
            break;
        }
        if(vid_t_input("\ndes vertical size:", buf) == 0)
        {
            info.des.vert_size = atoi(buf);
        }
        else
        {
            break;
        }
	/*
        info.src.hori_off = 0;
        info.src.hori_size = 200;
        info.src.vert_off = 0;
        info.src.vert_size = 200;

        info.des.hori_off = 0;
        info.des.hori_size = 200;
        info.des.vert_off = 0;
        info.des.vert_size = 200;
	*/

        ioctl(fd, MPEG_VID_SET_SCALE_POS, &info);
        ioctl(fd, MPEG_VID_SCALE_ON, 0);
    }
    ioctl(fd, MPEG_VID_SCALE_OFF, 0);
    printf("scale mode end\n");
#else
    vid_t_input("\nEnter scale mode: 0-normal 1-lbx 2-1/2 3-1/4 4-2X 5-no_expansion ",buf);
    
    switch(buf[0])
    {
      case '0':
          if(0 != ioctl(fd, MPEG_VID_SET_DISPMODE, VID_DISPMODE_NORM))
            printf("MPEG_VID_SET_DISPMODE failed\n");
          break;
      case '1':
          if(0 != ioctl(fd, MPEG_VID_SET_DISPMODE, VID_DISPMODE_LETTERBOX))
            printf("MPEG_VID_SET_DISPMODE failed\n");
          break;
      case '2':
          if(0 != ioctl(fd, MPEG_VID_SET_DISPMODE, VID_DISPMODE_1_2))
            printf("MPEG_VID_SET_DISPMODE failed\n");
          break;
      case '3':
          if(0 != ioctl(fd, MPEG_VID_SET_DISPMODE, VID_DISPMODE_1_4))
            printf("MPEG_VID_SET_DISPMODE failed\n");
          break;
      case '4':
          if(0 != ioctl(fd, MPEG_VID_SET_DISPMODE, VID_DISPMODE_2x))
            printf("MPEG_VID_SET_DISPMODE failed\n");
          break;
      case '5':
          if(0 != ioctl(fd, MPEG_VID_SET_DISPMODE, VID_DISPMODE_DISEXP))
            printf("MPEG_VID_SET_DISPMODE failed\n");
          break;
      default:
          printf("Invalid mode\n");
          break;
    }
#endif
    return 0;
}

void vid_t_audio(int fda)
{
    char buf[80];
    AUDVOL vol;
    while(1)
    {
        if(vid_t_input("q-quit, u-volume up, d-volume down, 3-mute, 4-unmute\n", buf))
            break;
        switch(buf[0])
        {
            case 'u':
                ioctl(fda, MPEG_AUD_GET_VOL, &vol);
                printf("audio front left = %d\n", vol.frontleft);
                printf("audio front right = %d\n", vol.frontright);
                printf("audio rear left = %d\n", vol.rearleft);
                printf("audio rear right = %d\n", vol.rearright);
                printf("audio lfe = %d\n", vol.lfe);
                printf("audio center = %d\n", vol.center);
                if(vol.center > 0)
                    vol.center--;
                if(vol.frontleft > 0)
                    vol.frontleft--;
                if(vol.frontright > 0)
                    vol.frontright--;
                if(vol.lfe > 0)
                    vol.lfe--;
                if(vol.rearleft > 0)
                    vol.rearleft--;
                if(vol.rearright > 0)
                    vol.rearright--;
                ioctl(fda, MPEG_AUD_SET_VOL, &vol);
                break;
            case 'd':
                ioctl(fda, MPEG_AUD_GET_VOL, &vol);
                printf("audio front left = %d\n", vol.frontleft);
                printf("audio front right = %d\n", vol.frontright);
                printf("audio rear left = %d\n", vol.rearleft);
                printf("audio rear right = %d\n", vol.rearright);
                printf("audio lfe = %d\n", vol.lfe);
                printf("audio center = %d\n", vol.center);
                if(vol.center < 63)
                    vol.center++;
                if(vol.frontleft < 63)
                    vol.frontleft++;
                if(vol.frontright < 63)
                    vol.frontright++;
                if(vol.lfe < 63)
                    vol.lfe++;
                if(vol.rearleft < 63)
                    vol.rearleft++;
                if(vol.rearright < 63)
                    vol.rearright++;
                ioctl(fda, MPEG_AUD_SET_VOL, &vol);
                break;
            case '3':
                ioctl(fda, MPEG_AUD_SET_MUTE, 1);
                break;
            case '4':
                ioctl(fda, MPEG_AUD_SET_MUTE, 0);
                break;
            default:
                break;
        }
    }
}

    





