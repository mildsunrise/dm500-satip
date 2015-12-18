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
|     COPYRIGHT   I B M   CORPORATION 1998
|     LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|
|   Author    :  Ian Govett
|   Component :  xp
|   File      :  xp0_inter.c
|   Purpose   :  Interrupt Management
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  ---------------------------------------------------------
|   15-Jan-98  IG   Created
|   22-Jun-99  PAG  Only clear unmasked status interrupt bits.
|   30-Sep-01  LGH  Ported to Linux, combined codes of 3 devices
|
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+

|                   Interrupt processing & management

+-----------------------------------------------------------------------------+

|

|   The following functions provide interrupt services for the demux

|   driver.  A large number of interrupts are available to the demux

|   driver.  The hardware provides a set of interrupt registers which are

|   organized in a hierarchical manner, with the primary interrupt register

|   at the top of the hierarchy.  Status registers for audio, video, and

|   channel registers provide the secondary interrupt layer.  See the demux

|   specification for more detailed information.

|

|   The demux driver processes all interrupts signaled by the transport

|   hardware.  The primary interrupt register is read and cleared before

|   executing any callback function which may have been registered.  This

|   is done to minimize the loss of any future interrupts delivered by the

|   hardware.  Information in the primary interrupt register indicates if

|   there are any status, audio, video, or channel interrupts pending.

|   Other primary interrupts such as clock recovery are processed first,

|   followed by the secondary interrupts.

|

|   The interface provided by the demux driver features the registration of

|   callback functions for each type of interrupt in any of the interrupt

|   registers.  Multiple functions may register their callback functions

|   for the same or a different set of interrupts.  Callback functions are

|   called in the order of their registration.

|

|   There are three callback functions which process the primary, status,

|   and channel interrupts.  The registration of callback functions require

|   a bit mask (channel interrupts also require a channel_id). This bit

|   mask indicates the interrupt types being registered for notification.

|   Only one callback is made for each registration no matter how many bits

|   may be set.

|

|   To minimize the number of interrupts received from the hardware, the

|   union of all the interrupt masks registered with callback functions is

|   calculated.  This value is used to set the hardware masks.

|

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

#include <linux/slab.h>

#include <linux/interrupt.h>

#endif

#include "hw/hardware.h"

#include "xp_osi_global.h"

#include "xp_atom_reg.h"



/*----------------------------------------------------------------------------+

| Local Defines

+----------------------------------------------------------------------------*/

#define QSTAT_INTERRUPT_MASK        0x0000FFFF  /* Valid Queue Status bits       */



#define XP0_FESTAT_INTERRUPT_MASK   0x00007F3F  /* Valid Front-End Status bits   */

#define XP0_PRIMARY_INTERRUPT_MASK  0xFFFE0003  /* Valid Primary Interrupt bits  */

#define XP0_PRIMARY_SET_MASK        0xF0000000  /* Primary Ints auto enabled     */



#define XP12_FESTAT_INTERRUPT_MASK   0x0000713F  /* Valid Front-End Status bits   */

#define XP12_PRIMARY_INTERRUPT_MASK  0xC6CE0000  /* Valid Primary Interrupt bits  */

#define XP12_PRIMARY_SET_MASK        0xC0000000  /* Primary Ints auto enabled     */



#define QUEUE_SIZE  20





typedef struct xp_os_msg_type

{

    GLOBAL_RESOURCES *pGlobal;

    SHORT   wChannelId;                     /* channel id for the interrupt     */

    ULONG   ulInterrupt;                    /* interrupt vector for channel     */

    XP_INTERRUPT_CHANNEL_FN notify_fn;      /* callback function                */

} XP_OS_MSG_TYPE, *XP_OS_MSG_PTR;



/*----------------------------------------------------------------------------+

| Static Variables

+----------------------------------------------------------------------------*/

static GLOBAL_RESOURCES *pXp0Global;

static GLOBAL_RESOURCES *pXp1Global;

static GLOBAL_RESOURCES *pXp2Global;



/*----------------------------------------------------------------------------+

| Local Prototype Definitions

+----------------------------------------------------------------------------*/

//Internal

static void process_channel_interrupt(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId);

static void status_interrupt(GLOBAL_RESOURCES *pGlobal);

static void audio_interrupt(GLOBAL_RESOURCES *pGlobal);

static void video_interrupt(GLOBAL_RESOURCES *pGlobal);

static void channel_interrupt(GLOBAL_RESOURCES *pGlobal);

static SHORT add_mask(GLOBAL_RESOURCES *pGlobal,ULONG ulMask,PFS notify_fn,

                      INTERRUPT_INFO_PTR pInfo);

static SHORT del_mask(GLOBAL_RESOURCES * pGlobal,ULONG ulMask,PFS notify_fn,

                      INTERRUPT_INFO_PTR pInfo);

