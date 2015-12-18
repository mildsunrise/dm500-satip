//vulcan/drv/rtc_fpc/rtc_fpc.c
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


#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/sched.h>

#include <linux/init.h>

#include <linux/fs.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/ioctl.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "rtc_fpc/rtc_fpc.h"
#include "rtc_fpc_local.h"


#include "os/pversion.h"
#define   RTC_FPC_DRIVER_NAME   "STBx25xx STBRTC"

//#define RTC_FPC_DEBUG


static unsigned char disp_seg_val[] = {0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6};

static unsigned long   ul_int_rtc;

static unsigned long   ul_24hr_fmt;

static DECLARE_WAIT_QUEUE_HEAD(wrtc_fpc_q);
static unsigned long  ul_alrm_wait;

static int rtc_fpc_get_time(STB_RTC_TIME *arg);
static int rtc_fpc_set_time(STB_RTC_TIME *arg);

static int rtc_fpc_get_alm_time(STB_RTC_TIME *arg);
static int rtc_fpc_set_alm_time(STB_RTC_TIME *arg);

static void rtc_fpc_interrupt(int irq, void *dev_id, struct pt_regs *pregs); 

static int rtc_fpc_open(struct inode *inode, struct file *file);
static int rtc_fpc_release(struct inode *inode, struct file *file);

static int rtc_fpc_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_cmd, unsigned long ioctl_parm);


static struct file_operations rtc_fpc_fops = 
{
    open:       rtc_fpc_open,
    release:    rtc_fpc_release, 
    ioctl:      rtc_fpc_ioctl
};


// just dummy open 
static int rtc_fpc_open(struct inode *inode, struct file *file)
{

    MOD_INC_USE_COUNT;  // MODULE

    return 0;
}

// just dummy close
static int rtc_fpc_release(struct inode *inode, struct file *file)
{

    MOD_DEC_USE_COUNT;

    return 0;
}




static int rtc_fpc_get_time(STB_RTC_TIME *pst_stb_rtc)
{  
  STB_RTC_TIME   st_rtc_time;
  unsigned int   min_10part, min_1part;
  unsigned int   hr_10part, hr_1part;
  unsigned long  ul_cur_rtc;
  unsigned long  flags;

  
//Protect this read from getting interrupted. Read the rtc register from the interrupt context
//Other option is block here till you get the next Update Ended Interrupt so that the real
//clock is read. Can protect it with spin_lock_irq, but this would do for time-being for STB system.
  save_flags(flags);
  cli();
  ul_cur_rtc = ul_int_rtc;
  restore_flags(flags);
//End of block.

  st_rtc_time.days = ul_cur_rtc & 0x3F;
  st_rtc_time.am_pm = (ul_cur_rtc >> 6) & 0x1;
  hr_1part = (ul_cur_rtc >> 8) & 0xF;
  hr_10part = (ul_cur_rtc >> 12) & 0x3;
  st_rtc_time.hours = hr_10part*10 + hr_1part;

  min_1part = (ul_cur_rtc >> 16) & 0xF;
  min_10part = (ul_cur_rtc >> 20) & 0x7;

  st_rtc_time.mins = min_10part*10 + min_1part;

  st_rtc_time.secs = (ul_cur_rtc >> 24) & 0x3F;

  copy_to_user(pst_stb_rtc, &st_rtc_time, sizeof(STB_RTC_TIME) );

  return 0;
}



static int rtc_fpc_set_time(STB_RTC_TIME *pst_stb_rtc)
{
  STB_RTC_TIME   st_rtc_time;
  unsigned int   min_10part, min_1part;
  unsigned int   hr_10part, hr_1part;
  unsigned long  val, ul_reg;

  copy_from_user(&st_rtc_time, pst_stb_rtc, sizeof(STB_RTC_TIME) );
  
  min_10part = st_rtc_time.mins /10;
  min_1part  = st_rtc_time.mins %10;

  hr_10part = st_rtc_time.hours /10;
  hr_1part = st_rtc_time.hours %10;

  val = (st_rtc_time.secs << 24) |
        (min_10part << 20) | (min_1part << 16) |
        (hr_10part << 12 ) | (hr_1part << 8)   |
        (st_rtc_time.am_pm << 6) |
        (st_rtc_time.days & 0x3F);

  ul_reg = mfdcr(DCR_RTC_FPC_CNTL);

  mtdcr(DCR_RTC_FPC_CNTL, (ul_reg & ~0x01000000) );

  mtdcr(DCR_RTC_FPC_TIME, val);
     
  mtdcr(DCR_RTC_FPC_CNTL, ul_reg);

  return 0;
}



