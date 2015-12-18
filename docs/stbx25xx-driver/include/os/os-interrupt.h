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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:		Ling Shao
| Component:	Include file.
| File:			os-interrupt.h
| Purpose:		IRQ definition.
| Changes:
| Date:         Comment:
| -----         --------
| 08-Mar-01     Created                                                     SL
+----------------------------------------------------------------------------*/
#ifndef __OS_INTERRUPT_H_
#define __OS_INTERRUPT_H_

#include "os-types.h"
#include "helper-queue.h"
#include "hw/hardware.h"


#define	MAX_IRQ             32
#define	IRQ_LEVEL_TRIG      0x01
#define	IRQ_EDGE_TRIG       0x02
#define	IRQ_POSITIVE_TRIG   0x04
#define	IRQ_NEGATIVE_TRIG   0x08

#define UICPR				0x44
#define UICTR				0x45

#ifdef __DRV_FOR_PALLAS__

#define IRQ_XPORT			1
#define IRQ_AUD             2
#define IRQ_VID             3
#define IRQ_DMA0            4
#define IRQ_DMA1            5
#define IRQ_IDE             6
#define IRQ_CTRL1394        7
#define IRQ_SC0             8
#define IRQ_IIC0            9
#define IRQ_IIC1            10
#define IRQ_GPT_PWM0        11
#define IRQ_GPT_PWM1        12
#define IRQ_SCP             13
#define IRQ_SSP             14
#define IRQ_GPT_PWM2        15
#define IRQ_SC1             16
#define IRQ_GFX             17
#define IRQ_USB0            18
#define IRQ_USB1            19
#define IRQ_SERIAL0         20
#define IRQ_SICC_REC        21
#define IRQ_SICC_TRAN       22
#define IRQ_OPB_PLB         23
#define IRQ_EXT_PROC        24
#define IRQ_EXT_INT0        25
#define IRQ_EXT_INT1        26
#define IRQ_EXT_INT2        27
#define IRQ_EXT_INT3        28
#define IRQ_EXT_INT4        29
#define IRQ_EXT_INT5        30
#define IRQ_SERIAL2         31

#elif defined(__DRV_FOR_VULCAN__)

#define IRQ_RTCFPC			0       
#define IRQ_XPORT			1
#define IRQ_AUD             2
#define IRQ_VID             3
#define IRQ_DMA0            4
#define IRQ_DMA1            5
#define IRQ_DMA2            6
#define IRQ_DMA3            7
#define IRQ_SC0             8
#define IRQ_IIC0            9
#define IRQ_IRR             10        
#define IRQ_GPT_PWM0        11
#define IRQ_GPT_PWM1        12
#define IRQ_SCP             13
#define IRQ_SSP             14
#define IRQ_GPT_PWM2        15
#define IRQ_SC1             16
#define IRQ_EXT_INT7        17
#define IRQ_EXT_INT8        18
#define IRQ_EXT_INT9        19
#define IRQ_SERIAL0         20
#define IRQ_SERIAL1         21
#define IRQ_SERIAL2         22
#define IRQ_XPTDMA          23
#define IRQ_DCR_IDE         24
#define IRQ_EXT_INT0        25
#define IRQ_EXT_INT1        26
#define IRQ_EXT_INT2        27
#define IRQ_EXT_INT3        28
#define IRQ_EXT_INT4        29
#define IRQ_EXT_INT5        30
#define IRQ_EXT_INT6        31

#elif defined(__DRV_FOR_VESTA__)

#define IRQ_RTCFPC			0       
#define IRQ_XPORT			1
#define IRQ_AUD             2
#define IRQ_VID             3
#define IRQ_DMA0            4
#define IRQ_DMA1            5
#define IRQ_DMA2            6
#define IRQ_DMA3            7
#define IRQ_SC0             8
#define IRQ_IIC0            9
#define IRQ_IIC1            10        
#define IRQ_GPT_PWM0        11
#define IRQ_GPT_PWM1        12
#define IRQ_SCP             13
#define IRQ_SSP             14
#define IRQ_GPT_PWM2        15
#define IRQ_SC1             16
#define IRQ_EXT_INT7        17
#define IRQ_EXT_INT8        18
#define IRQ_EXT_INT9        19
#define IRQ_SERIAL0         20
#define IRQ_SICC_RCV        21
#define IRQ_SICC_TSM        22
#define IRQ_PPU             23
#define IRQ_DCRX            24
#define IRQ_EXT_INT0        25
#define IRQ_EXT_INT1        26
#define IRQ_EXT_INT2        27
#define IRQ_EXT_INT3        28
#define IRQ_EXT_INT4        29
#define IRQ_EXT_INT5        30
#define IRQ_EXT_INT6        31

#else

#error "Unsupported architecture, please specify it in 'include/hw/hardware.h'"

#endif

INT		os_install_irq(UINT uIrq, UINT uAttr,
					   void (*handler)(UINT uIrq, void *pData), 
					   void *pData);
INT		os_uninstall_irq(UINT uIrq);
INT		os_enable_irq(UINT uIrq);
INT		os_disable_irq(UINT uIrq);
INT		os_get_irq_count(UINT uIrq);
INT		os_reset_irq_count(UINT uIrq);
INT		os_dump_irq(UINT uIrq);
INT		os_irq_init(void);
INT	    os_add_irq_task(UINT uIrq, void (*irq_task)(QUEUE_T *pQueue), 
						UINT uMsgSize, 
						UINT uQueueSize);
INT		os_call_irq_task(UINT uIrq, void *pMsg);
INT		os_delete_irq_task(UINT uIrq);
INT		os_irq_task_get_msg(QUEUE_T *pQueue, void *pMsg);

#endif