static void reset_mask(GLOBAL_RESOURCES *pGlobal,INTERRUPT_INFO_PTR pInfo);









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

|  process_channel_interrupt

+-----------------------------------------------------------------------------+

|

|  FUNCTION    :  process_channel_interrupt

|

|  DESCRIPTION :  process interrupts for audio, video, and system channels

|

+----------------------------------------------------------------------------*/

static void process_channel_interrupt(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId)

{

    short i, j;

    unsigned long addr;

    unsigned long interrupt;

    XP_OS_MSG_TYPE msg;



    switch(wChannelId)

    {

    case XP_INTERRUPT_VIDEO:

        addr  = XP_DCR_ADDR_VSTATUS;

        break;

    case XP_INTERRUPT_AUDIO:

        addr  = XP_DCR_ADDR_ASTATUS;

        break;

    default:

        addr  = XP_DCR_ADDR_BASE_QSTAT + wChannelId;

        break;

    }



    /*------------------------------------------------------------------------+

    |  Read & clear the primary interrupts

    +------------------------------------------------------------------------*/

//  flag = os_enter_critical_section();

    interrupt = xp_atom_dcr_read(pGlobal->uDeviceIndex,addr);

    xp_atom_dcr_write(pGlobal->uDeviceIndex, addr, interrupt);

//  os_leave_critical_section(flag);



    for(i=0, j=0; i<pGlobal->InterInfo.XpInterChanInt[wChannelId].wNotifyAlloc &&

                  j<pGlobal->InterInfo.XpInterChanInt[wChannelId].wNotifyCount; i++)

    {

        if(interrupt & pGlobal->InterInfo.XpInterChanInt[wChannelId].pNotify[i].ulMask)

        {

            j++;



            /*----------------------------------------------------------------+

            |  Process the current interrupt by adding the work to the

            |  Task queue. Note: direct call is

            |  (*notify_fn)(channel_id, interrupt)

            +----------------------------------------------------------------*/

            if (pGlobal->InterInfo.XpInterChanInt[wChannelId].pNotify[i].notify_fn)

            {

                msg.pGlobal    = pGlobal;

                msg.wChannelId = wChannelId;

                msg.ulInterrupt  = interrupt;

                msg.notify_fn  = (XP_INTERRUPT_CHANNEL_FN)

                    pGlobal->InterInfo.XpInterChanInt[wChannelId].pNotify[i].notify_fn;

                 if(os_call_irq_task(XP_IRQ,&msg))

                 {

                     ;

                 }

            }

        }

    }

}



/*----------------------------------------------------------------------------+

|  status_interrupt

+-----------------------------------------------------------------------------+

|

|  FUNCTION    :  status_interrupt

|

|  DESCRIPTION :  process all status interrupts

|

+----------------------------------------------------------------------------*/

static void status_interrupt(GLOBAL_RESOURCES *pGlobal)

{

    short i, j;

    unsigned long interrupt;


    /*------------------------------------------------------------------------+

    |  Read & clear the primary interrupts

    +------------------------------------------------------------------------*/

    //flag = os_enter_critical_section();

    interrupt = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_FESTAT) &

                xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_FEIMASK);





    xp_atom_dcr_write(pGlobal->uDeviceIndex, XP_DCR_ADDR_FESTAT, interrupt);



    //os_leave_critical_section(flag);



    for(i=0, j=0; i<pGlobal->InterInfo.XpInterStatInt.wNotifyAlloc &&

                  j<pGlobal->InterInfo.XpInterStatInt.wNotifyCount; i++)

    {

        if(interrupt & pGlobal->InterInfo.XpInterStatInt.pNotify[i].ulMask)

        {

            j++;

            if(pGlobal->InterInfo.XpInterStatInt.pNotify[i].notify_fn)

            {

              (*pGlobal->InterInfo.XpInterStatInt.pNotify[i].notify_fn)(pGlobal, interrupt);

            }

        }

    }

}



/*----------------------------------------------------------------------------+

|  audio_interrupt

+-----------------------------------------------------------------------------+

|

|  FUNCTION    :  audio_interrupt

|

|  DESCRIPTION :  process all status interrupts

|

+----------------------------------------------------------------------------*/

static void audio_interrupt(GLOBAL_RESOURCES *pGlobal)



{

    process_channel_interrupt(pGlobal,XP_INTERRUPT_AUDIO);

}



/*----------------------------------------------------------------------------+

|  video_interrupt

+-----------------------------------------------------------------------------+

|

|  FUNCTION    :  video_interrupt

|

|  DESCRIPTION :  process all status interrupts

|

+----------------------------------------------------------------------------*/

static void video_interrupt(GLOBAL_RESOURCES *pGlobal)



