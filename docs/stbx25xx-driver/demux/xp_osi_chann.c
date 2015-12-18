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
| Author    :  Ian Govett
| Component :  xp
| File      :  xp_chann.c
| Purpose   :  Channel Management Functions
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 15-Jan-98  IG   Created
| 30-Sep-01  LGH  Ported to Linux, combine codes of 3 devices, take it as
|                 part of os independent layer
| 22-Jul-03  TJC  Corrected bug in xp_osi_set_key and xp_osi_set_pid such that
|                 UnloadType was used instead of Descram to set the Descrambler
|                 enable bit.
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|                      Demux Channel Management
+-----------------------------------------------------------------------------+
|   The following functions provide channel management services for the
|   MPEG transport driver.  The xp0_channel_allocate() function allocates a
|   channel for the type specified.  The transport demux hardware has one
|   audio (30), and one video (31) channel available for the driver.  The
|   driver reserves a channel for subtitle (0), and one channel for
|   teletext (1).  The remaining 28 channels (2..29) may be freely used by
|   the application.  Allocation of all channels is managed by the driver
|   using the xp0_channel_allocate() function, which returns the channel id.
|   number.  This wChannelId is used to define queues, attach filters, and
|   manage interrupts.
|
|   After the channel is allocated, the xp0_channel_set_pid() defines the
|   pid value, and xp0_channel_set_unload_type() determines the type of data
|   (table sections, adaptation, payload, ...)  to deliver to the queue
|   associated with the channel.  See the transport specification for
|   information on the numerous unload types.
|
|   The xp0_channel_set_notification_fn() registers the application callback
|   function.  This function is called whenever data is available on the
|   channel.  The callback function is provided with a pointer to the data,
|   length of the data, wChannelId, and a match word.  The match word is
|   used for table section filtering to determine which filter(s) matched
|   the data.
|
|   The xp0_channel_free() frees the channel and any associated resources
|   such as queues, filters, and pending interrupts.
|
|   The application uses the xp0_channel_control() function to enable,
|   disable, or reset the channel.
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
#include <linux/types.h>

#include "xp_osi_global.h"
#include "xp_atom_reg.h"
/*----------------------------------------------------------------------------+
| Local Defines
+----------------------------------------------------------------------------*/
#define CHANNEL_MIN     0

/*----------------------------------------------------------------------------+
|  Define the list of interrupts to register for the table section channels
+----------------------------------------------------------------------------*/
#define CHANNEL_NOTIFY_SECTION_MASK XP_INTERRUPT_QSTAT_RPI       |          \
                                    XP_INTERRUPT_QSTAT_TSC       |          \
                                    XP_INTERRUPT_QSTAT_CRCE      |          \
                                    XP_INTERRUPT_QSTAT_TSLE      |          \
                                    XP_INTERRUPT_QSTAT_PSE

#define CHANNEL_NOTIFY_STREAM_MASK  XP_INTERRUPT_QSTAT_RPI       |          \
                                    XP_INTERRUPT_QSTAT_BTI       |          \
                                    XP_INTERRUPT_QSTAT_PSE

