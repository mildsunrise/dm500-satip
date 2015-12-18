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
#ifdef LINUX
#define __NO_VERSION__

#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/string.h>       /* for memset */
#include <asm/irq.h>

#endif  // LINUX

#include <os/os-sync.h>
#include <os/os-interrupt.h>
#include <os/os-io.h>

#undef __DRV_DEBUG
#include <os/drv_debug.h>

#include "os-interrupt-local.h"

#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif


static UINT _mask_tab[MAX_IRQ] = {
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001
};
static IRQ     _irq_tab[MAX_IRQ];
static UINT    _irq_tab_init = 0;

void inline os_irq_clear(IRQ *pIrq)
{
    pIrq->uIrq = 0;
    pIrq->uAttr = 0;
    pIrq->handler = NULL;
    pIrq->nCount = 0;
    pIrq->uTaskFlag = 0;
    pIrq->pData = NULL;
    pIrq->mutex = NULL;
}

INT    os_irq_init(void)
{
    int i;
    if(_irq_tab_init != 0)
        return -1;

    for(i =0; i < MAX_IRQ; i++)
    {
        os_irq_clear(&(_irq_tab[i]));
    }
    _irq_tab_init = 1;
    return 0;
}


//As an interrupt handler, current interrupt is disabled
void os_irq_standard_handler(int irq, void* dev_id, struct pt_regs *regs)
{
    IRQ *pIrq;
    pIrq = (IRQ*)dev_id;

//printk("<0>@"); // YYD

    PDEBUG("IRQ: irq handler = %d\n", irq);
    if(pIrq->uIrq == 0)
        return;
    if(pIrq->handler)
    {
        pIrq->nCount++;
        pIrq->handler(pIrq->uIrq, pIrq->pData);
    }
}


//install new interrupt handler
INT    os_install_irq(UINT uIrq, UINT uAttr,
                       void (*handler)(UINT uIrq, void *pData), 
                       void *pData)
{
    UINT uReg;
    UINT uRet;
    MUTEX_T mutex;
    UINT uFlags;

    PDEBUG("IRQ: install new IRQ = %d", uIrq);
    // Check validity of new interrupt
    if(uIrq == 0 || uIrq > MAX_IRQ)
    {
        PDEBUG("IRQ: irq number is out of range\n");
        return -1;
    }    

    if((uAttr & (IRQ_LEVEL_TRIG | IRQ_EDGE_TRIG)) == 0 )
    {
        PDEBUG("IRQ: haven't specified level/edge trigger\n");
        return -1;
    }

    if((uAttr & (IRQ_POSITIVE_TRIG | IRQ_NEGATIVE_TRIG)) == 0)
    {
        PDEBUG("IRQ: haven't specified positive/negative trigger\n");
        return -1;
    }

    // check if interrupt handler exists
    if (handler == NULL)
    {
        PDEBUG("IRQ: No irq_handler\n");
        return -1;
    }

    if (_irq_tab[uIrq].uIrq != 0)
    {
        PDEBUG("IRQ: irq %d exists\n", uIrq);
        return -1;
    }

    //create mutex for futher task operation
    if((mutex = os_create_mutex()) == NULL)
    {
        PDEBUG("TASK: can not create mutex\n");
        return -1;
    }

    //enter critical section
    uFlags = os_enter_critical_section();

    // set level/edge bit in UIC
    uReg = MF_DCR(UICTR) & (~_mask_tab[uIrq]);
    if( (uAttr & IRQ_EDGE_TRIG) != 0)
    {
        MT_DCR(UICTR, (uReg | _mask_tab[uIrq]));
    }
    else
    {
        MT_DCR(UICTR, uReg);
    }

    // set polarity bit in UIC
    uReg = MF_DCR(UICPR) & (~_mask_tab[uIrq]);
    if( (uAttr & IRQ_POSITIVE_TRIG) != 0)
    {
        MT_DCR(UICPR, (uReg | _mask_tab[uIrq]));
    }
    else
    {
        MT_DCR(UICPR, uReg);
    }

    _irq_tab[uIrq].uIrq = uIrq;
    _irq_tab[uIrq].nCount = 0;
    _irq_tab[uIrq].uAttr = uAttr;
    _irq_tab[uIrq].uTaskFlag = 0;
    _irq_tab[uIrq].pData = pData;
    _irq_tab[uIrq].handler = handler;
    _irq_tab[uIrq].mutex = mutex;

#ifdef LINUX
    // install hardware interrupt handler
    uRet = request_irq(uIrq,
        os_irq_standard_handler, SA_SHIRQ, NULL, &(_irq_tab[uIrq]));
#endif // LINUX

    if (uRet != 0)
    {
        os_irq_clear(&(_irq_tab[uIrq]));
        os_leave_critical_section(uFlags);
        os_delete_mutex(mutex);
        return -1;
    }

    os_leave_critical_section(uFlags);
    return 0;
}