{

    process_channel_interrupt(pGlobal,XP_INTERRUPT_VIDEO);

}



/*----------------------------------------------------------------------------+

|  channel_interrupt

+-----------------------------------------------------------------------------+

|

|   FUNCTION    :  channel_interrupt

|

|   DESCRIPTION :  process all the channel interrupts

|

+----------------------------------------------------------------------------*/

static void channel_interrupt(GLOBAL_RESOURCES *pGlobal)

{

    short i;

    unsigned long k;

    unsigned long ireq;


    /*------------------------------------------------------------------------+

    |  Get the interrupt vector which shows which channels have an

    |  interrupt pending

    +------------------------------------------------------------------------*/

//  flag = os_enter_critical_section();

    ireq = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_QINT) &

           xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_QINTMSK);

//  os_leave_critical_section(flag);



    /*------------------------------------------------------------------------+

    |  for each interrupt on a channel, process the interrupts

    +------------------------------------------------------------------------*/

    for(i=0, k=0x80000000; i<XP_CHANNEL_COUNT; i++, k>>=1) {

        if(k & ireq) {

            process_channel_interrupt(pGlobal,i);

        }

    }

}



/*----------------------------------------------------------------------------+

|  add_mask

+----------------------------------------------------------------------------*/

static SHORT add_mask(GLOBAL_RESOURCES *pGlobal,ULONG ulMask,PFS notify_fn,

                      INTERRUPT_INFO_PTR pInfo)

{

    short i;

    short index;                            /* array index available         */

    INTERRUPT_NOTIFY_PTR pNotify;



    /*------------------------------------------------------------------------+

    |  Make sure this mask isn't already registered

    +------------------------------------------------------------------------*/

    for(i=0, index=-1; i<pInfo->wNotifyAlloc; i++)

    {

        if((ulMask == pInfo->pNotify[i].ulMask) &&

           (notify_fn == pInfo->pNotify[i].notify_fn))

        {

            return(XP_ERROR_INTER_NOTIFY_DUP);

        }

        if(pInfo->pNotify[i].ulMask == 0)

        {

            index = i;

        }

    }



    /*------------------------------------------------------------------------+

    |  If there is no space, allocate more space

    +------------------------------------------------------------------------*/

    if (index < 0)

    {

        if (pInfo->wNotifyAlloc == 0)

        {                               //added by lingh

            index = pInfo->wNotifyAlloc;

            pInfo->wNotifyAlloc += 10;



            pNotify = (INTERRUPT_NOTIFY_PTR) MALLOC(pInfo->wNotifyAlloc * sizeof(XP_INTERRUPT_NOTIFY_TYPE));



            if(pNotify == NULL)

            {

                return(XP_ERROR_INTERNAL);

            }



            pInfo->pNotify = pNotify;



        /*------------------------------------------------------------------------+

        |  Initialize the new space

        +------------------------------------------------------------------------*/

            for(i=index; i<pInfo->wNotifyAlloc; i++)

            {

                pInfo->pNotify[i].ulMask = 0;

                pInfo->pNotify[i].notify_fn = NULL;

            }

        }

        else

        {

             return (XP_ERROR_INTERNAL);    /* which should not happen */

        }

    }



    pInfo->pNotify[index].ulMask      = ulMask;

    pInfo->pNotify[index].notify_fn = notify_fn;

    pInfo->wNotifyCount++;



    return(0);

}



/*----------------------------------------------------------------------------+

|  del_mask

+----------------------------------------------------------------------------*/

static SHORT del_mask(GLOBAL_RESOURCES * pGlobal,ULONG ulMask,PFS notify_fn,

                      INTERRUPT_INFO_PTR pInfo)

