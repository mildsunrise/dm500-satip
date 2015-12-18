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
/****************************************************************************/
/*                                                                          */
/* DESCRIPTION : Provide PVR API for Linux                  */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*  Author    :  Lin Guo Hui                                                */
/*  File      :                                                             */
/*  Purpose   :  Linux PVR API                                              */
/*  Changes   :                                                             */
/*  ----------------------------------------------------------------------  */
/*  16/6/2002   Add irq enable in open function, fix the error in           */
/*              ioctl(PVR_GET_STATUS)                                       */
/****************************************************************************/
/* The necessary header files */
/* The necessary header files */
#include <linux/config.h>
#include <linux/version.h>

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/kernel.h>

#define    __NO_VERSION__
#include <linux/module.h>

#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/ptrace.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/version.h>

#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/in.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/poll.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mman.h>

#include "xp_osi_global.h"
#include <xp/pvr_osd.h>
#include "pvr_atom_dcr.h"
#include <hw/physical-mem.h>

#define IRQ_DEFAULT_TRIG (IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG)   //1394 int

#define BUFFER_SIZE 1024*320    //Buffer Size 

//PVR status
#define PVR_STATUS_READY    0
#define PVR_STATUS_START    1
#define PVR_STATUS_STOP     2

#ifdef  __DRV_FOR_VULCAN__
#define PVR_IRQ             23
#else
#define PVR_IRQ             7   //PVR IRQ
#endif

#define PVRBUFS_MAX 16

typedef struct {
    void *mm;
    unsigned long vaddr;
    unsigned long physaddr;
    size_t        size;
    MEM_HANDLE_T  hMem;
    } PVRBUF_T;

static PVRBUF_T pvrbufs[PVRBUFS_MAX];

//default dma transfer mode
static PVR_PLAYBACK_MODE mode = PLAYBACK_WORD;

static int __Already_init = 0;
static int __pvr_status;
static int __buf_pingpong = 0;


static ULONG __last_phy_off = BUFFER_SIZE/2;
static ULONG __last_count = BUFFER_SIZE/2;

static int __first_time = 0;
static int __dma_complete = 0;

MEM_HANDLE_T            hMem;

//Physical allocation resources
ULONG ulPhysicalAddr;
ULONG ulLogicalAddr;

static DECLARE_WAIT_QUEUE_HEAD(WaitQ);
static DECLARE_WAIT_QUEUE_HEAD(TWaitQ);

void pvr_dma_complete();
static void pvr_isr(int irq, void *dev_id, struct pt_regs *regs);
static int pvr_allocate_buffer(struct file *filp, unsigned long size, unsigned long *vaddr);
static int pvr_free_buffer(unsigned long vaddr);
static int pvr_write_buffer(unsigned long vaddr, unsigned long len);
static void pvr_free_all_buffers();

int pvr_osd_open(struct inode *inode,struct file *filp)
{
    int rc;

    if(__Already_init)      //if has been initialized, return
    {
        return -EINVAL;
    }
    else
    {
        hMem = os_alloc_physical(BUFFER_SIZE);

        ulPhysicalAddr = os_get_physical_address(hMem); //use this Physical address temperaryly
        ulLogicalAddr = (ULONG)os_get_logical_address(hMem);//get logical address

        rc = os_install_irq(PVR_IRQ,IRQ_DEFAULT_TRIG,(void*)&pvr_isr, NULL);    //register the interrupt procedure

        if (rc)
        {
            printk("bad irq number\n");
            return -EINVAL;
        }
	
	memset(&pvrbufs,0,sizeof(pvrbufs));
	
        pvr_osi_playback_config();      //config PVR and XP to adapt pvr mode
        __Already_init++;
    }

//  os_enable_irq(PVR_IRQ);             //add enable irq code here

    __pvr_status = PVR_STATUS_READY;
    MOD_INC_USE_COUNT;
    __first_time=0;
    return 0;
}

