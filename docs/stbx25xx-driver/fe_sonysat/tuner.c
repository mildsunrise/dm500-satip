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
|       COPYRIGHT   I B M   CORPORATION 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/devfs_fs_kernel.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include "os/os-types.h"

#include "tuner.h"
#include "cstuner.h"
#include <linux/i2c.h>

static int _open_count = 0;
static int tuner_found = 0;

static int tuner_open(struct inode *inode, struct file *file);
static int tuner_release(struct inode *inode, struct file *file);
static int tuner_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_cmd, unsigned long ioctl_parm);

static int tuner_attach_adapter(struct i2c_adapter *adapter);
static int test_for_tuner(struct i2c_adapter *adapter,int addr);

static struct i2c_driver tuner = {
	name:			"fe_sonysat",
	id:			I2C_DRIVERID_TUNER,
	flags:			I2C_DF_NOTIFY,
	attach_adapter:	        tuner_attach_adapter,
	detach_client:	        NULL,
	command:		NULL,
	inc_use:		NULL,
	dec_use:		NULL
};

static struct file_operations tuner_fops = 
{
    open:       tuner_open, 
    release:    tuner_release, 
    ioctl:      tuner_ioctl
};

static struct i2c_adapter *tuner_adapter;

IOCTL_TUNE_PARAMETER tune_param;
IOCTL_STATUS_PARAMETER status_param;

/*----------------------------------------------------------------------------+
|
+----------------------------------------------------------------------------*/
static int tuner_open(struct inode *inode, struct file *file)
{
   
    if (_open_count != 0)
    {
        printk("Driver in use\n");
        return -EBUSY;
    }
    _open_count++;

    #ifdef MODULE
    MOD_INC_USE_COUNT;  // MODULE
    #endif

    return 0;
}

/*----------------------------------------------------------------------------+
|
+----------------------------------------------------------------------------*/
static int tuner_release(struct inode *inode, struct file *file)
{

    _open_count--;

    #ifdef MODULE
    MOD_DEC_USE_COUNT;
    #endif
 
#ifdef TUNER_DEBUG
    printk("Tuner driver close\n");
#endif
    return 0;
}

/*----------------------------------------------------------------------------+
|
+----------------------------------------------------------------------------*/
static int tuner_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_cmd, unsigned long ioctl_parm)
{
  int rc = 0;
  unsigned char tslock;
  unsigned char viterbi;
 
#ifdef TUNER_DEBUG
  printk("tuner_ioctl: cmd %08X\n", ioctl_cmd);
#endif
  switch(ioctl_cmd){

    case TUNER_IOCTL_TUNE:
      copy_from_user((void *) &tune_param, (const void *) ioctl_parm,
                      sizeof(IOCTL_TUNE_PARAMETER));
      rc = tuner_tune_to_freq(&tune_param);
      break; 

    case TUNER_IOCTL_STATUS: 
      status_param.lnb_power = tune_param.lnb_power;
      status_param.disecq_ctrl = tune_param.disecq_ctrl;
      status_param.symbolrate = tune_param.symbolrate;
      status_param.freq = tune_param.freq;
      tuner_read_status(0,&tslock,&viterbi);
      status_param.tslock = tslock;
      switch(viterbi)
      {
        case 0:                               // 1/2
            status_param.code_rate_n = 1;
            status_param.code_rate_d = 2;
            break;
        case 1:
            status_param.code_rate_n = 2;
            status_param.code_rate_d = 3;
            break;
        case 2:
            status_param.code_rate_n = 3;
            status_param.code_rate_d = 4;
            break;
        case 3:
            status_param.code_rate_n = 5;
            status_param.code_rate_d = 6;
            break;
        case 4:
            status_param.code_rate_n = 7;
            status_param.code_rate_d = 8;
            break;
        default:
            status_param.code_rate_n = 0;
            status_param.code_rate_d = 0;
            break;
      }
      copy_to_user((void *)ioctl_parm,(void *)&status_param, sizeof(IOCTL_STATUS_PARAMETER));
      break;
     
    default:
      break;
  }
  
  return rc;
}


/*----------------------------------------------------------------------------+
|
+----------------------------------------------------------------------------*/
static int tuner_attach_adapter(struct i2c_adapter *adapter)
{
  int result;

#ifdef TUNER_DEBUG
  printk("tuner_attach_adapter called\n");
#endif
  tuner_adapter = adapter;

  result = test_for_tuner(adapter, SONY_TUNER_ADDR);
#ifdef TUNER_DEBUG
  printk("i2c_probe return = 0x%X\n", result);
#endif
  return(result);
}

/*----------------------------------------------------------------------------+
|
+----------------------------------------------------------------------------*/
static int test_for_tuner(struct i2c_adapter *adapter,int addr)
{
  int result;
  char data[4];

#ifdef TUNER_DEBUG
  printk("test_for_tuner called: adapter=%s  addr=0x%X\n",adapter->name, addr);
#endif
  /* Program the I2C multiplexor to enable channel 0  */

  if(addr != SONY_TUNER_ADDR)
    return(0);
 
  result = tuner_reset(0,FE_STATE_ENABLE); 
  if(result != 0) {
    printk("tuner_reset failed, tuner driver not installed.\n");
    return(0);
  }  

  data[0] = CXM3002_STATUS_REG_ADD;
  result = do_write_i2c(SONY_TUNER_ADDR,0,1,data,0);
  if(result != 0)
  {
    printk("Failure writing reg number, tuner driver not installed.\n");
    return(-1);
  }

  result = do_read_i2c(SONY_TUNER_ADDR,0,1,data,0);
  if(result != 0)
  {
    printk("Failure reading status reg, tuner driver not installed.\n");
    return(-1);
  }
#ifdef TUNER_DEBUG
  printk("tuner status reg = 0x%02X\n",data[0]);
#endif
  printk("Tuner driver installed.\n");
  tuner_found = 1;
  return(0);
}