{

    short i;

    short count=0;



    for (i=0; i<pInfo->wNotifyAlloc; i++)

    {

        if((ulMask == pInfo->pNotify[i].ulMask) &&

           (notify_fn == pInfo->pNotify[i].notify_fn))

        {

            pInfo->pNotify[i].ulMask = 0;

            pInfo->pNotify[i].notify_fn = NULL;

            pInfo->wNotifyCount--;

            count++;

        }

    }



    if(count == 0)

    {

        return(XP_ERROR_INTER_MASK);

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|  reset_mask

+----------------------------------------------------------------------------*/

static void reset_mask(GLOBAL_RESOURCES *pGlobal,INTERRUPT_INFO_PTR pInfo)



{

    if(pInfo->wNotifyAlloc > 0)

    {

      pInfo->wNotifyAlloc = 0;

      FREE(pInfo->pNotify);

      pInfo->pNotify = NULL;

    }



    pInfo->wNotifyCount = 0;

}



/*----------------------------------------------------------------------------+

| XXXXXXX  XXX XXX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX

|  XX   X   XX XX    X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX

|  XX X      XXX       XX     XX X     XX  XX  XXXX XX   XX  XX   XX

|  XXXX       X        XX     XXXX     XXXXX   XX XXXX   XX  XX   XX

|  XX X      XXX       XX     XX X     XX XX   XX  XXX   XXXXXX   XX

|  XX   X   XX XX      XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX

| XXXXXXX  XXX XXX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX

+----------------------------------------------------------------------------*/

void *xp_osi_task(QUEUE_T *pQueue)

{

    XP_OS_MSG_TYPE msg;                  /* message received                 */



    /*------------------------------------------------------------------------+

    |  Wait for and process messages

    +------------------------------------------------------------------------*/

    while(os_irq_task_get_msg(pQueue, &msg) >= 0)

    {

        (*msg.notify_fn)(msg.pGlobal, msg.wChannelId, msg.ulInterrupt);

    }

    return (NULL);

}

/*----------------------------------------------------------------------------+

|  xp0_interrupt

+-----------------------------------------------------------------------------+

|

|  FUNCTION    :  xp0_interrupt

|

|  DESCRIPTION :  process all interrupts from transport

|

+----------------------------------------------------------------------------*/

void xp_osi_interrupt()

{

    short i, j;

    unsigned long interrupt;


    /*------------------------------------------------------------------------+

    |  Read & clear the primary interrupts.

    |  Process all the interrupt bits which were set

    +------------------------------------------------------------------------*/

    if(pXp0Global == NULL)

          return;

    interrupt = xp_atom_dcr_read_interrupt(pXp0Global->uDeviceIndex);



    for(i=0, j=0; i<pXp0Global->InterInfo.XpInterPriInt.wNotifyAlloc &&

                  j<pXp0Global->InterInfo.XpInterPriInt.wNotifyCount; i++)

    {

        if(interrupt & pXp0Global->InterInfo.XpInterPriInt.pNotify[i].ulMask)

        {

            j++;

            if(pXp0Global->InterInfo.XpInterPriInt.pNotify[i].notify_fn)

            {

                (*pXp0Global->InterInfo.XpInterPriInt.pNotify[i].notify_fn)(pXp0Global, interrupt);

            }

        }

    }



    /*------------------------------------------------------------------------+

    |  Call primary handlers for interrupt hierarchy

    +------------------------------------------------------------------------*/

    if(interrupt & XP_INTERRUPT_IR_QUE)

    {

        channel_interrupt(pXp0Global);

    }



    if(interrupt & XP_INTERRUPT_IR_FES)

    {

        status_interrupt(pXp0Global);

    }



    if(interrupt & XP_INTERRUPT_IR_VID)

    {

        video_interrupt(pXp0Global);

    }



    if(interrupt & XP_INTERRUPT_IR_AUD)

    {

        audio_interrupt(pXp0Global);

    }



}



void xp1_interrupt(GLOBAL_RESOURCES *pXp0Global, ULONG ulXp0Int)

{



    short         i, j;

    unsigned long interrupt;

    GLOBAL_RESOURCES *pGlobal;


    pGlobal = pXp1Global;

    if(pGlobal == NULL)

    {

        return;

    }



    /*------------------------------------------------------------------------+

    |  Read & clear the primary interrupts.

    |  Process all the interrupt bits which were set

    +------------------------------------------------------------------------*/

//  flag = os_enter_critical_section();

    interrupt = xp_atom_dcr_read_interrupt(pGlobal->uDeviceIndex);

//  os_leave_critical_section(flag);



    for(i=0, j=0; i<pGlobal->InterInfo.XpInterPriInt.wNotifyAlloc &&

                  j<pGlobal->InterInfo.XpInterPriInt.wNotifyCount; i++)

    {

        if(interrupt & pGlobal->InterInfo.XpInterPriInt.pNotify[i].ulMask)

        {

            j++;

            if(pGlobal->InterInfo.XpInterPriInt.pNotify[i].notify_fn)

            {

                (*pGlobal->InterInfo.XpInterPriInt.pNotify[i].notify_fn)(pGlobal, interrupt);

            }

        }

    }



    /*------------------------------------------------------------------------+

    |  Call primary handlers for interrupt hierarchy

    +------------------------------------------------------------------------*/

    if(interrupt & XP_INTERRUPT_IR_QUE)

    {

        channel_interrupt(pGlobal);

    }



    if(interrupt & XP_INTERRUPT_IR_FES)

    {

        status_interrupt(pGlobal);

    }



}



void xp2_interrupt(GLOBAL_RESOURCES *pXp0Global, ULONG ulXp0Int)

{



    short         i, j;

    unsigned long interrupt;

    GLOBAL_RESOURCES *pGlobal;


    pGlobal = pXp2Global;

    if(pGlobal == NULL)

    {

        return;

    }



    /*------------------------------------------------------------------------+

    |  Read & clear the primary interrupts.

    |  Process all the interrupt bits which were set

    +------------------------------------------------------------------------*/

//  flag = os_enter_critical_section();

    interrupt = xp_atom_dcr_read_interrupt(pGlobal->uDeviceIndex);

//  os_leave_critical_section(flag);



    for(i=0, j=0; i<pGlobal->InterInfo.XpInterPriInt.wNotifyAlloc &&

                  j<pGlobal->InterInfo.XpInterPriInt.wNotifyCount; i++)

    {

        if(interrupt & pGlobal->InterInfo.XpInterPriInt.pNotify[i].ulMask)

        {

            j++;

            if(pGlobal->InterInfo.XpInterPriInt.pNotify[i].notify_fn)

            {

                (*pGlobal->InterInfo.XpInterPriInt.pNotify[i].notify_fn)(pGlobal, interrupt);

            }

        }

    }



    /*------------------------------------------------------------------------+

    |  Call primary handlers for interrupt hierarchy

    +------------------------------------------------------------------------*/

    if(interrupt & XP_INTERRUPT_IR_QUE)

    {

        channel_interrupt(pGlobal);

    }



    if(interrupt & XP_INTERRUPT_IR_FES)

    {

        status_interrupt(pGlobal);

    }



}





/*----------------------------------------------------------------------------+

|  xp0_interrupt_init

+----------------------------------------------------------------------------*/

SHORT xp_osi_interrupt_init(GLOBAL_RESOURCES *pGlobal)

{

    short rc = 0;

    UINT32  flag;



    PDEBUG("pGlobal->uDeviceIndex = %x\n",pGlobal->uDeviceIndex);

    switch(pGlobal->uDeviceIndex)

    {

    case 0:

        pXp0Global = pGlobal;

        break;

    case 1:

        pXp1Global = pGlobal;

        break;

    case 2:

        pXp2Global = pGlobal;

        break;

    default:

        break;

    }



     /*------------------------------------------------------------------------+

    |  Initialize all the interrupt structures

    +------------------------------------------------------------------------*/

    memset(&pGlobal->InterInfo.XpInterStatInt, 0, sizeof(pGlobal->InterInfo.XpInterStatInt));

    memset(&pGlobal->InterInfo.XpInterPriInt,  0, sizeof(pGlobal->InterInfo.XpInterPriInt));

    memset(&pGlobal->InterInfo.XpInterChanInt, 0, sizeof(pGlobal->InterInfo.XpInterChanInt));





    if(pGlobal->uDeviceIndex == 0)

    {

        flag = os_enter_critical_section();

        xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_INTMASK, XP0_PRIMARY_SET_MASK);

        os_leave_critical_section(flag);

    }

    else

    {

        flag = os_enter_critical_section();

        xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_INTMASK, XP12_PRIMARY_SET_MASK);

        os_leave_critical_section(flag);

    }



    pGlobal->InterInfo.XpInterChanMask = 0;

    pGlobal->InterInfo.XpInterPriMask = 0;



    if(pXp0Global == NULL)

        return -1;



    if(pGlobal->uDeviceIndex == 0)

    {

            if(os_add_irq_task(XP_IRQ,(void*)xp_osi_task,sizeof(XP_OS_MSG_TYPE),QUEUE_SIZE))

                return -1;

    }



    if(pGlobal->uDeviceIndex == 1)

        rc = xp_osi_interrupt_notify(pXp0Global,XP_INTERRUPT_NOTIFY_ADD,

        XP_INTERRUPT_IR_X1INT, (PFS)xp1_interrupt);

    else if(pGlobal->uDeviceIndex == 2)

        rc = xp_osi_interrupt_notify(pXp0Global,XP_INTERRUPT_NOTIFY_ADD,

        XP_INTERRUPT_IR_X2INT, (PFS)xp2_interrupt);





    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_interrupt_channel_control

+----------------------------------------------------------------------------*/

SHORT xp_osi_interrupt_channel_control(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,

                                       XP_INTERRUPT_CONTROL_TYPE cmd)



{

    unsigned long addr;

    unsigned long bit;

    unsigned long ireq;

    unsigned long new_ireq;

    UINT32  flag;



    if((wChannelId < 0) || (wChannelId >= XP_INTERRUPT_COUNT))

    {

        return(XP_ERROR_CHANNEL_INVALID);

    }



    /*------------------------------------------------------------------------+

    |  Clear any interrupts which may be pending

    +------------------------------------------------------------------------*/

    if(cmd == XP_INTERRUPT_CONTROL_RESET)

    {

        switch(wChannelId)

        {

        case XP_INTERRUPT_AUDIO:

            addr = XP_DCR_ADDR_ASTATUS;

            break;

        case XP_INTERRUPT_VIDEO:

            addr = XP_DCR_ADDR_VSTATUS;

            break;

        default:

            addr = XP_DCR_ADDR_BASE_QSTAT + wChannelId;

            break;

        }



        flag = os_enter_critical_section();

        xp_atom_dcr_write(pGlobal->uDeviceIndex,addr, 0xFFFFFFFF);

        os_leave_critical_section(flag);

    }



    /*------------------------------------------------------------------------+

    |  Enable or disable any interrupts for a channel using the ireq register

    |  Contruct the value used to control the ireq field

    +------------------------------------------------------------------------*/

    else

    {

        if (wChannelId < XP_CHANNEL_COUNT)

        {

            bit = 1 << (31 - wChannelId);



            flag = os_enter_critical_section();

            new_ireq = ireq = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_QINTMSK);

            os_leave_critical_section(flag);



          switch(cmd)

          {

          case XP_INTERRUPT_CONTROL_DISABLE:

              new_ireq &= ~bit;

              break;

          case XP_INTERRUPT_CONTROL_ENABLE:

              new_ireq |= bit;

              break;

          default:

              return(XP_ERROR_INTERNAL);

          }



          if(ireq != new_ireq)

          {

              flag = os_enter_critical_section();

              xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_QINTMSK, new_ireq);

              os_leave_critical_section(flag);

          }

       }

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|  xp0_interrupt_channel_free

+----------------------------------------------------------------------------*/

SHORT xp_osi_interrupt_channel_free(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId)

{

    unsigned long value = 0;

    UINT32  flag;



    flag = os_enter_critical_section();



    if((wChannelId < 0) || (wChannelId >= XP_INTERRUPT_COUNT)) {

        return(XP_ERROR_CHANNEL_INVALID);

    }



    /*------------------------------------------------------------------------+

    |  Delete all notifications associated with the channel

    +------------------------------------------------------------------------*/

    reset_mask(pGlobal,&pGlobal->InterInfo.XpInterChanInt[wChannelId]);



    os_leave_critical_section(value);



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

|  xp0_interrupt_channel_notify

+-----------------------------------------------------------------------------+

|

|  FUNCTION    :  xp0_interrupt_channel_notify

|

|  DESCRIPTION :  register a function to call for specific interrupts

|

|  PROTOTYPE  :  short xp0_interrupt_channel_notify(

|                short cmd,

|                short channel_id,

|                unsigned long int_mask,

|                PFS notify_fn)

|

|  ARGUMENTS  :  cmd           -  XP_INTERRUPT_NOTIFY_ADD to add a function

|                                 XP_INTERRUPT_NOTIFY_DELETE to remove

|                channel_id    -  the channel id as returned from the

|                                 xp0_channel_allocate().

|                notify_fn     -  function to call when an interrupt

|                                 occurs for a specific channel

|                                 NULL, the notification is disabled

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel_id is not defined

|

|  COMMENTS   :  xp0_interrupt_channel_notify() is available to register

|                a callback function.  The callback function executes

|                under interrupt control and is expected to complete it's

|                work quickly so other interrupts may be services. The

|                be serviced.

|                  The cmd parameter should be XP_INTERRUPT_NOTIFY_ADD, or

|                XP_INTERRUPT_NOTIFY_DELETE to add or remove the notification

|                callback function.

|                  The int_mask parameter is a bit string which represents

|                a set of interrupt types which are or'd together to form

|                a mask.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_interrupt_channel_notify(GLOBAL_RESOURCES *pGlobal,SHORT wCmd,SHORT wChannelId,

                                      ULONG ulIntMask,XP_INTERRUPT_CHANNEL_FN notify_fn)

{

    short rc;

    short i;

    short j;

    unsigned long dcr_mask=0;

    UINT32  flag;



    flag = os_enter_critical_section();



    if(notify_fn == NULL)

    {

        rc = XP_ERROR_INTER_NOTIFY_INVALID;

    }

    else if((wChannelId < 0) || (wChannelId >= XP_INTERRUPT_COUNT))

    {

        rc = XP_ERROR_CHANNEL_INVALID;

    }

    /*------------------------------------------------------------------------+

    |  If the function is NULL, then find the matching mask and cancel

    |  the callback function

    +------------------------------------------------------------------------*/

    else

    {

        ulIntMask &= QSTAT_INTERRUPT_MASK;

        if(wCmd == XP_INTERRUPT_NOTIFY_DELETE)

        {

            rc = del_mask(pGlobal,ulIntMask, (PFS)notify_fn,

                          &pGlobal->InterInfo.XpInterChanInt[wChannelId]);

            if((rc == 0) &&

               (pGlobal->InterInfo.XpInterChanInt[wChannelId].wNotifyCount == 0))

            {

                xp_osi_interrupt_channel_control(pGlobal,

                            wChannelId,

                          XP_INTERRUPT_CONTROL_DISABLE);

            }

        }

        else if(wCmd == XP_INTERRUPT_NOTIFY_ADD)

        {

            rc = add_mask(pGlobal,ulIntMask, (PFS) notify_fn,

                          &pGlobal->InterInfo.XpInterChanInt[wChannelId]);

            if((rc == 0) &&

               (pGlobal->InterInfo.XpInterChanInt[wChannelId].wNotifyCount > 0))

            {

                xp_osi_interrupt_channel_control(pGlobal,

                            wChannelId,

                          XP_INTERRUPT_CONTROL_ENABLE);

            }

        }

        else

        {

            rc = XP_ERROR_INTER_NOTIFY_CMD;

        }

    }



    /*------------------------------------------------------------------------+

    |  Recalculate the mask and write it.  The channel mask is constructed

    |  based on all the bits over all the dram channels.  The audio & video

    |  mask are built upon their own channel.

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

        if(wChannelId < XP_CHANNEL_COUNT)

        {

            /*----------------------------------------------------------------+

            |  Construct the channel mask

            +----------------------------------------------------------------*/

            for(i=0, pGlobal->InterInfo.XpInterChanMask=0; i<XP_INTERRUPT_COUNT; i++)

            {

                for(j=0; j<pGlobal->InterInfo.XpInterChanInt[i].wNotifyAlloc; j++)

                {

                   pGlobal->InterInfo.XpInterChanMask |=

                       pGlobal->InterInfo.XpInterChanInt[i].pNotify[j].ulMask;

                }

            }


#ifndef __DRV_FOR_VESTA__
            dcr_mask = pGlobal->InterInfo.XpInterChanMask;
            xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_QSTATMASK, dcr_mask);
#else
            dcr_mask = pGlobal->InterInfo.XpInterChanMask | pGlobal->InterInfo.XpInterPriMask | XP0_PRIMARY_SET_MASK;
            xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_INTMASK, dcr_mask);
#endif

        }

        else

        {

            for(j=0; j<pGlobal->InterInfo.XpInterChanInt[wChannelId].wNotifyAlloc; j++)

            {

                dcr_mask |= pGlobal->InterInfo.XpInterChanInt[wChannelId].pNotify[j].ulMask;

            }



            if(wChannelId == XP_INTERRUPT_AUDIO)

            {

                xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_AINTMSK, dcr_mask);

            }

            else

            {

               xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_VINTMSK, dcr_mask);

            }

        }

    }



    os_leave_critical_section(flag);



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_interrupt_notify

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  register a callback function for primary interrupts