int pvr_osd_release(struct inode *inode, struct file *file)
{
    if(__pvr_status == PVR_STATUS_START)
        pvr_osi_playback_stop();

    __pvr_status = PVR_STATUS_STOP;

    pvr_osi_playback_deconfig();        //restore the original configuration

    os_free_physical(hMem);
    pvr_free_all_buffers();
    
//  os_disable_irq(PVR_IRQ);
    os_uninstall_irq(PVR_IRQ);

    __Already_init--;

    MOD_DEC_USE_COUNT;
    return 0;
}

int pvr_osd_ioctl(struct inode *inode, struct file *file,
                  unsigned int ioctl_num, unsigned long ioctl_param)
{
    PVR_STATUS  pvr_status;

    switch(ioctl_num)
    {
    case PVR_SET_MODE:                  //Set the PVR mode: word or line
        __pvr_status = PVR_STATUS_START;
        mode = (PVR_PLAYBACK_MODE)ioctl_param;
        break;
    case PVR_STOP:
        if(__pvr_status != PVR_STATUS_START)
            return -1;

        __pvr_status = PVR_STATUS_STOP;
        pvr_osi_playback_stop();
        break;
    case PVR_GET_STATUS:
        pvr_osi_get_status(&pvr_status);

        //the first two arguments are the wrong way round, fixed here
        if (copy_to_user((void *)ioctl_param,&pvr_status,sizeof(pvr_status)))
            return -EFAULT;
        break;
//lingh added for PVR

    case PVR_BUFF_FLUSH:
        {
            pvr_osi_flush();
            interruptible_sleep_on_timeout(&TWaitQ,20);
            pvr_osi_unflush();
	    __first_time = 0;
            break;
        }

    case PVR_SET_VID_PID:
        {
            pvr_osi_set_vid(ioctl_param);
            break;
        }
    case PVR_ALLOCATE_BUFFER:
        {
	    PVR_ALLOCATE_BUFFER_PARAM param;
	    unsigned long vaddr;
	    int ret;
	     
	    if(copy_from_user(&param, (void *)ioctl_param, sizeof(PVR_ALLOCATE_BUFFER_PARAM)))
	      return -EFAULT;
	    ret = pvr_allocate_buffer(file,param.len,&vaddr);
	    if(ret == 0)
	    {
              param.vaddr = (void *)vaddr;

	      if (copy_to_user((void *)ioctl_param,&param,sizeof(PVR_ALLOCATE_BUFFER_PARAM)))
                return -EFAULT;
	    }  
	    return(ret);
	}
    case PVR_FREE_BUFFER:
        {
	    return(pvr_free_buffer(ioctl_param));
	}
    case PVR_WRITE_BUFFER:
        {
	    PVR_WRITE_BUFFER_PARAM param;
	    
	    if(copy_from_user(&param, (void *)ioctl_param, sizeof(PVR_WRITE_BUFFER_PARAM)))
	      return -EFAULT;
	    return(pvr_write_buffer((int)param.vaddr, param.len)); 
	}
    default:
        return -EINVAL;
    }
    return 0;
}

int pvr_device_mmap(struct file *file, struct vm_area_struct *vma)
{
  unsigned long size;
  MEM_HANDLE_T  hMem = NULL;
  unsigned long physaddr;
  int i;
  
  for(i = 0; i < PVRBUFS_MAX; i++)
  {
    if(pvrbufs[i].size == 0)
    {
      size = vma->vm_end - vma->vm_start;
      if(size == 0)
        return(-EINVAL);
      size = (size + 4095) & -4096;
      hMem = os_alloc_physical_justify(size,4096);
      if(hMem == NULL )
      {
        printk("pvr_device_mmap: os_alloc_physical_justify failed\n");
        return(-ENOMEM);
      }
      physaddr = os_get_physical_address(hMem);
       
      vma->vm_page_prot.pgprot |= _PAGE_NO_CACHE;     // map without caching
      if(remap_page_range(vma->vm_start,
                          physaddr,
                          size, vma->vm_page_prot))
      {
        printk("pvr_device_mmap: remap_page_range failed\n");
	os_free_physical(hMem);
        return -5;
      }

      pvrbufs[i].mm = current->mm;
      pvrbufs[i].vaddr = vma->vm_start;
      pvrbufs[i].physaddr = physaddr;
      pvrbufs[i].size = size;
      pvrbufs[i].hMem = hMem;
      return(0);
    }
  }
  return(-ENOMEM);
}

