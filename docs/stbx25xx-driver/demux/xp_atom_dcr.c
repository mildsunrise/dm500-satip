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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|   Author    :  Ian Govett
|   Component :  xp
|   File      :  xp_dcr.c
|   Purpose   :  DCR access to Transport Registers
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  --------------------------------------------------------
|   15-Jan-98  IG   Created
|   04-May-01  TJC  Updated for Pallas
|   30-Sep-01  LGH  change the file name, ported to linux, Combine codes for
|                   three Devices
|   29-Oct-02  LGH  Add negative filter support for Vesta
|
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|                   DCR Access To The Transport Registers
+-----------------------------------------------------------------------------+
|
|   The following functions provide access methods to the DCR registers.
|   The transport hardware uses an indirect addressing scheme for access to
|   it's registers.  Read and write operations are a two step process.  The
|   address is first written to the transport DCR address register, and
|   then the read or write operation is done.
|
|   A critical section is required around the indirect address operation
|   since an interrupt could occur between the writing of an address and
|   the read or write operation.
|
|   The xp_atom_dcr_read_interrupt() routine reads and immediately clears the
|   interrupt vector. This reduces the likelihood of missing any interrupts
|   delivered by the hardware.
|
|   These low level functions are used by the driver to access the
|   transport demux registers.
|
+----------------------------------------------------------------------------*/

/* The necessary header files */

#include <linux/config.h>
#include <linux/version.h>
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif

#define  __NO_VERSION__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <os/os-types.h>
#include <hw/hardware.h>

#include "av_dcr.h"
#include "xp_atom_dcr.h"
#include "xp_atom_reg.h"



/*----------------------------------------------------------------------------+

| XXXX   XX   XX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX

|  XX    XXX  XX   X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX

|  XX    XXXX XX     XX     XX X     XX  XX  XXXX XX   XX  XX   XX

|  XX    XX XXXX     XX     XXXX     XXXXX   XX XXXX   XX  XX   XX

|  XX    XX  XXX     XX     XX X     XX XX   XX  XXX   XXXXXX   XX

|  XX    XX   XX     XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX

| XXXX   XX   XX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX

+----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------+

| XXXXXXX  XXX XXX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX

|  XX   X   XX XX    X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX

|  XX X      XXX       XX     XX X     XX  XX  XXXX XX   XX  XX   XX

|  XXXX       X        XX     XXXX     XXXXX   XX XXXX   XX  XX   XX

|  XX X      XXX       XX     XX X     XX XX   XX  XXX   XXXXXX   XX

|  XX   X   XX XX      XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX

| XXXXXXX  XXX XXX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX

+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+

|   xp0_dcr_init_queue

+----------------------------------------------------------------------------*/

void xp_atom_dcr_init_queue(UINT uDeviceIndex,SHORT wChannelId,ULONG ulQueueTop,ULONG ulQueueBottom)

{

    ULONG    ulAddr;

    ULONG    ulCa;

    ULONG    ulCb;

    XP_QCONFIGA_REG *pCa;

    XP_QCONFIGB_REG *pCb;





    pCa = (XP_QCONFIGA_REG *)(void *) &ulCa;

    pCb = (XP_QCONFIGB_REG *)(void *) &ulCb;



    /*------------------------------------------------------------------------+

    |  Read the current queue values

    +------------------------------------------------------------------------*/

    ulAddr = XP_DCR_ADDR_BASE_QCONFIGAB + (wChannelId * 2);

    ulCa = xp_atom_dcr_read(uDeviceIndex,ulAddr);

    ulCb = xp_atom_dcr_read(uDeviceIndex,ulAddr+1);



    /*------------------------------------------------------------------------+

    |   if the unload type is for streaming data, then use

    |   the BTI interrupt and set the Boundary Threshold (queueInt)

    |   to interrupt every 512 bytes.

    +------------------------------------------------------------------------*/

    pCa->enda    = ulQueueTop;

    pCa->starta  = ulQueueBottom;

    pCb->rptr    = ulQueueTop << 4;

    pCb->apus    = 1;

    pCb->enbl    = 0;



    /*------------------------------------------------------------------------+

    |  If the unload type is for streaming data, then use

    |  the BTI interrupt and set the Boundary Threshold (queueInt)

    |  to interrupt every 512 bytes.

    +------------------------------------------------------------------------*/

    if(pCb->dtype < 8) {

        pCa->bthres = 2;

        pCb->scpc   = 0;

    } else {

        pCa->bthres = 0;

        pCb->scpc   = 1;

    }



    xp_atom_dcr_write(uDeviceIndex,ulAddr, ulCa);

    xp_atom_dcr_write(uDeviceIndex,ulAddr+1, ulCb);

}