|

|  PROTOTYPE  :  short xp0_interrupt_notify(

|                short cmd,

|                unsigned long int_mask,

|                PFS notify_fn)

|

|  ARGUMENTS  :  cmd           -  XP_INTERRUPT_NOTIFY_ADD to add a function

|                                 XP_INTERRUPT_NOTIFY_DELETE to remove

|                int_mask      -  interrupts to register function for

|                notify_fn     -  notification function to call-back

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_INTER_NOTIFY_CMD - invalid command specified in 'cmd'

|

|  COMMENTS   :  This function registers a callback function to be called

|                when at least one of the interrupt bits matches a bit

|                in the 'int_mask' argument.  The interrupt bits are

|                defined in the 'xp0_interrupt.h' header file.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_interrupt_notify(GLOBAL_RESOURCES *pGlobal,SHORT wCmd,ULONG ulIntMask,

                              PFS notify_fn)

{

    short rc;

    short i;

    unsigned long dcr_mask;

    UINT32  flag;



    flag = os_enter_critical_section();



    if(notify_fn == NULL)

    {

        rc = XP_ERROR_INTER_NOTIFY_INVALID;

    }



    /*------------------------------------------------------------------------+

    |  If the function is NULL, then find the matching mask and cancel

    |  the callback function

    +------------------------------------------------------------------------*/

    else

    {

        if(pGlobal->uDeviceIndex == 0)

            ulIntMask &= XP0_PRIMARY_INTERRUPT_MASK;

        else

            ulIntMask &= XP12_PRIMARY_INTERRUPT_MASK;



        if(wCmd == XP_INTERRUPT_NOTIFY_DELETE)

        {

            rc = del_mask(pGlobal,ulIntMask, notify_fn, &pGlobal->InterInfo.XpInterPriInt);

        }



        else if(wCmd == XP_INTERRUPT_NOTIFY_ADD)

        {

            rc = add_mask(pGlobal,ulIntMask, notify_fn, &pGlobal->InterInfo.XpInterPriInt);

        }

        else

        {

            rc = XP_ERROR_INTER_NOTIFY_CMD;

        }

    }



    /*------------------------------------------------------------------------+

    |  Recalculate the mask and write it

    |  Always turn on the STATUS, IREQ, AUDIO & VIDEO bits

    +------------------------------------------------------------------------*/

    if (rc == 0)

    {

        for(i=0, pGlobal->InterInfo.XpInterPriMask=0;

                i<pGlobal->InterInfo.XpInterPriInt.wNotifyAlloc; i++)

                {

                    pGlobal->InterInfo.XpInterPriMask |=

                        pGlobal->InterInfo.XpInterPriInt.pNotify[i].ulMask;

                }



                if(pGlobal->uDeviceIndex == 0)

                {

                    dcr_mask = pGlobal->InterInfo.XpInterPriMask | XP0_PRIMARY_SET_MASK;

                }

                else

                {

                    dcr_mask = pGlobal->InterInfo.XpInterPriMask | XP12_PRIMARY_SET_MASK;

                }

#ifdef __DRV_FOR_VESTA__
            dcr_mask = pGlobal->InterInfo.XpInterChanMask | pGlobal->InterInfo.XpInterPriMask | XP0_PRIMARY_SET_MASK;
#endif

                flag = os_enter_critical_section();

                xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_INTMASK, dcr_mask);

                os_leave_critical_section(flag);

    }



    os_leave_critical_section(flag);



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_interrupt_status_notify

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  register a callback function for demux status interrupts

