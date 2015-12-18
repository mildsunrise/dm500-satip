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
| Author: Ling shao
| File:   vid_atom_denc.h
| Purpose: video decoder atom layer for PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#ifndef PALLAS_DENC_ATOM_LOCAL_H
#define PALLAS_DENC_ATOM_LOCAL_H

#include "os/os-types.h"
#include <vid/vid_types.h>

#define DENC0_CR1		0x131
#define DENC0_RR1               0x132
#define DENC0_CR2               0x133
#define DENC0_RR2               0x134 
#define DENC0_RR3               0x135
#define DENC0_RR4               0x136
#define DENC0_RR5               0x137
#define DENC0_CCDR              0x138 
#define DENC0_CCCR              0x139
#define DENC0_TRR               0x13A
#define DENC0_TOSR              0x13B
#define DENC0_TESR              0x13C
#define DENC0_RLSR              0x13D
#define DENC0_VLSR              0x13E
#define DENC0_VSR               0x13F

#define DENC1_CR1		0x2E1
#define DENC1_RR1               0x2E2
#define DENC1_CR2               0x2E3
#define DENC1_RR2               0x2E4 
#define DENC1_RR3               0x2E5
#define DENC1_RR4               0x2E6
#define DENC1_RR5               0x2E7
#define DENC1_CCDR              0x2E8 
#define DENC1_CCCR              0x2E9
#define DENC1_TRR               0x2EA
#define DENC1_TOSR              0x2EB
#define DENC1_TESR              0x2EC
#define DENC1_RLSR              0x2ED
#define DENC1_VLSR              0x2EE
#define DENC1_VSR               0x2EF

#define SECAM1                  0x2F1
#define SECAM2                  0x2F2
#define DENCMUX			0x2F3

#define DENC_OUTFMT_MASK		0x000B0000
#define DENC_OUTFMT_MASK_RGB	0x00000000
#define DENC_OUTFMT_MASK_LBR    0x00040000
#define DENC_OUTFMT_MASK_LC     0x00040000
#define DENC_DAC_DISABLE        0x00001F00
#define DENC_DISPFMT_MASK		0x07000000

#define DENC_DENCMUX_RGB		0x40000000
#define DENC_DENCMUX_LBR		0x40000000
#define DENC_DENCMUX_LC0		0x40000000
#define DENC_DENCMUX_CVBS0		0x20000000
#define DENC_DENCMUX_LC1		0x18000000
#define DENC_DENCMUX_CVBS1		0x18000000
#define DENC_DENCMUX_DENC0_VP	0x02000000
#define DENC_DENCMUX_DENC1_VP	0x01000000

#define DENC_TSTPAT_MASK	    0x000000f0
#define DENC_TSTPAT_CBON	    0x80100020
#define DENC_TSTPAT_CBOFF       0x80100040


INT    denc_init(DENC_MODE mode);
INT    denc_atom_set_outfmt(DENC_ID id, DENC_OUTFMT fmt);
void   denc_atom_on(DENC_ID id);
void   denc_atom_off(DENC_ID id);
INT    denc_atom_set_dispfmt(DENC_ID id, DENC_MODE fmt);
INT    denc_atom_set_colorbar(DENC_ID id, UINT uFlag);

#endif
