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
|       IBM CONFIDENTIAL
|       STB025XX VXWORKS EVALUATION KIT SOFTWARE
|       (C) COPYRIGHT IBM CORPORATION 2003
|
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Tony J. Cerreto
| Component: TTX
| File:      ttx_osd_inf.h
| Purpose:   The interface function of TTX for Linux
|
| Changes:
| Date:      Author  Comment:
| ---------  ------  --------
| 25-Sep-03  TJC     Created
+----------------------------------------------------------------------------*/
#include <linux/config.h>
#include <linux/version.h>
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/version.h>
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
#include <../include/ttx/ttx_osd_user.h>
#include "ttx.h"

/*----------------------------------------------------------------------------+
| Local Defines
+----------------------------------------------------------------------------*/
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
#endif

#define TTX_MAJOR_NUM     252
#define TTX_MINOR_NUM     192
#define TTX_DEVICE_NAME   "ttx0"

/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/
static int     ttx_device_open   (struct inode *inode, struct file *filp);
static int     ttx_device_release(struct inode *inode, struct file *filp);
static ssize_t ttx_device_write  (struct file *filp, const char *buffer, size_t length, loff_t * offset);
static int     ttx_device_ioctl  (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

/*----------------------------------------------------------------------------+
| FILE Declarations
+----------------------------------------------------------------------------*/
static struct file_operations ttx_device_fops =
{
   open:    ttx_device_open,
   release: ttx_device_release,
   write:   ttx_device_write,
   ioctl:   ttx_device_ioctl
};

/*----------------------------------------------------------------------------+
| XXXX   XX   XX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX
|  XX    XXX  XX   X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX
|  XX    XXXX XX     XX     XX X     XX  XX  XXXX XX   XX  XX   XX
|  XX    XX XXXX     XX     XXXX     XXXXX   XX XXXX   XX  XX   XX
|  XX    XX  XXX     XX     XX X     XX XX   XX  XXX   XXXXXX   XX
|  XX    XX   XX     XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX
| XXXX   XX   XX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| XXXXXXX  XXX XXX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX
|  XX   X   XX XX    X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX
|  XX X      XXX       XX     XX X     XX  XX  XXXX XX   XX  XX   XX
|  XXXX       X        XX     XXXX     XXXXX   XX XXXX   XX  XX   XX
|  XX X      XXX       XX     XX X     XX XX   XX  XXX   XXXXXX   XX
|  XX   X   XX XX      XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX
| XXXXXXX  XXX XXX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
|   XX     XXXXXX    XXXXXX    XXXXX
|  XXXX    XX   XX     XX     XX   XX
| XX  XX   XX   XX     XX      XX
| XX  XX   XXXXX       XX        XX
| XXXXXX   XX          XX         XX
| XX  XX   XX          XX     XX   XX
| XX  XX   XX        XXXXXX    XXXXX
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|  ttx_device_open
+----------------------------------------------------------------------------*/
static int ttx_device_open(
  struct inode *inode,
  struct file  *filp)

{

  if (MINOR(inode->i_rdev) != TTX_MINOR_NUM) {
    printk("Error opening TTX device\n");
    printk("Incorrect Minor Number.  Expected %d, Received %d\n",
            TTX_MINOR_NUM, MINOR(inode->i_rdev));
    return(-1);
  }

  return(ttx_initialize());
}

/*----------------------------------------------------------------------------+
|  ttx_device_release
+----------------------------------------------------------------------------*/
static int ttx_device_release(
  struct inode *inode,
  struct file  *filp)

{
  return(ttx_terminate());
}

/*----------------------------------------------------------------------------+
|  ttx_device_write
+----------------------------------------------------------------------------*/
static ssize_t ttx_device_write (
  struct file *filp,
  const char  *buffer,
  size_t      length,
  loff_t      *offset)

{
  return ttx_write((void*)buffer, length);
}

/*----------------------------------------------------------------------------+
|  ttx_device_ioctl
+----------------------------------------------------------------------------*/
static int ttx_device_ioctl(
  struct inode  *inode,
  struct file   *filp,
  unsigned int  cmd,
  unsigned long arg)

{

  switch (cmd) {
     case TTX_START:
          return(ttx_insert_pid(arg));
          break;

     case TTX_STOP:
          return(ttx_delete_pid());
          break;

     case TTX_STATS_GET:
          ttx_stats_get(arg);
          return(0);
          break;

     case TTX_STATS_CLEAR:
          ttx_stats_clear();
          return(0);
          break;

     default:
          return(-EINVAL);
          break;
  }

  return(0);
}

static devfs_handle_t devfs_handle;

/*----------------------------------------------------------------------------+
|  ttx_init_module
+----------------------------------------------------------------------------*/
int ttx_init_module()
{


    if (devfs_register_chrdev(TTX_MAJOR_NUM, TTX_DEVICE_NAME, &ttx_device_fops))
    {
      printk("Error registering TTX device\n");
      return(-1);
    }

    devfs_handle = devfs_find_handle(NULL, TTX_DEVICE_NAME,
                                0, 0, DEVFS_SPECIAL_CHR,0);
    
    if(devfs_handle == NULL)
    {
      devfs_handle = devfs_register(NULL, TTX_DEVICE_NAME, DEVFS_FL_DEFAULT,
                                TTX_MAJOR_NUM, TTX_MINOR_NUM,
                                S_IFCHR | S_IRUSR | S_IWUSR,
                                &ttx_device_fops, NULL);
    }
    else
      devfs_handle = NULL;
      
    return(0);
}

/*----------------------------------------------------------------------------+
|  ttx_cleanup_module
+----------------------------------------------------------------------------*/
void ttx_cleanup_module()
{
    int rc;

    rc = devfs_unregister_chrdev(TTX_MAJOR_NUM, TTX_DEVICE_NAME);
    if(devfs_handle != NULL)
      devfs_unregister(devfs_handle);
    if (rc < 0) {
      printk("Error unregistering TTX device\n");
    }

    return;
}

/*----------------------------------------------------------------------------+
|  Modularization
+----------------------------------------------------------------------------*/
module_init(ttx_init_module);
module_exit(ttx_cleanup_module);