/*----------------------------------------------------------------------------+

|   xp_atom_dcr_read_interrupt

+-----------------------------------------------------------------------------+

|

|   DESCRIPTION:  read & clear the transport demux interrupt

|

|   PROTOTYPE  :  unsigned long xp_atom_dcr_read_interrupt(void)

|

|   ARGUMENTS  :

|

|   RETURNS    :  value of the transport demux interrupt vector

|

|   ERRORS     :

|

|   COMMENTS   :  This function reads the current interrupt vector, and

|                 immediately clears the interrupt.  This reduces the

|                 likelihood of missing an interrupt while the handler is

|                 executing. A critical section is used to read and clear

|                 the interrupt vector.  The interrupt read is returned

|                 to the caller.  The caller is responsible for handling

|                 all interrupts returned.

|

+----------------------------------------------------------------------------*/

//When using this atom function, you must use is as critical section in the upper layer

ULONG xp_atom_dcr_read_interrupt(UINT uDeviceIndex)   /* read & clear interrupt           */

{

    ULONG ulData = 0;



    switch (uDeviceIndex)

    {

    case 0:

        ulData = MF_DCR(XPT0_IR);

        MT_DCR(XPT0_IR,ulData);

        break;

#ifdef __DRV_FOR_PALLAS__

    case 1:

        ulData = MF_DCR(XPT1_IR);

        MT_DCR(XPT1_IR,ulData);

        break;

    case 2:

        ulData = MF_DCR(XPT2_IR);

        MT_DCR(XPT2_IR,ulData);

        break;
#endif

    default:

        break;

    }

    return(ulData);

}



/*----------------------------------------------------------------------------+

|   xp_atom_dcr_read_queue_config

+----------------------------------------------------------------------------*/

void xp_atom_dcr_read_queue_config(UINT uDeviceIndex,SHORT wChannelId,ULONG *ppQueueTop,ULONG *ppQueueBottom)

{

    ULONG    ulAddr;

    ULONG    ulCa;

    XP_QCONFIGA_REG *pCa;



    ulAddr = XP_DCR_ADDR_BASE_QCONFIGAB + (wChannelId * 2);

    pCa  = (XP_QCONFIGA_REG *)(void *) &ulCa;

    ulCa   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

    *ppQueueTop    = pCa->enda;

    *ppQueueBottom = pCa->starta;

}



/*----------------------------------------------------------------------------+

|   xp0_dcr_reset_queue

+----------------------------------------------------------------------------*/

void xp_atom_dcr_reset_queue(UINT uDeviceIndex,SHORT wChannelId)

{

    ULONG ulAddr = XP_DCR_ADDR_BASE_QCONFIGAB + (wChannelId * 2);



    xp_atom_dcr_write(uDeviceIndex,ulAddr, 0);

    xp_atom_dcr_write(uDeviceIndex,ulAddr+1, 0);

}



/*----------------------------------------------------------------------------+

|   xp_atom_dcr_write_dram_filter

+----------------------------------------------------------------------------*/

void xp_atom_dcr_write_dram_filter(UINT uDeviceIndex,SHORT wFilterId,

                                   ULONG ulData,ULONG ulMask,ULONG ulControl, ULONG ulPolarity)

{

    ULONG    ulAddr;



    ulAddr = XP_DCR_ADDR_BASE_FILTER + (wFilterId * 4);



    xp_atom_dcr_write(uDeviceIndex,ulAddr, ulControl);

    ulAddr++;

    xp_atom_dcr_write(uDeviceIndex,ulAddr, ulData);

    ulAddr++;

    xp_atom_dcr_write(uDeviceIndex,ulAddr, ulMask);

#ifndef __DRV_FOR_VESTA__       // for vulcan and pallas ...
    ulAddr++;

    xp_atom_dcr_write(uDeviceIndex,ulAddr, ulPolarity);
#else
    {
        ULONG value;
        ulAddr = XP_DCR_ADDR_BASE_FILTER + (wFilterId * 4);
        value = xp_atom_dcr_read(uDeviceIndex,ulAddr);
        if((ulPolarity & 0xff) != 0xff)
            value |= 0x00010000;
        if((ulPolarity & 0xff00) != 0xff00)
            value |= 0x00020000;
        if((ulPolarity & 0xff0000) != 0xff0000)
            value |= 0x00040000;
        if((ulPolarity & 0xff000000) != 0xff000000)
            value |= 0x00080000;

        xp_atom_dcr_write(uDeviceIndex,ulAddr,value);
    }

#endif

}