//Internal
static void channel_data_notify(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,ULONG ulInterrupt);
//External

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
|  channel_data_notify
+----------------------------------------------------------------------------*/
static void channel_data_notify(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId, ULONG ulInterrupt)
{
    /*------------------------------------------------------------------------+
    |  Process available data
    +------------------------------------------------------------------------*/

    if (ulInterrupt & (XP_INTERRUPT_QSTAT_TSC | XP_INTERRUPT_QSTAT_BTI))
    {
        xp_osi_queue_process_data(pGlobal,
            wChannelId,
            pGlobal->ChannInfo.XpChannelData[wChannelId].UnloadType,
            pGlobal->ChannInfo.XpChannelData[wChannelId].notify_fn);
    }

    /*------------------------------------------------------------------------+
    |  Process errors data including the read pointer error
    +------------------------------------------------------------------------*/

    xp_osi_queue_process_interrupt(pGlobal,wChannelId, ulInterrupt);
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
/*----------------------------------------------------------------------------+
|  xp0_channel_get_status
+----------------------------------------------------------------------------*/

SHORT xp_osi_channel_get_status(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,
                                XP_CHANNEL_STATUS *pStatus)
{

    SHORT wRc;

//  xp_os_semaphore_wait();

    wRc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(wRc == 0)
    {
        *pStatus = pGlobal->ChannInfo.XpChannelData[wChannelId].Status;
    }

//  xp_os_semaphore_signal();

    return(wRc);
}
/*----------------------------------------------------------------------------+
|  xp0_channel_restart
+----------------------------------------------------------------------------*/

SHORT xp_osi_channel_restart(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId)
{
    SHORT wRc;
    UINT32  flag;

//    xp_os_semaphore_wait();
    PDEBUG("entering into xp_osi_channel_restart\n");

    wRc = xp_osi_channel_valid(pGlobal,wChannelId);

    /*------------------------------------------------------------------------+
    |  Clear the interrupts to get rid of any latent interrupts
    +------------------------------------------------------------------------*/
    if(wRc == 0)
    {
        PDEBUG("xp_osi_channel_valid success\n");
        wRc = xp_osi_interrupt_channel_control(pGlobal,
            wChannelId,
            XP_INTERRUPT_CONTROL_RESET);
    }

    /*------------------------------------------------------------------------+
    |  Rewrite the pid to start the channel
    +------------------------------------------------------------------------*/
    if(wRc == 0)
    {
        PDEBUG("xp_osi_interrupt_channel_control success\n");
        if(pGlobal->ChannInfo.XpChannelData[wChannelId].Status == XP_CHANNEL_ENABLED)
        {
            PDEBUG(" if(pGlobal->ChannInfo.XpChannelData[wChannelId].Status == XP_CHANNEL_ENABLED)\n");
            if(wChannelId == pGlobal->ChannInfo.wXpChannelBucketId)
            {
                flag = os_enter_critical_section();
                xp_atom_dcr_write_register(pGlobal->uDeviceIndex,
                    XP_BUCKET1Q_REG_INDX,
                    pGlobal->ChannInfo.wXpChannelBucketId);
                os_leave_critical_section(flag);
            }
            else
            {
                flag = os_enter_critical_section();
                xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,
                    XP_PID_FILTER_REG_PIDV,
                    wChannelId,
                    pGlobal->ChannInfo.XpChannelData[wChannelId].uwPid);
                os_leave_critical_section(flag);
            }
        }
    }

//    xp_os_semaphore_signal();
    PDEBUG("before return\n");

    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp_osi_channel_valid
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_valid(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId)
{
    if((wChannelId < CHANNEL_MIN) || (wChannelId >= XP_CHANNEL_COUNT))
    {
        return((SHORT)XP_ERROR_CHANNEL_INVALID);
    }

    if((pGlobal->ChannInfo.XpChannelData[wChannelId].Status == XP_CHANNEL_UNUSED))
    {
        return((SHORT)XP_ERROR_CHANNEL_UNUSED);
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
|  xp0_channel_init
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_init(GLOBAL_RESOURCES *pGlobal)
{
    short i;

    /*------------------------------------------------------------------------+
    |  Setup the channels
    +------------------------------------------------------------------------*/
    for(i=0; i < XP_CHANNEL_COUNT; i++)
    {
        pGlobal->ChannInfo.XpChannelData[i].Status      = XP_CHANNEL_UNUSED;
        pGlobal->ChannInfo.XpChannelData[i].uwPid       = XP_CHANNEL_NULL_PID;
    }
    pGlobal->ChannInfo.wXpChannelBucketId = XP_CHANNEL_COUNT;

    return(0);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_allocate
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  allocates a channel for the requested type
|
|  PROTOTYPE  :  xp0_channel_allocate(
|                SHORT channel_type)
|
|  ARGUMENTS  :  channel_type  -  type of data channel
|
|                    XP_CHANNEL_AUDIO   -  audio pes data
|                    XP_CHANNEL_VIDEO   -  video pes data
|                    XP_CHANNEL_SYSTEM  -  non audio/video data
|                    XP_CHANNEL_TELETEXT-  teletext data
|                    XP_CHANNEL_PES     -  non audio/video data
|
|  RETURNS    :  a channel id, or a negative value if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INUSE
|
|  COMMENTS   :  This reserves a new channel.  Only 1 AUDIO, and 1 VIDEO
|                channel are available. This function fails with an error,
|                XP_ERROR_CHANNEL_INUSE if the audio or video channels have
|                not been freed.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_allocate(GLOBAL_RESOURCES *pGlobal, XP_CHANNEL_TYPE type)
{
    SHORT wIndex;
    UINT32  flag;

//    xp_os_semaphore_wait();

    switch(type)
    {

    case XP_CHANNEL_TYPE_AUDIO:
        wIndex = (pGlobal->ChannInfo.XpChannelData[XP_CHANNEL_AUDIO].Status == XP_CHANNEL_UNUSED) ? XP_CHANNEL_AUDIO : XP_ERROR_CHANNEL_INUSE;
        break;

    case XP_CHANNEL_TYPE_VIDEO:
        wIndex = (pGlobal->ChannInfo.XpChannelData[XP_CHANNEL_VIDEO].Status == XP_CHANNEL_UNUSED) ? XP_CHANNEL_VIDEO : XP_ERROR_CHANNEL_INUSE;
        break;

    case XP_CHANNEL_TYPE_SUBTITLE:
        wIndex = (pGlobal->ChannInfo.XpChannelData[XP_CHANNEL_SUBTITLE].Status == XP_CHANNEL_UNUSED) ? XP_CHANNEL_SUBTITLE : XP_ERROR_CHANNEL_INUSE;
        break;

    case XP_CHANNEL_TYPE_TELETEXT:
        wIndex = (pGlobal->ChannInfo.XpChannelData[XP_CHANNEL_TELETEXT].Status == XP_CHANNEL_UNUSED) ? XP_CHANNEL_TELETEXT : XP_ERROR_CHANNEL_INUSE;
        break;

    case XP_CHANNEL_TYPE_BUCKET:

    /*------------------------------------------------------------------------+
    |  Make sure we've not already allocated the bucket
    +------------------------------------------------------------------------*/

        if(pGlobal->ChannInfo.wXpChannelBucketId != XP_CHANNEL_COUNT)
        {
            wIndex = XP_ERROR_CHANNEL_INUSE;
        }

        else
        {
            for(wIndex=XP_CHANNEL_PES_MAX; wIndex >= XP_CHANNEL_PES_MIN; wIndex--)
            {
                if (pGlobal->ChannInfo.XpChannelData[wIndex].Status == XP_CHANNEL_UNUSED)
                {
                    pGlobal->ChannInfo.wXpChannelBucketId = wIndex;
                    break;
                }
            }

            if(wIndex < XP_CHANNEL_PES_MIN)
            {
                wIndex = XP_ERROR_CHANNEL_INUSE;
            }
        }

        break;

    default:
        for(wIndex=XP_CHANNEL_PES_MAX; wIndex >= XP_CHANNEL_PES_MIN; wIndex--)
        {
            if(pGlobal->ChannInfo.XpChannelData[wIndex].Status == XP_CHANNEL_UNUSED)
            {
                break;
            }
        }

        if(wIndex < XP_CHANNEL_PES_MIN)
        {
            wIndex = XP_ERROR_CHANNEL_INUSE;
        }

        break;
    }

    /*------------------------------------------------------------------------+
    |  Check for channel unavailable
    |  Initialize the queue to disable
    +------------------------------------------------------------------------*/

    if(wIndex >= 0)
    {
        pGlobal->ChannInfo.XpChannelData[wIndex].Status         = XP_CHANNEL_DISABLED;
        pGlobal->ChannInfo.XpChannelData[wIndex].UnloadType     = XP_CHANNEL_UNLOAD_UNDEFINED;
        pGlobal->ChannInfo.XpChannelData[wIndex].Descram        = XP_DESCRAMBLE_OFF;
        pGlobal->ChannInfo.XpChannelData[wIndex].uwKeyId        = 0;

        flag = os_enter_critical_section();
        xp_atom_dcr_reset_queue(pGlobal->uDeviceIndex,wIndex);
        os_leave_critical_section(flag);
    }

//  xp_os_semaphore_signal();
    return(wIndex);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_control
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  define a queue for data unloaded on this channel
|
|  PROTOTYPE  :  SHORT xp0_channel_control(
|                SHORT wChannelId,
|                XP_CHANNEL_CONTROL_TYPE Cmd)
|
|  ARGUMENTS  :  wChannelId    -  the channel id as returned from the
|                                 xp0_channel_allocate().
|                Cmd           -  XP_CHANNEL_CONTROL_ENABLE
|                                 XP_CHANNEL_CONTROL_DISABLE
|                                 XP_CHANNEL_CONTROL_RESET
|
|  RETURNS    :  0 if successful, or non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel id is not defined
|
|  COMMENTS   :  The XP_CHANNEL_CONTROL_ENABLE writes the pid filter in the
|                demux hardware.  If the queue is properly set up, data is
|                delivered to memory.
|                The XP_CHANNEL_CONTROL_DISABLE writes the pid filter to NULL
|                which prevents the arrival and processing of data.
|                The XP_CHANNEL_CONTROL_RESET stops the current channel, and
|                reset's the queue.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_control(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,
                             XP_CHANNEL_CONTROL_TYPE Cmd)
{
    SHORT wRc;
    ULONG ulData;
    UINT32  flag;

//    xp_os_semaphore_wait();
   PDEBUG("entering into xp_osi_channel_control\n");

    wRc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(wRc == 0)
    {
        switch(Cmd)
        {
        case XP_CHANNEL_CONTROL_ENABLE:
            pGlobal->ChannInfo.XpChannelData[wChannelId].Status = XP_CHANNEL_ENABLED;
            PDEBUG("before xp_osi_channel_restart\n");
            wRc = xp_osi_channel_restart(pGlobal,wChannelId);
            PDEBUG("after xp_osi_channel_restart\n");
            PDEBUG("after1 xp_osi_channel_restart\n");
            PDEBUG("after2 xp_osi_channel_restart\n");
            PDEBUG("after3 xp_osi_channel_restart\n");
            PDEBUG("after4 xp_osi_channel_restart\n");
            break;

        case XP_CHANNEL_CONTROL_DISABLE:
        /*------------------------------------------------------------+
        |  Write the "stops" register to stop any "in-process" packets
        |  from being delivered
        |  clear any pending filter operations
        |  clear the interrupts to get rid of any latent interrupts
        +------------------------------------------------------------*/
            ulData = (1 << (31 - wChannelId));

            flag = os_enter_critical_section();
            xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_QSTOPS, ulData);
            os_leave_critical_section(flag);

            xp_osi_filter_process_pending(pGlobal,wChannelId);

            wRc = xp_osi_interrupt_channel_control(pGlobal,wChannelId,
                                XP_INTERRUPT_CONTROL_RESET);
              /*------------------------------------------------------------+
              |  Disable the bucket pid
              |  Stop packets in the front-end by
              |  Setting the channel Pid to NULL
              +------------------------------------------------------------*/
            if(wChannelId == pGlobal->ChannInfo.wXpChannelBucketId)
            {
                flag = os_enter_critical_section();
                xp_atom_dcr_write_register(pGlobal->uDeviceIndex,XP_BUCKET1Q_REG_BVALID, 0);
                os_leave_critical_section(flag);
            }
            else
            {
                flag = os_enter_critical_section();
                xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,XP_PID_FILTER_REG_PIDV,
                    wChannelId, XP_CHANNEL_NULL_PID);
                os_leave_critical_section(flag);
            }

              pGlobal->ChannInfo.XpChannelData[wChannelId].Status = XP_CHANNEL_DISABLED;
              break;

        case XP_CHANNEL_CONTROL_RESET:
              /*------------------------------------------------------------+
              |  Disable the channel, and then reset the queue
              +------------------------------------------------------------*/

            if(pGlobal->ChannInfo.XpChannelData[wChannelId].Status != XP_CHANNEL_DISABLED)
            {
                xp_osi_channel_control(pGlobal,wChannelId, XP_CHANNEL_CONTROL_DISABLE);
            }

            wRc = xp_osi_queue_control(pGlobal,wChannelId, XP_QUEUE_CONTROL_RESET);

            break;
        default:
            break;
        }
    }

    PDEBUG("before return from xp_osi_channel_control\n");
//  xp_os_semaphore_signal();
    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_free
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  free an allocated channel
|
|  PROTOTYPE  :  SHORT xp0_channel_free(
|                SHORT wChannelId)
|
|  ARGUMENTS  :  wChannelId    -  id returned from xp0_channel_allocate()
|
|  RETURNS    :  zero if channel was freed, non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID
|
|  COMMENTS   :  This releases all resouwRces (queue's, filters) associated
|                with this channel id.  Data is no longer delivered for
|                this channel.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_free(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId)
{
    SHORT wRc;

//    xp_os_semaphore_wait();

    /*------------------------------------------------------------------------+
    |  Check that the channel is valid, and then disable the channel
    +------------------------------------------------------------------------*/

    wRc = xp_osi_channel_control(pGlobal,wChannelId, XP_CHANNEL_CONTROL_DISABLE);

    if(wRc == 0)
    {
    /*--------------------------------------------------------------------+
    |  Remove all interrupt notification registrations
    |  Free the queue and all filters
    +--------------------------------------------------------------------*/

        xp_osi_interrupt_channel_free(pGlobal,wChannelId);
        xp_osi_queue_free(pGlobal,wChannelId);
        xp_osi_filter_free_channel(pGlobal,wChannelId);

        pGlobal->ChannInfo.XpChannelData[wChannelId].Status = XP_CHANNEL_UNUSED;
	
	
        if(wChannelId == pGlobal->ChannInfo.wXpChannelBucketId)
          pGlobal->ChannInfo.wXpChannelBucketId = XP_CHANNEL_COUNT;

    }

//    xp_os_semaphore_signal();

    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_get_available
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_get_available(GLOBAL_RESOURCES *pGlobal, XP_CHANNEL_TYPE Type)
{
    SHORT wI;
    SHORT wCount;

//    xp_os_semaphore_wait();

    switch(Type)
    {
    case XP_CHANNEL_TYPE_AUDIO:
        wCount = (pGlobal->ChannInfo.XpChannelData[XP_CHANNEL_AUDIO].Status == XP_CHANNEL_UNUSED) ? 1 : 0;
        break;

    case XP_CHANNEL_TYPE_VIDEO:
        wCount = (pGlobal->ChannInfo.XpChannelData[XP_CHANNEL_VIDEO].Status == XP_CHANNEL_UNUSED) ? 1 : 0;
        break;

    case XP_CHANNEL_TYPE_SUBTITLE:
        wCount = (pGlobal->ChannInfo.XpChannelData[XP_CHANNEL_SUBTITLE].Status == XP_CHANNEL_UNUSED) ? 1 : 0;
        break;

    case XP_CHANNEL_TYPE_TELETEXT:
        wCount = (pGlobal->ChannInfo.XpChannelData[XP_CHANNEL_TELETEXT].Status == XP_CHANNEL_UNUSED) ? 1 : 0;
        break;

    default:
        for(wI=XP_CHANNEL_PES_MIN, wCount=0; wI<=XP_CHANNEL_PES_MAX; wI++)
        {
            if(pGlobal->ChannInfo.XpChannelData[wI].Status == XP_CHANNEL_UNUSED)
            {
                wCount++;
            }
        }
        break;
    }

//  xp_os_semaphore_signal();

    return(wCount);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_get_key
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  get a packet identifier for the channel
|
|  PROTOTYPE  :  SHORT xp0_channel_get_key(
|                SHORT wChannelId,
|                XP_CHANNEL_DESCRAMBLE *descram,
|                USHORT *keyId)
|
|  ARGUMENTS  :  wChannelId    -  id returned from xp0_channel_allocate()
|                descram       -  type of descrambling for this channel
|                keyId         -  key set number for descrambling keys
|
|  RETURNS    :  zero if successful, non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID
|
|  COMMENTS   :  The 'descram' and keyId value are returned.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_get_key(          /* get the current descrambling values  */
                          GLOBAL_RESOURCES *pGlobal,
                          SHORT wChannelId,
                          XP_CHANNEL_DESCRAMBLE *pDescram,     /* 0=ts level, 1=PES, 2=none            */
                          USHORT *pKeyId)              /* key number to use                    */
{
    SHORT     wRc;
    ULONG     ulValue;
    XP_PID_FILTER_REGP p;
    UINT32  flag;

//    xp_os_semaphore_wait();

    wRc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(wRc == 0)
    {
        flag = os_enter_critical_section();
        ulValue = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_BASE_PID + wChannelId);
        os_leave_critical_section(flag);

        p = (XP_PID_FILTER_REGP)(void *)&ulValue;

        if(p->denbl == 0)
        {
            *pDescram = XP_DESCRAMBLE_OFF;
        }
        else if(p->pesl == 0)
        {
            *pDescram = XP_DESCRAMBLE_TS;
        }
        else
        {
            *pDescram = XP_DESCRAMBLE_PES;
        }

        *pKeyId = p->kid;
    }

//    xp_os_semaphore_signal();

    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_get_pid
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  get a packet identifier for the channel
|
|  PROTOTYPE  :  SHORT xp0_channel_get_pid(
|                SHORT wChannelId,
|                USHORT *p_pid)
|
|  ARGUMENTS  :  wChannelId    -  id returned from xp0_channel_allocate()
|                pid           -  packet identifier
|
|  RETURNS    :  zero if successful, non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID
|
|  COMMENTS   :  The pid value is returned at the location pointed to by
|                the p_pid parameter.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_get_pid(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, USHORT *pPid)
{
    SHORT wRc;

//    xp_os_semaphore_wait();

    wRc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(wRc == 0)
    {
        *pPid = pGlobal->ChannInfo.XpChannelData[wChannelId].uwPid;
    }

//    xp_os_semaphore_signal();

    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_get_unload_type
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  query the type of data to unload on this channel
|
|  PROTOTYPE  :  SHORT xp0_channel_get_unload_type(
|                SHORT wChannelId,
|                XP_CHANNEL_UNLOAD_PTR p_unload_type)
|
|  ARGUMENTS  :  wChannelId    -  the channel id as returned from the
|                                 xp_channel_allocate().
|                unload_type   -  type of data to unload
|
|  RETURNS    :  0 if successful, or non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID      -  channel id is not defined
|                XP_ERROR_CHANNEL_UNDEFINED_UT -  unload type not defined
|
|  COMMENTS   :  defines the type of data to unload for the channel.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_get_unload_type(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,
                                     XP_CHANNEL_UNLOAD_TYPE *pUnloadType)
{
    SHORT wRc;

//    xp_os_semaphore_wait();

    wRc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(wRc == 0)
    {
        if(pGlobal->ChannInfo.XpChannelData[wChannelId].UnloadType ==
            XP_CHANNEL_UNLOAD_UNDEFINED)
        {
           wRc = XP_ERROR_CHANNEL_UNDEFINED_UT;
        }
    }

    if(wRc == 0) {
        *pUnloadType = pGlobal->ChannInfo.XpChannelData[wChannelId].UnloadType;
    }

//    xp_os_semaphore_signal();

    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_set_key
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  set up a packet identifier for the channel
|
|  PROTOTYPE  :  SHORT xp0_channel_set_key(
|                SHORT wChannelId,
|                XP_CHANNEL_DESCRAMBLE descram,
|                USHORT keyId)
|
|  ARGUMENTS  :  wChannelId    -  id returned from xp0_channel_allocate()
|                descram       -  type of UnloadTypebling for this channel
|                keyId         -  key set number for descrambling keys
|
|  RETURNS    :  zero if successful, non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID
|
|  COMMENTS   :  The xp0_channel_set_key() defines the descrambling type
|                and key set associated with the channel.  The values of
|                this API take effect on the channel with the next call
|                to the xp0_channel_set_pid() function for the same channel
|                id.   The 'descram' parameter defines the type of
|                hardware descrambling available.  The following values
|                are available:
|                     XP_DESCRAMBLE_TS   -  descramble transport packets
|                     XP_DESCRAMBLE_PES  -  descramble pes portion of packets
|                     XP_DESCRAMBLE_OFF  -  no descrambling required
|                The keyId refers to a key set which is setup using the
|                xp0_key_set() or xp0_key_setall() API's.
|
+----------------------------------------------------------------------------*/
SHORT xp_channel_set_key(          /* set the next scrambling level        */
                         GLOBAL_RESOURCES *pGlobal,
                         SHORT wChannelId,
                         XP_CHANNEL_DESCRAMBLE Descram,      /* 0=ts level, 1=PES, 2=none            */
                         USHORT uwKeyId)               /* key number to use                    */
{
    SHORT wRc;
//    xp_os_semaphore_wait();


    if(((SHORT) uwKeyId < 0) || (uwKeyId >= XP_KEY_INDEX_COUNT))
    {
        wRc = XP_ERROR_KEY_INDEX;
    }
    else
    {
        wRc = xp_osi_channel_valid(pGlobal,wChannelId);
    }

    if(wRc == 0)
    {
        pGlobal->ChannInfo.XpChannelData[wChannelId].Descram = Descram;
        pGlobal->ChannInfo.XpChannelData[wChannelId].uwKeyId = uwKeyId;
    }

//    xp_os_semaphore_signal();

    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_set_notification_fn
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  register a callback function when data is available
|
|  PROTOTYPE  :  SHORT xp0_channel_set_notification_fn(
|                SHORT wChannelId,
|
|
|                typedef void (*XP_CHANNEL_NOTIFY_FN)
|
|
|                typedef struct xp0_channel_notify_data {
|                    SHORT wChannelId;
|                    ULONGmatch_word;
|                    unsigned char *data;
|                    ULONGlength;
|                } XP_CHANNEL_NOTIFY_DATA;
|
|  ARGUMENTS  :  wChannelId    -  the channel id as returned from the
|                                 xp0_channel_allocate().
|                notify_fn     -  address of the callback function
|
|  RETURNS    :  0 if successful, or non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel id is not defined
|
|  COMMENTS   :  registers a function which is called when data for the
|                channel has been delivered to DRAM.  The data structure
|                XP_CHANNEL_NOTIFY_DATA is passed as a parameter to the
|                callback function and provides:
|                      wChannelId      - channel containing the data
|                      match_word      - filter group id's which matched
|                      data            - pointer to data in DRAM
|                      length          - length of the available data
|                Note:  the match_word is 0 for all types of data unloaded
|                except the filtered table sections.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_set_notification_fn(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,
                                         XP_CHANNEL_NOTIFY_FN notify_fn)
{
    SHORT wRc;
    ULONG ulMask;
    XP_INTERRUPT_CHANNEL_FN callback_fn;

//    xp_os_semaphore_wait();

    wRc = xp_osi_channel_valid(pGlobal,wChannelId);

    /*------------------------------------------------------------------------+
    |  Make sure the unload type is defined first
    +------------------------------------------------------------------------*/
    if(wRc == 0)
    {
        if (pGlobal->ChannInfo.XpChannelData[wChannelId].UnloadType ==
            XP_CHANNEL_UNLOAD_UNDEFINED)
        {
            wRc = XP_ERROR_CHANNEL_UNDEFINED_UT;
        }
    }

    if (wRc == 0)
    {
        if(pGlobal->ChannInfo.XpChannelData[wChannelId].UnloadType < 8)
        {
            if(pGlobal->ChannInfo.XpChannelData[wChannelId].UnloadType == XP_CHANNEL_UNLOAD_TRANSPORT)
              ulMask = XP_INTERRUPT_QSTAT_RPI | XP_INTERRUPT_QSTAT_BTI;
            else
              ulMask = CHANNEL_NOTIFY_STREAM_MASK;
        }
        else
        {
            ulMask = CHANNEL_NOTIFY_SECTION_MASK;
        }

        /*--------------------------------------------------------------------+
        |  If there is a notification function specified, then setup the
        |  interrupts to receive notification when table section is available
        +--------------------------------------------------------------------*/
        pGlobal->ChannInfo.XpChannelData[wChannelId].notify_fn = notify_fn;
        callback_fn = (XP_INTERRUPT_CHANNEL_FN) channel_data_notify;

        if(notify_fn == (XP_CHANNEL_NOTIFY_FN) NULL)
        {
            xp_osi_interrupt_channel_notify(pGlobal,XP_INTERRUPT_NOTIFY_DELETE,
            wChannelId, ulMask, callback_fn);
        }
        else
        {
            xp_osi_interrupt_channel_notify(pGlobal,XP_INTERRUPT_NOTIFY_ADD,
            wChannelId,ulMask, callback_fn);
        }
    }

//    xp_os_semaphore_signal();

    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_set_pid
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  set up a packet identifier for the channel
|
|  PROTOTYPE  :  SHORT xp0_channel_set_pid(
|                SHORT wChannelId,
|                USHORT pid)
|
|  ARGUMENTS  :  wChannelId    -  id returned from xp0_channel_allocate()
|                pid           -  packet identifier
|
|  RETURNS    :  zero if successful, non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID
|
|  COMMENTS   :  The xp0_channel_set_pid() updates the current pid value
|                associated with the channel.  If the channel was enabled
|                using the xp0_channel_control() function, then the new
|                pid is written to the transport demux hardware.  If the
|                channel is not currently enabled, the pid value is
|                retained and written to hardware when the channel is
|                enabled.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_set_pid(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, USHORT uwPid)
{
    SHORT               wRc;
    SHORT               wChid;
    ULONG               ulAddr;
    ULONG               ulValue=0;
    XP_PID_FILTER_REGP  p;
    UINT32              flag;

//    xp_os_semaphore_wait();

    /*------------------------------------------------------------------------+
    |  Map the channel change Pid to a real pid
    +------------------------------------------------------------------------*/
    if(wChannelId == XP_CHANNEL_CC_AUDIO)
    {
        wChid = XP_CHANNEL_AUDIO;
        ulAddr = XP_DCR_ADDR_ACCHNG;
    }
    else if(wChannelId == XP_CHANNEL_CC_VIDEO)
    {
        wChid = XP_CHANNEL_VIDEO;
        ulAddr = XP_DCR_ADDR_VCCHNG;
    }
    else
    {
        wChid = wChannelId;
        ulAddr = XP_DCR_ADDR_BASE_PID + wChid;
    }
    wRc = xp_osi_channel_valid(pGlobal,wChid);

    if(wRc == 0)
    {
        p       = (XP_PID_FILTER_REGP)(void *)&ulValue;
        p->kid  = pGlobal->ChannInfo.XpChannelData[wChid].uwKeyId;
        p->pidv = (pGlobal->ChannInfo.XpChannelData[wChid].Status == XP_CHANNEL_ENABLED)
                   ? uwPid : XP_CHANNEL_NULL_PID;

        switch(pGlobal->ChannInfo.XpChannelData[wChid].Descram)
        {
        case XP_DESCRAMBLE_TS:
            p->denbl = 1;
            p->pesl  = 0;
            break;
        case XP_DESCRAMBLE_PES:
            p->denbl = 1;
            p->pesl  = 1;
            break;
        default:
            p->denbl = 0;
            p->pesl  = 0;
            break;
        }

        pGlobal->ChannInfo.XpChannelData[wChid].uwPid = uwPid;

        flag = os_enter_critical_section();
        xp_atom_dcr_write(pGlobal->uDeviceIndex,ulAddr, ulValue);
        os_leave_critical_section(flag);
    }
//    xp_os_semaphore_signal();

    return(wRc);
}

/*----------------------------------------------------------------------------+
|  xp0_channel_set_unload_type
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  define the type of data to unload on this channel
|
|  PROTOTYPE  :  SHORT xp0_channel_set_unload_type(
|                SHORT wChannelId,
|                XP_CHANNEL_UNLOAD_TYPE unload_type)
|
|  ARGUMENTS  :  wChannelId    -  the channel id as returned from the
|                                 xp0_channel_allocate().
|                unload_type   -  type of data to unload
|
|                    XP_CHANNEL_UNLOAD_TRANSPORT
|                    XP_CHANNEL_UNLOAD_ADAPTATION
|                    XP_CHANNEL_UNLOAD_ADAPTATION_PRIVATE
|                    XP_CHANNEL_UNLOAD_PAYLOAD
|                    XP_CHANNEL_UNLOAD_PAYLOAD_AND_BUCKET
|                    XP_CHANNEL_UNLOAD_BUCKET
|                    XP_CHANNEL_UNLOAD_PSI
|                    XP_CHANNEL_UNLOAD_FILTER_PSI
|                    XP_CHANNEL_UNLOAD_PSI_CRC
|                    XP_CHANNEL_UNLOAD_FILTER_PSI_CRC
|                    XP_CHANNEL_UNLOAD_PSI_BUCKET
|                    XP_CHANNEL_UNLOAD_FILTER_PSI_BUCKET
|                    XP_CHANNEL_UNLOAD_PSI_CRC_BUCKET
|                    XP_CHANNEL_UNLOAD_FILTER_PSI_CRC_BUCKET
|
|  RETURNS    :  0 if successful, or non-zero if an error occurs
|
|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel id is not defined
|
|  COMMENTS   :  defines the type of data to unload for the channel.  The
|                application should disable the channel before changing
|                the unload type.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_channel_set_unload_type(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,
                                     XP_CHANNEL_UNLOAD_TYPE UnloadType)
{
    SHORT wRc;
    UINT32  flag;

//    xp_os_semaphore_wait();

    wRc = xp_osi_channel_valid(pGlobal,wChannelId);
    if(wRc == 0)
    {
        if(( (int)UnloadType < XP_CHANNEL_UNLOAD_TRANSPORT) ||
             (UnloadType >= XP_CHANNEL_UNLOAD_UNDEFINED))
        {
            wRc = XP_ERROR_CHANNEL_UNDEFINED_UT;
        }
    }
    if(wRc == 0)
    {
        if(pGlobal->ChannInfo.XpChannelData[wChannelId].Status == XP_CHANNEL_ENABLED)
        {
            wRc = XP_ERROR_CHANNEL_ENABLED;
        }
    }
    if(wRc == 0)
    {
        flag = os_enter_critical_section();
        xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex, XP_QCONFIGB_REG_DTYPE,
        wChannelId, UnloadType);
        os_leave_critical_section(flag);

        pGlobal->ChannInfo.XpChannelData[wChannelId].UnloadType = UnloadType;
    }

//    xp_os_semaphore_signal();

    return(wRc);

}
