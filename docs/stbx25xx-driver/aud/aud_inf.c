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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| File:   aud_inf.c
| Purpose: audio driver interface layer PALLAS
| Changes:
| Date:     Comment:
| -----     --------
| 15-Oct-01 create
| 17-Jun-03 Added IOCTL to get current audio status
| 25-Jun-03 Added support for DTS/LPCM coreload
| 23-Jul-03 Added microcode version print during module init
+----------------------------------------------------------------------------*/
#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/module.h>       /* Specifically, a module */
#include <linux/version.h>

/* For character devices */
//#include <linux/sched.h>
//#include <linux/tqueue.h>
//#include <linux/interrupt.h>

#include <linux/ioport.h>
#include <linux/mm.h>           /* for verify_area */

/* The character device definitions are here */
#include <linux/fs.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/uaccess.h>        /* for get_user, copy_from_user */
#include <asm/io.h>

#include <os/drv_debug.h>

#include "aud_osi.h"
#include "aud_osd.h"
#include <aud/aud_inf.h>


AUD_WRITE_STRU _adec_main ;
AUD_WRITE_STRU _adec_mixer;

extern void* _seg1_log;

static int _open_count = 0;
static int _mixer_count = 0;
static int _src = -1;       //no source


void aud_inf_init_main()
{
    PDEBUG("set adec main parameter\n");
    memset(&_adec_main, 0, sizeof(_adec_main));
    _adec_main.ulBufPhyStart = MPEG_A_CLIP_BUF_START;
    _adec_main.ulBufSize = MPEG_A_CLIP_BUF_LEN;
    _adec_main.aud_osi_clip_init = aud_osi_init_clip;
    _adec_main.aud_osi_clip_close = aud_osi_close_clip;
    _adec_main.aud_osi_get_dev = aud_osi_get_clipdev;
}

void aud_inf_init_mixer()
{
    memset(&_adec_mixer, 0, sizeof(_adec_mixer));
    _adec_mixer.ulBufPhyStart = MPEG_A_MIXER_BUF_START;
    _adec_mixer.ulBufSize = MPEG_A_MIXER_BUF_LEN;
    _adec_mixer.aud_osi_clip_init = aud_osi_init_mixer;
    _adec_mixer.aud_osi_clip_close = aud_osi_close_mixer;
    _adec_mixer.aud_osi_get_dev = aud_osi_get_mixerdev;
    //_adec_mixer.uMapped = 0;
}

static int aud_inf_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);

    /* We don't talk to two processes at the same time */
    PDEBUG("open\n");

    if (_open_count != 0)
    {
        if(minor == DRV_MINOR_AUD_MIXER && _mixer_count == 0)
        {
            aud_inf_init_mixer();
            if(aud_osd_init_clip(&_adec_mixer) == 0)
            {
                _mixer_count ++ ;
                MOD_INC_USE_COUNT;
                return 0;
            }
            else
            {
                PDEBUG("open mixer error\n");
                return -1;
            }
        }
        else
        {
            PDEBUG("MIXER in use\n");
            return -EBUSY;
        }
    }

    _seg1_log = ioremap(MPEG_A_SEGMENT1, MPEG_A_SEGMENT1_SIZE);
    if(_seg1_log == NULL)
    {
        PDEBUG("ioremap error\n");
        return -1;
    }
    PDEBUG("seg1 log = %p\n", _seg1_log);

    aud_inf_init_main();

    switch(minor)
    {
        case DRV_MINOR_AUD_MPEG:
             if(aud_osi_init(AUD_MODE_STB_MPEG) != 0)
             {
                PDEBUG("MPEG init osi layer error\n");
                goto openFail;
             }             
             break;
        case DRV_MINOR_AUD_AC3:
             if(aud_osi_init(AUD_MODE_AC3) != 0)
             {
                PDEBUG("AC3 init osi layer error\n");
                goto openFail;
             }
             break;
        case DRV_MINOR_AUD_PCM:
             if(aud_osi_init(AUD_MODE_STB_PCM) != 0)
             {
                PDEBUG("mpeg init osi layer error\n");
                goto openFail;
             }  
             break;
        case DRV_MINOR_AUD_MIXER:
             PDEBUG("open main device first\n");
             goto openFail;
        case DRV_MINOR_AUD_DTS:
             if(aud_osi_init(AUD_MODE_DTS) != 0)
             {
                PDEBUG("AC3 init osi layer error\n");
                goto openFail;
             }
             break;
        case DRV_MINOR_AUD_LPCM:
             if(aud_osi_init(AUD_MODE_LPCM) != 0)
             {
                PDEBUG("AC3 init osi layer error\n");
                goto openFail;
             }
             break;
        default:
             goto openFail;
    }

	_open_count++;
    _adec_main.uMapped = 0;
    _src = -1;
    if(_seg1_log)
    {
        iounmap(_seg1_log);
        _seg1_log = NULL;
    }
    MOD_INC_USE_COUNT;
    return 0;