|

|  PROTOTYPE  :  short xp0_interrupt_status_notify(

|                short cmd,

|                unsigned long int_mask,

|                PFS notify_fn)

|

|  ARGUMENTS  :  cmd           -  XP_INTERRUPT_NOTIFY_ADD to add a function

|                                 XP_INTERRUPT_NOTIFY_DELETE to remove

|                int_mask      -  interrupts to register function for

|                notify_fn     -  notification function to call-back

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_INTER_NOTIFY_CMD - invalid command specified in 'cmd'

|

|  COMMENTS   :  This function registers a callback function to be called

|                when at least one of the interrupt bits matches a bit

|                in the 'int_mask' argument.  The interrupt bits are

|                defined in the 'xp0_interrupt.h' header file.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_interrupt_status_notify(GLOBAL_RESOURCES *pGlobal,SHORT wCmd,ULONG ulIntMask,

                                     PFS notify_fn)

{

    short rc;

    short i;

    unsigned long dcr_mask;


    UINT32  flag;



    flag = os_enter_critical_section();



    if(notify_fn == NULL)

    {

        rc = XP_ERROR_INTER_NOTIFY_INVALID;

    }



    /*------------------------------------------------------------------------+

    |  if the function is NULL, then find the matching mask and cancel

    |  the callback function

    +------------------------------------------------------------------------*/

    else

    {

        if(pGlobal->uDeviceIndex == 0)

            ulIntMask &= XP0_FESTAT_INTERRUPT_MASK;

        else

            ulIntMask &= XP12_FESTAT_INTERRUPT_MASK;



        if(wCmd == XP_INTERRUPT_NOTIFY_DELETE)

        {

            rc = del_mask(pGlobal,ulIntMask, notify_fn, &pGlobal->InterInfo.XpInterStatInt);

        }

        else if(wCmd == XP_INTERRUPT_NOTIFY_ADD)

        {

            rc = add_mask(pGlobal,ulIntMask, notify_fn, &pGlobal->InterInfo.XpInterStatInt);

        }

        else

        {

            rc = XP_ERROR_INTER_NOTIFY_CMD;

        }

    }



    /*------------------------------------------------------------------------+

    |  Recalculate the mask and write it

    +------------------------------------------------------------------------*/

    if (rc == 0)

    {

        for(i=0, dcr_mask=0; i<pGlobal->InterInfo.XpInterStatInt.wNotifyAlloc; i++)

        {

            dcr_mask |= pGlobal->InterInfo.XpInterStatInt.pNotify[i].ulMask;

        }



        xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_FEIMASK, dcr_mask);

    }



    os_leave_critical_section(flag);



    return(rc);

}



int xp_osi_interrupt_de_init(GLOBAL_RESOURCES *pGlobal)

{
    int rc = 0;

    if(pGlobal->uDeviceIndex == 0)
        return os_delete_irq_task(XP_IRQ);

        else if(pGlobal->uDeviceIndex == 1)

                rc = xp_osi_interrupt_notify(pXp0Global,XP_INTERRUPT_NOTIFY_DELETE,

                XP_INTERRUPT_IR_X1INT, (PFS)xp1_interrupt);

        else if(pGlobal->uDeviceIndex == 2)

                rc = xp_osi_interrupt_notify(pXp0Global,XP_INTERRUPT_NOTIFY_DELETE,

                XP_INTERRUPT_IR_X2INT, (PFS)xp2_interrupt);

    return 0;


}

