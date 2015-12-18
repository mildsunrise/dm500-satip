//pallas/drv/ircombo/inf_ircombo.c
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
//Comment: 
//  Linux redwood IR driver
//  IR Keyboard and Remove Controller is supported by default
//  #define _IR_MOUSE_SUPPORT  to enable IR mouse support
//Revision Log:   
//  Sept/12/2001                                               Created by YYD
//  Nov/22/2001       Removed the wrongly used old mouse movement code by YYD

//FixMe:  Logically the queue used in rawir will not cause races
//        I'm not using any locks when dequeue and enqueue now.
//        Because the most dangerous is not be able to dequeue
//        when there are data or not be able to enqueue when there
//        are spaces. The first is more possible.

// The necessary header files
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <asm/system.h>
#include <linux/string.h>
#include <linux/threads.h>
#include <linux/interrupt.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/hardirq.h>
#include <asm/softirq.h>
#include <asm/io.h>

// for irkey
#include <linux/kbd_ll.h>

// for rawir
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/spinlock.h>

// local includes
#include "os/os-generic.h"
#include "os/os-sync.h"
#include "os/helper-queue.h"
#include "gpt/gpt_atom.h"
#include "gpt/gpt_ports.h"
#include "ir/ir_osi_device.h"
#include "ir/ir_osi_acer_kbms.h"
#include "ir/ir_osi_hitachi.h"


#ifdef __IRCOMBO_DEBUG
#define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#include "os/pversion.h"


#ifdef _IR_MOUSE_SUPPORT

#include <busmouse.h>           // linux/drivers/char/busmouse.h

#define ACER_MOUSE_MINOR  11    // do you have a better minor code ? :-)
//To prepare the device
//  cd /dev
//  mknod irmouse c 10 11
//  ln -f irmouse mouse

static int acer_mouse_init(void);
static void acer_mouse_cleanup(void);

#endif  // _IR_MOUSE_SUPPORT

#define RAWIR_MAJOR         100     // the major device number of rawir
#define RAWIR_DRIVER_NAME   "rawir" // the module name

#define RAWIR_MAX_OPENS     5       // we enable rawir device to be opened by 5 times simutaniously
#define RAWIR_QUEUE_SIZE    32      // the max number of events awaiting reading

static int acer_kbms_init(void);
static void acer_kbms_cleanup(void);

static int raw_ir_init(void);
static void raw_ir_cleanup(void);

static void acer_kbms_event(UINT32 timediff);
static void raw_ir_event(UINT32 timediff);



#ifdef MODULE

MODULE_AUTHOR("Yudong Yang / IBM CRL");
MODULE_DESCRIPTION("Input driver for IBM Redwood onboard IR receiver port");

#endif

///////////////////////////////////////////////////////////////////////////////
//   Generic driver section
///////////////////////////////////////////////////////////////////////////////

static void *pMouseHandle=NULL;

static void *_gpGPTPort=NULL;


static __IR_DEVICE  _gDevKBMS =
{
    init:   NULL,
    deinit: NULL,
    event:  acer_kbms_event
};

static __IR_DEVICE  _gDevRawIR = 
{
    init:   NULL,
    deinit: NULL,
    event:  raw_ir_event
};


static int ir_inf_init(void)
{
    if(_gpGPTPort) return 0;  // already initialized

#ifdef CONFIG_LAMAR
    PVERSION("IR-Combo for IBM Lamar");
#else
    PVERSION("IR-Combo for IBM Redwood5");
#endif

    if(acer_kbms_init() < 0) 
    {
        PFATAL("AcerKBMS init failed!\n");
        return -1;
    }
    if(raw_ir_init() < 0) 
    {
        PFATAL("RawIR init failed!\n");
        acer_kbms_cleanup();
        return -1;
    }

#if 0
    _gpGPTPort = ioremap(__GPT_PORT_BASE, __GPT_PORT_RANGE);

    if(!_gpGPTPort)
    {
        PFATAL("GPT port IO Remap failed!\n");
        raw_ir_cleanup();
        acer_kbms_cleanup();
        return -1;
    }
    
    gpt_atom_init_port(_gpGPTPort);
#endif
    ir_osi_register_device(&_gDevKBMS);
    ir_osi_register_device(&_gDevRawIR);

    if(0 > ir_osi_init())
    {
        PFATAL("IR OSI init failed!\n");
        ir_osi_unregister_device(&_gDevKBMS);
        ir_osi_unregister_device(&_gDevRawIR);
//        gpt_atom_init_port(NULL);
//        iounmap(_gpGPTPort);
//        _gpGPTPort = NULL;
        raw_ir_cleanup();
        acer_kbms_cleanup();
        return -1;
    }

    if(ir_osi_start() == 0) return 0;
    PFATAL("start IR failed!\n");

    // clean up now
    ir_osi_unregister_device(&_gDevKBMS);
    ir_osi_unregister_device(&_gDevRawIR);
//    gpt_atom_init_port(NULL);
//    iounmap(_gpGPTPort);
//    _gpGPTPort = NULL;
    raw_ir_cleanup();
    acer_kbms_cleanup();
    return -1;
}

