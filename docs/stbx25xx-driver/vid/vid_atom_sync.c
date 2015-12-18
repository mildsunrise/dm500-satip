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
| File:   vid_sync.c
| Purpose: video decoder atom layer synchroniztion function PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01             create                                                                                          SL
+----------------------------------------------------------------------------*/
#include <os/os-io.h>
#include <os/drv_debug.h>
#include "vid_atom_hw.h"
#include "vid_atom_local.h"

void vid_atom_sync_on()
{
    MT_DCR(VID_CHIP_CTRL, MF_DCR(VID_CHIP_CTRL) & (~DECOD_CHIP_CONTROL_DIS_SYNC));
}


void vid_atom_set_pts(STC_T *pData)
{
    int sync;
    sync = vid_atom_disable_sync();

    MT_DCR(VID_SYNC_PTS1, (pData->bit0 & 0x01) << 9);
    MT_DCR(VID_SYNC_PTS0, pData->bit32_1);

    if(sync)
        vid_atom_sync_on();
}


void vid_atom_set_stc(STC_T *pData)
{
    int sync;
    sync = vid_atom_disable_sync();

    MT_DCR(VID_SYNC_STC1, (pData->bit0 & 0x01)<< 9);
    MT_DCR(VID_SYNC_STC0, pData->bit32_1);

    if(sync)
        vid_atom_sync_on();
}

void vid_atom_get_stc(STC_T *pData)
{
    int sync;
    sync = vid_atom_disable_sync();

   pData->bit32_1 = MF_DCR(VID_SYNC_STC0);
   pData->bit0 = (MF_DCR(VID_SYNC_STC1) >> 9) & 0x01;

    if(sync)
        vid_atom_sync_on();
}

ULONG vid_atom_get_sync()
{
    unsigned long reg;
    reg = MF_DCR(VID_CHIP_CTRL);
    reg &=  DECOD_CHIP_CONTROL_DIS_SYNC |
            DECOD_CHIP_CONTROL_AUD_MAS |
            DECOD_CHIP_CONTROL_VID_MAS;
    return reg;
}

INT vid_atom_disable_sync()
{
    unsigned long reg;
    int sync;

    reg = MF_DCR(VID_CHIP_CTRL);
    sync = (reg & DECOD_CHIP_CONTROL_DIS_SYNC)?0:1;
    MT_DCR(VID_CHIP_CTRL, reg | DECOD_CHIP_CONTROL_DIS_SYNC);
    //notify demux
    return sync;
}

void vid_atom_enable_sync(unsigned int uVidMaster)
{
    unsigned long reg;

    reg = MF_DCR(VID_CHIP_CTRL) &
        (~(DECOD_CHIP_CONTROL_DIS_SYNC|
           DECOD_CHIP_CONTROL_VID_MAS |
           DECOD_CHIP_CONTROL_AUD_MAS ));

    if(uVidMaster)
        reg |= DECOD_CHIP_CONTROL_VID_MAS;
    else
        reg |= DECOD_CHIP_CONTROL_AUD_MAS;

    MT_DCR(VID_CHIP_CTRL, reg);
}
