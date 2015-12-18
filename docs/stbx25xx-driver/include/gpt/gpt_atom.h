//pallas/drv/include/atom_gptc.h
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
//  physical layer atom driver of GPT on Pallas 
//Revision Log:   
//  Sept/07/2001    Created by YYD
//                  Only GPTC is supported

#ifndef _DRV_INCLUDE_ATOM_GPT_H_INC_
#define _DRV_INCLUDE_ATOM_GPT_H_INC_

#include "os/os-types.h"


// gptc attr
#define __GPTC_ENABLE      0x001       // * using interrupt mode
#define __GPTC_DISABLE     0x002       //   don't using interrupt mode

#define __GPTC_FALL_EDGE   0x004       // * detect falling edge
#define __GPTC_RAISE_EDGE  0x008       //   detect raising edge

#define __GPTC_SYNC_CAPT   0x010       // * syncronized capture
#define __GPTC_ASYNC_CAPT  0x020       //   asyncronized capture

typedef enum __GPT_DEVICS_MASK__
{
    __GPT_DEV_CAPT0=0x80000000,
    __GPT_DEV_CAPT1=0x40000000,
    __GPT_DEV_COMP0=0x00008000,
    __GPT_DEV_COMP1=0x00004000,
    __GPT_DEV_COMP2=0x00002000,
    __GPT_DEV_COMP3=0x00001000,
    __GPT_DEV_COMP4=0x00000800,
    __GPT_DEV_COMP5=0x00000400,
    __GPT_DEV_COMP6=0x00000200
} __GPT_DEV_MASK;



void gpt_atom_init_port(void *pPortBase);

UINT32  gpt_atom_set_tbc(UINT32 v);

UINT32  gpt_atom_get_tbc(void);

void    gpt_atom_set_gptc(__GPT_DEV_MASK eCaptN, UINT uAttr);  

UINT32  gpt_atom_get_gptc(__GPT_DEV_MASK eCaptN);  

void    gpt_atom_enable_interrupt(__GPT_DEV_MASK eGptDev);
void    gpt_atom_disable_interrupt(__GPT_DEV_MASK eGptDev);

void    gpt_atom_mask_interrupt(__GPT_DEV_MASK eGptDev);
void    gpt_atom_unmask_interrupt(__GPT_DEV_MASK eGptDev);

UINT32  gpt_atom_get_interrupt_status(void);
UINT32  gpt_atom_set_interrupt_status(UINT uSet);
UINT32  gpt_atom_reset_interrupt_status(__GPT_DEV_MASK eGptDev);


#endif // _DRV_INCLUDE_ATOM_GPT_H_INC_
