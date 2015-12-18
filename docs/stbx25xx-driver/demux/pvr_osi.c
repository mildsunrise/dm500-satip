/*----------------------------------------------------------------------------+
|   This source code has been made available to you by IBM on an AS-IS
|   basis.  Anyone receiving this source is licensed under IBM
|   copyrights to use it in any way he or she deems fit, including
|   copying it, modifying it, compiling it, and redistributing it either
|   with or without modifications.  No license under IBM patents or
|   patent applications is to be implied by the copyright license.
|   Any user of this software should understand that IBM cannot provide
|   technical support for this software and will not be responsible for
|   any consequences resulting from the use of this software.
|
|   Any person who transfers this source code or any derivative work
|   must include the IBM copyright notice, this paragraph, and the
|   preceding two paragraphs in the transferred software.
|
|   COPYRIGHT   I B M   CORPORATION 1998
|   LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
|   Author    :  Lin Guo Hui
|   Component :  PVR
|   File      :  pvr_osi.c
|   Purpose   :  PVR playback OS Independent functions
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  ------------------------------------------------------
|   10-Oct-01  LGH  Created
+----------------------------------------------------------------------------*/
/* The necessary header files */
#if 0
#include <linux/config.h>
#include <linux/version.h>

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif



#define  __NO_VERSION__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#endif

#include "xp_osi_global.h"
#include "xp_atom_reg.h"
#include "pvr_atom_reg.h"
#include "pvr_atom_dcr.h"

XP_CONFIG3_REG config3_save;


int pvr_osi_set_pvr_as_source();

//Config to pvr mode
int pvr_osi_playback_config()
{
    ULONG   reg;
    ULONG   crit;

    crit = os_enter_critical_section();                 //select PVR mode
    reg = MF_DCR(PVR_CONFIG_DCR) | PVR_PLAYBACK_MODE_SEL;
    MT_DCR(PVR_CONFIG_DCR,reg);

    reg = MF_DCR(PVR_INT_MASK_DCR) | PVR_INT_MASK;
    MT_DCR(PVR_INT_MASK_DCR,reg);

    os_leave_critical_section(crit);

    pvr_osi_set_pvr_as_source();            //Change the source of XP0 to PVR

    crit = os_enter_critical_section();
    xp_atom_dcr_write_register(0,XP_CONFIG1_REG_APWMA, 0);
    xp_atom_dcr_write(0,XP_DCR_ADDR_PWM,0xC00);
    os_leave_critical_section(crit);

    return 0;
}

//Change the source of XP0 to PVR input
int pvr_osi_set_pvr_as_source()
{
#ifndef __DRV_FOR_VESTA__       // for vulcan and pallas ...
    ULONG crit;
    XP_CONFIG3_REG     config3;


    crit = os_enter_critical_section();

    *(ULONG *)(void *)&config3 = xp_atom_dcr_read(0, XP_DCR_ADDR_CONFIG3);  //sele pvr as source of XP0

    config3_save = config3;                                         //save the previous value

    config3.insel = XP_CONFIG3_INSEL_PVR;
    xp_atom_dcr_write(0, XP_DCR_ADDR_CONFIG3, *(ULONG*)(void *)&config3);

    os_leave_critical_section(crit);
#endif
    return 0;
}


int pvr_osi_playback_deconfig()
{
#ifndef __DRV_FOR_VESTA__       // for vulcan and pallas ...
    ULONG crit;
    XP_CONFIG3_REG     config3;
    ULONG reg;

    crit = os_enter_critical_section();
    *(ULONG *)(void *)&config3 = xp_atom_dcr_read(0, XP_DCR_ADDR_CONFIG3);
    if(config3.insel != XP_CONFIG3_INSEL_PVR)
        return 0;

    //restore the XP0 sources config
    xp_atom_dcr_write(0, XP_DCR_ADDR_CONFIG3, *(ULONG*)(void *)&config3_save);

    reg = MF_DCR(PVR_CONFIG_DCR) & ~PVR_PLAYBACK_MODE_SEL;
    MT_DCR(PVR_CONFIG_DCR,reg);

    //mask the PVR dma interrupt
    reg = MF_DCR(PVR_INT_MASK_DCR) & ~PVR_INT_MASK;
    MT_DCR(PVR_INT_MASK_DCR, reg);

    xp_atom_dcr_write_register(0,XP_CONFIG1_REG_APWMA, 1);
    os_leave_critical_section(crit);
#endif

    return 0;


}

//Start PVR DMA, mode = 0: Word transfer
//               mode = 1: Line transfer
int pvr_osi_playback_start(PVR_PLAYBACK_MODE mode)
{
    PVR_START_REGP pPvrStart;
    ULONG reg;
    ULONG crit;

    pPvrStart =(PVR_START_REGP)(void *)&reg;

    crit = os_enter_critical_section();
    reg = MF_DCR(PVR_START_DCR);

    pPvrStart->line = mode;
    pPvrStart->start = 1;

    MT_DCR(PVR_START_DCR,reg);
    reg = MF_DCR(PVR_START_DCR);
    os_leave_critical_section(crit);

    return 0;
}

//Stop PVR dma
//em...., i don't if it works well
int pvr_osi_playback_stop(void)
{
    PVR_START_REGP pPvrStart;
    ULONG reg;

    pPvrStart = (PVR_START_REGP)(void *)&reg;

    reg = MF_DCR(PVR_START_DCR);

    pPvrStart->start = 0;

    MT_DCR(PVR_START_DCR,reg);

    return 0;
}

//Set the DMA transfer, including start address and transfer count
// if mode is Word, the start address must be word bundary
//note the count is 16 bit
int pvr_osi_set_dma(ULONG addr,USHORT uwCount)
{

    ULONG crit = os_enter_critical_section();
    MT_DCR(PVR_ADDR_DCR, addr);

    MT_DCR(PVR_COUNT_DCR, uwCount);
    os_leave_critical_section(crit);

    return 0;
}

//Get the status of DMA buffer
int pvr_osi_get_status(PVR_STATUS *pPvrStatus)
{
    ULONG crit = os_enter_critical_section();

    if((MF_DCR(PVR_STAT_DCR) & PVR_STAT_BUF_NOT_EMPTY) == 0)
    {
        pPvrStatus->BufEmpty = 1;
    }

    else
    {
        pPvrStatus->BufEmpty = 0;
    }

    if((MF_DCR(PVR_STAT_DCR) & PVR_STAT_BUF_FULL) == 1)
    {
        pPvrStatus->BufFull = 1;
    }
    else
    {
        pPvrStatus->BufFull = 0;
    }
    os_leave_critical_section(crit);

    return 0;
}

//lingh added for PVR demo
void pvr_osi_flush()
{
  ULONG reg;
  ULONG crit = os_enter_critical_section();
  reg = xp_atom_dcr_read(0, XP_DCR_ADDR_CONFIG2) | 0xC000;
  xp_atom_dcr_write(0, XP_DCR_ADDR_CONFIG2, reg);
  os_leave_critical_section(crit);

 }
void pvr_osi_unflush(void)
{
  ULONG reg;
  ULONG crit = os_enter_critical_section();
  reg = xp_atom_dcr_read(0, XP_DCR_ADDR_CONFIG2) & ~0xC000;
  xp_atom_dcr_write(0, XP_DCR_ADDR_CONFIG2, reg);

  os_leave_critical_section(crit);

 }
//lingh added for pvr demo

void pvr_osi_set_vid(ULONG reg1)
{
  xp_atom_dcr_write(0,0x11f,reg1);
}