static int rtc_fpc_get_alm_time(STB_RTC_TIME *pst_stb_rtc)
{
  STB_RTC_TIME   st_alm_time;
  unsigned int   min_10part, min_1part;
  unsigned int   hr_10part, hr_1part;
  unsigned long  ul_cur_alm;

  ul_cur_alm = mfdcr(DCR_RTC_FPC_ALRM);

  st_alm_time.days = ul_cur_alm & 0x3F;
  st_alm_time.am_pm = (ul_cur_alm >> 6) & 0x1;
  hr_1part = (ul_cur_alm >> 8) & 0xF;
  hr_10part = (ul_cur_alm >> 12) & 0x3;
  st_alm_time.hours = hr_10part*10 + hr_1part;

  min_1part = (ul_cur_alm >> 16) & 0xF;
  min_10part = (ul_cur_alm >> 20) & 0x7;

  st_alm_time.mins = min_10part*10 + min_1part;

  st_alm_time.secs = (ul_cur_alm >> 24) & 0x3F;

  copy_to_user(pst_stb_rtc, &st_alm_time, sizeof(STB_RTC_TIME) );


  return 0;
}



static int rtc_fpc_set_alm_time(STB_RTC_TIME *pst_stb_rtc)
{
  STB_RTC_TIME   st_alm_time;
  unsigned int   min_10part, min_1part;
  unsigned int   hr_10part, hr_1part;
  unsigned long  val;

  copy_from_user(&st_alm_time, pst_stb_rtc, sizeof(STB_RTC_TIME) );
  
  min_10part = st_alm_time.mins /10;
  min_1part  = st_alm_time.mins %10;

  hr_10part = st_alm_time.hours /10;
  hr_1part = st_alm_time.hours %10;

  val = (st_alm_time.secs << 24) |
        (min_10part << 20) | (min_1part << 16) |
        (hr_10part << 12 ) | (hr_1part << 8)   |
        (st_alm_time.am_pm << 6) |
        (st_alm_time.days & 0x3F);

  mtdcr(DCR_RTC_FPC_ALRM, val);
  
  return 0;
}



static int rtc_fpc_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_cmd, unsigned long ioctl_parm)
{
  int rc = 0;
  unsigned long ul_regs;
 
#ifdef RTC_FPC_DEBUG
  printk("rtc_fpc_ioctl: cmd %d\n", ioctl_cmd);
#endif

  switch(ioctl_cmd){
     
  case IOC_RTC_FPC_GET_TIME:
    rc = rtc_fpc_get_time( (STB_RTC_TIME *)ioctl_parm );
    break;
     
  case IOC_RTC_FPC_SET_TIME:
    rc = rtc_fpc_set_time( (STB_RTC_TIME *)ioctl_parm );
    break;

  case IOC_RTC_FPC_GET_ALRM_TIME:
    rc = rtc_fpc_get_alm_time( (STB_RTC_TIME *)ioctl_parm );
    break;

  case IOC_RTC_FPC_SET_ALRM_TIME:
    rc = rtc_fpc_set_alm_time( (STB_RTC_TIME *)ioctl_parm );
    break;

  case IOC_RTC_FPC_SET_FP_DATA:
  {
    unsigned char dat[5], i;    
    copy_from_user(dat, (unsigned char *)ioctl_parm, 5);
    
    for(i = 0; i < 5; i++){
      if(dat[i] > 9)
        dat[i] = 9;
    }

    mtdcr(DCR_RTC_FPC_D1, (disp_seg_val[dat[0]] << 24) );
    mtdcr(DCR_RTC_FPC_D2, (disp_seg_val[dat[1]] << 24) );
    mtdcr(DCR_RTC_FPC_D3, (disp_seg_val[dat[2]] << 24) );
    mtdcr(DCR_RTC_FPC_D4, (disp_seg_val[dat[3]] << 24) );
    mtdcr(DCR_RTC_FPC_D5, (disp_seg_val[dat[4]] << 24) );
    break;
  }

  case IOC_RTC_FPC_ENA_ALMINT:
    mtdcr(DCR_RTC_FPC_CNTL, (mfdcr(DCR_RTC_FPC_CNTL) | 0x20000000) );
    break;

  case IOC_RTC_FPC_DIS_ALMINT:
    mtdcr(DCR_RTC_FPC_CNTL, (mfdcr(DCR_RTC_FPC_CNTL) & ~0x20000000) );
    break;  

  case IOC_RTC_FPC_ENA_UPEINT:
    mtdcr(DCR_RTC_FPC_CNTL, (mfdcr(DCR_RTC_FPC_CNTL) | 0x10000000) );
    break;

  case IOC_RTC_FPC_DIS_UPEINT:
    mtdcr(DCR_RTC_FPC_CNTL, (mfdcr(DCR_RTC_FPC_CNTL) & ~0x10000000) );
    break;

  //Front Panel Controller Ioctl
  case IOC_RTC_FPC_ENA_DISPDAT:
    mtdcr(DCR_RTC_FPC_FCNTL, (mfdcr(DCR_RTC_FPC_FCNTL) & ~0x03000000) );
    break;

  case IOC_RTC_FPC_ENA_DISPRTC:
    mtdcr(DCR_RTC_FPC_FCNTL, (mfdcr(DCR_RTC_FPC_FCNTL) | 0x03000000) );
    break;

  case IOC_RTC_FPC_SET_24TOFM:
    mtdcr(DCR_RTC_FPC_FCNTL, (mfdcr(DCR_RTC_FPC_FCNTL) | 0x04000000) );
    ul_24hr_fmt = 1;
    break;

  case IOC_RTC_FPC_SET_12TOFM:
    mtdcr(DCR_RTC_FPC_FCNTL, (mfdcr(DCR_RTC_FPC_FCNTL) & ~0x04000000) );
    ul_24hr_fmt = 0;
    break;
 
    //This is for a process to wait for Alarm to happen.
  case IOC_RTC_FPC_WAIT_FOR_ALRM:
    ul_alrm_wait = 1;
    ul_regs =  mfdcr(DCR_RTC_FPC_CNTL);  
 
    //Put the process to sleep. Alarm time got to be bigger than 
    //the current time. Otherwise, it would wrap for a long time.
    //The process should enable the Alarm interrupt, just be safe

    mtdcr(DCR_RTC_FPC_CNTL, (ul_regs | 0x20000000) );
    interruptible_sleep_on(&wrtc_fpc_q);
    mtdcr(DCR_RTC_FPC_CNTL, ul_regs);

    break;

  }
  
  return rc;
}