openFail:
    if(_seg1_log)
    {
        iounmap(_seg1_log);
        _seg1_log = NULL;
    }
    return -1;
}

static int aud_inf_release(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);
    PDEBUG("close\n");
    if(minor == DRV_MINOR_AUD_MIXER)
    {
        aud_osd_close_clip(&_adec_mixer);
        _mixer_count--;
        _adec_mixer.uMapped = 0;
    }
    else
    {
	    if(_src == 1)
        {
            aud_osd_close_clip(&_adec_main);
        }
        aud_osi_close();
	    _open_count--;
         _adec_main.uMapped = 0;
        _src = -1;
    }
    MOD_DEC_USE_COUNT;
    return 0;
}

static ssize_t aud_inf_read(struct file *file, char *buffer, /* The buffer to fill with the data 
                                                                 */
                               size_t length,   /* The length of the buffer */
                               loff_t * offset) /* offset to the file */
{
    /* Number of bytes actually written to the buffer */
    printk("code not finished\n");
    return length;
}

/* This function is called when somebody tries to
 * write into our device file. */

static ssize_t aud_inf_write(struct file *file,
                                const char *buffer,
                                size_t length, loff_t * offset)
{
    int minor;
    struct inode *inode = file->f_dentry->d_inode;
    minor = MINOR(inode->i_rdev);

    if( minor == DRV_MINOR_AUD_MIXER)
    {
        PDEBUG("mixer write\n");
        return aud_osd_write(&_adec_mixer, file, (void*)buffer, length);
    }
    else
    {
        PDEBUG("main write\n");
        return aud_osd_write(&_adec_main, file, (void*)buffer, length);
    }
}

static int aud_inf_mmap(struct file *file, struct vm_area_struct *vma)
{
    int minor;
    struct inode *inode = file->f_dentry->d_inode;
    minor = MINOR(inode->i_rdev);

    if(minor == DRV_MINOR_AUD_MIXER)
    {
        return aud_osd_mmap(&_adec_mixer, vma);
    }
    else
        return aud_osd_mmap(&_adec_main, vma);
}


/* This function is called whenever a process tries to
 * do an ioctl on our device file. We get two extra 
 * parameters (additional to the inode and file 
 * structures, which all device functions get): the number
 * of the ioctl called and the parameter given to the 
 * ioctl function.
 *
 * If the ioctl is write or read/write (meaning output 
 * is returned to the calling process), the ioctl call 
 * returns the output of this function.
 */
