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
|       COPYRIGHT   I B M   CORPORATION 1999
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Lin Guo Hui
| Component: XP
| File:      xp_osd_inf.h
| Purpose:   The interface function of XP for Linux
|
| Changes:
| Date:      Author  Comment:
| ---------  ------  --------
| 30-Sep-01  LGH    Created
+----------------------------------------------------------------------------*/
/* The necessary header files */
/* The necessary header files */
#include <linux/config.h>
#include <linux/version.h>

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/in.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#include "xp_osi_global.h"
#include "xp_atom_reg.h"
#include "xp_osd_drv.h"
#include <xp/pvr_osd.h>

/* In 2.2.3 /usr/include/linux/version.h includes a
 * macro for this, but 2.0.35 doesn't - so I add it
 * here if necessary. */

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
#endif

#define XP_MAJOR_NUM 102
#define XP_DEVICE_NAME "demux"
 /*
 * Split minors in two parts
 */

#define TYPE(dev)   (MINOR(dev) >> 4)  /* high nibble */
#define NUM(dev)    (MINOR(dev) & 0xf) /* low  nibble */

static ssize_t xp0_device_read(struct file *file, char *buffer, size_t length, loff_t *offset);
static int xp0_device_open(struct inode *inode, struct file *filp);
static int xp0_device_release(struct inode *inode, struct file *filp);
static int xp0_device_ioctl(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg);
static int xp0_device_fasync(int fd, struct file *filp, int mode);
static int xp0_device_mmap(struct file *file, struct vm_area_struct *vma);
static unsigned int xp0_device_poll(struct file *file,struct poll_table_struct *wait);

static ssize_t xp1_device_read(struct file *file, char *buffer, size_t length, loff_t *offset);
static int xp1_device_open(struct inode *inode, struct file *filp);
static int xp1_device_release(struct inode *inode, struct file *filp);
static int xp1_device_ioctl(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg);
static int xp1_device_fasync(int fd, struct file *filp, int mode);
static int xp1_device_mmap(struct file *file, struct vm_area_struct *vma);
static unsigned int xp1_device_poll(struct file *file,struct poll_table_struct *wait);

static ssize_t xp2_device_read(struct file *file, char *buffer, size_t length, loff_t *offset);
static int xp2_device_open(struct inode *inode, struct file *filp);
static int xp2_device_release(struct inode *inode, struct file *filp);
static int xp2_device_ioctl(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg);
static int xp2_device_fasync(int fd, struct file *filp, int mode);
static int xp2_device_mmap(struct file *file, struct vm_area_struct *vma);
static unsigned int xp2_device_poll(struct file *file,struct poll_table_struct *wait);
  
static int pvr_device_open(struct inode *inode, struct file *filp);
static int pvr_device_release(struct inode *inode, struct file *filp);
static ssize_t pvr_device_write(struct file *file,const char *buffer,size_t length, loff_t * offset);
static int pvr_device_ioctl(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg);
int pvr_device_mmap(struct file *file, struct vm_area_struct *vma);

int pvr_osd_open(struct inode *inode,struct file *filp);
int pvr_osd_release(struct inode *inode, struct file *file);
int pvr_osd_ioctl(struct inode *inode, struct file *file, 
				  unsigned int ioctl_num, unsigned long ioctl_param);
ssize_t pvr_osd_write(struct file *file,const char *buffer,
						 size_t length, loff_t * offset);

//A global variant contains information of all the filters
DEMUX_DEVICE demux_dev[MAX_XP_NUM];

/**********************************************************



 * demux device

 *********************************************************/



static struct file_operations xp0_device_fops =

{

ioctl: xp0_device_ioctl,

poll: xp0_device_poll,

read: xp0_device_read,

open: xp0_device_open,

release: xp0_device_release,

mmap: xp0_device_mmap,

fasync: xp0_device_fasync
};



static struct file_operations xp1_device_fops =

{

ioctl: xp1_device_ioctl,

poll: xp1_device_poll,

read: xp1_device_read,

open: xp1_device_open,

release: xp1_device_release,

mmap: xp1_device_mmap,

fasync: xp1_device_fasync

};



