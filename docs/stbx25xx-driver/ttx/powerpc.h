/*----------------------------------------------------------------------------+
|     This source code has been made available to you by IBM on an AS-IS
|     basis.  Anyone receiving this source is licensed under IBM
|     copyrights to use it in any way he or she deems fit, including
|     copying it, modifying it, compiling it, and redistributing it either
|     with or without modifications.  No license under IBM patents or
|     patent applications is to be implied by the copyright license.
|
|     Any user of this software should understand that IBM cannot provide
|     technical support for this software and will not be responsible for
|     any consequences resulting from the use of this software.
|
|     Any person who transfers this source code or any derivative work
|     must include the IBM copyright notice, this paragraph, and the
|     preceding two paragraphs in the transferred software.
|
|       IBM CONFIDENTIAL
|       STB025XX VXWORKS EVALUATION KIT SOFTWARE
|       (C) COPYRIGHT IBM CORPORATION 2003
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author    :  Tony J. Cerreto
| Component :  TTX
| File      :  powerpc.h
| Purpose   :  Teletext Driver include file
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 22-Sep-03  TJC  Created
+----------------------------------------------------------------------------*/
#ifndef _POWERPC_H_
#define _POWERPC_H_

#include <os/os-io.h>
#include "../vid/vid_atom_hw.h"
#include "../vid/vid_atom_denc.h"
#include "../demux/xp_atom_dcr.h"

/*----------------------------------------------------------------------------+
| Defines to convert VxWorks PowerPC Functions to Linux
+----------------------------------------------------------------------------*/
#define powerpcMtdenc0_cr1(_A)     MT_DCR(DENC0_CR1,     _A)
#define powerpcMtdenc0_tesr(_A)    MT_DCR(DENC0_TESR,    _A)
#define powerpcMtdenc0_tosr(_A)    MT_DCR(DENC0_TOSR,    _A)
#define powerpcMtdenc0_trr(_A)     MT_DCR(DENC0_TRR,     _A)
#define powerpcMtvid0_dispd(_A)    MT_DCR(VID_DISP_DLY,  _A)
#define powerpcMtvid0_memcntl(_A)  MT_DCR(VID_DRAM_CMD,  _A)
#define powerpcMtvid0_vbcntl(_A)   MT_DCR(VID_VBI_CNTL,  _A)
#define powerpcMtvid0_ttxcntl(_A)  MT_DCR(VID_TTX_CNTL,  _A)
#define powerpcMtxpt0_lr(_A)       MT_DCR(XP_DCR_ADDR_LR,_A)

#define powerpcMfdenc0_cr1()       MF_DCR(DENC0_CR1)
#define powerpcMfdenc0_trr()       MF_DCR(DENC0_TRR)
#define powerpcMfvid0_01base()     MF_DCR(VID_VBI_BASE)
#define powerpcMfvid0_dispd()      MF_DCR(VID_DISP_DLY)
#define powerpcMfvid0_dispm()      MF_DCR(VID_DISP_MODE)
#define powerpcMfvid0_memcntl()    MF_DCR(VID_DRAM_CMD)
#define powerpcMfvid0_mseg0()      MF_DCR(VID_SEG0)
#define powerpcMfvid0_mseg1()      MF_DCR(VID_SEG1)
#define powerpcMfvid0_mseg2()      MF_DCR(VID_SEG2)
#define powerpcMfvid0_mseg3()      MF_DCR(VID_SEG3)
#define powerpcMfvid0_vbcntl()     MF_DCR(VID_VBI_CNTL)
#define powerpcMfxpt0_dt()         MF_DCR(XP_DCR_ADDR_DT)

/*----------------------------------------------------------------------------+
| Type Definitions
+----------------------------------------------------------------------------*/
typedef unsigned long kernel_state;

/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/
unsigned long beginCriticalSection(void);
void endCriticalSection(unsigned long);
unsigned long xp0_dcr_read(unsigned long address);

#endif