static int aud_inf_ioctl(struct inode *inode, 
                     struct file *file, 
                     unsigned int ioctl_num, 
                     unsigned long ioctl_param) /* The parameter to it */
{
    int ret = 0;
    int minor = MINOR(inode->i_rdev);
    audioStatus aud_stat;

    PDEBUG("AUD_INF:device ioctl param = %d\n", ioctl_num);

    switch (ioctl_num)
    {

        case MPEG_AUD_STOP:
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                PDEBUG("stop-- not support in mixer\n");
                return -1;
            }
            PDEBUG("AUDIO_STOP\n");
            ret = aud_osi_stop();
            break;

        //case MPEG_AUD_CONTINUE:

        case MPEG_AUD_PLAY:
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                PDEBUG("play-- not support in mixer\n");
                return -1;
            }

            PDEBUG("AUDIO_PLAY\n");
            ret = aud_osi_play();
            break;

        case MPEG_AUD_SET_MUTE:
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                PDEBUG("set mute-- not support in mixer\n");
                return -1;
            }

            if(ioctl_param)
            {
                PDEBUG("set mute\n");
                ret = aud_osi_mute();
            }
            else
            {
                PDEBUG("set mute\n");
                ret = aud_osi_unmute();
            }
            break;
        case MPEG_AUD_SET_STREAM_TYPE:
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                PDEBUG("set stream type-- not support in mixer\n");
                return -1;
            }

            PDEBUG("set stream type = %ld\n", ioctl_param);
            ret = aud_osi_set_stream_type(ioctl_param);
            break;
        case MPEG_AUD_SELECT_SOURCE:
            PDEBUG("select source = %d\n", ioctl_param);
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                PDEBUG("select source-- not support in mixer\n");
                return -1;
            }

            ret = -1;
            if(_src == -1)
            {
                //set source to tv
                if(ioctl_param == 0)
                {
                    ret = aud_osi_init_tv();
                    goto SetSource;
                }
                if(ioctl_param == 1)
                {
                    PDEBUG("init clip\n");
                    //aud_inf_init_main();
                    ret = aud_osd_init_clip(&_adec_main);
                    goto SetSource;
                }
            }
            if(_src == 0)
            {
                if(ioctl_param == 0)
                {
                    ret = 0;
                    goto SetSource;
                }
                if(ioctl_param == 1)
                {
                    aud_osi_close_tv();
                    //aud_inf_init_main();
                    ret = aud_osd_init_clip(&_adec_main);
                    goto SetSource;
                }
            }
            if(_src == 1)
            {
                if(ioctl_param == 1)
                {
                    ret = 0;
                    goto SetSource;
                }
                if(ioctl_param == 0)
                {
                    aud_osd_close_clip(&_adec_main);
                    ret = aud_osi_init_tv();
                    goto SetSource;
                }
            }