static struct file_operations xp2_device_fops =
{

ioctl: xp2_device_ioctl,

poll: xp2_device_poll,

read: xp2_device_read,

open: xp2_device_open,

release: xp2_device_release,

mmap: xp2_device_mmap,

fasync: xp2_device_fasync

};

static struct file_operations pvr_device_fops =
{

ioctl: pvr_device_ioctl,
open: pvr_device_open,
write:  pvr_device_write,
release: pvr_device_release,
mmap: pvr_device_mmap
};


/**********************************************************
 * device array
*********************************************************/

static struct file_operations *xp_fop_array[] =
{
    &xp0_device_fops,
    &xp1_device_fops,
    &xp2_device_fops,
    &pvr_device_fops,
};



static int xp0_device_open(struct inode *inode,struct file *filp)
{

    int type = NUM(inode->i_rdev);

    if ((type < 0)||((type >= MAX_XP_NUM) && (type != XP_TYPE_PVR) )) return -ENODEV;

    /* dispatch to  specific open */
    if (type)
    {
      filp->f_op = xp_fop_array[type];
      return filp->f_op->open(inode, filp);
    }
    return demux_filter_open(&demux_dev[0],XP0,inode,filp);
}



static int xp1_device_open(struct inode *inode,
                       struct file *filp)
{
    return demux_filter_open(&demux_dev[1],XP1,inode,filp);
}

static int xp2_device_open(struct inode *inode,
                       struct file *filp)
{
    return demux_filter_open(&demux_dev[2],XP2,inode,filp);
}

static int pvr_device_open(struct inode *inode,
                       struct file *filp)
{
    return pvr_osd_open(inode,filp);
}

static ssize_t xp0_device_read(struct file *file, char *buffer,
                               size_t length, loff_t *offset)
{
    PDEBUG("entering into xp0_device_read\n");
    return demux_filter_read(XP0, file, buffer, length, offset);
}


static ssize_t xp1_device_read(struct file *file, char *buffer,
                               size_t length, loff_t *offset)
{
    return demux_filter_read(XP1,file, buffer, length, offset);
}

static ssize_t xp2_device_read(struct file *file, char *buffer,
                               size_t length, loff_t *offset)
{
    return demux_filter_read(XP2,file,buffer,length, offset);
}

static int xp0_device_release(struct inode *inode, struct file *filp)
{
    return demux_filter_release(XP0, inode,  filp);
}

static int xp1_device_release(struct inode *inode, struct file *filp)
{
   return demux_filter_release(XP1,inode, filp);
}

static int xp2_device_release(struct inode *inode, struct file *filp)
{
    return demux_filter_release(XP2,inode, filp);
}

static int pvr_device_release(struct inode *inode, struct file *filp)
{
    return pvr_osd_release(inode, filp);
}

static int xp0_device_ioctl(struct inode *inode, struct file *filp,
                            unsigned int cmd, unsigned long arg)
{
    return demux_filter_ioctl(XP0, inode,filp, cmd,  arg);
}

static int xp1_device_ioctl(struct inode *inode, struct file *filp,
                            unsigned int cmd, unsigned long arg)
{
    return demux_filter_ioctl(XP1, inode, filp,  cmd,  arg);
}

static int xp2_device_ioctl(struct inode *inode, struct file *filp,
                            unsigned int cmd, unsigned long arg)
{
    return demux_filter_ioctl(XP2, inode, filp, cmd, arg);
}

static int pvr_device_ioctl(struct inode *inode, struct file *filp,
                            unsigned int cmd, unsigned long arg)
{
    return pvr_osd_ioctl(inode,filp, cmd, arg);
}

static unsigned int xp0_device_poll(struct file *file,  struct poll_table_struct *wait)
{
    return demux_filter_poll(XP0,file,  wait);
}

static unsigned int xp1_device_poll(struct file *file,
                            struct poll_table_struct *wait)
{
    return demux_filter_poll(XP1,file,wait);
}

static unsigned int xp2_device_poll(struct file *file,
                            struct poll_table_struct *wait)
{
    return demux_filter_poll(XP2,file,  wait);
}

