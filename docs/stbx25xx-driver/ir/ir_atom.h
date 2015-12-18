//vulcan/drv/ircombo/ir_atom.h
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

#ifndef __IR_ATOM_H__
#define __IR_ATOM_H__

#define _IR_INT_LINE          10              // irq 10 is used for IRR in vulcan

#define _IR_IRR0_INT_FIFOS	0x80000000
#define _IR_INT_FIFO_LEVEL      0x40000000
#define _IR_INT_FIFO_TIMEOUT    0x20000000
#define _IR_INT_FIFO_OVERFLOW   0x10000000
#define _IR_IRR0_CNTL_ENIR	0x08000000
#define _IR_INT_STAT_MASK       (_IR_INT_FIFO_OVERFLOW | _IR_INT_FIFO_TIMEOUT |_IR_INT_FIFO_LEVEL)

#define _IR_MAX_FIFO_LEVEL   31

#define _IR_SAMPLE_BASE_FREQ    (63*1000*1000)

#define _IR_MAX_SAMPLE_CLOCK    _IR_SAMPLE_BASE_FREQ                 // 63 MHz

#define _IR_MIN_SAMPLE_CLOCK    ((_IR_SAMPLE_BASE_FREQ+255)/256)     // max 256 divisor

#define _IR_MAX_SAMPLE_LENGTH   65535                                // 65535 clocks

// port = 0, 1,  positive_polarity = 0, 1
void ir_atom_init(UINT port, UINT positive_polarity,UINT32 sample_clock); 

// return the actual sample frequency been set
UINT32 ir_atom_set_sampling_clock(UINT32 uFreqHz);

// set time out
INT ir_atom_set_timeout(UINT32 uClocks);

//  enable ir 
void ir_atom_enable_ir(void);
void ir_atom_disable_ir(void);
  
// return current enabled int flags
UINT32 ir_atom_enable_int(UINT32 uFlags);
UINT32 ir_atom_disable_int(UINT32 uFlags);

// return current int req status
UINT32 ir_atom_get_int_status(void);

// clear correspondent int req status
UINT32 ir_atom_clear_int_status(UINT32 uFlags);

// set the fifo trigger level
INT ir_atom_set_fifo_trigger_level(UINT32 uFifoLevel);

// get current in fifo count
UINT ir_atom_get_fifo_level(void);

// fetch codes out of fifo
// return number of codes get
//UINT ir_atom_get_codes(UINT32 *pFullWidth,  UINT32 *pPulseWidth,  UINT uMaxCodes);
UINT ir_atom_get_codes(UINT32 *pPulse,  UINT uMaxCodes);

// restart ir receiver by stop then start 
void ir_atom_restart(void);


#endif //__IR_ATOM_H__
