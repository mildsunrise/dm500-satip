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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| Author:    Mark Detrick
| Component: scp
| File:      scp_inf.c
| Purpose:   Interface layer for serial control port
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 09/19/2003  MSD               Created.
+----------------------------------------------------------------------------*/
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/system.h>
#include <linux/string.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/mm.h> 
#include <asm/pgtable.h>

#include "scp/scp_inf.h"
#include "scp_osd.h"
#include "xilinx.h"

#define SCP_DRIVER_NAME   "STBx25xx/04xxx SCP"

#include "os/pversion.h"
#include "os/drv_debug.h"

#ifdef MODULE

MODULE_AUTHOR("Mark Detrick / IBM Endicott");
MODULE_DESCRIPTION("Serial Control Port (SCP) driver for IBM STBx25xx/04xxx Chip");

#endif

static int scp_inf_open(struct inode *inode, struct file *file);
static int scp_inf_release(struct inode *inode, struct file *file);

static int scp_inf_ioctl(struct inode *inode, 
                     struct file *filp, 
                     unsigned int ioctl_cmd, 
                     unsigned long ioctl_param);


static struct file_operations scpFops = 
{
    open:       scp_inf_open,
    release:    scp_inf_release,      /* a.k.a. close */
    ioctl:      scp_inf_ioctl,
  };

static int _open_count = 0;

void sflash_inf_init();


static int scp_inf_open(struct inode *inode, struct file *file)
{
    PDEBUG("scp_inf_open: filep=0x%8.8x\n", (unsigned int)file);
    /* We don't talk to two processes at the same time */
    if (_open_count != 0)
    {
        printk("SCP: Driver in use\n");
        return -EBUSY;
    }

    _open_count++;
    
    MOD_INC_USE_COUNT;  // MODULE
        
    return 0;
}

static int scp_inf_release(struct inode *inode, struct file *file)
{
    PDEBUG("scp_inf_release: filep=0x%8.8x\n", (unsigned int)file);

    _open_count--;
    
    MOD_DEC_USE_COUNT;

    return 0;
}


