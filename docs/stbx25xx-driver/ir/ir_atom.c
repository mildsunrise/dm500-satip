//vulcan/drv/ircombo/ir_atom.c
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
//  IR driver for STBx25xx
//Revision Log:   
//  Jun/03/2002                                               Created by YYD

//#define __IR_ATOM_DEBUG
 
#include "os/os-types.h"

#include "os/os-io.h"

#include "ir_atom.h"

#ifdef __IR_ATOM_DEBUG
#define __DRV_DEBUG
#endif
#include "os/drv_debug.h"


// defines the IRR0 DCRs
#define _DCR_IRR0_CNTL      0x300
#define _DCR_IRR0_INT       0x301
#define _DCR_IRR0_FIFO      0x302
#define _DCR_IRR0_DIV       0x303


void ir_atom_init(UINT port, UINT positive_polarity, UINT32 sample_clock)
{
    	MT_DCR(_DCR_IRR0_CNTL, 0);
    	ir_atom_set_sampling_clock(sample_clock);
    	MT_DCR(_DCR_IRR0_CNTL, (  (port ? 0x04000000 : 0) 
                            | (positive_polarity ? 0 : 0x01000000)
                            | 0x0a01ffff));     
    	// default: enable ir, fifoil = 1,  tov  = 65535 (max), no interrupts
}

void ir_atom_enable_ir(void)
{
    	UINT32  uOld;
    
    	uOld = MF_DCR(_DCR_IRR0_CNTL);
    	MT_DCR(_DCR_IRR0_CNTL,uOld|_IR_IRR0_CNTL_ENIR);
}

void ir_atom_disable_ir(void)
{
    UINT32  uOld;
    
    uOld = MF_DCR(_DCR_IRR0_CNTL);
    uOld &= (~_IR_IRR0_CNTL_ENIR);
    MT_DCR(_DCR_IRR0_CNTL,uOld);
}

// return the actual sample frequency been set
UINT32 ir_atom_set_sampling_clock(UINT32 uFreqHz)
{
    UINT32  uDiv;
    UINT32  uOld;
    if(uFreqHz < _IR_MIN_SAMPLE_CLOCK || uFreqHz > _IR_MAX_SAMPLE_CLOCK) return 0;
    uDiv = (_IR_SAMPLE_BASE_FREQ + uFreqHz/2) / uFreqHz;

    uOld = MF_DCR(_DCR_IRR0_DIV) & 0x00ffffff;
    MT_DCR(_DCR_IRR0_DIV, (uOld | (uDiv << 24)));

    return (_IR_SAMPLE_BASE_FREQ + uDiv-1) / uDiv;
}

INT ir_atom_set_timeout(UINT32 uClocks)
{
    UINT32  uOld;
    if(uClocks > _IR_MAX_SAMPLE_LENGTH) uClocks = _IR_MAX_SAMPLE_LENGTH;

    uOld = MF_DCR(_DCR_IRR0_CNTL) & 0xffff0000;
    MT_DCR(_DCR_IRR0_CNTL, (uOld | uClocks));

    return 0;
}



UINT32 ir_atom_enable_int(UINT32 uFlags)
{
    UINT32  uOld;

    uFlags &= _IR_INT_STAT_MASK;

    uOld = MF_DCR(_DCR_IRR0_CNTL);
    MT_DCR(_DCR_IRR0_CNTL, (uOld | uFlags));

    return MF_DCR(_DCR_IRR0_CNTL) & _IR_INT_STAT_MASK;
}

UINT32 ir_atom_disable_int(UINT32 uFlags)
{
    UINT32  uOld;

    uFlags &= _IR_INT_STAT_MASK;

    uOld = MF_DCR(_DCR_IRR0_CNTL) & (~uFlags);
    MT_DCR(_DCR_IRR0_CNTL, uOld);

    return MF_DCR(_DCR_IRR0_CNTL) & _IR_INT_STAT_MASK;
}

UINT32 ir_atom_get_int_status(void)
{
    return MF_DCR(_DCR_IRR0_INT) & _IR_INT_STAT_MASK;
}

UINT32 ir_atom_clear_int_status(UINT32 uFlags)
{
    UINT32  uOld;

    uFlags &= _IR_INT_STAT_MASK;
    uOld = MF_DCR(_DCR_IRR0_INT) &  (~_IR_INT_STAT_MASK);
    MT_DCR(_DCR_IRR0_INT, uOld | uFlags);
    return MF_DCR(_DCR_IRR0_INT) & _IR_INT_STAT_MASK;
}


INT ir_atom_set_fifo_trigger_level(UINT32 uFifoLevel)
{
    UINT32  uOld;

    if(uFifoLevel > _IR_MAX_FIFO_LEVEL || uFifoLevel < 1) return -1;
    
    uOld = MF_DCR(_DCR_IRR0_CNTL) & 0xffe0ffff;

    MT_DCR(_DCR_IRR0_CNTL, uOld | (uFifoLevel << 16));

    return 0;
}


UINT ir_atom_get_fifo_level(void)
{
    return (MF_DCR(_DCR_IRR0_INT)>> 20 ) & 0x1f;
}

//UINT ir_atom_get_codes(UINT32 *pFullWidth,  UINT32 *pPulseWidth,  UINT uMaxCodes)
UINT ir_atom_get_codes(UINT32 *pPulse,  UINT uMaxCodes)
{
    UINT i;
    if(uMaxCodes < 1) return 0;

    for(i=0; i<uMaxCodes; i++)
    {
        if(((MF_DCR(_DCR_IRR0_INT)>> 20 ) & 0x1f) > 0)
        {
            pPulse[i]= MF_DCR(_DCR_IRR0_FIFO);
//            pFullWidth[i] = uVal>>16;
//          pPulseWidth[i] = uVal&0xffff;
        }
        else
            break;
    }
	PDEBUG("code num %d \n",i);
    return i;
}

void ir_atom_restart(void)
{
    UINT32 uOld;

    uOld = MF_DCR(_DCR_IRR0_CNTL) & 0xf7ffffff;

    MT_DCR(_DCR_IRR0_CNTL, uOld);

    MT_DCR(_DCR_IRR0_CNTL, uOld | 0x08000000);

    return;
}

