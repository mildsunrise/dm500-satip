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
| File:   vid_inf.c
| Purpose: video decoder interface layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01     create                                                      SL
| 30-April-03   Removed vid_osd_init() call from vid_init_module because it was
|               not uninitialized in vid_cleanup_module and because it is called
|               in vid_inf_open()
| 10-Sep-03     Added MPEG_VID_ENABLE_DISP_14_9 and MPEG_VID_DISABLE_DISP_14_9
+----------------------------------------------------------------------------*/
#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/module.h>       /* Specifically, a module */
#include <linux/version.h>
#include <linux/sched.h>

#include <linux/ioport.h>
#include <linux/mm.h>           /* for verify_area */

/* The character device definitions are here */

#include <linux/fs.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/uaccess.h>        /* for get_user, copy_from_user */

#include <os/os-interrupt.h>

#include <os/drv_debug.h>
#include "vid_osi.h"
#include "vid_osi_scr.h"
#include "vid_osd.h"
#include <vid/vid_inf.h>

extern VDEC _videoDecoder;

#ifdef MODULE
MODULE_PARM(_fmt, "i");
MODULE_PARM_DESC(_fmt,
                 "select default format: 0-NTSC, 1-PAL, 2-color bar on");
#endif

UINT    _denc_id = DENC_INTERNAL;
UINT    _fmt = SCRMAN_FMT_NTSC;

//INT vid_atom_verify_microcode(USHORT *pCode, INT nCount);

int video_open_count = 0;
//static int _open_count = 0;
static int _mapped = 0;
static int _src = -1;        //0: tv, 1: clip, -1: no source
static int _still_p = 0;

static int vid_inf_open(struct inode *inode, struct file *file)
{
    /* We don't talk to two processes at the same time */
    PDEBUG("open\n");

//    if (_open_count != 0)
    if (video_open_count != 0)
    {
        PDEBUG("VID: Driver in use\n");
        return -EBUSY;
    }
    if(vid_osd_init() != 0)
    {
        PDEBUG("init osi layer error\n");
    }
    //vid_osd_reset_frame_buffer();

//     _open_count++;
     video_open_count++;
    
    //vid_atom_reg_dump();
    MOD_INC_USE_COUNT;
    return 0;
}

static int vid_inf_release(struct inode *inode, struct file *file)
{
    PDEBUG("close\n");
        vid_osd_close();
    //vid_osd_reset_frame_buffer();
    //vid_atom_clear_framebuffer();
//    _open_count--;
    if(video_open_count != 0)
       video_open_count--;      
      
    _src = -1;

    //vid_atom_reg_dump();
    MOD_DEC_USE_COUNT;
    return 0;
}

static ssize_t vid_inf_read(struct file *file, char *buffer, /* The buffer to fill with the data
                                                                 */
                               size_t length,   /* The length of the buffer */
                               loff_t * offset) /* offset to the file */
{
    //void* spb;
    /* Number of bytes actually written to the buffer */
    if(_still_p == 0)
        return -1;

    //copy still picture to the read buffer
    /*if((spb = vid_osd_get_stillp_buf()) == NULL)
        return -1;
    copy_to_user(buffer, spb, length);
    return length;*/
        return length;
}

/* This function is called when somebody tries to
 * write into our device file. */

static ssize_t vid_inf_write(struct file *file,
                                const char *buffer,
                                size_t length, loff_t * offset)
{
    //added by shaol for stillp function
    //if in still pic mode, other processes must wait
    if(_still_p)
    {
         //interruptible_sleep_on(&WaitQ);
        return -1;
    }
    return vid_osd_write(file, (void*)buffer, length);
}