INT    os_uninstall_irq(UINT uIrq)
{
    MUTEX_T mutex;
    UINT uFlags;

    PDEBUG("IRQ: uninstall IRQ = %d", uIrq);
    // Check validity of new interrupt
    if(uIrq == 0 || uIrq > MAX_IRQ)
    {
        PDEBUG("IRQ: irq number is out of range\n");
        return -1;
    }

    if(_irq_tab[uIrq].uIrq == 0)
    {
        PDEBUG("IRQ: this interrupt not installed\n");
        return -1;
    }
    if(_irq_tab[uIrq].uTaskFlag != 0)
    {
        PDEBUG("IRQ: attached task is activing, remove first\n");
        return -1;
    }
    mutex = _irq_tab[uIrq].mutex;

    //enter critical section
    uFlags = os_enter_critical_section();
    {

#ifdef LINUX
        // uninstall hardware interrupt handler
        free_irq(uIrq, &(_irq_tab[uIrq]));     
#endif // LINUX

    }
    
    os_irq_clear(&(_irq_tab[uIrq]));
    os_leave_critical_section(uFlags);

    //delete mutex
    if(mutex)
        os_delete_mutex(mutex);
    return 0;
}

INT    os_enable_irq(UINT uIrq)
{
#ifdef LINUX
//printk("<0>["); // YYD

    enable_irq(uIrq);
    return 0;
#endif
}

INT    os_disable_irq(UINT uIrq)
{
#ifdef LINUX
//printk("<0>]"); // YYD
    disable_irq(uIrq);
    return 0;
#endif
}

INT    os_get_irq_count(UINT uIrq)
{
    if(uIrq == 0 || uIrq > MAX_IRQ)
    {
        PDEBUG("IRQ: irq number is out of range\n");
        return -1;
    }

    if(_irq_tab[uIrq].uIrq == 0)
    {
        PDEBUG("IRQ: this interrupt not installed\n");
        return -1;
    }

    return _irq_tab[uIrq].nCount;
}

INT    os_reset_irq_count(UINT uIrq)
{
    UINT uFlags;

    if(uIrq == 0 || uIrq > MAX_IRQ)
    {
        PDEBUG("IRQ: irq number is out of range\n");
        return -1;
    }

    if(_irq_tab[uIrq].uIrq == 0)
    {
        PDEBUG("IRQ: this interrupt not installed\n");
        return -1;
    }

    uFlags = os_enter_critical_section();
    {
        _irq_tab[uIrq].nCount = 0;
    }
    os_leave_critical_section(uFlags);
    return 0;
}

INT    os_dump_irq(UINT uIrq)
{
    if(uIrq == 0 || uIrq > MAX_IRQ)
    {
        PDEBUG("IRQ: irq number is out of range\n");
        return -1;
    }

    PDEBUG("IRQ: irq number = %u \n", _irq_tab[uIrq].uIrq);
    PDEBUG("IRQ: irq attribute = 0x%8.8x\n", _irq_tab[uIrq].uAttr);
    PDEBUG("IRQ: irq count = %u\n", _irq_tab[uIrq].nCount);
    PDEBUG("IRQ: irq handler = 0x%8.8x\n", (UINT)_irq_tab[uIrq].handler);
    PDEBUG("IRQ: irq task = 0x%8.8x\n", _irq_tab[uIrq].uTaskFlag);
    if(_irq_tab[uIrq].uTaskFlag == 1)
    {
        PDEBUG("IRQ: task function = 0x%8.8x\n", (UINT)_irq_tab[uIrq].task.irq_task);
        PDEBUG("IRQ: task msg size = %u\n", _irq_tab[uIrq].task.uMsgSize);
        PDEBUG("IRQ: task active number = %u\n", _irq_tab[uIrq].task.nActiveNum);
    }
    return 0;
}