/*----------------------------------------------------------------------------+

|   xp_atom_dcr_write_filter_link

+----------------------------------------------------------------------------*/

SHORT xp_atom_dcr_write_filter_link(UINT uDeviceIndex,SHORT wFilterId,

                                    ULONG ulNextFilterId,ULONG ulEndOfColumn)

{

    ULONG    ulAddr;

    ULONG    ulValue;

    XP_FILTER_CONTROL_REGP pFilter;



    ulAddr         = XP_DCR_ADDR_BASE_FILTER + (wFilterId * 4);

    ulValue          = xp_atom_dcr_read(uDeviceIndex,ulAddr);

    pFilter        = (XP_FILTER_CONTROL_REGP)(void *) &ulValue;

    pFilter->nfilt = ulNextFilterId;

    pFilter->ncol  = ulEndOfColumn;



    xp_atom_dcr_write(uDeviceIndex,ulAddr, ulValue);



    return(0);

}



/*----------------------------------------------------------------------------+

|   xp_atom_dcr_write_register

+----------------------------------------------------------------------------*/

SHORT xp_atom_dcr_write_register(UINT uDeviceIndex,XP_DCR_REGISTER_TYPE regtype,ULONG ulData)

{

    ULONG    ulValue;

    XP_BUCKET1Q_REGP pBucket;

    XP_CONFIG1_REGP  pConfig1;



    /*------------------------------------------------------------------------+

    |  Read the value from hardware

    +------------------------------------------------------------------------*/

    switch(regtype) {

        case XP_BUCKET1Q_REG_BVALID:

            ulValue           = xp_atom_dcr_read(uDeviceIndex,XP_DCR_ADDR_BUCKET1Q);

            pBucket         = (XP_BUCKET1Q_REGP)(void *) &ulValue;

            pBucket->bvalid = ulData;

            xp_atom_dcr_write(uDeviceIndex,XP_DCR_ADDR_BUCKET1Q, ulValue);

            break;



        case XP_BUCKET1Q_REG_INDX:

            ulValue           = xp_atom_dcr_read(uDeviceIndex,XP_DCR_ADDR_BUCKET1Q);

            pBucket         = (XP_BUCKET1Q_REGP)(void *) &ulValue;

            pBucket->indx   = ulData;

            xp_atom_dcr_write(uDeviceIndex,XP_DCR_ADDR_BUCKET1Q, ulValue);

            break;



            if(uDeviceIndex == 0)

            {

        case XP_CONFIG1_REG_APWMA:

            ulValue         = xp_atom_dcr_read(uDeviceIndex,XP_DCR_ADDR_CONFIG1);

            pConfig1        = (XP_CONFIG1_REGP)(void *) &ulValue;

            pConfig1->apwma = ulData;

            xp_atom_dcr_write(uDeviceIndex,XP_DCR_ADDR_CONFIG1, ulValue);

            break;

            }



        default:

            return(-1);

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|   XX     XXXXXX    XXXXXX    XXXXX

|  XXXX    XX   XX     XX     XX   XX

| XX  XX   XX   XX     XX      XX

| XX  XX   XXXXX       XX        XX

| XXXXXX   XX          XX         XX

| XX  XX   XX          XX     XX   XX

| XX  XX   XX        XXXXXX    XXXXX

+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+

|   xp_atom_dcr_read

+-----------------------------------------------------------------------------+

|

|   DESCRIPTION:  Read a transport demux DCR register

|

|   PROTOTYPE  :  unsigned long xp_atom_dcr_read(

|                 unsigned long address)

|

|   ARGUMENTS  :  address      -  demux DCR address to read

|

|   RETURNS    :  contents of the DCR location

|

|   ERRORS     :

|

|   COMMENTS   :  The transport demux uses indirect DCR addressing.  An DCR

|                 register (0x180) is first written with the demux index,

|                 and then a read operation is performed and loaded into

|                 the 'data' register A critical section must surround this

|                 indirect addressing to eliminate any interrupts which

|                 could affect the operation.

|

+----------------------------------------------------------------------------*/

ULONG xp_atom_dcr_read(UINT uDeviceIndex,ULONG ulAddress)

{

    ULONG ulData = 0;



    switch (uDeviceIndex)

    {

    case 0:

        MT_DCR(XPT0_LR,ulAddress);

        ulData = MF_DCR(XPT0_DT);

        break;
#ifdef __DRV_FOR_PALLAS__

    case 1:

        MT_DCR(XPT1_LR,ulAddress);

        ulData = MF_DCR(XPT1_DT);

        break;

    case 2:

        MT_DCR(XPT2_LR,ulAddress);

        ulData = MF_DCR(XPT2_DT);

        break;
#endif

    default:

        break;

    }



    return(ulData);

}



/*----------------------------------------------------------------------------+

|   xp_atom_dcr_read_register_channel

+----------------------------------------------------------------------------*/

short xp_atom_dcr_read_register_channel(UINT uDeviceIndex,XP_DCR_REGISTER_TYPE regtype,

                                        SHORT wId,ULONG *pData)

{

    ULONG                  ulAddr;

    ULONG                  ulValue;

    XP_PID_FILTER_REGP     pPid;

    XP_QCONFIGB_REGP       pCb;

    XP_FILTER_CONTROL_REGP pFilter;

    XP_QSTATB_REGP         pQb;

    XP_QSTATD_REGP         pQd;



    /*------------------------------------------------------------------------+

    |  Read the value from hardware

    +------------------------------------------------------------------------*/

    switch(regtype) {

        case XP_PID_FILTER_REG_PIDV:

            ulAddr    = XP_DCR_ADDR_BASE_PID + wId;

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pPid    = (XP_PID_FILTER_REGP)(void *) &ulValue;

            *pData = pPid->pidv;

            break;



        case XP_QCONFIGB_REG_DTYPE:

            ulAddr    = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2) + 1;

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCb     = (XP_QCONFIGB_REGP)(void *) &ulValue;

            *pData = pCb->dtype;

            break;



        case XP_QCONFIGB_REG_ENBL:

            ulAddr    = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2) + 1;

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCb     = (XP_QCONFIGB_REGP)(void *) &ulValue;

            *pData = pCb->enbl;

            break;



        case XP_QCONFIGB_REG_RPTR:

            ulAddr    = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2) + 1;

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCb     = (XP_QCONFIGB_REGP)(void *) &ulValue;



            /*----------------------------------------------------------------+

            |  Convert from a 256 byte block

            +----------------------------------------------------------------*/

            *pData = pCb->rptr << 8;

            break;



        case XP_FILTER_CONTROL_REG_ENBL:

            ulAddr    = XP_DCR_ADDR_BASE_FILTER + (wId * 4);

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pFilter = (XP_FILTER_CONTROL_REGP)(void *) &ulValue;

            *pData = pFilter->enbl;

            break;



        case XP_FILTER_CONTROL_REG_NFILT:

            ulAddr    = XP_DCR_ADDR_BASE_FILTER + (wId * 4);

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pFilter = (XP_FILTER_CONTROL_REGP)(void *) &ulValue;

            *pData = pFilter->nfilt;

            break;



        case XP_QCONFIGB_REG_FSF:

            ulAddr    = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2) + 1;

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCb     = (XP_QCONFIGB_REGP)(void *) &ulValue;

            *pData = pCb->fsf;

            break;



        case XP_QSTATB_REG_WPTR:

            ulAddr    = XP_DCR_ADDR_BASE_QSTATABCD + (wId * 4) + 1;

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pQb     = (XP_QSTATB_REG *)(void *) &ulValue;

            *pData = pQb->wptr;

            break;



        case XP_QSTATD_REG_WSTART:

            ulAddr    = XP_DCR_ADDR_BASE_QSTATABCD + (wId * 4) + 3;

            ulValue   = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pQd     = (XP_QSTATD_REG *)(void *) &ulValue;

            *pData = pQd->wstart;

            break;



        default:

            return(-1);

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|   xp_atom_dcr_write