static void ir_inf_deinit(void)
{
    raw_ir_cleanup();
    acer_kbms_cleanup();
    ir_osi_unregister_device(&_gDevKBMS);
    ir_osi_unregister_device(&_gDevRawIR);
    ir_osi_stop();
    ir_osi_deinit();
//    gpt_atom_init_port(NULL);
//    iounmap(_gpGPTPort);
//    _gpGPTPort = NULL;
}


module_init(ir_inf_init);
module_exit(ir_inf_deinit);


///////////////////////////////////////////////////////////////////////////////
//   RawIR driver section
///////////////////////////////////////////////////////////////////////////////

static ssize_t rawir_read(struct file *file, char *buffer, size_t length, loff_t * offset);
static int rawir_open(struct inode *inode, struct file *file);
static int rawir_release(struct inode *inode, struct file *file);
static unsigned int rawir_poll(struct file *, struct poll_table_struct *);
static int rawir_fasync(int fd, struct file *filp, int mode);

static int rawir_open_count = 0;

static struct file_operations RawIRFops = 
{
    read:       rawir_read,
    poll:       rawir_poll,         /* poll */
    open:       rawir_open,
    release:    rawir_release,      /* a.k.a. close */
    fasync:     rawir_fasync,       /* fasync */
};

typedef struct __RAWIR_DEVICE_SLOTS_STRUCT
{
    UINT32      queuebuf[RAWIR_QUEUE_SIZE];     //  the buffer for queue
    QUEUE_T     dataq;                          // queue where data is saved
    struct fasync_struct    *async_queue;       // for asyncronized access
    wait_queue_head_t wait;                     // my wait queue
    spinlock_t  lock;                           // a lock for the queue when needed, hope will not
    struct __RAWIR_DEVICE_SLOTS_STRUCT *pNext;  // my list of opened devices
}  RAWIR_DEVICE_SLOTS;


static RAWIR_DEVICE_SLOTS *_gpRawIRSlots=NULL;

static spinlock_t _g_irlock;

static devfs_handle_t devfs_handle;

static int raw_ir_init(void)
{
    spin_lock_init(_g_irlock);
    _gpRawIRSlots=NULL;
    rawir_open_count = 0;
    if (devfs_register_chrdev(RAWIR_MAJOR, RAWIR_DRIVER_NAME, &RawIRFops) < 0)
        return -1;
    
    devfs_handle = devfs_find_handle(NULL, RAWIR_DRIVER_NAME,
                                0, 0, DEVFS_SPECIAL_CHR,0);
    
    if(devfs_handle == NULL)
    {
        
      devfs_handle = devfs_register(NULL, RAWIR_DRIVER_NAME, DEVFS_FL_DEFAULT,
                                RAWIR_MAJOR, 0,
                                S_IFCHR | S_IRUSR | S_IWUSR,
                                &RawIRFops, NULL);
    }
    else
      devfs_handle = NULL;
      
    return 0;
}

static void raw_ir_cleanup()
{
    devfs_unregister_chrdev(RAWIR_MAJOR, RAWIR_DRIVER_NAME);
    if(devfs_handle != NULL)
      devfs_unregister(devfs_handle);
}

static void handle_raw_ir_code(USHORT uCode)
{
    // I'm using spin_lock_bh, so no problem
    RAWIR_DEVICE_SLOTS *pSlot = _gpRawIRSlots;
    while(pSlot)
    {
        os_enqueue(&pSlot->dataq, &uCode); // FixMe: if this will race ?
        if(pSlot->async_queue) 
            kill_fasync(&pSlot->async_queue, SIGIO, POLL_IN);
        wake_up_interruptible(&pSlot->wait);
        pSlot = pSlot->pNext;
    }
}