static devfs_handle_t devfs_handle;

/*----------------------------------------------------------------------------+
|
+----------------------------------------------------------------------------*/
static int __init tuner_init(void)
{

  int result;

#ifdef TUNER_DEBUG
  printk("\ntuner_init\n");
#endif

  tuner_found = 0;
#if 1
  result = tuner_reset(0,FE_STATE_ENABLE); 
  if(result != 0) {
    printk("tuner_reset failed, tuner driver not installed.\n");
    return(-1);
  }
#endif
#ifdef TUNER_DEBUG
  printk("Calling i2c_add_driver\n");    
#endif
  if ((result = i2c_add_driver (&tuner))){
    printk("i2c_add_driver failed\n");
    return (result);
  }
#ifdef TUNER_DEBUG
  printk("\nReturn from i2c_add_driver = %d.  tuner_found = %d\n",result,tuner_found);
#endif
  if(result == 0 && 0 == tuner_found)
  {
    i2c_del_driver(&tuner);
    return(-1);
  }
#ifdef TUNER_DEBUG
  printk("Calling register_chrdev\n");
#endif
  if(devfs_register_chrdev(TUNER_DEV_MAJOR, TUNER_DEV_NAME, &tuner_fops) < 0){
    printk("register_chrdev failed\n");
    i2c_del_driver (&tuner);
    return -1;
  }
  
  devfs_handle = devfs_find_handle(NULL, TUNER_DEV_NAME,
                                0, 0, DEVFS_SPECIAL_CHR,0);
    
  if(devfs_handle == NULL)
  {
      
      devfs_handle = devfs_register(NULL, TUNER_DEV_NAME, DEVFS_FL_DEFAULT,
                                TUNER_DEV_MAJOR, 0,
                                S_IFCHR | S_IRUSR | S_IWUSR,
                                &tuner_fops, NULL);
  }
  else
    devfs_handle = NULL;
        
  _open_count = 0;
  return 0;
}

/*----------------------------------------------------------------------------+
|
+----------------------------------------------------------------------------*/
static void __exit tuner_exit(void)
{
#ifdef TUNER_DEBUG
  printk("\ntuner_exit\n");
#endif

  i2c_del_driver (&tuner);
  devfs_unregister_chrdev(TUNER_DEV_MAJOR, TUNER_DEV_NAME);
  if(devfs_handle != NULL)
    devfs_unregister(devfs_handle);
  tuner_found = 0;
  return;
}

int do_write_i2c(unsigned char devaddr, unsigned char subaddr, unsigned char count,
                 unsigned char *data, unsigned char flags)
{
  struct i2c_msg *msgs;
  int msgn;
  struct i2c_msg msg[2];
  int result;
   
  if(flags & IIC_FLAGS_SUB_ADDR)
  {
    msg[0].addr = devaddr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &subaddr;
    msgs = &msg[0];
    if(count != 0)
      msgn = 2;
    else
      msgn = 1;
  }
  else
  {
    msgs = &msg[1];
    msgn = 1;
  }
   
  msg[1].addr = devaddr;
  msg[1].flags = 0;
  msg[1].len = count;
  msg[1].buf = data;

  result = i2c_transfer(tuner_adapter,msgs, msgn); 
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,20)  
  /* before MVL 3.1 i2c_transfer would return the number of bytes  */
  /* transferred if the message count was one or the number of     */
  /* messages completed if the message count was greater than one. */
  if((msgn > 1 && result != msgn) || (msgn == 1 && result != msgs->len))
#else
  /* In MVL 3.1 i2c_transfer always returns the number of messages */
  /* completed.                                                    */
  if(result != msgn)
#endif  
  {
    printk("do_write_i2c: i2c_transfer failed\n");
    printk("              flags   = 0x%x\n",flags);
    printk("              msgn    = 0x%x\n",msgn);
    printk("              addr    = 0x%x\n",devaddr);
    printk("              subaddr = 0x%x\n",subaddr);
    printk("              count   = 0x%x\n",count);
    printk("              result  = 0x%x\n",result); 
    return(-1);
  }
  return(0);


}

int do_read_i2c(unsigned char devaddr, unsigned char subaddr, unsigned char count,
                 unsigned char *data, unsigned char flags)
{
  struct i2c_msg *msgs;
  int msgn;
  struct i2c_msg msg[2];
  int result;
   
  if(flags & IIC_FLAGS_SUB_ADDR)
  {
    msg[0].addr = devaddr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &subaddr;
    msgs = &msg[0];
    if(count != 0)
      msgn = 2;
    else
      msgn = 1;
  }
  else
  {
    msgs = &msg[1];
    msgn = 1;
  }
   
  msg[1].addr = devaddr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = count;
  msg[1].buf = data;

  result = i2c_transfer(tuner_adapter,msgs, msgn); 
  if((msgn > 1 && result != msgn) || (msgn == 1 && result != msgs->len))
  {
    printk("do_read_i2c: i2c_transfer failed\n");
    printk("              addr    = 0x%x\n",devaddr);
    printk("              subaddr = 0x%x\n",subaddr);
    printk("              count   = 0x%x\n",count);
    printk("              result  = 0x%x\n",result); 
    return(-1);
  }
  return(0);



}


EXPORT_NO_SYMBOLS;

module_init(tuner_init);
module_exit(tuner_exit);