ssize_t pvr_osd_write(struct file *file,const char *buffer,
                         size_t length, loff_t * offset)
{
    ULONG ulBufOffset;
    ULONG left;
    ULONG phy_off;
    UCHAR *src;
    UCHAR *dest;
    ULONG count;
    ULONG flag;
    ULONG ulbuffer_size;
    ulBufOffset = 0;
    left = length;


    if(mode == PLAYBACK_WORD)
    {
      if(length & 3)
        return -EINVAL;
      ulbuffer_size = (BUFFER_SIZE/2/4)*4;
    }
    else
    {
      if(length & 31)
        return -EINVAL;
      ulbuffer_size = (BUFFER_SIZE/2/32)*32;
    }
    while(left)
    {
        if(left >= ulbuffer_size)
            count = ulbuffer_size;
        else
        {
            count = left;
        }
        if (__buf_pingpong == 0 || __first_time == 0)
        {
            __buf_pingpong = 1;
            phy_off = 0x00000000;
        }
        else
        {
            __buf_pingpong = 0;
            phy_off = ulbuffer_size;
        }

        src = (UCHAR *)((ULONG)buffer + ulBufOffset);
        dest = (UCHAR*)ulLogicalAddr + phy_off;

        if(mode == PLAYBACK_WORD)
           pvr_osi_set_dma(ulPhysicalAddr+__last_phy_off,__last_count>>2);
        
        else if(mode == PLAYBACK_LINE)
           pvr_osi_set_dma(ulPhysicalAddr+__last_phy_off,__last_count>>5);
        

        __last_count = count;
        __last_phy_off = phy_off;

        ulBufOffset += count;
        left -= count;

        if(!__first_time)
        {
            __first_time = 1;
            copy_from_user(dest,src,count);          //get TS data from user space
            continue;
        }

        __dma_complete = 0;
        pvr_osi_playback_start(mode);       //start DMA operation
            copy_from_user(dest,src,count);          //get TS data from user space

        flag = os_enter_critical_section(); //disable interrupt

        if(!__dma_complete)
        {
            interruptible_sleep_on(&WaitQ);

            os_leave_critical_section(flag);

        }
        else
        {
            os_leave_critical_section(flag);
        }

    }

    return ulBufOffset;
}

static void pvr_isr(int irq, void *dev_id, struct pt_regs *regs)

{
        ULONG interrupt = MF_DCR(PVR_INT_DCR);

        if((interrupt & PVR_DMA_COMPLETE) == 0)
                return;
        pvr_dma_complete();

}


void pvr_dma_complete()
{
    __dma_complete = 1;
    wake_up_interruptible(&WaitQ);
}


static int pvr_allocate_buffer(struct file *filp, unsigned long size, unsigned long *vaddr)
{
  int ret;
  
#if LINUX_VERSION_CODE <= 0x020402
  down( &current->mm->mmap_sem );
#else
  down_write( &current->mm->mmap_sem );
#endif
  ret = do_mmap(filp,0, size, PROT_WRITE, MAP_SHARED | MAP_LOCKED, 0);
#if LINUX_VERSION_CODE <= 0x020402
  up( &current->mm->mmap_sem );
#else
  up_write( &current->mm->mmap_sem );
#endif
  if( (unsigned long)ret > -1024UL )
  {
    printk("pvr_allocate_buffer: mmap failed = %X\n",ret);
    return(ret);
  }
  else
  {
    *vaddr = (unsigned long)ret;
    return(0);
  }
}

