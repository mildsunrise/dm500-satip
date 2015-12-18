/*---------------------------------------------------------------------------+
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
|       COPYRIGHT   I B M   CORPORATION 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Zongwei Liu
| Component: sci
| File:      sci_inf.c
| Purpose:   POSIX interface of Smart Card Interface device driver
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/26/2001  Zongwei Liu       Initial check-in.
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/13/2001  MAL, Zongwei Liu  Added sci_osd_init() parameters to set detect 
|                               and Vcc enable active polarities 
|                               (which are board dependent).
| 12/13/2001  MAL, Zongwei Liu  Added EMV2000 support and made several changes 
|                               to improve PIO efficiency.
| 01/11/2002  MAL, zongwei Liu  Add timeout to read/write function
+----------------------------------------------------------------------------*/

/* The necessary header files */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/system.h>

#include <linux/ioctl.h>
#include <asm/uaccess.h>

#include "os/drv_debug.h"
#include "os/os-types.h"

/* local includes */
#include "sci/sci_inf.h"
#include "sci_osi.h"
#include "sci_osd.h"

MODULE_AUTHOR("Mike Lepore, Zongwei Liu");
MODULE_DESCRIPTION("Driver for Smart Card Interface of IBM Redwood5");
EXPORT_NO_SYMBOLS;

#ifdef MODULE
MODULE_PARM(detect_p, "i");
MODULE_PARM_DESC(detect_p,
                 "Select the detect polarity of smart card: 0 - ACTIVE_LOW, 1 - ACTIVE_HIGH");
MODULE_PARM(vcc_p, "i");
MODULE_PARM_DESC(vcc_p,
                 "Select the VCC polarity of smart card: 0 - ACTIVE_LOW, 1 - ACTIVE_HIGH");
#endif
static INT detect_p = ACTIVE_LOW;
static INT vcc_p    = ACTIVE_LOW;

/* allocate device control blocks */
SCI_CONTROL_BLOCK sci_cb[SCI_NUMBER_OF_CONTROLLERS];

/* Public API function prototypes */
static int sci_inf_open(struct inode *inode, 
                        struct file *filp);

static int sci_inf_release(struct inode *inode, 
                           struct file *filp);

static ssize_t sci_inf_read(struct file *file,
                            char *buffer, 
                            size_t length, 
                            loff_t * offset);

static ssize_t sci_inf_write(struct file *file,
                             const char *buffer,
                             size_t length, 
                             loff_t * offset);

static int sci_inf_ioctl(struct inode *inode,
                         struct file *file,
                         unsigned int ioctl_num,
                         unsigned long ioctl_param);

static struct file_operations Fops = 
{
    open:    sci_inf_open,
    release: sci_inf_release,
    read:    sci_inf_read,
    write:   sci_inf_write,
    ioctl:   sci_inf_ioctl
};

void sci_dump_regs(ULONG sci_id);

/****************************************************************************
** Name:        sci_inf_open
**
** Purpose:     POSIX device open
**
** Parameters:  inode:
**              filp:
**
** Returns      =0: success
**              <0: if any error occur
****************************************************************************/
static int sci_inf_open(struct inode *inode, struct file *filp)
{
    ULONG sci_id;
    int rc;

    if ((sci_id = MINOR(inode->i_rdev)) <= SCI_NUMBER_OF_CONTROLLERS)
    {
        if (sci_cb[sci_id].driver_inuse == 0)
        {
            if (sci_osi_is_card_present(sci_id) == 1)
            {
                sci_cb[sci_id].driver_inuse ++;
                rc = 0;
                MOD_INC_USE_COUNT;
            }
            else
            {
                PDEBUG("num %d card not present\n", (UINT) sci_id);
                rc = -EINVAL;
            }
        }
        else
        {
            PDEBUG("num %d card is busy\n", (UINT) sci_id);
            rc = -EBUSY;
        }
    }
    else
    {
        PDEBUG("error card num: %d\n", (UINT) sci_id);
        rc = -ENODEV;
    }

    return (rc);
}