static void raw_ir_event(UINT32 timediff)
{   
    UINT32 uCode;

    if(ir_osi_detect_hitachi_code(timediff, &uCode) > 0)
    {
        ir_osi_process_hitachi_code(uCode,  handle_raw_ir_code);
    }
}


static int rawir_open(struct inode *inode, struct file *file)
{
    RAWIR_DEVICE_SLOTS *pSlot;

    PDEBUG("rawir_open(%p)\n", file);

    /* We don't want to talk to two processes at the * same time */

    if (rawir_open_count >= RAWIR_MAX_OPENS)
    {
        return -EBUSY;
    }
    if(file->f_mode & FMODE_WRITE)
    {
        return -EPERM;
    }

    pSlot = (RAWIR_DEVICE_SLOTS *)MALLOC(sizeof(RAWIR_DEVICE_SLOTS));

    if(!pSlot) 
    {
        PFATAL("Out of memeory!\n");
        return -EIO;
    }

    memset(pSlot, 0, sizeof(RAWIR_DEVICE_SLOTS));

    if(os_create_queue(&pSlot->dataq, pSlot->queuebuf, 
        RAWIR_QUEUE_SIZE, sizeof(UINT32)) < 0)
    {
        PDEBUG("this os_create_queue should never fail!\n");
        FREE(pSlot);
        return -EIO;
    }

    init_waitqueue_head(&pSlot->wait);
    spin_lock_init(&pSlot->lock);
    file->private_data = pSlot;    // ok, assign the slot
    pSlot->pNext = _gpRawIRSlots;   // always add before to avoid locks
    _gpRawIRSlots = pSlot;          // unless there can be another open (never for linux)

    rawir_open_count++;

    MOD_INC_USE_COUNT;  // MODULE

    return 0;
}

/* This function is called when a process closes the
* device file. It doesn't have a return value because 
* it cannot fail. Regardless of what else happens, you 
* should always be able to close a device (in 2.0, a 2.2
* device file could be impossible to close). */

static int rawir_release(struct inode *inode, struct file *file)
{
    RAWIR_DEVICE_SLOTS *pSlot;

    PDEBUG("rawir_release(%p,%p)\n", inode, file);

    if (!rawir_open_count)
    {
        PDEBUG("try to close before open!\n");
        return -EBADF;
    }

    if (!file->private_data) 
    {
        PFATAL("Release: Where is my private data ?\n");
        return -EBADF;
    }

    pSlot = (RAWIR_DEVICE_SLOTS *)file->private_data;

    if(_gpRawIRSlots == pSlot)  // oh, it's the head
    {
        spin_lock_bh(&_g_irlock);
        _gpRawIRSlots = _gpRawIRSlots->pNext;
        spin_unlock_bh(&_g_irlock);
    }
    else
    {
        RAWIR_DEVICE_SLOTS *pMySlot;

        pMySlot = _gpRawIRSlots;
        while(pMySlot->pNext  && pMySlot->pNext != pSlot) pMySlot = pMySlot->pNext;
        if(pMySlot->pNext != pSlot)
        {
            PFATAL("Release: who messed up my private data ?\n");
            return -EBADF;
        }
        spin_lock_bh(&_g_irlock);
        pMySlot->pNext = pMySlot->pNext->pNext;
        spin_unlock_bh(&_g_irlock);
    }

    rawir_fasync(-1, file, 0);

    spin_lock(&pSlot->lock);
    os_delete_queue(&pSlot->dataq);
    spin_unlock(&pSlot->lock);
    FREE(pSlot);

    rawir_open_count--;

    MOD_DEC_USE_COUNT;

    return 0;
}

static int rawir_fasync(int fd, struct file *filp, int mode)
{
    RAWIR_DEVICE_SLOTS *pSlot = (RAWIR_DEVICE_SLOTS *)filp->private_data;
    return fasync_helper(fd, filp, mode, &pSlot->async_queue);
}

/* This function is called whenever a process which
 * has already opened the device file attempts to 
 * read from it. */
