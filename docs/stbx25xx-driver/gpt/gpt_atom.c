//pallas/drv/gpt/stb_gpt.c
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
//  physical layer driver of GPT on Pallas  
//Revision Log:   
//  Sept/07/2001                         Created by YYD
//                  Only GPTC is supported

#include "os/os-io.h"
#include "gpt/gpt_ports.h"
#include "gpt/gpt_atom.h"


#ifdef __GPT_ATOM_DEBUG
#define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

static UINT32 _gPortBase;

void gpt_atom_init_port(void *pPortBase)
{
    _gPortBase = (UINT32)pPortBase;
}


UINT32  gpt_atom_set_tbc(UINT32 v)
{
    _OS_OUTL(_gPortBase + __GPT_TBC, v);
    return _OS_INL(_gPortBase + __GPT_TBC);   // gpt tbc requires an read before next write
}


UINT32  gpt_atom_get_tbc(void)
{
    return _OS_INL(_gPortBase + __GPT_TBC); 
}

void    gpt_atom_set_gptc(__GPT_DEV_MASK eCaptN, UINT uAttr)
{
    switch(eCaptN)
    {
    case __GPT_DEV_CAPT0:
    case __GPT_DEV_CAPT1:
        // enable / disable
        if(uAttr & __GPTC_ENABLE)
        {
            UINT uData = _OS_INL(_gPortBase + __GPT_GPTCE) | (UINT)eCaptN;
            _OS_OUTL(_gPortBase + __GPT_GPTCE, uData);
        }
        else if(uAttr & __GPTC_DISABLE) 
        {
            UINT uData = _OS_INL(_gPortBase + __GPT_GPTCE) & ~(UINT)eCaptN;
            _OS_OUTL(_gPortBase + __GPT_GPTCE, uData);
        }
        // edge polarity
        if(uAttr & __GPTC_FALL_EDGE)
        {
            UINT uData = _OS_INL(_gPortBase + __GPT_GPTEC) | (UINT)eCaptN;
            _OS_OUTL(_gPortBase + __GPT_GPTEC, uData);
        }
        else if(uAttr & __GPTC_RAISE_EDGE)
        {
            UINT uData = _OS_INL(_gPortBase + __GPT_GPTEC) & ~(UINT)eCaptN;
            _OS_OUTL(_gPortBase + __GPT_GPTEC, uData);
        }
        // sync / async
        if(uAttr & __GPTC_SYNC_CAPT)
        {
            UINT uData = _OS_INL(_gPortBase + __GPT_GPTSC) | (UINT)eCaptN;
            _OS_OUTL(_gPortBase + __GPT_GPTSC, uData);
        }
        else if(uAttr & __GPTC_ASYNC_CAPT)
        {
            UINT uData = _OS_INL(_gPortBase + __GPT_GPTSC) & ~(UINT)eCaptN;
            _OS_OUTL(_gPortBase + __GPT_GPTSC, uData);
        }
        break;
    default:
        PDEBUG("INVALID Input parameter\n");
        break;
    }
    return;
}

UINT32  gpt_atom_get_gptc(__GPT_DEV_MASK eCaptN)
{
    switch(eCaptN)
    {
    case __GPT_DEV_CAPT0:
        return _OS_INL(_gPortBase + __GPT_CAPT0);
    case __GPT_DEV_CAPT1:
        return _OS_INL(_gPortBase + __GPT_CAPT1);
    default:
        break;
    }
    return 0;
}


void    gpt_atom_enable_interrupt(__GPT_DEV_MASK eGptDev)
{
     UINT uData = _OS_INL(_gPortBase + __GPT_GPTIE) | (UINT)eGptDev;
     _OS_OUTL(_gPortBase + __GPT_GPTIE, uData);
}


void    gpt_atom_disable_interrupt(__GPT_DEV_MASK eGptDev)
{
     UINT uData = _OS_INL(_gPortBase + __GPT_GPTIE) & ~(UINT)eGptDev;
     _OS_OUTL(_gPortBase + __GPT_GPTIE, uData);
}

void    gpt_atom_mask_interrupt(__GPT_DEV_MASK eGptDev)
{
     UINT uData = _OS_INL(_gPortBase + __GPT_GPTIM) | (UINT)eGptDev;
     _OS_OUTL(_gPortBase + __GPT_GPTIM, uData);
}

void    gpt_atom_unmask_interrupt(__GPT_DEV_MASK eGptDev)
{
     UINT uData = _OS_INL(_gPortBase + __GPT_GPTIM) & ~(UINT)eGptDev;
     _OS_OUTL(_gPortBase + __GPT_GPTIM, uData);
}


UINT32  gpt_atom_get_interrupt_status(void)
{
    return _OS_INL(_gPortBase + __GPT_GPTISS);
}

UINT32  gpt_atom_set_interrupt_status(UINT uSet)
{
     _OS_OUTL(_gPortBase + __GPT_GPTISS, uSet);
    return _OS_INL(_gPortBase + __GPT_GPTISS);
}

UINT32  gpt_atom_reset_interrupt_status(__GPT_DEV_MASK eGptDev)
{
     _OS_OUTL(_gPortBase + __GPT_GPTISC, (UINT)eGptDev);
    return _OS_INL(_gPortBase + __GPT_GPTISS);
}