/****************************************************************************
** Name:        sci_inf_release
**
** Purpose:     POSIX device close
**
** Parameters:  inode:
**              filp:
**
** Returns      =0: success
**              <0: if any error occur
****************************************************************************/
static int sci_inf_release(struct inode *inode, struct file *filp)
{
    ULONG sci_id;
    int rc;

    if ((sci_id = MINOR(inode->i_rdev)) <= SCI_NUMBER_OF_CONTROLLERS)
    {
        if (sci_cb[sci_id].driver_inuse != 0)
        {
            sci_osd_deactivate(sci_id);
            sci_cb[sci_id].driver_inuse = 0;
            rc = 0;
        }
        else
        {
            PDEBUG("driver of num%d card is not opend\n", (UINT) sci_id);
            rc = -EINVAL;
        }
    }
    else
    {
        PDEBUG("error card num: %d\n", (UINT) sci_id);
        rc = -ENODEV;
    }

    if (rc == 0)
    {
        MOD_DEC_USE_COUNT;
    }

    return rc;
}

/****************************************************************************
** Name:        sci_inf_read
**
** Purpose:     POSIX smart card read 
**
** Parameters:  file:   input pointer to the file handle
**              buffer: output pointer to the output data buffer
**              length: the expect characters number
**              offset: offset of the file
**
** Returns      >=0: the actually number of read characters
**              <0: if any error occur
****************************************************************************/
static ssize_t sci_inf_read(struct file *file,
                            char *buffer, 
                            size_t length, 
                            loff_t * offset
)
{
    ULONG sci_id;
    UCHAR *p_buffer;
    ULONG *p_bytes_read;
    ULONG count;
    ssize_t rc;
    SCI_ERROR sci_rc;

    /* sci_id is the Minor Num of this device */
    sci_id = MINOR(file->f_dentry->d_inode->i_rdev);

    p_bytes_read = &count;

    if (length <= SCI_BUFFER_SIZE)
    {
        p_buffer = sci_cb[sci_id].read_buf;
        sci_rc = sci_osd_read(sci_id, 
                              p_buffer, 
                              (ULONG) length, 
                              p_bytes_read,
                              sci_cb[sci_id].sci_modes.rw_mode);

        if ((count = *p_bytes_read) > 0)
        {
            copy_to_user((void *)buffer, (const void *)p_buffer, count);
            rc = (ssize_t) count;
        }
        else
        {
            PDEBUG("card %d error=%d\n", (UINT) sci_id, sci_rc);
            rc = sci_rc;
        }
    }
    else
    {
        PDEBUG("error buffer size\n");
        rc = -EINVAL;
    }

    return (rc);
}

/****************************************************************************
** Name:        sci_inf_write
**
** Purpose:     POSIX smart card write
**
** Parameters:  file:   input pointer to the file handle
**              buffer: input pointer to the input data buffer
**              length: the characters number of input buffer
**              offset: offset of the file
**
** Returns      >=0: the actually number of writen characters
**              <0: if any error occur
****************************************************************************/
static ssize_t sci_inf_write(struct file *file, 
                             const char *buffer, 
                             size_t length, 
                             loff_t * offset
)
{
    ssize_t rc;
    ULONG sci_id;
    UCHAR *p_buffer;
    SCI_ERROR sci_rc;

    /* sci_id is the Minor Num of this device */
    sci_id = MINOR(file->f_dentry->d_inode->i_rdev);

    if (length <= SCI_BUFFER_SIZE)
    {
        p_buffer = sci_cb[sci_id].write_buf;
        copy_from_user((void *)p_buffer, (const void *)buffer, length);

        sci_rc = sci_osd_write(sci_id, 
                               p_buffer, 
                               (ULONG) length,
                               sci_cb[sci_id].sci_modes.rw_mode);

        if (sci_rc == SCI_ERROR_OK)
        {
            rc = (ssize_t) length;
        }
        else
        {
            PDEBUG("card%d error=%d\n", (UINT) sci_id, sci_rc);
            rc = sci_rc;
        }
    }
    else
    {
        PDEBUG("error buffer size\n");
        rc = -EINVAL;
    }

    return (rc);
}