static ssize_t rawir_read(struct file *filp, 
                          char *buffer,      /* The buffer to fill with the data */
                          size_t length,     /* The length of the buffer */
                          loff_t * offset)   /* offset to the file */
{
    /* Number of bytes actually written to the buffer */
    RAWIR_DEVICE_SLOTS *pSlot;
    USHORT key;
    INT rtn;
    ssize_t char_read=0;

    /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) int i, is_sig=0; #endif */

    PDEBUG("rawir_read(%p,%p,%d)\n", filp, buffer, (int) length);

    if (length < sizeof(USHORT))
        return 0;

    pSlot = (RAWIR_DEVICE_SLOTS *)filp->private_data;

    rtn = os_dequeue(&pSlot->dataq, &key); // FixMe: shall we use locks ?

    if(rtn < 0)  // queue empty
    {
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        interruptible_sleep_on(&pSlot->wait); // FixMe: can this die when race happens?
        if(signal_pending(current))
            return -EINTR;
        rtn = os_dequeue(&pSlot->dataq, &key); // FixMe: shall we use locks ?
        if(rtn < 0) return 0;
    }

    do 
    {
        if(copy_to_user(buffer, &key, sizeof(USHORT))) return -EFAULT;
        char_read += 2;
        if(char_read > length-2) break;
        rtn = os_dequeue(&pSlot->dataq, &key); // FixMe: shall we use locks ?
    } while(!rtn);

    return char_read;
}

static unsigned int rawir_poll(struct file *filp,
                               struct poll_table_struct *wait)
{
    RAWIR_DEVICE_SLOTS *pSlot = (RAWIR_DEVICE_SLOTS *)filp->private_data;

    poll_wait(filp, &pSlot->wait, wait);
    if (os_get_queue_status(&pSlot->dataq)>0)
        return POLLIN | POLLRDNORM;

    return 0;
}




///////////////////////////////////////////////////////////////////////////////
//   Acer Keyboard Mouse driver section
///////////////////////////////////////////////////////////////////////////////


static int acer_kbms_init(void)
{
#ifdef _IR_MOUSE_SUPPORT
    if( acer_mouse_init() < 0) return -1;
#endif
    ir_osi_reset_acer_detector();
    return 0;
}

static void acer_kbms_cleanup(void)
{
#ifdef _IR_MOUSE_SUPPORT
    acer_mouse_cleanup();
#endif
}

static void handle_acer_keyboard_scan_code(USHORT wCode, USHORT wDone)
{
    handle_scancode((unsigned char)wCode, (int)wDone);
}

static void acer_kbms_event(UINT32 timediff)
{
    UINT32 uCode;

    if (ir_osi_detect_acer_keycode(timediff, &uCode)>0)
    {
        ir_osi_process_acer_keycode(uCode, 
            handle_acer_keyboard_scan_code,
            pMouseHandle
            );
    }
}



#ifdef _IR_MOUSE_SUPPORT
                                     
/*----------------------------------------------------
|  Linux Mouse driver for Redwood 4 and Acer IR KB   |
|  by Yudong Yang IBM CRL, May/10/2001               |
------------------------------------------------------
|  It requires the busmouse common driver in kernel  |                                     |
----------------------------------------------------*/

static int msedev;


static void acer_mouse_event_handle(INT dx, INT dy, USHORT button)
{
    if (dx || dy)
    {
        busmouse_add_movement(msedev, -dx, -dy);
    }

    if (button & __IR_MOUSE_LBUTTON_DOWN)
    {
        busmouse_add_buttons(msedev, 0x04, 0);
    }
    else if (button & __IR_MOUSE_LBUTTON_UP)
    {
        busmouse_add_buttons(msedev, 0x04, 4);
    }

    if (button & __IR_MOUSE_RBUTTON_DOWN)
    {
        busmouse_add_buttons(msedev, 0x01, 0);
    }
    else if (button & __IR_MOUSE_RBUTTON_UP)
    {
        busmouse_add_buttons(msedev, 0x01, 1);
    }
}

static int release_mouse(struct inode *inode, struct file *file)
{
    pMouseHandle = NULL;

    MOD_DEC_USE_COUNT;

    return 0;
}

static int open_mouse(struct inode *inode, struct file *file)
{
    pMouseHandle = acer_mouse_event_handle;

    MOD_INC_USE_COUNT;
 
    return 0;
}

static struct busmouse acermouse = {
    ACER_MOUSE_MINOR, "acerirmouse", THIS_MODULE, open_mouse, release_mouse, 7
};

static int acer_mouse_init(void)
{
    msedev = register_busmouse(&acermouse);

    if (msedev < 0)
        printk(KERN_WARNING "Unable to register Acer IR mouse driver.\n");
    else
        printk(KERN_INFO "Acer IR mouse installed.\n");
    pMouseHandle = NULL;

    return msedev < 0 ? msedev : 0;
}

static void acer_mouse_cleanup(void)
{
    pMouseHandle = NULL;
    unregister_busmouse(msedev);
    msedev = 0;
}

#endif  // _IR_MOUSE_SUPPORT