+-----------------------------------------------------------------------------+

|

|   DESCRIPTION:  write to a demux DCR register

|

|   PROTOTYPE  :  void xp_atom_dcr_write(

|                 unsigned long address,

|                 unsigned long data)

|

|   ARGUMENTS  :  address      -  demux DCR address to write

|                 data         -  data to write to the 'address'

|

|   RETURNS    :

|

|   ERRORS     :

|

|   COMMENTS   :  The transport demux uses indirect DCR addressing.  An DCR

|                 register (0x180) is first written with the demux index,

|                 and then a write operation is performed using the 'data'

|                 argument.  A critical section must surround this indirect

|                 addressing to eliminate any interrupts which could affect

|                 the operation.

|

+----------------------------------------------------------------------------*/

void xp_atom_dcr_write(UINT uDeviceIndex,ULONG ulAddress,ULONG ulData)

{



    switch (uDeviceIndex)

    {

    case 0:

        MT_DCR(XPT0_LR,ulAddress);

        MT_DCR(XPT0_DT,ulData);

        break;

#ifdef __DRV_FOR_PALLAS__

    case 1:

        MT_DCR(XPT1_LR,ulAddress);

        MT_DCR(XPT1_DT,ulData);

        break;

    case 2:

        MT_DCR(XPT2_LR,ulAddress);

        MT_DCR(XPT2_DT,ulData);

        break;
#endif

    default:

        break;

    }



 }