static void rtc_fpc_interrupt(int irq, void *dev_id, struct pt_regs *pregs)
{
  unsigned long ul_int_stat;
  unsigned long ul_cntl_reg;

  ul_int_stat = mfdcr(DCR_RTC_FPC_INT);

  if(ul_int_stat & 0x10000000){

    //Read the real time every time there is Update End Interrupt
    //This is assigned to the value when user wants to get the real time.
    ul_int_rtc =  mfdcr(DCR_RTC_FPC_TIME);

#ifdef RTC_FPC_DEBUG
    printk("rtc_fpc_interrupt: Update Ended Interrupt, Time 0x%x\n", (unsigned int) ul_int_rtc);
#endif

  } 

  if(ul_int_stat & 0x20000000){
#ifdef RTC_FPC_DEBUG
    printk("rtc_fpc_interrupt: Alarm Interrupt\n");
#endif

    //printk("***ALARM INTERRUPT ***\n");

    if(ul_alrm_wait){
         ul_alrm_wait = 0;
         //Wake up the process that was put to sleep
         wake_up_interruptible(&wrtc_fpc_q);
    }

    ul_cntl_reg =  mfdcr(DCR_RTC_FPC_CNTL);  
    mtdcr(DCR_RTC_FPC_CNTL, (ul_cntl_reg & ~0x20000000) );
  }

  //Clear the interrupt by writing the bits back into the stat register.
  //The UIC interrupt gets cleared through the main interrupt handler.
  mtdcr(DCR_RTC_FPC_INT, ul_int_stat);

  return;
}


static devfs_handle_t devfs_handle;

static int __init rtc_fpc_init(void)
{
#ifdef RTC_FPC_DEBUG
  printk("\nrtc_fpc_init\n");
#endif
  
  PVERSION(RTC_FPC_DRIVER_NAME);
    
  if(devfs_register_chrdev(RTC_FPC_DEV_MAJOR, RTC_FPC_DEV_NAME, &rtc_fpc_fops) < 0){
    return -1;
  }
     
  devfs_handle = devfs_find_handle(NULL, RTC_FPC_DEV_NAME,
                                0, 0, DEVFS_SPECIAL_CHR,0);
    
  if(devfs_handle == NULL)
  {
        
    devfs_handle = devfs_register(NULL, RTC_FPC_DEV_NAME, DEVFS_FL_DEFAULT,
                                RTC_FPC_DEV_MAJOR, 0,
                                S_IFCHR | S_IRUSR | S_IWUSR,
                                &rtc_fpc_fops, NULL);
  }
  else
    devfs_handle = NULL;

  if(request_irq(IRQ_RTCFPC, rtc_fpc_interrupt, SA_INTERRUPT, "stbrtc", NULL) != 0){
    printk("rtc_fpc_init: could not install interrupt handler \n");
    return -1; 
  }  

  mtdcr(DCR_RTC_FPC_CNTL,  0x11000000);
  mtdcr(DCR_RTC_FPC_FCNTL, 0x83000000);
 
  return 0;
}

static void __exit rtc_fpc_exit(void)
{
#ifdef RTC_FPC_DEBUG
  printk("\nrtc_fpc_exit\n");
#endif

  devfs_unregister_chrdev(RTC_FPC_DEV_MAJOR, RTC_FPC_DEV_NAME);
  if(devfs_handle != NULL)
    devfs_unregister(devfs_handle);

  free_irq(IRQ_RTCFPC, NULL);

  mtdcr(DCR_RTC_FPC_CNTL,  0x0);
  mtdcr(DCR_RTC_FPC_FCNTL, 0x0);

  return;
}


module_init(rtc_fpc_init);
module_exit(rtc_fpc_exit);