static int scp_inf_ioctl(struct inode *inode, 
                     struct file *filp, 
                     unsigned int ioctl_cmd, 
                     unsigned long ioctl_param) 
{
    int rc = -1;
    unsigned long value;
    char input[SCP_BUFFER_SIZE];
    char output[SCP_BUFFER_SIZE];
   
    PDEBUG("scp_inf_ioctl: cmd magic=0x%2.2x, id=%d, mode=0x%2.2x, parm= 0x%8.8x\n", _IOC_TYPE(ioctl_cmd), _IOC_NR(ioctl_cmd), _IOC_DIR(ioctl_cmd), (UINT)ioctl_param);
    
    switch(ioctl_cmd)
    {
        case IOCTL_SCP_DISPLAY_REGS:
           scp_osi_display_regs();
           rc = 0;
           break;
        case IOCTL_SCP_GET_CDM:
           if(scp_osi_get_cmd(&value) == 0)
           { 
              copy_to_user((void *) ioctl_param,(const void *) &value, sizeof(value));
              rc = 0;
           }
           else
           {
              rc = -1;
           }      
           break;
        case IOCTL_SCP_SET_CDM:
           copy_from_user((void *) &value,(const void *) ioctl_param, sizeof(value));
           rc = scp_osi_set_cmd(value);
           break;
        case IOCTL_SCP_GET_REV_DATA:
           if(scp_osi_get_reverse_data(&value) == 0)
           { 
              copy_to_user((void *) ioctl_param,(const void *) &value, sizeof(value));
              rc = 0;
           }
           else
           {
              rc = -1;
           }      
           break;
        case IOCTL_SCP_SET_REV_DATA:
           copy_from_user((void *) &value,(const void *) ioctl_param, sizeof(value));
           rc = scp_osi_set_reverse_data(value);
           break;
        case IOCTL_SCP_GET_CLK_INV:
           if(scp_osi_get_clock_invert(&value) == 0)
           { 
              copy_to_user((void *) ioctl_param,(const void *) &value, sizeof(value));
              rc = 0;
           }
           else
           {
              rc = -1;
           }      
           break;
        case IOCTL_SCP_SET_CLK_INV:
           copy_from_user((void *) &value,(const void *) ioctl_param, sizeof(value));
           rc = scp_osi_set_clock_invert(value);
           break;
       case IOCTL_SCP_GET_LOOPBACK:
           if(scp_osi_get_loopback(&value) == 0)
           { 
              copy_to_user((void *) ioctl_param,(const void *) &value, sizeof(value));
              rc = 0;
           }
           else
           {
              rc = -1;
           }   
           break;
        case IOCTL_SCP_SET_LOOPBACK:
           copy_from_user((void *) &value,(const void *) ioctl_param, sizeof(value));
           rc = scp_osi_set_loopback(value);
           break;
           
        // the following are sflash functions used for testing the SCP
        case IOCTL_SCP_RW:
           copy_from_user((void *) &value, &(((SCP_RW_STRUCT*)ioctl_param)->count), sizeof(value));
           if(value>SCP_BUFFER_SIZE)
           {
              PDEBUG("scp_inf_ioctl: IOCTL_SCP_RW failed. Count parameter 0x%x is greater than 0x%x\n",value, SCP_BUFFER_SIZE); 
              rc = -1;
              break;
           }   
           copy_from_user(input, (((SCP_RW_STRUCT*)ioctl_param)->write_ptr), value);
           if(scp_osd_rw(input, output, (int) value, 0)==0)
           { 
              copy_to_user((((SCP_RW_STRUCT*)ioctl_param)->read_ptr), output, value);
              rc = 0;
           }
           else
           {
              rc = -1;
           }   
           break;
        case IOCTL_SFLASH_INIT:
           sflash_inf_init();
           rc = 0;
           break;
        default:
           PDEBUG("error ioctl_cmd %d\n", ioctl_cmd);
           rc = -1;
    }

    if (rc != 0)
    {
        PDEBUG("ioctl failed\n");
    }

    return rc;
}

static devfs_handle_t devfs_handle;

static int __init scp_inf_init(void)
{
    int rtn = -1; 
       
    rtn = scp_osd_init();
  
    PDEBUG("scp_inf_init: installing the scp module\n");
    // print the driver verision info for futher reference
    PVERSION(SCP_DRIVER_NAME);

    if(rtn < 0) 
    {
        PFATALE("SCP: Failed to initialize device!\n"); 
        return -1;
    }

    if (devfs_register_chrdev(SCP_DEV_MAJOR, SCP_DRIVER_NAME, &scpFops) < 0)
    {
        PFATALE("SCP: Failed to register device!\n"); 
        return -1;
    }

    devfs_handle = devfs_find_handle(NULL, "scp",
                                0, 0, DEVFS_SPECIAL_CHR,0);
    
    if(devfs_handle == NULL)
    {
      devfs_handle = devfs_register(NULL, "scp", DEVFS_FL_DEFAULT,
                                SCP_DEV_MAJOR, 0,
                                S_IFCHR | S_IRUSR | S_IWUSR,
                                &scpFops, NULL);
    }
    else
      devfs_handle = NULL;
      
    return 0;
}

static void __exit scp_inf_deinit(void)
{
    scp_osd_uninit();
    devfs_unregister_chrdev(SCP_DEV_MAJOR, SCP_DRIVER_NAME);
    if(devfs_handle != NULL)
      devfs_unregister(devfs_handle);
}

void sflash_inf_init()
{
    /* Turn off the reset on SFLASH */
    xilinx_sflash_reset_off();

    /* disable the flash write protect signal */
    xilinx_sflash_wp_off();
}

module_init(scp_inf_init);
module_exit(scp_inf_deinit);