/*----------------------------------------------------------------------------+

|   xp_atom_dcr_write_register_channel

+----------------------------------------------------------------------------*/

SHORT xp_atom_dcr_write_register_channel(UINT uDeviceIndex,XP_DCR_REGISTER_TYPE regtype,

                                         SHORT wId,ULONG ulData)

{

    ULONG ulAddr;

    ULONG ulValue;

    XP_PID_FILTER_REGP     pPid;

    XP_QCONFIGA_REGP       pCa;

    XP_QCONFIGB_REGP       pCb;

    XP_QSTATB_REGP         pQb;

    XP_QSTATD_REGP         pQd;

    XP_FILTER_CONTROL_REGP pFilter;



    /*------------------------------------------------------------------------+

    |  Read the ulValue from hardware

    +------------------------------------------------------------------------*/

    switch(regtype) {

        case XP_PID_FILTER_REG_PIDV:

            ulAddr        = XP_DCR_ADDR_BASE_PID + wId;

            ulValue       = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pPid        = (XP_PID_FILTER_REGP)(void *) &ulValue;

            pPid->pidv  = ulData;

            break;



        case XP_QCONFIGB_REG_DTYPE:

            ulAddr        = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2) + 1;

            ulValue       = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCb         = (XP_QCONFIGB_REGP)(void *) &ulValue;

            pCb->dtype  = ulData;



            /*----------------------------------------------------------------+

            |  If the unload type is for streaming data, then use

            |  the BTI interrupt and set the Boundary Threshold (queueInt)

            |  to interrupt every 512 bytes.

            +----------------------------------------------------------------*/

            if(ulData < 8) {

                pCb->scpc = 0;

                xp_atom_dcr_write_register_channel(uDeviceIndex,XP_QCONFIGA_REG_BTHRES, wId, 2);

            }

            else {

                pCb->scpc = 1;

                xp_atom_dcr_write_register_channel(uDeviceIndex,XP_QCONFIGA_REG_BTHRES, wId, 0);

            }

            break;



        case XP_QCONFIGB_REG_ENBL:

            ulAddr        = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2) + 1;

            ulValue       = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCb         = (XP_QCONFIGB_REGP)(void *) &ulValue;

            pCb->enbl   = ulData;

            break;



        case XP_QCONFIGA_REG_BTHRES:

            ulAddr        = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2);

            ulValue       = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCa         = (XP_QCONFIGA_REGP)(void *) &ulValue;

            pCa->bthres = ulData;

            break;



        case XP_QCONFIGB_REG_RPTR:

            ulAddr        = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2) + 1;

            ulValue       = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCb         = (XP_QCONFIGB_REGP)(void *) &ulValue;

            /*----------------------------------------------------------------+

            |  Convert to a 256 byte block

            +----------------------------------------------------------------*/

            pCb->rptr   = ulData >> 8;

            break;



        case XP_FILTER_CONTROL_REG_ENBL:

            ulAddr           = XP_DCR_ADDR_BASE_FILTER + (wId * 4);

            ulValue          = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pFilter        = (XP_FILTER_CONTROL_REGP)(void *) &ulValue;

            pFilter->enbl  = ulData;

            break;



        case XP_FILTER_CONTROL_REG_NFILT:

            ulAddr           = XP_DCR_ADDR_BASE_FILTER + (wId * 4);

            ulValue          = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pFilter        = (XP_FILTER_CONTROL_REGP)(void *) &ulValue;

            pFilter->nfilt = ulData;

            break;



        case XP_FILTER_CONTROL_REG_SFID:

            ulAddr           = XP_DCR_ADDR_BASE_FILTER + (wId * 4);

            ulValue          = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pFilter        = (XP_FILTER_CONTROL_REGP)(void *) &ulValue;

            pFilter->sfid  = ulData;

            break;



        case XP_QCONFIGB_REG_FSF:

            ulAddr           = XP_DCR_ADDR_BASE_QCONFIGAB + (wId * 2) + 1;

            ulValue          = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pCb            = (XP_QCONFIGB_REGP)(void *) &ulValue;

            pCb->fsf       = ulData;

            break;



        case XP_QSTATB_REG_WPTR:

            ulAddr           = XP_DCR_ADDR_BASE_QSTATABCD + (wId * 4) + 1;

            ulValue          = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pQb            = (XP_QSTATB_REG *)(void *) &ulValue;

            pQb->wptr      = ulData;

            break;



        case XP_QSTATD_REG_WSTART:

            ulAddr           = XP_DCR_ADDR_BASE_QSTATABCD + (wId * 4) + 3;

            ulValue          = xp_atom_dcr_read(uDeviceIndex,ulAddr);

            pQd            = (XP_QSTATD_REG *)(void *) &ulValue;

            pQd->wstart    = ulData;

            break;



        default:

            return(-1);

    }



    xp_atom_dcr_write(uDeviceIndex,ulAddr, ulValue);



    return(0);

}

