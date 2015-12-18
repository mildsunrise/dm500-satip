//pallas/drv/ircombo/osi_irdevice.c
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
//
//Comment: 
//  OS Independent IR device driver 
//Revision Log:   
//  Sept/10/2001                         Created by YYD
//  Nov/28/2001    Add GPT atom critical section during init/deinit by YYD


#include "ir/ir_osi_device.h"
#include "os/os-sync.h"             // we need to get critical section working
#include "os/os-interrupt.h"
#include "os/helper-queue.h"
//#include "gpt/gpt_atom.h"               // for gptc routines
#include "ir_atom.h"			// for ir routines

#ifdef __IRCOMBO_DEBUG
#define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#define	__GPT_DEV_CAPT0 	0
#define	__GPT_DEV_CAPT1 	1

#ifdef   CONFIG_VULCAN
#define  __IR_DEVICE_GPTC          __GPT_DEV_CAPT0       // for Lamar
#else
#define  __IR_DEVICE_GPTC          __GPT_DEV_CAPT1       // for Redwood 5
#endif

//#define  __IR_DEVICE_GPTC_ATTR     (__GPTC_RAISE_EDGE | __GPTC_SYNC_CAPT)
//#define  __IR_DEVICE_IRQ           IRQ_GPT_PWM0  
#define  __IR_DEVICE_IRQ           _IR_INT_LINE
#define  __IR_DEVICE_IRQ_ATTR      (IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG)
#define  __IR_DEVICE_EVENTQ_SZ     32       // 32 events sized queue


#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif

//*****************************************************
static int __ir_init=0;

static __IR_DEVICE *_gpDevice=NULL;

static UINT32 _guLastTime;
static void _ir_int_handler(UINT uIrq, void *v)
{
	UINT32  intmask;
	UINT32	pPulse[32];
	UINT	i,j;

	if ( (intmask= (ir_atom_get_int_status() & _IR_INT_STAT_MASK)) )
	{
 		if (intmask & _IR_INT_FIFO_OVERFLOW)
		{	//overflow, reset IR
			ir_atom_restart();
		}
		else if ( intmask & _IR_INT_FIFO_LEVEL)
		{	// fifo level int. read the code from fifo
			i = ir_atom_get_codes(pPulse,32);
			for (j = 0; i > j ; j++)
        			os_call_irq_task(uIrq, pPulse+j);
		}
		// no timeout handler now.
		ir_atom_clear_int_status(intmask);
	}	
    return;
}

static void _ir_int_task(QUEUE_T *pQueue)
{
    UINT32  timediff;
    __IR_DEVICE *pLink;

    while(os_irq_task_get_msg(pQueue, &timediff) >= 0)
    {
	PDEBUG("I %d\n", timediff);
        pLink = _gpDevice;
        while(pLink)
        {
            if (pLink->event) pLink->event(timediff);
            pLink = pLink->pNext;
        }
    }
}
//*******************************************************



INT ir_osi_init(void)  // requires GPIO capture intialized
{
    	UINT32 crt;

    	if(__ir_init) return 0;
    	if(os_install_irq(__IR_DEVICE_IRQ, __IR_DEVICE_IRQ_ATTR, _ir_int_handler, NULL) < 0)
    	{
        	PFATAL("IR irq install failed!\n");
        	return -1;
    	}
    	os_disable_irq(__IR_DEVICE_IRQ); // Fixme: The IRQ is enabled by install_irq

   	if(os_add_irq_task(__IR_DEVICE_IRQ, _ir_int_task, sizeof(UINT32), __IR_DEVICE_EVENTQ_SZ) < 0)
    	{
        	PFATAL("IR add irq task failed!\n");
        	os_uninstall_irq(__IR_DEVICE_IRQ);
        	return -1;
    	}
    	__ir_init = 1;
    	{
        	__IR_DEVICE *pLink = _gpDevice;
        	while(pLink)
        	{
            	if(pLink->init) pLink->init();
            	pLink = pLink->pNext;
        	}
    	}
    
    	crt = os_enter_critical_section();
   	ir_atom_init(__IR_DEVICE_GPTC,0,_IR_SAMPLE_CLOCK); 
    	// capture device, negative polarity, sample_clock.
    	// disable IR ? GY
    	os_leave_critical_section(crt);

    	return 0;
}

INT ir_osi_deinit(void)
{
    	UINT32 crt;
    	if(!__ir_init) return 0;

    	crt = os_enter_critical_section();
    	ir_atom_disable_int(_IR_INT_STAT_MASK);
    	ir_atom_clear_int_status(_IR_INT_STAT_MASK);
    	//disable IR ? GY
    	ir_atom_disable_ir();
    	os_leave_critical_section(crt);

    	os_delete_irq_task(__IR_DEVICE_IRQ);
    	os_uninstall_irq(__IR_DEVICE_IRQ);
    	{
        	__IR_DEVICE *pLink = _gpDevice;
        	while(pLink)
        	{
            		if(pLink->deinit) pLink->deinit();
            		pLink = pLink->pNext;
        	}
    	}
    	__ir_init = 0;
    	return 0;
}

INT ir_osi_start(void)
{
    	UINT32 crt;

    	if(!__ir_init) return -1;
    	crt = os_enter_critical_section();
   	// enable IR GY
//    	ir_atom_enable_ir();
    	ir_atom_enable_int(_IR_INT_STAT_MASK);
    	os_leave_critical_section(crt);
    	os_enable_irq(__IR_DEVICE_IRQ);
    	return 0;
}

INT ir_osi_stop(void)
{
    	UINT32 crt;

    	if(!__ir_init) return -1;
    	os_disable_irq(__IR_DEVICE_IRQ);
    	crt = os_enter_critical_section();
	// disable IR?
//    	ir_atom_disable_ir();
	ir_atom_disable_int(_IR_INT_STAT_MASK);
    	ir_atom_clear_int_status(_IR_INT_STAT_MASK);
    	os_leave_critical_section(crt);
    	return 0;
}


INT ir_osi_register_device(__IR_DEVICE *pDev)
{
        UINT32 crt;
    	__IR_DEVICE **pLink = &_gpDevice;
    	if(NULL == pDev) return -1;
    	// make sure pDev->pNext = NULL is first, such that we don't need enter critical section
    	if(NULL != *pLink) // check if it is registered
    	{
       		while(*pLink)
        	{
            		if(*pLink == pDev) return 0;  // already registered
            		pLink =&(*pLink)->pNext;
        	}
   	}
    	// add the device to link, pay attention to the orders
        crt = os_enter_critical_section();
    	pDev->pNext = NULL;
    	*pLink = pDev;
        os_leave_critical_section(crt);
    	return 0;
}


INT ir_osi_unregister_device(__IR_DEVICE *pDev)
{
    __IR_DEVICE **pLink = &_gpDevice;
    if(NULL == pDev) return -1;
    if(NULL == *pLink) // check if it is registered
        return 0;

    while(*pLink)
    {
        if(*pLink == pDev) break;  // yes, registered
        pLink = &(*pLink)->pNext;
    }

    if( *pLink == pDev) // ok, here it is
    {
        UINT32 crt;
        // this time we need to enter critical section
        crt = os_enter_critical_section();
        *pLink = (*pLink)->pNext;
        pDev->pNext = NULL;
        os_leave_critical_section(crt);
        return 0;
    }
    return -1;
}