static int pvr_free_buffer(unsigned long vaddr)
{
  struct mm_struct *mm;
  int i;

  for(i = 0; i < PVRBUFS_MAX; i++)
  {
    if(pvrbufs[i].size == 0)
      continue;

    if(pvrbufs[i].mm != current->mm)      
      continue;
      
    if(pvrbufs[i].vaddr != vaddr)
      continue;
      
    mm = pvrbufs[i].mm;  
    
#if LINUX_VERSION_CODE <= 0x020402
    down( &mm->mmap_sem );
#else
    down_write( &mm->mmap_sem );
#endif
    
#if LINUX_VERSION_CODE <= 0x020411
    do_munmap(mm, pvrbufs[i].vaddr, pvrbufs[i].size);
#else
    do_munmap(mm, pvrbufs[i].vaddr, pvrbufs[i].size, 1);
#endif

#if LINUX_VERSION_CODE <= 0x020402
    up( &mm->mmap_sem );
#else
    up_write( &mm->mmap_sem );
#endif
    os_free_physical(pvrbufs[i].hMem);
    pvrbufs[i].size = 0;
    pvrbufs[i].mm = 0;
    pvrbufs[i].vaddr = 0;
    pvrbufs[i].physaddr = 0;
    pvrbufs[i].hMem = 0;
    return(0);
  }
  
  return(-1);
}

static void pvr_free_all_buffers()
{
  int i;
  struct mm_struct *mm;
  
  for(i = 0; i < PVRBUFS_MAX; i++)
  {
    if(pvrbufs[i].size == 0)
      continue;
      
    mm = pvrbufs[i].mm;  
    
#if LINUX_VERSION_CODE <= 0x020402
    down( &mm->mmap_sem );
#else
    down_write( &mm->mmap_sem );
#endif
    
#if LINUX_VERSION_CODE <= 0x020411
    do_munmap(mm, pvrbufs[i].vaddr, pvrbufs[i].size);
#else
    do_munmap(mm, pvrbufs[i].vaddr, pvrbufs[i].size, 1);
#endif

#if LINUX_VERSION_CODE <= 0x020402
    up( &mm->mmap_sem );
#else
    up_write( &mm->mmap_sem );
#endif
    os_free_physical(pvrbufs[i].hMem);
    pvrbufs[i].size = 0;
    pvrbufs[i].mm = 0;
    pvrbufs[i].vaddr = 0;
    pvrbufs[i].physaddr = 0;
    pvrbufs[i].hMem = 0;

  }
  return;
}

static int pvr_write_buffer(unsigned long vaddr, unsigned long len)
{
  int i;
  int flag;
    
  for(i = 0; i < PVRBUFS_MAX; i++)
  {
    if(pvrbufs[i].size == 0)
      continue;

    if(pvrbufs[i].mm != current->mm)      
      continue;
      
    if(pvrbufs[i].vaddr != vaddr)
      continue;
      
  
    if(mode == PLAYBACK_WORD)
    {
      if((len%4) != 0)
        return -EINVAL;
      pvr_osi_set_dma(pvrbufs[i].physaddr,len>>2);
    }
    else if(mode == PLAYBACK_LINE)
    {
      if((len%32) != 0)
        return -EINVAL;
      pvr_osi_set_dma(pvrbufs[i].physaddr,len>>5);
    }
    __dma_complete = 0;
    pvr_osi_playback_start(mode);       //start DMA operation

    flag = os_enter_critical_section(); //disable interrupt

    if(!__dma_complete)
    {
      interruptible_sleep_on(&WaitQ);

      os_leave_critical_section(flag);

    }
    else
    {
      os_leave_critical_section(flag);
    }
    return(len);
  }    
  return(-1);
}




