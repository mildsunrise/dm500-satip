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
#ifndef PALLAS_OS_INTERRUPT_LOCAL_H
#define PALLAS_OS_INTERRUPT_LOCAL_H

#include <asm/ptrace.h>   /*for struct pt_regs*/

typedef struct tagTask
{
	UINT		uIrq;		//interrupt number this task attaching
	void        (*irq_task)(QUEUE_T *pQueue);	//task function
	UINT		uMsgSize;	//size of each message
	QUEUE_T		Queue;		//msg queue of this task
    UINT        uQueueFlag; //queue flag 0: clear 1: initialized
	INT			nActiveNum; //active task count
#ifdef LINUX
	struct		tq_struct task; //linux task queue structure
#endif
} TASK;

typedef struct tagIRQ
{
    UINT		uIrq;		//interrupt number
	UINT		uAttr;		//interrupt attribute
	INT			nCount;		//invoked interrupt count
	void    	(*handler)(UINT uIrq, void *pData);	//interrupt handler
	void		*pData;		//parameter for handler
	UINT		uTaskFlag;		//uTaskFlag = 1 task attached, uTaskFlag = 0 no task attached
	TASK		task;		//attached task
	MUTEX_T     mutex;		//mutex of task operation 
}IRQ;

void    os_irq_standard_handler(int irq, void* dev_id, struct pt_regs *regs);
void	os_standard_task(void *pData);
void    os_irq_clear(IRQ *pIrq);

#endif