INT    os_add_irq_task(UINT uIrq, void (*irq_task)(QUEUE_T *pQueue), 
                        UINT uMsgSize, 
                        UINT uQueueSize)
{
    TASK *pTask;

    PDEBUG("IRQ: add irq task = %d", uIrq);
    // Check validity of new interrupt
    if(uIrq == 0 || uIrq > MAX_IRQ)
    {
        PDEBUG("TASK: irq number is out of range\n");
        return -1;
    }

    if(irq_task == NULL)
    {
        PDEBUG("TASK: no task function\n");
        return -1;
    }

    if(_irq_tab[uIrq].uIrq == 0)
    {
        PDEBUG("IRQ: this interrupt not installed\n");
        return -1;
    }

    //synchronize to access uTaskFlag
    os_get_mutex(_irq_tab[uIrq].mutex);
    if(_irq_tab[uIrq].uTaskFlag != 0)
    {
        PDEBUG("TASK: attached task is activing, remove first\n");
        os_release_mutex(_irq_tab[uIrq].mutex);
        return -1;
    }

    pTask = &(_irq_tab[uIrq].task);
    //clear task
    memset((void*)pTask, 0, sizeof(TASK));
    //create message queue
    if(uQueueSize != 0 && uMsgSize != 0)
    {
        if( os_create_queue(&(pTask->Queue), NULL, uQueueSize, uMsgSize) != 0)
        {
            PDEBUG("IRQ.TASK: create msg queue error\n");
            return -1;
        }
        pTask->uQueueFlag = 1;
    }
    else
        pTask->uQueueFlag = 0;

    pTask->task.routine = os_standard_task;
    pTask->task.data = (void*)pTask;
    pTask->uIrq = uIrq;
    pTask->uMsgSize = uMsgSize;
    pTask->irq_task = irq_task;
    pTask->nActiveNum = 0;
    _irq_tab[uIrq].uTaskFlag = 1;

    os_release_mutex(_irq_tab[uIrq].mutex);
    return 0;
}

//count the number of active task
void os_standard_task(void *pData)
{
    UINT uFlags;
    TASK* pTask = pData;

    uFlags = os_enter_critical_section();
    {
        if(pTask->irq_task)
        {
            void (*irq_task)(QUEUE_T *pQueue);
            irq_task = pTask->irq_task;
            os_leave_critical_section(uFlags);
            //call user's task function
            if(pTask->uQueueFlag)
                irq_task(&(pTask->Queue));
            else
                irq_task(NULL);
        }
        else
        {
            os_leave_critical_section(uFlags);
            PDEBUG("TASK: active task without handler!\n");
        }
    }
}


/*This function must be called from interrupt handler and we
  assume interrupt is disabled here*/
INT    os_call_irq_task(UINT uIrq, void *pMsg)
{
    /*we do not check parameter here because this function is usually called 
      by interrupt handler*/
    TASK *pTask;

    PDEBUG("TASK: call irq task %d\n", uIrq);
    if(_irq_tab[uIrq].uTaskFlag == 0)
        return -1;
    pTask = &(_irq_tab[uIrq].task);

    //enqueue new message
    if(pTask->uQueueFlag)
        if( os_enqueue(&(pTask->Queue), pMsg) < 0 )
            return -1;

    //install task function to system task queue
#ifdef LINUX
    queue_task(&(pTask->task), &tq_immediate);
    mark_bh(IMMEDIATE_BH);
#endif
    return 0;
}

INT    os_delete_irq_task(UINT uIrq)
{
    TASK *pTask;
    UINT uFlags;

    PDEBUG("TASK: try to delete irq task = %d", uIrq);
    // Check validity of new interrupt
    if(uIrq == 0 || uIrq > MAX_IRQ)
    {
        PDEBUG("IRQ.TASK: irq number is out of range\n");
        return -1;
    }
    //keep synchronize with os_add_irq_task
    os_get_mutex(_irq_tab[uIrq].mutex);

    if(_irq_tab[uIrq].uTaskFlag == 0)
    {
        os_release_mutex(_irq_tab[uIrq].mutex);
        return 0;
    }

    pTask = &(_irq_tab[uIrq].task);
    //check if all activated tasks have been executed

    uFlags = os_enter_critical_section();
    if(os_get_queue_status(&(pTask->Queue)) > 0)
    {
        PDEBUG("TASK: activated task exist, please try again!\n");
        os_leave_critical_section(uFlags);
        os_release_mutex(_irq_tab[uIrq].mutex);
        return -1;
    }
    
    // stop all futher calling tasks now
    _irq_tab[uIrq].uTaskFlag = 0;

    os_leave_critical_section(uFlags);

    if(pTask->uQueueFlag)
        os_delete_queue(&(pTask->Queue));
    pTask->uIrq = 0;
    pTask->irq_task = NULL;
    os_release_mutex(_irq_tab[uIrq].mutex);

    return 0;
}    

INT    os_irq_task_get_msg(QUEUE_T *pQueue, void *pMsg)    
{
    return os_dequeue(pQueue, pMsg);
}