/****************************************************************************
** Name:        sci_inf_ioctl
**
** Purpose:     POSIX ioctl
**
** Parameters:  inode:
**              file:
**              ioctl_num:   special operation number of this device
**              ioctl_param: input/output parameters of ioctl
**
** Returns      =0: success
**              <0: if any error occur
****************************************************************************/
int sci_inf_ioctl(struct inode *inode, 
                  struct file *file, 
                  unsigned int ioctl_num,    /* The number of ioctl */
                  unsigned long ioctl_param  /* The parameter to it */
)
{
    ULONG sci_id;
    int rc;
    SCI_MODES sci_mode;
    SCI_PARAMETERS sci_param;
    SCI_ERROR sci_rc;

    PDEBUG("enter ioctl\n");

    sci_id = MINOR(inode->i_rdev);

    switch (ioctl_num)
    {
        case IOCTL_SET_RESET:
            if (sci_osd_reset(sci_id) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_SET_MODES:
            copy_from_user((void *) &sci_mode, 
                           (const void *) ioctl_param,
                           sizeof(SCI_MODES));
            if (sci_osi_set_modes(sci_id, &sci_mode) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_GET_MODES:
            if (sci_osi_get_modes(sci_id, &sci_mode) == SCI_ERROR_OK)
            {
                copy_to_user((void *) ioctl_param, 
                             (const void *) &sci_mode,
                             sizeof(SCI_MODES));
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_SET_PARAMETERS:
            copy_from_user((void *) &sci_param, 
                           (const void *) ioctl_param,
                           sizeof(SCI_PARAMETERS));
            if (sci_osi_set_parameters(sci_id, &sci_param) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_GET_PARAMETERS:
            if (sci_osi_get_parameters(sci_id, &sci_param) == SCI_ERROR_OK)
            {
                copy_to_user((void *) ioctl_param, 
                             (const void *) &sci_param,
                             sizeof(SCI_PARAMETERS));
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_SET_CLOCK_START:
            if (sci_osi_clock_start(sci_id) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_SET_CLOCK_STOP:
            if (sci_osi_clock_stop(sci_id) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_GET_IS_CARD_PRESENT:
            sci_rc = sci_osi_is_card_present(sci_id);
            put_user(sci_rc, (unsigned long *) ioctl_param);
            rc = 0;
            break;

        case IOCTL_GET_IS_CARD_ACTIVATED:
            sci_rc = sci_osi_is_card_activated(sci_id);
            put_user(sci_rc, (unsigned long *) ioctl_param);
            rc = 0;
            break;

        case IOCTL_SET_DEACTIVATE:
            if (sci_osd_deactivate(sci_id) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_DUMP_REGS:
            sci_dump_regs(sci_id);
            rc = 0;
            break;

        case IOCTL_SET_ATR_READY:
            sci_cb[sci_id].atr_status = SCI_ATR_READY;
            rc = 0;
            break;

        case IOCTL_GET_ATR_STATUS:
            put_user(sci_cb[sci_id].atr_status,
                     (unsigned long *) ioctl_param);
            rc = 0;
            break;

        default:
            PDEBUG("error ioctl_num %d\n", ioctl_num);
            rc = -1;
    }

    if (rc != 0)
    {
        PDEBUG("ioctl failed\n");
    }

    PDEBUG("ioctl exit\n");
    return rc;
}

/****************************************************************************
**
**      POSIX Module Declarations
**
****************************************************************************/

static struct {
                char *name;
		int minor;
		struct file_operations *fops;
		devfs_handle_t devfs_handle;
              } devnodes[] = { 
			        {"sci0",0,&Fops,0},
			        {"sci1",1,&Fops,0},
	                     };

static int no_devnodes = sizeof(devnodes)/sizeof(devnodes[0]);

/* init module */
int __init sci_module_init(void)
{
    int rc;
    devfs_handle_t devfs_handle;
    int i;

    if (sci_osd_init(detect_p, vcc_p) == SCI_ERROR_OK)
    {
        /* Register the character device (at least try) */
        if ((rc = devfs_register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops)) >= 0)
        {
            PDEBUG("success\n");
            rc = 0;
        }

        for(i=0; i < no_devnodes; i++)
        {
          devfs_handle = devfs_find_handle(NULL, devnodes[i].name,
                                    0, 0, DEVFS_SPECIAL_CHR,0);
    
          if(devfs_handle == NULL)
          {
            devfs_handle = devfs_register(NULL, devnodes[i].name, DEVFS_FL_DEFAULT,
                                          MAJOR_NUM, devnodes[i].minor,
                                          S_IFCHR | S_IRUSR | S_IWUSR,
                                          devnodes[i].fops, NULL);
            devnodes[i].devfs_handle = devfs_handle;
          }
          else
          {
            devnodes[i].devfs_handle = NULL;
          }
        }
    }
    else
    {
        PDEBUG("failed\n");
        rc = -1;
    }

    return (rc);
}

/* cleanup module */
void __exit sci_module_cleanup(void)
{
  int i;
  
    sci_osd_uninit();

    /* unregister the device */
    if (devfs_unregister_chrdev(MAJOR_NUM, DEVICE_NAME) < 0)
    {
        PDEBUG("error in module_unregister_chrdev\n");
    }

    for(i = 0; i < no_devnodes; i++)
    {
      if(devnodes[i].devfs_handle != NULL)
        devfs_unregister(devnodes[i].devfs_handle);

    }

}

/* Module loading/unloading */
module_init(sci_module_init);
module_exit(sci_module_cleanup);

/****************************************************************************
** Function:    sci_dump_regs
**
** Purpose:     Print contents of all SCI registers
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_dump_regs(ULONG sci_id)
{
    printk("\n");
    printk("Smart Card Interface %x Registers:\n", (UINT) sci_id);
    printk("------------------------------------");
    printk("------------------------------------\n");
    printk("SCCTL0     (0x04)   0x%02x   |    ",
           *(UCHAR *) sci_cb[sci_id].scctl0);
    printk("SCCTL1     (0x05)   0x%02x\n",
           *(UCHAR *) sci_cb[sci_id].scctl1);
    printk("SCINTEN01  (0x06)   0x%04x |    ",
           *(USHORT *) sci_cb[sci_id].scinten);
    printk("SCINT01    (0x08)   0x%04x\n",
           *(USHORT *) sci_cb[sci_id].scint);
    printk("SCSTAT     (0x0A)   0x%02x   |    ",
           *(UCHAR *) sci_cb[sci_id].scstat);
    printk("SCBMR      (0x0B)   0x%02x\n",
           *(UCHAR *) sci_cb[sci_id].scbmr);
    printk("SCCLK_CNT0 (0x0C)   0x%02x   |    ",
           *(UCHAR *) sci_cb[sci_id].scclk_cnt0);
    printk("SCCTL3     (0x0D)   0x%02x\n",
           *(UCHAR *) sci_cb[sci_id].scctl3);
    printk("SCETU      (0x0E)   0x%04x |    ",
           *(USHORT *) sci_cb[sci_id].scetu);
    printk("SCRXLEN    (0x10)   0x%04x\n",
           *(USHORT *) sci_cb[sci_id].scrxlen);
    printk("SCTXLEN    (0x12)   0x%04x |    ",
           *(USHORT *) sci_cb[sci_id].sctxlen);
    printk("SCCWT      (0x14)   0x%04x\n",
           *(USHORT *) sci_cb[sci_id].sccwt);
    printk("SCEGT      (0x16)   0x%02x   |    ",
           *(UCHAR *) sci_cb[sci_id].scegt);
    printk("SCBWT      (0x18)   0x%08x\n",
           *(UINT *) sci_cb[sci_id].scbwt);
    printk("------------------------------------");
    printk("------------------------------------\n");
}