SetSource:  if(ret == 0)
                _src = ioctl_param;
            break;

           case MPEG_AUD_SELECT_CHANNEL:
                    ret = aud_osi_set_channel(ioctl_param);
                        break;
       case MPEG_AUD_GET_BUF_NOWAIT:
            PDEBUG("GET BUF NOWAIT\n");
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                if(_adec_mixer.uMapped == 0)
                {
                    PDEBUG("physical memory not mapped\n");
                    ret = -1;
                    break;
                }
                
                ret = aud_osd_get_buf_nowait(&_adec_mixer, (CLIPINFO*)ioctl_param);
            }
            else
            {
                if(_adec_main.uMapped == 0)
                {
                    PDEBUG("physical memory not mapped\n");
                    ret = -1;
                    break;
                }
                ret = aud_osd_get_buf_nowait(&_adec_main, (CLIPINFO*)ioctl_param);
            }
            break;

        case MPEG_AUD_GET_BUF_WAIT:
            PDEBUG("GET BUF WAIT\n");
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                if(_adec_mixer.uMapped == 0)
                {
                    PDEBUG("physical memory not mapped\n");
                    ret = -1;
                    break;
                }
                
                ret = aud_osd_get_buf_wait(&_adec_mixer, (CLIPINFO*)ioctl_param);
            }
            else
            {
                if(_adec_main.uMapped == 0)
                {
                    PDEBUG("physical memory not mapped\n");
                    ret = -1;
                    break;
                }
                ret = aud_osd_get_buf_wait(&_adec_main, (CLIPINFO*)ioctl_param);
            }
            break;

        case MPEG_AUD_CLIP_WRITE:
            PDEBUG("CLIP WRITE\n");
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                if(_adec_mixer.uMapped == 0)
                {
                    PDEBUG("physical memory not mapped\n");
                    ret = -1;
                    break;
                }
                
                ret = aud_osd_clip_write(&_adec_mixer, (CLIPINFO*)ioctl_param);
            }
            else
            {
                if(_adec_main.uMapped == 0)
                {
                    PDEBUG("physical memory not mapped\n");
                    ret = -1;
                    break;
                }
                ret = aud_osd_clip_write(&_adec_main, (CLIPINFO*)ioctl_param);
            }
            break;
        case MPEG_AUD_GET_BUF_SIZE:
            PDEBUG("AUDIO get buffer size\n");
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                *((unsigned long*)ioctl_param) = MPEG_A_MIXER_BUF_LEN;
            }
            else
                *((unsigned long*)ioctl_param) = MPEG_A_CLIP_BUF_LEN;
            break;
        case MPEG_AUD_END_OF_STREAM:
            PDEBUG("END OF STREAM\n");
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                ret = aud_osd_end_stream(&_adec_mixer);
            }
            else
                ret = aud_osd_end_stream(&_adec_main);
            break;

        case MPEG_AUD_SET_PCM_FORMAT:
            {
                 AUD_PCM_FORMAT_CONFIG pcmcfg;

                 PDEBUG("PCM format\n");
                 copy_from_user(&pcmcfg, (AUD_PCM_FORMAT_CONFIG*)ioctl_param, sizeof(pcmcfg));

                 if(minor == DRV_MINOR_AUD_MIXER)
                 {
                    PDEBUG("set mixer fmt\n");
                    ret = aud_osi_set_mixer_fmt((AUD_PCM_FORMAT_CONFIG*)&pcmcfg);
                 }
                 else
                 {
                     PDEBUG("set main pcm fmt\n");
                     ret = aud_osi_set_pcm_fmt((AUD_PCM_FORMAT_CONFIG*)&pcmcfg);
                 }
                         break;
            }
        case MPEG_AUD_SET_VOL:
            {
                AUDVOL vol;
                
                copy_from_user((void*)&vol, (void*)ioctl_param, sizeof(AUDVOL));
                if(minor == DRV_MINOR_AUD_MIXER)
                {   
                    ret = aud_osi_set_mixer_vol(&vol);
                }
                else
                    ret = aud_osi_set_vol(&vol);
                break;
            }
        case MPEG_AUD_GET_VOL:
            {
                AUDVOL vol;
                if(minor == DRV_MINOR_AUD_MIXER)
                {
                    if(aud_osi_get_mixer_vol(&vol) != 0)
                    {
                        ret = -1;
                        break;
                    }
                }
                else
                {
                    if(aud_osi_get_vol(&vol) != 0)
                    {
                        ret = -1;
                        break;
                    }
                }
                copy_to_user((void*)ioctl_param, (void*)&vol, sizeof(AUDVOL));
                ret = 0;
                break;
            }
                
        case MPEG_AUD_GET_SYNC_INFO:
            {
                STC_T stc, pts;

                if(minor == DRV_MINOR_AUD_MIXER)
                {
                    PDEBUG("not support in mixer\n");
                    return -1;
                }
                aud_osi_get_stc(&stc);
                aud_osi_get_pts(&pts);

                copy_to_user(&(((SYNCINFO*)ioctl_param)->stc), &stc, sizeof(STC_T));
                copy_to_user(&(((SYNCINFO*)ioctl_param)->pts), &pts, sizeof(STC_T));
                break;
            }
        case MPEG_AUD_SET_SYNC_STC:
            {
                STC_T stc;

                if(minor == DRV_MINOR_AUD_MIXER)
                {
                    PDEBUG("not support in mixer\n");
                    return -1;
                }
                copy_from_user(&stc, &(((SYNCINFO*)ioctl_param)->stc), sizeof(STC_T));
                aud_osi_set_stc(&stc);

                break;
            }
        case MPEG_AUD_SYNC_ON:
            ret = aud_osi_enable_sync(ioctl_param);
            break;
        case MPEG_AUD_SYNC_OFF:
            ret = aud_osi_disable_sync();
            break;
        case MPEG_AUD_RESET_CLIPBUF:
            ret = aud_osi_reset_clipbuf(&_adec_main);
            break;
        case MPEG_AUD_GET_STATUS:
            ret = aud_osi_get_status(&aud_stat);
            copy_to_user ((audioStatus *)ioctl_param, &aud_stat, sizeof(audioStatus));
            break;
        case MPEG_AUD_CLIP_FLUSH:
            if(minor == DRV_MINOR_AUD_MIXER)
            {
                if(_adec_mixer.uMapped == 0)
                {
                    PDEBUG("physical memory not mapped\n");
                    ret = -1;
                    break;
                }
                
                ret = aud_osi_clip_flush(_adec_mixer.clipdev);
            }
            else
            {
                if(_adec_main.uMapped == 0)
                {
                    PDEBUG("physical memory not mapped\n");
                    ret = -1;
                    break;
                }
                ret = aud_osi_clip_flush(_adec_main.clipdev);
            }
            break;
        default:
            ret = -1;
            break;           
    }
    return ret;
}