static int vid_inf_mmap(struct file *file, struct vm_area_struct *vma)
{
    int ret = -EINVAL;
    unsigned long size;

    //write clip buffer
    if(vma->vm_flags & VM_WRITE)
    {
        PDEBUG("vid_inf_mmap: vma->vm_pgoff = %d \n", vma->vm_pgoff);

        if(vma->vm_pgoff != 0)
            return ret;
        size = vma->vm_end - vma->vm_start;
        PDEBUG("vid_inf_mmap: vma->vm_end = 0x%8.8x vma->vm_start = 0x%8.8x \n", vma->vm_end, vma->vm_start );

        PDEBUG("mmap size = 0x%8.8lx vid_osd_get_clip_mem_size = 0x%8.8x \n", size,vid_osd_get_clip_mem_size());
        if(size > vid_osd_get_clip_mem_size())
            return ret;
        ret = -EAGAIN;
        vma->vm_page_prot.pgprot |= _PAGE_NO_CACHE;     // map without caching
        if(remap_page_range(vma->vm_start, _videoDecoder.clipbuf.uAddr,  
                            size, vma->vm_page_prot))
            return -EINVAL;
        PDEBUG("vid_inf_mmap: _videoDecoder.clipbuf.uAddr = 0x%08x\n",_videoDecoder.clipbuf.uAddr);

        _mapped = 1;
        return 0;
    }
    return ret;
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
static int vid_inf_ioctl(struct inode *inode,
                     struct file *file,
                     unsigned int ioctl_num,
                     unsigned long ioctl_param) /* The parameter to it */
{
    int ret = 0;


    switch (ioctl_num)
    {

        case MPEG_VID_STOP:
            PDEBUG("VIDEO_STOP\n");
            ret = vid_osi_stop(ioctl_param);
            break;

        case MPEG_VID_CONTINUE:

        case MPEG_VID_PLAY:
            PDEBUG("VIDEO_PLAY\n");
            ret = vid_osi_play();
            break;

        case MPEG_VID_FREEZE:
            PDEBUG("VIDEO_FREEZE\n");
            ret = vid_osi_freeze();
            break;

        case MPEG_VID_SET_BLANK:
            PDEBUG("VIDEO_SET_BLANK\n");

            if (ioctl_param)
                vid_osi_blank();
            else
                vid_osi_show();

            break;
        case MPEG_VID_SELECT_SOURCE:
            ret = -1;
            if(_src == -1)
            {
                //set source to tv
                if(ioctl_param == 0)
                {
                    ret = vid_osi_init_tv();
                    goto SetSource;
                }
                if(ioctl_param == 1)
                {
                    ret = vid_osd_init_clip();
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
                    vid_osi_close_tv();
                    ret = vid_osd_init_clip();
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
                    vid_osi_close_clip();
                    ret = vid_osi_init_tv();
                    goto SetSource;
                }
            }
SetSource:  if(ret == 0)
                _src = ioctl_param;

            break;


        /*case MPEG_VID_GET_STATUS:
            PDEBUG("VID_INF: VIDEO_GET_STATUS\n");

            vid_get_status(pVDEC, (struct videoStatus *) ioctl_param);

            break;*/

                case MPEG_VID_PAUSE:
                        PDEBUG("VIDEO PAUSED\n");
                        ret = vid_osi_pause();
                        break;

                case MPEG_VID_FASTFORWARD:
                        PDEBUG("FAST FORWARD\n");
                        ret = vid_osi_fast_forward(ioctl_param);
                        break;

                case MPEG_VID_SLOWMOTION:
                        PDEBUG("SLOW MOTION\n");
                        ret = vid_osi_slow_motion(ioctl_param);
                        break;

        case MPEG_VID_GET_BUF_NOWAIT:
            PDEBUG("GET BUF NOWAIT\n");
            if(_mapped == 0)
            {
                PDEBUG("physical memory not mapped\n");
                ret = -1;
                break;
            }
            ret = vid_osd_get_buf_nowait((CLIPINFO*)ioctl_param);
            break;

        case MPEG_VID_GET_BUF_WAIT:
            PDEBUG("GET BUF WAIT\n");
            if(_mapped == 0)
            {
                PDEBUG("physical memory not mapped\n");
                ret = -1;
                break;
            }
            ret = vid_osd_get_buf_wait((CLIPINFO*)ioctl_param);
            break;

        case MPEG_VID_CLIP_WRITE:
            PDEBUG("CLIP WRITE\n");
            if(_mapped == 0)
            {
                PDEBUG("physical memory not mapped\n");
                ret = -1;
                break;
            }
            ret = vid_osd_clip_write((CLIPINFO*)ioctl_param);
            break;
        case MPEG_VID_END_OF_STREAM:
            PDEBUG("END OF STREAM\n");
            ret = vid_osd_end_stream();
            break;

        case MPEG_VID_GET_BUF_SIZE:     //get clip buffer size
            PDEBUG("GET CLIP BUFFER SIZE\n");
            *((unsigned long *)ioctl_param) = vid_osd_get_clip_mem_size();
            break;
        case MPEG_VID_SET_SCALE_POS:
            {
                RECT src, des;
                PDEBUG("SET SCALE POSITION\n");
                copy_from_user(&src, &(((SCALEINFO*)ioctl_param)->src), sizeof(RECT));
                copy_from_user(&des, &(((SCALEINFO*)ioctl_param)->des), sizeof(RECT));

                vid_osi_set_scalepos(&src, &des);
                break;
            }
        case MPEG_VID_SCALE_ON:
            PDEBUG("SCALE ON\n");
            vid_osi_scale_on(ioctl_param);
            break;
        case MPEG_VID_SCALE_OFF:
            PDEBUG("SCALE OFF\n");
            vid_osi_scale_off(ioctl_param);
            break;
        case MPEG_VID_GET_SYNC_INFO:
            {
                STC_T stc, pts;
                vid_osi_get_stc(&stc);
                //actully video pts can not be read, dummy read
                vid_osi_get_pts(&pts);
                copy_to_user(&(((SYNCINFO*)ioctl_param)->stc), &stc, sizeof(STC_T));
                copy_to_user(&(((SYNCINFO*)ioctl_param)->pts), &pts, sizeof(STC_T));
                break;
            }
        case MPEG_VID_SET_SYNC_STC:
            {
                STC_T stc;
                copy_from_user(&stc, &(((SYNCINFO*)ioctl_param)->stc), sizeof(STC_T));
                vid_osi_set_stc(&stc);
                break;
            }
/*        case MPEG_VID_SET_SYNC_TYPE:
            vid_osi_set_sync_type(ioctl_param);
            break;
*/
        case MPEG_VID_SYNC_ON:
            vid_osi_set_sync_type(ioctl_param);
            ret = vid_osi_enable_sync();
            break;
        case MPEG_VID_SYNC_OFF:
            vid_osi_disable_sync();
            break;
        case MPEG_VID_SET_DISPSIZE:
            ret = vid_osi_set_dispsize(ioctl_param);
            break;
        case MPEG_VID_GET_V_INFO:
            {
                VIDEOINFO v_info;
                ret = vid_osi_get_v_info(&v_info);
                if(ret == 0)
                    copy_to_user((VIDEOINFO*)ioctl_param, &v_info, sizeof(VIDEOINFO));
                break;
            }
        case MPEG_VID_SET_DISPFMT:
                if(ioctl_param == VID_DISPFMT_NTSC)
                    vid_osi_set_dispfmt(VID_DISPFMT_NTSC);
                else if(ioctl_param == VID_DISPFMT_PAL)
                    vid_osi_set_dispfmt(VID_DISPFMT_PAL);
                break;
         case MPEG_VID_SET_OUTFMT:
             ret = scrman_osi_set_output(ioctl_param);
             break;
         case MPEG_VID_ENABLE_DISP_14_9:
            ret = vid_osi_enable_disp_14_9(ioctl_param);
            break;
         case MPEG_VID_DISABLE_DISP_14_9:
            ret = vid_osi_disable_disp_14_9(ioctl_param);
            break;
        case MPEG_VID_SET_DISPMODE:
                ret = vid_osi_set_dispmode(ioctl_param);
                break;
        case MPEG_VID_START_STILLP:
                ret = -1;
                if(_src == 1)
                {
                    /*-------------------------------------------------------
                    |clear last clip operation
                    | WARNING: this operation will disrupt last clip
                    +--------------------------------------------------------*/
                    vid_osi_close_clip();
                }
                else if(_src == 0)
                {
                    vid_osi_close_tv();
                }
                if(vid_osd_init_stillp() < 0)
                {
                    ret = -1;
                    break;
                }
                _still_p = 1;
                        _src = 1;
                ret = 0;
                break;
        case MPEG_VID_STOP_STILLP:
                ret = 0;
                if(_still_p == 0)
                    break;
                vid_osi_close_stillp();
                _still_p = 0;
                _src = -1;
                break;
        case MPEG_VID_STILLP_READY:
                ret = vid_osi_stillp_ready();
                break;
        case MPEG_VID_STILLP_WRITE:
            {
                BUFINFO buf;
                ret = -1;
                copy_from_user(&buf, (BUFINFO*)ioctl_param, sizeof(BUFINFO));
                if( vid_osd_write(file, (void*)(buf.ulStartAdrOff), buf.ulLen)
                    == buf.ulLen)
                    ret = 0;
                break;
            }
        case MPEG_VID_STILLP_READ:
            {
                READINFO ri;
                ULONG adr;
                ret = 0;
                copy_from_user(&ri, (READINFO*)ioctl_param, sizeof(READINFO));
                adr = vid_atom_get_stillp_buf(ri.color) + ri.offset;
                copy_to_user(ri.buf, (void*)adr, ri.size);
                break;
            }

//lingh added for PVR demo
                case MPEG_VID_SET_RB_SIZE:
                        {
                                vid_atom_set_rb_size(ioctl_param);
                                break;
                        }
                case MPEG_VID_SINGLE_FRAME:
                        {
                                ret = vid_osi_single_frame(ioctl_param);
                                break;
                        }
                case MPEG_VID_SF_READY:
                        {
                                ret = vid_osi_single_frame_complete();
                                break;
                        }
                case MPEG_VID_RESUME_FROM_SF:
                        {
                                ret = vid_atom_resume_from_sf(ioctl_param);
                                break;
                        }
                case MPEG_VID_RESET_RB:
                        {
                                ret = vid_atom_reset_ratebuf(ioctl_param);
                                break;
                        }

                case MPEG_VID_RESET_CLIPBUF:
                        //make sure video has stopped before this call
                        {
                                ret = vid_osi_reset_clipbuf();
                break;
                        }

        case MPEG_VID_CLIP_CC:
            {
                ret = vid_osi_clip_cc();
                break;
            }

        case MPEG_VID_CLIP_FLUSH:
            {
                ret = vid_osi_clip_flush();
                break;
            }

        case MPEG_VID_CC_COMPLETE:
            {
                ret = vid_osd_cc_complete();
                break;
            }

	case MPEG_VID_PLAY_STATUS:
	    {
	        VIDPLAYSTATUS status;

//              copy_from_user(&status, (void *)ioctl_param, sizeof(status));
                vid_osi_play_status(&status);
                copy_to_user((void *)ioctl_param, &status, sizeof(status));
	        break;
	    }


        case MPEG_VID_SET_SFM:
	    {
	      ret = vid_osi_set_sfm((vidsfm_t)ioctl_param);
	      break;
	    }
	    
        default:
            PDEBUG("NOT SUPPORT\n");
            ret = -1;
    }

    return ret;
}

static unsigned int vid_inf_poll(struct file *filp,
                                 struct poll_table_struct *wait)
{
    PDEBUG("not finished yet\n");
    return 0;
}

static struct file_operations Fops = {
    read:   vid_inf_read,
    write:  vid_inf_write,
    ioctl:  vid_inf_ioctl,
    open:   vid_inf_open,
    release:vid_inf_release,
    poll:   vid_inf_poll,
    mmap:   vid_inf_mmap
};


static struct {
                char *name;
		int minor;
		struct file_operations *fops;
		devfs_handle_t devfs_handle;
              } devnodes[] = { 
			                  {"vid",0,&Fops,0},
					  {"vdec_dev",0,&Fops,0},
				       };

static int no_devnodes = sizeof(devnodes)/sizeof(devnodes[0]);

/* Initialize the module - Register the character device */
int vid_init_module()
{
    int ret_val;
    devfs_handle_t devfs_handle;
    int i;

    scrman_osi_init();
    vid_osd_init();
    vid_osd_close();

    /* Register the character device (atleast try) */
    ret_val = devfs_register_chrdev(MAJOR_NUM_VDEC, DEVICE_NAME_VDEC, &Fops);
    /* Negative values signify an error */
    if (ret_val < 0)
    {
        PDEBUG("VID: %s failed with %d\n",
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
                                      MAJOR_NUM_VDEC, devnodes[i].minor,
                                      S_IFCHR | S_IRUSR | S_IWUSR,
                                      devnodes[i].fops, NULL);
        devnodes[i].devfs_handle = devfs_handle;
      }
      else
      {
        devnodes[i].devfs_handle = NULL;
      }
    }
    

    //the following was moved from vid_osi_init    
    //install interrupt handler
     if(os_install_irq(IRQ_VID, 
                      IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG,
					  vid_atom_irq_handle,
					  (void*)&_videoDecoder) != 0)
    {
        PDEBUG("install interrupt handler error\n");
        devfs_unregister_chrdev(MAJOR_NUM_VDEC, DEVICE_NAME_VDEC);
        for(i = 0; i < no_devnodes; i++)
        {
          if(devnodes[i].devfs_handle != NULL)
            devfs_unregister(devnodes[i].devfs_handle);
        }
        return -1;
    }

    //DEMUX_REG_AVCB(&vid_callback, 1, 1);
    PDEBUG("VID: Initialize vdec_dev OK!\n");
    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
void vid_cleanup_module()
{
    int ret;
    int i;

   // the following was moved from vid_osi_close 
   os_uninstall_irq(IRQ_VID);      //remove irq handler


    /* Unregister the device */
    //DEMUX_REG_AVCB(NULL, 1, 0);
    ret = devfs_unregister_chrdev(MAJOR_NUM_VDEC, DEVICE_NAME_VDEC);
    /* If there's an error, report it */
    if (ret < 0)
        printk("Error in module_unregister_chrdev: %d\n", ret);

    for(i = 0; i < no_devnodes; i++)
    {
      if(devnodes[i].devfs_handle != NULL)
        devfs_unregister(devnodes[i].devfs_handle);

    }

    return;
}