static int xp0_device_fasync(int fd, struct file *filp, int mode)
{
    DEMUX_FILTER *pDmxfilter;
    int rc;
       
    pDmxfilter = (DEMUX_FILTER *)filp->private_data;
    rc = fasync_helper(fd, filp, mode, &pDmxfilter->async_queue);
    return(rc);
}

static int xp1_device_fasync(int fd, struct file *filp, int mode)
{
    DEMUX_FILTER *pDmxfilter;
    int rc;
    
    pDmxfilter = (DEMUX_FILTER *)filp->private_data;
    rc = fasync_helper(fd, filp, mode, &pDmxfilter->async_queue);
    return(rc);
}

static int xp2_device_fasync(int fd, struct file *filp, int mode)
{
    DEMUX_FILTER *pDmxfilter;
    int rc;
    
    pDmxfilter = (DEMUX_FILTER *)filp->private_data;
    rc = fasync_helper(fd, filp, mode, &pDmxfilter->async_queue);
    return(rc);
}    

static int xp0_device_mmap(struct file *file, struct vm_area_struct *vma)
{
  int rc;
  
  rc = demux_filter_mmap(XP0,file,vma);
  return(rc);  
}
 
static int xp1_device_mmap(struct file *file, struct vm_area_struct *vma)
{
  int rc;
  
  rc = demux_filter_mmap(XP1,file,vma);
  return(rc);  
}
 
static int xp2_device_mmap(struct file *file, struct vm_area_struct *vma)
{
  int rc;
  
  rc = demux_filter_mmap(XP2,file,vma);
  return(rc);  
}

static ssize_t pvr_device_write(struct file *file,
                    const char *buffer,size_t length, loff_t * offset)
{
    return pvr_osd_write(file,buffer,length,offset);
}

static struct {
                char *name;
		int minor;
		struct file_operations *fops;
		devfs_handle_t devfs_handle;
              } devnodes[] = { 
			                  {"demuxapi0",0,&xp0_device_fops,0},
#ifdef __DRV_FOR_PALLAS__
					  {"demuxapi1",1,&xp0_device_fops,0},
					  {"demuxapi2",2,&xp0_device_fops,0},
#endif					  
					  {"pvr",3,&pvr_device_fops,0}
				       };

static int no_devnodes = sizeof(devnodes)/sizeof(devnodes[0]);


int demux_init_module()
{
    int i;
    devfs_handle_t devfs_handle;

    PDEBUG("entering into init_module\n");

    if (devfs_register_chrdev(XP_MAJOR_NUM, XP_DEVICE_NAME, &xp0_device_fops))
    {
      printk("register device error\n");
      return -1;
    }
    else
    {

    }

    for(i=0; i < no_devnodes; i++)
    {
      devfs_handle = devfs_find_handle(NULL, devnodes[i].name,
                                0, 0, DEVFS_SPECIAL_CHR,0);
    
      if(devfs_handle == NULL)
      {
        devfs_handle = devfs_register(NULL, devnodes[i].name, DEVFS_FL_DEFAULT,
                                      XP_MAJOR_NUM, devnodes[i].minor,
                                      S_IFCHR | S_IRUSR | S_IWUSR,
                                      devnodes[i].fops, NULL);
        devnodes[i].devfs_handle = devfs_handle;
      }
      else
      {
        devnodes[i].devfs_handle = NULL;
      }
    }
    
    
    for(i = 0;i<MAX_XP_NUM;i++)
    {
      demux_dev[i].uDeviceIndex = i;
      demux_dev[i].users = 0;
      demux_dev[i].uAlreadyInit = 0;
      demux_dev[i].uFilterNum = MAX_DEMUX_FILTER_NUM;
    }

    return 0;
}

void demux_cleanup_module()
{
    int rc;
    int i;
    
    rc = devfs_unregister_chrdev(XP_MAJOR_NUM, XP_DEVICE_NAME);
    if (rc < 0)
      printk("Error in module_unregister\n");

    for(i = 0; i < no_devnodes; i++)
    {
      if(devnodes[i].devfs_handle != NULL)
        devfs_unregister(devnodes[i].devfs_handle);

    }
    return;
}