static unsigned int aud_inf_poll(struct file *filp,
                                 struct poll_table_struct *wait)
{
    PDEBUG("not finished yet\n");
    return 0;
}

static struct file_operations Fops = {
    read:   aud_inf_read,
    write:  aud_inf_write,
    ioctl:  aud_inf_ioctl,
    open:   aud_inf_open,
    release:aud_inf_release,
    poll:   aud_inf_poll,
    mmap:   aud_inf_mmap 
};


static struct {
                char *name;
		int minor;
		struct file_operations *fops;
		devfs_handle_t devfs_handle;
              } devnodes[] = { 
			                  {DEVICE_NAME_ADEC,0,&Fops,0},
					  {"adec_mpg",0,&Fops,0},
					  {"aud_mpeg",0,&Fops,0},
					  {"adec_ac3",1,&Fops,0},
					  {"adec_pcm",2,&Fops,0},
					  {"adec_mixer",3,&Fops,0},
					  {"adec_dts",4,&Fops,0},
					  {"adec_lpcm",5,&Fops,0}
				       };

static int no_devnodes = sizeof(devnodes)/sizeof(devnodes[0]);

/* Initialize the module - Register the character device */
int aud_init_module()
{
    int ret_val;
    int *ver;
    struct aud_ucode_info **uc_ptr;
    devfs_handle_t devfs_handle;
    int i;

    /* Register the character device (atleast try) */
    ret_val = devfs_register_chrdev(MAJOR_NUM_ADEC, DEVICE_NAME_ADEC, &Fops);
    /* Negative values signify an error */
    if (ret_val < 0)
    {
        PDEBUG("AUD: %s failed with %d\n",
               "Sorry, registering the character device ", ret_val);
        return ret_val;
    }

    for(i=0; i < no_devnodes; i++)
    {
      devfs_handle = devfs_find_handle(NULL, devnodes[i].name,
                                0, 0, DEVFS_SPECIAL_CHR,0);
    
      if(devfs_handle == NULL)
      {
        devfs_handle = devfs_register(NULL, devnodes[i].name, DEVFS_FL_DEFAULT,
                                      MAJOR_NUM_ADEC, devnodes[i].minor,
                                      S_IFCHR | S_IRUSR | S_IWUSR,
                                      devnodes[i].fops, NULL);
        devnodes[i].devfs_handle = devfs_handle;
      }
      else
      {
        devnodes[i].devfs_handle = NULL;
      }
    }
    

    /* print version numbers for audio microcode */
    for (uc_ptr = aud_ucode_info; *uc_ptr != NULL; uc_ptr++) {
      if (((*uc_ptr)->ucode[248] != 0) || ((*uc_ptr)->ucode[249] != 0)) {
	ver = (int *)(&(*uc_ptr)->ucode[248]);
	printk(KERN_NOTICE "audio microcode %s **test version** built on %d/%d/%d\n", 
	       (*uc_ptr)->name, (*ver%10000)/100, *ver%100, *ver/10000);
      } else {
	printk(KERN_NOTICE "audio microcode %s ver %d.%d\n", (*uc_ptr)->name,
	       (*uc_ptr)->ucode[250], (*uc_ptr)->ucode[251]);
      }
    }

    //DEMUX_REG_AVCB(&aud_callback, 1, 1);
    PDEBUG("AUD: Initialize adec_dev OK!\n");
    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
void aud_cleanup_module()
{
    int ret;
    int i;

    /* Unregister the device */
    ret = devfs_unregister_chrdev(MAJOR_NUM_ADEC, DEVICE_NAME_ADEC);
    if (ret < 0)
        printk("Error in module_unregister_chrdev: %d\n", ret);

    for(i = 0; i < no_devnodes; i++)
    {
      if(devnodes[i].devfs_handle != NULL)
        devfs_unregister(devnodes[i].devfs_handle);

    }

}