void xp_atom_disable_aud_sync()
{
    ULONG reg;

    reg = MF_DCR(a_ctrl0) & (~DECOD_AUD_CTRL0_ENABLE_SYNC);
    MT_DCR(a_ctrl0,reg);
}

void xp_atom_disable_vid_sync()
{
    ULONG reg;

    reg = MF_DCR(v_c_cntl) |DECOD_CHIP_CONTROL_DIS_SYNC;
    MT_DCR(v_c_cntl,reg);
}

int xp_atom_a_hw_cc_inprogress()
{
    if((MF_DCR(a_dsr) & DECOD_AUD_DSR_CHAN_CH) != 0)
    {
        return 1;
    }
    return 0;
}

void xp_atom_a_hw_sync_off()
{
    ULONG reg;

    reg = MF_DCR(a_ctrl0) & (~DECOD_AUD_CTRL0_ENABLE_SYNC);
    MT_DCR(a_ctrl0,reg);
}

void xp_atom_a_hw_sync_on()
{
    ULONG reg;

    reg = MF_DCR(a_ctrl0) | DECOD_AUD_CTRL0_ENABLE_SYNC;
    MT_DCR(a_ctrl0,reg);
}


void xp_atom_v_hw_sync_off()
{
    ULONG reg;

    reg = MF_DCR(v_c_cntl) |DECOD_CHIP_CONTROL_DIS_SYNC;
    MT_DCR(v_c_cntl,reg);
}

void xp_atom_v_hw_sync_on()
{
    ULONG reg;

    reg = MF_DCR(v_c_cntl) & (~DECOD_CHIP_CONTROL_DIS_SYNC);
    MT_DCR(v_c_cntl,reg);
}


int xp_atom_a_hw_write_stc(stc_t *data)
{
    unsigned long value;
    if((MF_DCR(a_dsr) & DECOD_AUD_DSR_COMMAND_COM) != 0)
        return -1;

    value = (data->bit_0|((data->bits_32_1<<1)&0x0000fffe));
    MT_DCR(a_stc,value);
    value = ((data->bits_32_1>>15) & (0x0000ffff));
    MT_DCR(a_stc,value);
    value = ((data->bits_32_1>>31) & (0x000000001));
    MT_DCR(a_stc,value);
    return 0;
}

int xp_atom_v_hw_write_stc(stc_t *data)
{
    unsigned long value;

    value = data->bit_0<<9;
    MT_DCR(v_s_stc1,value);
    value = data->bits_32_1;
    MT_DCR(v_s_stc0,value);

    return 0;
}
