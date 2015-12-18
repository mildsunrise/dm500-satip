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
|
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|
|  Author    :  Lin Guo Hui
|  File      :  xp_osd_drv.c
|  Purpose   :  The set of functions provide a interface to Linux user
|               space, as os dependent layer
|  Changes   :
|  Date         Comments
|  ----------------------------------------------------------------------
|  25-Jun-2001  Created
|  06-Jul-2001  Add audio and video PES reading feature
|  24-Jul-2001  Fix the semaphore bug, and add demux_channel_free() when
|               DmxFilterAlloc() fail. Add xp_os_enter_critical_section()
|               in DmxFilterRead()
|  30-Sep-2001  Updated for Pallas
|  10-Oct-2001  Add ts_res set and select source
|  08-Jan-2002  Fix filter length bug when section filtering
|  10-Jan-2002  Add PCR sync when enable filterring PCR
|  25-Mar-2002  Allow two and more section filters added to a channel
|  10-Apil-02   Add positive enable in section filter para, add get
|               filter num ioctl, for PLR
|  11-May-2002  Change the buffer to circle queue for section filter
|  02-Jun-2002  Add STC event
|  18-jun-2002  Support VESTA and VULCAN
|  19-Jun-2002  Add notification when the filter is stoped to avoid reading
|               waiting forever
|  12-aug-2002  Add condition(pDmxfilter->chid != ILLEGAL_CHANNEL) for the
|               operation chid[pDmxfilter->chid].inuse++
|  17-aug-2002  Fix the bug foe queue management when multiple filters
|               attached to on queue
|  22-Jul-2003  Corrected problem such that filters with the same PID and
|               channel can be opened, set, and started in any order instead
|               of in-sequence.
|  22-Jul-2003  Removed all "demux_" functions to eliminate similarity to
|               proprietary API definitions.
+----------------------------------------------------------------------------*/
#include <linux/config.h>
#include <linux/version.h>
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/kernel.h>
#define    __NO_VERSION__

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/ptrace.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/in.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/irq.h>

#include "xp_osi_global.h"
#include "xp_atom_reg.h"
#include "xp_osd_drv.h"

/*----------------------------------------------------------------------------+
| Local Defines
+----------------------------------------------------------------------------*/
#ifdef PDEBUG
#undef PDEBUG
#endif
#define PDEBUG(fmt,args...)
#define PDEBUG1(fmt,args...) 

/*----------------------------------------------------------------------------+
| Type Declarations
+----------------------------------------------------------------------------*/
typedef struct demux_channel_acquire_type {
    XP_CHANNEL_TYPE         channel_type;   /* channel type                  */
    XP_CHANNEL_UNLOAD_TYPE  unload_type;    /* type of data to unload        */
    XP_CHANNEL_NOTIFY_FN    notify_data_fn; /* func to call when data recv   */
    unsigned short          bthreshold;     /* boundary threshold            */
    unsigned short          pid;            /* program id                    */
    unsigned long           queue_size;     /* queue size                    */
} DEMUX_CHANNEL_ACQUIRE_TYPE, *DEMUX_CHANNEL_ACQUIRE_PTR;

/*----------------------------------------------------------------------------+
| Static Declarations
+----------------------------------------------------------------------------*/
static INT uAlreadyInit = 0;
static INT DEFAULT_FILTER_LENGTH = 9;

static void vma_open(struct vm_area_struct *vma);
static void vma_close(struct vm_area_struct *vma);
static struct vm_operations_struct my_vm_ops =
{
    open:       vma_open,
    close:      vma_close
};

/*----------------------------------------------------------------------------+
| Global Declarations
+----------------------------------------------------------------------------*/
DEMUX_DEVICE     *pDemux_dev[MAX_XP_NUM];
GLOBAL_RESOURCES XpGlobal[MAX_XP_NUM];
MUTEX_T          hMutex;

/*----------------------------------------------------------------------------+
| External Declarations
+----------------------------------------------------------------------------*/
extern XP_STC_NOTIFY_FN stc_notify_fn;

/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/
static DECLARE_WAIT_QUEUE_HEAD(stc_WaitQ);
DECLARE_WAIT_QUEUE_HEAD(temp_queue);

/*----------------------------------------------------------------------------+
|  XX   XX   XXXXXX   XXXXXX   XXXX   XXXXXXX  XX   XXX
|  XXX  XX   XX  XX   X XX X    XX     XX       XX  XX
|  XXXX XX   XX  XX     XX      XX     XX        XXXX
|  XX XXXX   XX  XX     XX      XX     XXXX       XX
|  XX  XXX   XX  XX     XX      XX     XX         XX
|  XX   XX   XX  XX     XX      XX     XX         XX
|  XX   XX   XXXXXX    XXXX    XXXX   XXXX        XX
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|  DemuxGetQPBase
+----------------------------------------------------------------------------*/
static UCHAR *DemuxGetQPBase(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter,
   UCHAR            *plBuffer)

{
   UCHAR *ppBQueue;
   UCHAR *ppBuffer;
   UCHAR *plBQueue;

   plBQueue = (UCHAR*)os_get_logical_address(
              pGlobal->QueueInfo.XpQueueChData[pDmxfilter->chid].hMem);
   ppBQueue = (UCHAR*)os_get_physical_address(
              pGlobal->QueueInfo.XpQueueChData[pDmxfilter->chid].hMem);
   ppBuffer = (UCHAR*)((ULONG)ppBQueue + (ULONG)plBuffer  - (ULONG)plBQueue);

   return(ppBuffer);
}

/*----------------------------------------------------------------------------+
|  DemuxCopyToCBuf
+----------------------------------------------------------------------------*/
static void DemuxCopyToCBuf(
   DEMUX_FILTER_BUF        *pDmxfbuf,
   XP_CHANNEL_NOTIFY_DATA  *pInfo)

{
   ULONG       BErem, QErem;
   ULONG       nulWrite;
   UCHAR       *pBloc, *pQloc;
   UCHAR       *pBstart, *pQstart;

   /*-------------------------------------------------------------------------+
   |  Determine Queue and Circular Buffer Start and Wrap point
   +-------------------------------------------------------------------------*/
   nulWrite = (pDmxfbuf->ulWrite + 1) % pDmxfbuf->ulSize;
   pBstart  = pDmxfbuf->plData;
   pQstart  = pDmxfbuf->plBQueue;
   pBloc    = &pDmxfbuf->plData[nulWrite];
   pQloc    = pInfo->plData;
   BErem    = pDmxfbuf->ulSize   - nulWrite;
   QErem    = pDmxfbuf->plEQueue - pInfo->plData;

  /*-------------------------------------------------------------------------+
   |  Transfer Transport Queue Data to Internal Circular Buffer
   |  Manage Wrap on both Circular Queues
   +-------------------------------------------------------------------------*/
   if ((BErem < pInfo->ulLength) && (QErem < pInfo->ulLength)) {
      if (QErem > BErem) {
         memcpy(&pBloc[0],     &pQloc[0],     BErem);
         memcpy(&pBstart[0],   &pQloc[BErem], QErem-BErem);
         memcpy(&pBstart[QErem-BErem], &pQstart[0], pInfo->ulLength-QErem);
      } else if (BErem > QErem) {
         memcpy(&pBloc[0],     &pQloc[0],     QErem);
         memcpy(&pBloc[QErem], &pQstart[0],   BErem-QErem);
         memcpy(&pBstart[0],   &pQstart[BErem-QErem], pInfo->ulLength-BErem);
      } else {
         memcpy(&pBloc[0],     &pQloc[0],     BErem);
         memcpy(&pBstart[0],   &pQstart[0],   pInfo->ulLength-BErem);
      }
   } else if (BErem < pInfo->ulLength) {
         memcpy(&pBloc[0],     &pQloc[0],     BErem);
         memcpy(&pBstart[0],   &pQloc[BErem], pInfo->ulLength-BErem);
   } else if (QErem < pInfo->ulLength) {
         memcpy(&pBloc[0],     &pQloc[0],     QErem);
         memcpy(&pBloc[QErem], &pQstart[0],   pInfo->ulLength-QErem);
   } else {
         memcpy(&pBloc[0],     &pQloc[0],     pInfo->ulLength);
   }

   /*-------------------------------------------------------------------------+
   |  Bump Internal Circular Buffer pointers
   +-------------------------------------------------------------------------*/
   pDmxfbuf->count   = pDmxfbuf->count + pInfo->ulLength;
   pDmxfbuf->ulWrite = (pDmxfbuf->ulWrite + pInfo->ulLength) % pDmxfbuf->ulSize;
   return;
}

/*----------------------------------------------------------------------------+
|  DemuxSectioncallback
+----------------------------------------------------------------------------*/
static void DemuxSectionCallback(
   XP_CHANNEL_NOTIFY_DATA * pInfo)

{
   int                i;
   int                j;
   int                MatchCount = 0;
   int                match[32];
   ULONG              bytes_available;
   ULONG              notify_length;
   unsigned char      *data;
   unsigned char      *b_data;
   unsigned char      *e_data;
   unsigned char      *s;
   UCHAR              *ppBuffer;
   DEMUX_FILTER       *pDmxfilter=NULL;
   SECTION_HEADER_PTR SectionHeader;
   GLOBAL_RESOURCES   *pGlobal;
   DEMUX_DEVICE       *pDemuxDev;


   PDEBUG1("DemuxSectioncallback(): Entering\n");

   pGlobal = pInfo->pGlobal;
   pDemuxDev = pDemux_dev[pGlobal->uDeviceIndex];

   //Find the filter which matched the recieved section
   for (i = 0; i < pDemuxDev->uFilterNum; i++)
   {
      if (pDemuxDev->filter[i].chid == pInfo->wChannelId &&
         ((pDemuxDev->filter[i].ulMatchWord & pInfo->ulMatchWord) ==
           pDemuxDev->filter[i].ulMatchWord))
      {
         match[MatchCount] = i;
         MatchCount++;
      }
   }

   if (!MatchCount)
   {
      printk("DemuxSectioncallback(): Error - no filter matched section received\n");
      return;
   }

   for(j=0;j<MatchCount;j++)
   {
      i = match[j];
      pDmxfilter = (DEMUX_FILTER *) (&pDemuxDev->filter[i]);

      //If received section data 's size is smaller than 3 bytes, ehich is the minimum byte
      //of the section. retun
      if (pInfo->ulLength < 3)
      {
         ppBuffer = DemuxGetQPBase(pGlobal,pDmxfilter,pInfo->plData);
         xp_osi_queue_unlock_data(pGlobal, pDmxfilter->chid,
                (UCHAR *)ppBuffer, (ULONG)pInfo->ulLength);
         printk("DemuxSectioncallback(): Error - notify length =0\n");
         return;
      }

      //bytes_availbale is the size of the received data
      bytes_available = pDmxfilter->buffer.plEQueue - pInfo->plData;
      data = pInfo->plData;
      SectionHeader = (SECTION_HEADER_PTR) (data);

      //Notify_length is the size of a complete table section
      notify_length = SectionHeader->sectionLength + 3;

      if (SectionHeader->sectionLength == 0)
      {
         ppBuffer = DemuxGetQPBase(pGlobal,pDmxfilter,pInfo->plData);
         xp_osi_queue_unlock_data(pGlobal, pDmxfilter->chid,
                (UCHAR *) ppBuffer, (ULONG) pInfo->ulLength);
         printk("DemuxSectioncallback(): Error - Section Filter length =0\n");
         return;
      }

      if (notify_length > pInfo->ulLength)
      {
         ppBuffer = DemuxGetQPBase(pGlobal,pDmxfilter,pInfo->plData);
         xp_osi_queue_unlock_data(pGlobal,pDmxfilter->chid,
                (UCHAR *)ppBuffer, (ULONG)pInfo->ulLength);
         printk("DemuxSectioncallback(): Warning - notify length > available_length\n");
         notify_length = pInfo->ulLength;
         return;
      }

      b_data = pInfo->plData;
      if (notify_length >= bytes_available)
      {
         e_data = pDmxfilter->buffer.plBQueue + (notify_length - bytes_available);
      } else {
         e_data = pInfo->plData + notify_length;
      }

      pDmxfilter->ulNotifySize = notify_length;

      //special process when the queue is wrap
      if (pDmxfilter->buffer.plData)
      {
         if(pDmxfilter->buffer.count + notify_length >= pDmxfilter->buffer.ulSize)
         {
            printk("DemuxSectioncallback(): Error - Buffer is full\n");
         } else {
            for (i = 0, s = b_data; i < notify_length; i++, s++)
            {
               if (s == pDmxfilter->buffer.plEQueue)
               {
                  s = pDmxfilter->buffer.plBQueue;
               }
               pDmxfilter->buffer.count++;
               pDmxfilter->buffer.ulWrite =
                  (pDmxfilter->buffer.ulWrite + 1)%pDmxfilter->buffer.ulSize;
               pDmxfilter->buffer.plData[pDmxfilter->buffer.ulWrite] = *s;
            }
         }
      }

      if(pDmxfilter->async_queue != NULL)
        kill_fasync(&pDmxfilter->async_queue, SIGIO, POLL_IN);

      // unlock queue
      wake_up_interruptible(&pDmxfilter->buffer.queue);
   }

   ppBuffer = DemuxGetQPBase(pGlobal,pDmxfilter,pInfo->plData);
   xp_osi_queue_unlock_data(pGlobal,pDmxfilter->chid,
              (UCHAR *)ppBuffer, (ULONG)pInfo->ulLength);

   return;
}

/*----------------------------------------------------------------------------+
|  DemuxPESCallback
+----------------------------------------------------------------------------*/
static void DemuxPESCallback(
   XP_CHANNEL_NOTIFY_DATA * pInfo)

{
   int              i;
   ULONG            BTrem;
   UCHAR            *ppBuffer;
   DEMUX_FILTER     *pDmxfilter;
   DEMUX_DEVICE     *pDemuxDev;
   GLOBAL_RESOURCES *pGlobal;


   PDEBUG1("DemuxPESCallback(): Entering\n");

   /*-------------------------------------------------------------------------+
   |  Find Filter Associated with PES Callback
   +-------------------------------------------------------------------------*/
   pGlobal = pInfo->pGlobal;
   pDemuxDev = pDemux_dev[pGlobal->uDeviceIndex];
   for (i = 0; i < pDemuxDev->uFilterNum; i++) {
      if (pDemuxDev->filter[i].chid == pInfo->wChannelId) {
         break;
      }
   }

   if (i == pDemuxDev->uFilterNum) {
      printk("DemuxPESCallback(): Error - Cannot find filter for PES Callback\n");
      return;
   }

   /*-------------------------------------------------------------------------+
   |  Get No. of Free Bytes and check for Internal Circular Buffer Overflow
   +-------------------------------------------------------------------------*/
   pDmxfilter = (DEMUX_FILTER *) (&pDemuxDev->filter[i]);
   ppBuffer = DemuxGetQPBase(pGlobal,pDmxfilter,pInfo->plData);
   BTrem = pDmxfilter->buffer.ulSize - pDmxfilter->buffer.count;

   if (BTrem < pInfo->ulLength) {
       xp_osi_queue_unlock_data(pGlobal,pDmxfilter->chid,
              (UCHAR *)ppBuffer, (ULONG)pInfo->ulLength);
       printk("DemuxPESCallback(): Error - Internal Buffer is full for chid=%d\n",
               pDmxfilter->chid);
       return;
   }

   /*-------------------------------------------------------------------------+
   |  Transfer data in Transport Queue to Internal Circular Buffer
   +-------------------------------------------------------------------------*/
   DemuxCopyToCBuf(&pDmxfilter->buffer, pInfo);

   /*-------------------------------------------------------------------------+
   | Unlock Transport Queue
   +-------------------------------------------------------------------------*/
   xp_osi_queue_unlock_data(pGlobal,pDmxfilter->chid,
                           (UCHAR *)ppBuffer, (ULONG)pInfo->ulLength);

   /*-------------------------------------------------------------------------+
   |  Wake-up Signal Handler
   +-------------------------------------------------------------------------*/
   if(pDmxfilter->async_queue != NULL) {
      kill_fasync(&pDmxfilter->async_queue, SIGIO, POLL_IN);
   }

   /*-------------------------------------------------------------------------+
   |  Wake-up Blocked Reads waiting for Data
   +-------------------------------------------------------------------------*/
   wake_up_interruptible(&pDmxfilter->buffer.queue);

   return;
}

/*----------------------------------------------------------------------------+
|  DemuxCallback_nb
+----------------------------------------------------------------------------*/
static void DemuxCallback_nb(
   GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,ULONG ulInterrupt)

{
   int              i;
   DEMUX_FILTER     *pDmxfilter;
   DEMUX_DEVICE     *pDemuxDev;
   QUEUE_PTR        pQue;     /* channel information          */
   ULONG            writep;
   ULONG            ppBQueue;
   ULONG            ppEQueue;
   ULONG            rp;


   if(ulInterrupt & XP_INTERRUPT_QSTAT_RPI)
   {
     printk("DemuxCallback_nb: RPI interrupt\n");
   }

   if(~ulInterrupt & XP_INTERRUPT_QSTAT_BTI)
     return;
     
   pDemuxDev = pDemux_dev[pGlobal->uDeviceIndex];
   for (i = 0; i < pDemuxDev->uFilterNum; i++)
   {
      if (pDemuxDev->filter[i].chid == wChannelId)
      {
         break;
      }
   }

   if (i == pDemuxDev->uFilterNum)
   {
      printk("DemuxCallback_nb(): Error - Cannot find filter for Callback\n");
      return;
   }

   pDmxfilter = (DEMUX_FILTER *) (&pDemuxDev->filter[i]);

   pQue = &pGlobal->QueueInfo.XpQueueChData[pDmxfilter->chid];
   if(pQue->hMem == 0)
   {
     printk("DemuxCallback_nb: queue not allocated");
     return;
   }

   ppBQueue = (ULONG)(pQue->ulQueueAddr) & 0x00FFFF00;
   ppEQueue = ppBQueue + pQue->ulQueueSize;

   xp_atom_dcr_read_register_channel(pGlobal->uDeviceIndex,XP_QSTATD_REG_WSTART,
                                     pDmxfilter->chid, &writep);
  
   xp_atom_dcr_read_register_channel(pGlobal->uDeviceIndex,XP_QCONFIGB_REG_RPTR, 
                                     pDmxfilter->chid, &rp);
				     
   writep = writep & 0x00FFFF00;
   
   if(writep < ppBQueue || writep > ppEQueue)
     return;

   if(writep == ppEQueue)
     writep = ppBQueue;
          
   pDmxfilter->buffer.ulWrite = writep - ppBQueue;
     
   if(pDmxfilter->async_queue != NULL)
     kill_fasync(&pDmxfilter->async_queue, SIGIO, POLL_IN);
       
   wake_up_interruptible(&pDmxfilter->buffer.queue);

   return;
}

/*----------------------------------------------------------------------------+
| XXXX    XXXXX    XXXXXXX
|  XX    XX   XX    X   XX
|  XX     XX        XX  XX
|  XX       XX      XXXXX
|  XX        XX     XX XX
|  XX    XX   XX    XX  XX
| XXXX    XXXXX    XXX  XX
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|  xp_isr
+----------------------------------------------------------------------------*/
static void xp_isr(
   int            irq,
   void           *dev_id,
   struct pt_regs *regs)

{
   xp_osi_interrupt();
}

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
|  DemuxStcEvent
+----------------------------------------------------------------------------*/
static void DemuxStcEvent(
   GLOBAL_RESOURCES *pGlobal,
   XP_STC_EVENT     stc_event)

{
   PDEBUG("DemuxStcEvent(): Entering\n");

   pGlobal->stc_event = stc_event;
   wake_up_interruptible(&stc_WaitQ);
}

/*----------------------------------------------------------------------------+
|  DemuxSelectSource
+----------------------------------------------------------------------------*/
static int DemuxSelectSource(
   GLOBAL_RESOURCES *pGlobal,
   STREAM_SOURCE    source)

{
#ifndef __DRV_FOR_VESTA__
   XP_CONFIG_VALUES Config;

   PDEBUG("DemuxSelectSource(): Entering - source=%d\n", source);

   switch(source)
   {
      case INPUT_FROM_CHANNEL0:
         os_get_mutex(hMutex);
         xp_osi_get_config_values(pGlobal,&Config);
         Config.insel = 0;
         xp_osi_set_config_values(pGlobal,&Config);
         os_release_mutex(hMutex);
         break;

#ifdef __DRV_FOR_PALLAS__
      case INPUT_FROM_CHANNEL1:
         os_get_mutex(hMutex);
         xp_osi_get_config_values(pGlobal,&Config);
         Config.insel = 1;
         xp_osi_set_config_values(pGlobal,&Config);
         os_release_mutex(hMutex);
         break;

      case INPUT_FROM_1394:
         os_get_mutex(hMutex);
         xp_osi_get_config_values(pGlobal,&Config);
         Config.insel = 2;
         xp_osi_set_config_values(pGlobal,&Config);
         os_release_mutex(hMutex);
         break;
#endif

      case INPUT_FROM_PVR:
         if (pGlobal->uDeviceIndex != 0)
         {
            return(-1);
         }
         os_get_mutex(hMutex);
         xp_osi_get_config_values(pGlobal,&Config);
         Config.insel = 3;
         xp_osi_set_config_values(pGlobal,&Config);
         os_release_mutex(hMutex);
         break;

      default:
         return(-1);
   }

#endif
   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterStop
+----------------------------------------------------------------------------*/
static int DemuxFilterStop(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   int              rc=0;
   ULONG            reg;
   ULONG            flag;
   XP_BUCKET1Q_REGP pBucket;
   DEMUX_DEVICE     *pDemuxDev;


   PDEBUG("DemuxFilterStop(): Entering - pDmxfilter->type=%d\n", pDmxfilter->type);
   pDemuxDev = pDemux_dev[pGlobal->uDeviceIndex];

   if (pDmxfilter->states < FILTER_STAT_START)
   {
      return(0);
   }

   switch (pDmxfilter->type)
   {
      case FILTER_TYPE_SEC:
         os_get_mutex(hMutex);
         rc = xp_osi_filter_control(pGlobal,(short)pDmxfilter->fid,
              XP_FILTER_CONTROL_DISABLE);
         os_release_mutex(hMutex);
         if (rc)
         {
            printk("DemuxFilterStop(): Error disabling filter\n");
            return(-1);
         }

         if ((pDmxfilter->chid != ILLEGAL_CHANNEL) &&
             (pDemuxDev->chid[pDmxfilter->chid].inuse == 1))
         {
            os_get_mutex(hMutex);
            rc = xp_osi_channel_control(pGlobal,(short)pDmxfilter->chid,
                 XP_CHANNEL_CONTROL_DISABLE);
            os_release_mutex(hMutex);
            if (rc)
            {
               printk("DemuxFilterStop(): Error disabling Channel\n");
               return(-1);
            }
            pDemuxDev->chid[pDmxfilter->chid].state = XP_CHANNEL_DISABLED;
         }
         break;

      case FILTER_TYPE_PES:
         if (pDmxfilter->pes_para.pesType == DMX_PES_PCR)
         {
            os_get_mutex(hMutex);
            xp_osi_pcr_cchan_auto(pGlobal,0x1fff);
            os_release_mutex(hMutex);

            if (stc_notify_fn != NULL)
            {
               (*stc_notify_fn)(pGlobal,STC_REMOVED);
            }
            break;
         }

         if (pDmxfilter->chid != ILLEGAL_CHANNEL)
         {
            os_get_mutex(hMutex);
            rc = xp_osi_channel_control(pGlobal,(short)pDmxfilter->chid,
                 XP_CHANNEL_CONTROL_DISABLE);
            os_release_mutex(hMutex);
            if (rc)
            {
               printk("DemuxFilterStop(): Error disabling PES channel\n");
               return(-1);
            }
         }
         break;

      case FILTER_TYPE_TS:
         if (pDmxfilter->chid != ILLEGAL_CHANNEL)
         {
            os_get_mutex(hMutex);
            rc = xp_osi_channel_control(pGlobal,(short)pDmxfilter->chid,
                 XP_CHANNEL_CONTROL_DISABLE);
            os_release_mutex(hMutex);
            if (rc)
            {
               printk("DemuxFilterStop(): Error disabling TS channel\n");
               return(-1);
            }
            xp_osi_stop_parse_bypass(pGlobal);
         }
         break;

      case FILTER_TYPE_BUCKET:
         reg = 0;
         pBucket = (void *)&reg;
         pBucket->bqdt   = 0;
         pBucket->bvalid = 0;
         pBucket->indx   = ILLEGAL_CHANNEL;

         flag = os_enter_critical_section();
         xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_BUCKET1Q,reg);
         os_leave_critical_section(flag);
         break;

      default:
         return(-EINVAL);
   }

   pDmxfilter->states = FILTER_STAT_READY;

   //wake up thread sleeping on that queue
   wake_up_interruptible(&pDmxfilter->buffer.queue);

   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterStart
+----------------------------------------------------------------------------*/
static int DemuxFilterStart(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   int              rc=0;
   ULONG            reg;
   ULONG            flag;
   XP_BUCKET1Q_REGP pBucket;
   DEMUX_DEVICE     *pDemuxDev;


   PDEBUG("DemuxFilterStart(): Entering - pDmxfilter->type=%d\n", pDmxfilter->type);
   pDemuxDev = pDemux_dev[pGlobal->uDeviceIndex];

   //If the filter has already been started for channel changes, return
   if (pDmxfilter->states == FILTER_STAT_START_CC)
   {
      return(0);
   }

   //If the filter has been started, stop the filter
   if (pDmxfilter->states == FILTER_STAT_START)
   {
      DemuxFilterStop(pGlobal,pDmxfilter);
   }

   if (pDmxfilter->states == FILTER_STAT_SET)
   {
      if((pDmxfilter->flags & FILTER_FLAG_UNBUFFERED) == 0)
      {
        if (pDmxfilter->buffer.plData)
        {
           vfree(pDmxfilter->buffer.plData);
        }

        pDmxfilter->buffer.plData = vmalloc(pDmxfilter->buffer.ulSize);

        if (!pDmxfilter->buffer.plData)
        {
           return(-ENOMEM);
        }
      }
      pDmxfilter->states = FILTER_STAT_READY;
   }

   switch (pDmxfilter->type)
   {
      case FILTER_TYPE_SEC:
         os_get_mutex(hMutex);
         rc = xp_osi_filter_control(pGlobal,(short)pDmxfilter->fid,
              XP_FILTER_CONTROL_ENABLE);
         os_release_mutex(hMutex);
         if (rc)
         {
            printk("DemuxFilterStart(): Error enabling Section filter\n");
            return(-1);
         }

         if ((pDmxfilter->chid != ILLEGAL_CHANNEL) &&
             (pDemuxDev->chid[pDmxfilter->chid].state != XP_CHANNEL_ENABLED))
         {
            os_get_mutex(hMutex);
            rc = xp_osi_channel_control(pGlobal,(short)pDmxfilter->chid,
                 XP_CHANNEL_CONTROL_ENABLE);
            os_release_mutex(hMutex);
            if (rc)
            {
               printk("DemuxFilterStart(): Error enabling section channel\n");
               return(-1);
            }
            pDemuxDev->chid[pDmxfilter->chid].state = XP_CHANNEL_ENABLED;
         }
         break;

      case FILTER_TYPE_TS:
         os_get_mutex(hMutex);
         rc = xp_osi_channel_control(pGlobal,(short)pDmxfilter->chid,
              XP_CHANNEL_CONTROL_ENABLE);
         os_release_mutex(hMutex);
         if (rc)
         {
            printk("DemuxFilterStart(): Error enabling TS channel\n");
            return(-1);
         }
         xp_osi_start_parse_bypass(pGlobal);
         break;

      case FILTER_TYPE_PES:
         //If the PES type is PCR, enable the pcr processing
         if (pDmxfilter->pes_para.pesType == DMX_PES_PCR)
         {
            os_get_mutex(hMutex);
            xp_osi_pcr_cchan_auto(pGlobal,pDmxfilter->pid);
            os_release_mutex(hMutex);
            break;
         }

         os_get_mutex(hMutex);
         rc = xp_osi_channel_control(pGlobal,(short)pDmxfilter->chid,
              XP_CHANNEL_CONTROL_ENABLE);
         os_release_mutex(hMutex);
         if (rc)
         {
            printk("DemuxFilterStart(): Error enabling PES channel\n");
            return(-1);
         }
         break;

      case FILTER_TYPE_BUCKET:
         //osi
         reg = 0;
         pBucket = (void *)&reg;
         pBucket->bqdt   = 1;
         pBucket->bvalid = 1;
         pBucket->indx   = pDmxfilter->chid;

         flag = os_enter_critical_section();
         xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_BUCKET1Q,reg);
         os_leave_critical_section(flag);
         break;

      default:
         return(-EINVAL);
   }

   pDmxfilter->states = FILTER_STAT_START;
   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterReset
|
|  DESCRIPTION: This function is used to reset the filter to its initial state
+----------------------------------------------------------------------------*/
static int DemuxFilterReset(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   int           rc = 0;
   DEMUX_DEVICE *pDemuxDev;


   PDEBUG("DemuxFilterReset(): Entering - pDmxfilter->type=%d\n", pDmxfilter->type);
   pDemuxDev = pDemux_dev[pGlobal->uDeviceIndex];

   switch (pDmxfilter->type)
   {
      case FILTER_TYPE_SEC:
         if (pDmxfilter->states > FILTER_STAT_READY)
         {
            DemuxFilterStop(pGlobal,pDmxfilter);
         }

         os_get_mutex(hMutex);
         if (pDmxfilter->chid != ILLEGAL_CHANNEL)
         {
            pDemuxDev->chid[pDmxfilter->chid].inuse--;
         }
         os_release_mutex(hMutex);
         interruptible_sleep_on_timeout(&temp_queue,40);

         os_get_mutex(hMutex);
         if ((pDmxfilter->chid != ILLEGAL_CHANNEL) &&
             (pDemuxDev->chid[pDmxfilter->chid].inuse == 0))
         {
            xp_osi_channel_free(pGlobal,pDmxfilter->chid);
            pDemuxDev->chid[pDmxfilter->chid].state = XP_CHANNEL_UNUSED;
         } else {
            if ((pDmxfilter->fid != ILLEGAL_FILTER) &&
                (pDemuxDev->chid[pDmxfilter->chid].inuse >0))
            {
               if (xp_osi_filter_delete_from_channel(pGlobal,
                   pDmxfilter->chid, pDmxfilter->fid))
               {
                  printk("DemuxFilterReset(): Error deleting section filter from channel\n");
               }

               if (xp_osi_filter_free(pGlobal,pDmxfilter->fid))
               {
                  printk("DemuxFilterReset(): Error freeing section filter\n");
               }
            }
         }

         os_release_mutex(hMutex);
         pDmxfilter->chid = (short) ILLEGAL_CHANNEL;
         pDmxfilter->fid  = (short) ILLEGAL_FILTER;
         break;

      case FILTER_TYPE_PES:
         if (pDmxfilter->states > FILTER_STAT_READY)
         {
            DemuxFilterStop(pGlobal,pDmxfilter);
         }

         if (pDmxfilter->chid != (short) ILLEGAL_CHANNEL &&
             pDmxfilter->pes_para.output == OUT_MEMORY)
         {
            os_get_mutex(hMutex);
            xp_osi_channel_free(pGlobal,pDmxfilter->chid);
            os_release_mutex(hMutex);
         }
         pDmxfilter->chid = (short) ILLEGAL_CHANNEL;
         break;

      case FILTER_TYPE_TS:
         if (pDmxfilter->states > FILTER_STAT_READY)
         {
            DemuxFilterStop(pGlobal,pDmxfilter);
         }
         os_get_mutex(hMutex);
         xp_osi_channel_free(pGlobal,pDmxfilter->chid);
         os_release_mutex(hMutex);
         pDmxfilter->chid = (short) ILLEGAL_CHANNEL;
         break;

      case FILTER_TYPE_BUCKET:
         if (pDmxfilter->states > FILTER_STAT_READY)
         {
            DemuxFilterStop(pGlobal,pDmxfilter);
         }
         os_get_mutex(hMutex);
         xp_osi_channel_free(pGlobal,pDmxfilter->chid);
         os_release_mutex(hMutex);
         pDmxfilter->chid = (short) ILLEGAL_CHANNEL;
         break;

      default:
         rc = -EINVAL;
         break;
   }

   //re-init the circular buffer
   pDmxfilter->buffer.ulRead  = 0;
   pDmxfilter->buffer.ulWrite = -1;
   pDmxfilter->buffer.count   = 0;
   pDmxfilter->states         = FILTER_STAT_ALLOC;

   return(rc);
}

/*----------------------------------------------------------------------------+
|  DemuxChannelAcquire
+----------------------------------------------------------------------------*/
static short DemuxChannelAcquire(
   GLOBAL_RESOURCES           *pGlobal,
   DEMUX_FILTER               *pDmxfilter,
   DEMUX_CHANNEL_ACQUIRE_PTR  pDemuxAcq)

{
   DEMUX_DEVICE *pDemuxDev;
   UINT32       flag;
   short        rc=0;


   pDemuxDev = pDemux_dev[pGlobal->uDeviceIndex];
   pDmxfilter->chid   = ILLEGAL_CHANNEL;
   os_get_mutex(hMutex);

   /*------------------------------------------------------------------------+
   |  Allocate Channel and set Unload Type
   +------------------------------------------------------------------------*/
   rc = xp_osi_channel_allocate(pGlobal,pDemuxAcq->channel_type);
   if (rc >= 0) {
      pDmxfilter->chid = rc;
      rc = 0;
   }

   if (rc == 0) {
      if (pDemuxAcq->unload_type != XP_CHANNEL_UNLOAD_UNDEFINED) {
         rc = xp_osi_channel_set_unload_type(pGlobal,pDmxfilter->chid,
                                             pDemuxAcq->unload_type);
      }
   }

   /*------------------------------------------------------------------------+
   |  Allocate and Enable Queue
   +------------------------------------------------------------------------*/
   if (pDemuxAcq->queue_size > 0) {
      if (rc == 0) {
         rc = xp_osi_queue_allocate(pGlobal,pDmxfilter->chid,pDemuxAcq->queue_size);
      }

      if (rc == 0) {
         rc = xp_osi_queue_get_config(pGlobal,pDmxfilter->chid,
                                 &pDmxfilter->buffer.plBQueue,
                                 &pDmxfilter->buffer.plEQueue);
      }

      /*---------------------------------------------------------------------+
      |  Set Notification Function
      +---------------------------------------------------------------------*/
      if (rc == 0) {
         if (pDemuxAcq->notify_data_fn != NULL) {
            rc = xp_osi_channel_set_notification_fn(pGlobal,pDmxfilter->chid,
                 pDemuxAcq->notify_data_fn);
         }
      }

      /*---------------------------------------------------------------------+
      |  Set Boundary Threshold
      +---------------------------------------------------------------------*/
      if (rc == 0) {
         flag = os_enter_critical_section();
         xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,
              XP_QCONFIGA_REG_BTHRES,pDmxfilter->chid,pDemuxAcq->bthreshold);
         os_leave_critical_section(flag);
      }

      if (rc == 0) {
         rc = xp_osi_queue_control(pGlobal,pDmxfilter->chid,XP_QUEUE_CONTROL_ENABLE);
      }
   }


   /*------------------------------------------------------------------------+
   |  Set PID and Disable Channel
   +------------------------------------------------------------------------*/
   if (rc == 0) {
      if (pDemuxAcq->pid <= XP_CHANNEL_NULL_PID) {
         rc = xp_osi_channel_set_pid(pGlobal,pDmxfilter->chid,pDemuxAcq->pid);
      }
   }

   if (rc == 0) {
      rc = xp_osi_channel_control(pGlobal,pDmxfilter->chid,XP_CHANNEL_CONTROL_DISABLE);
   }

   /*------------------------------------------------------------------------+
   |  Check and Process Errors
   +------------------------------------------------------------------------*/
   if (rc != 0) {
      if (pDmxfilter->chid != ILLEGAL_CHANNEL) {
         xp_osi_channel_free(pGlobal,pDmxfilter->chid);
         pDmxfilter->chid = ILLEGAL_CHANNEL;
      }
   }
   os_release_mutex(hMutex);

   if (pDemuxAcq->queue_size < pDemuxAcq->bthreshold*256) {
      PDEBUG("DemuxChannelAcquire(): Warning - Queue is less than Threshold\n");
   }

   PDEBUG("DemuxChannelAcquire(): pDmxfilter->chid=%d, pDemuxAcq->pid=0x%x\n",
           pDmxfilter->chid, pDemuxAcq->pid);

   return(rc);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterAcquire
+----------------------------------------------------------------------------*/
static short DemuxFilterAcquire(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   short   rc=0;


   os_get_mutex(hMutex);

   /*------------------------------------------------------------------------+
   |  Allocate filter, set filter values and add to channel
   +------------------------------------------------------------------------*/
   if (pDmxfilter->fid == ILLEGAL_FILTER) {
      rc = xp_osi_filter_allocate(pGlobal,pDmxfilter->para.filter_length);
      if (rc >= 0) {
         pDmxfilter->fid = rc;
         rc = 0;
      }
   }

   if (rc == 0) {
      rc = xp_osi_filter_set(pGlobal,(short)pDmxfilter->fid,
                             pDmxfilter->para.filter_length,
                             (unsigned char *)pDmxfilter->para.filter,
                             (unsigned char *)pDmxfilter->para.mask,
                             (unsigned char *)pDmxfilter->para.positive);
   }

   if (rc == 0) {
      rc = xp_osi_filter_add_to_channel(pGlobal, (short)pDmxfilter->chid,
                                       (short)pDmxfilter->fid);
      if(rc >=0) {
         pDmxfilter->ulMatchWord = (0x80000000)>>rc;
         rc = 0;
      }
   }

   if (rc == 0) {
      rc = xp_osi_filter_control(pGlobal,(short)pDmxfilter->fid,
                                     XP_FILTER_CONTROL_DISABLE);
   }

   /*------------------------------------------------------------------------+
   |  Check and Process Errors
   +------------------------------------------------------------------------*/
   if (rc != 0) {
      if (pDmxfilter->fid != ILLEGAL_FILTER) {
         xp_osi_filter_free(pGlobal,pDmxfilter->fid);
         pDmxfilter->fid = ILLEGAL_FILTER;
      }
   }

   os_release_mutex(hMutex);
   PDEBUG("DemuxFilterAcquire(): pDmxfilter->fid = %d\n", pDmxfilter->fid);
   return(rc);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterSectionSet
+----------------------------------------------------------------------------*/
static int DemuxFilterSectionSet(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   DEMUX_DEVICE               *pDemuxDev;
   DEMUX_CHANNEL_ACQUIRE_TYPE DemuxAcq;
   int                        rc=0;
   int                        i;


   PDEBUG("DemuxFilterSectionSet(): Entering\n");
   pDemuxDev = pDemux_dev[pGlobal->uDeviceIndex];

   /*----------------------------------------------------------------------------+
   |  Reset Filter if alaready started
   +----------------------------------------------------------------------------*/
   if (pDmxfilter->states >= FILTER_STAT_SET) {
      if(pDmxfilter->flags & FILTER_FLAG_MMAPPED)
      {
        printk("DemuxFilterSectionSet(): Error - filter is mmapped\n");
        return(-1);
      }
      DemuxFilterReset(pGlobal,pDmxfilter);
   }

   /*----------------------------------------------------------------------------+
   |  Determine if the filter's pid is already allocated to a channel
   |  Same PID cannot be allocated to more than one channel
   +----------------------------------------------------------------------------*/
   for (i = 0; i < pDemuxDev->uFilterNum; i++) {
      if ((pDemuxDev->filter[i].para.pid == pDmxfilter->para.pid) &&
          (pDemuxDev->filter[i].chid != (short)ILLEGAL_CHANNEL)) {
         break;
      }
   }

   /*----------------------------------------------------------------------------+
   |  Attach filter to existing Channel with same PID
   +----------------------------------------------------------------------------*/
   if (i < pDemuxDev->uFilterNum) {
      PDEBUG("DemuxFilterSectionSet(): New filter attached to an existing channel\n");
      pDmxfilter->chid = pDemuxDev->filter[i].chid;
      pDmxfilter->buffer.plBQueue = pDemuxDev->filter[i].buffer.plBQueue;
      pDmxfilter->buffer.plEQueue = pDemuxDev->filter[i].buffer.plEQueue;
   }

   /*----------------------------------------------------------------------------+
   |  Create New Channel for Filter
   +----------------------------------------------------------------------------*/
   else if (pDmxfilter->chid == ILLEGAL_CHANNEL) {
      DemuxAcq.channel_type   = XP_CHANNEL_TYPE_PES;
      DemuxAcq.pid            = pDmxfilter->para.pid;
      DemuxAcq.unload_type    = XP_CHANNEL_UNLOAD_FILTER_PSI_CRC;
      DemuxAcq.bthreshold     = 0;
      DemuxAcq.queue_size     = pDmxfilter->buffer.ulSize;
      DemuxAcq.notify_data_fn = DemuxSectionCallback;

      if (DemuxChannelAcquire(pGlobal,pDmxfilter,&DemuxAcq)) {
         printk("DemuxFilterSectionSet(): Error acquiring channel for Section Filter\n");
         return(-EMFILE);
      }
      pDemuxDev->chid[pDmxfilter->chid].state = XP_CHANNEL_DISABLED;
   }

   /*----------------------------------------------------------------------------+
   |  Disable Channel
   +----------------------------------------------------------------------------*/
   else {
      os_get_mutex(hMutex);
      rc = xp_osi_channel_control(pGlobal,(short)pDmxfilter->chid,
           XP_CHANNEL_CONTROL_DISABLE);
      os_release_mutex(hMutex);
      if (rc) {
         printk("DemuxFilterSectionSet(): Error enabling channel for Section Filter\n");
         return(-1);
      }
      pDemuxDev->chid[pDmxfilter->chid].state = XP_CHANNEL_DISABLED;
   }

   /*----------------------------------------------------------------------------+
   |  Create New Filter for existing channel
   +----------------------------------------------------------------------------*/
   if (DemuxFilterAcquire(pGlobal,pDmxfilter)) {
       pDmxfilter->para.pid = -1;
       printk("DemuxFilterSectionSet(): Error acquiring Section Filter\n");
       return(-EMFILE);
   }

   pDmxfilter->pid    = pDmxfilter->para.pid;
   pDmxfilter->type   = FILTER_TYPE_SEC;
   pDmxfilter->states = FILTER_STAT_SET;

   os_get_mutex(hMutex);
   pDemuxDev->chid[pDmxfilter->chid].inuse++;
   os_release_mutex(hMutex);

   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterBucketSet
+----------------------------------------------------------------------------*/
static int DemuxFilterBucketSet(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   DEMUX_CHANNEL_ACQUIRE_TYPE DemuxAcq;


   PDEBUG("DemuxFilterBucketSet(): Entering\n");

   if (pDmxfilter->buffer.ulSize < pDmxfilter->bucket_para.unloader.threshold*256) {
      printk("DemuxFilterBucketSet(): Error - Illegal Buffer size\n");
      return(-1);
   }

   if (pDmxfilter->states >= FILTER_STAT_SET) {
      if(pDmxfilter->flags & FILTER_FLAG_MMAPPED)
      {
        printk("DemuxFilterBucketSet(): Error - filter is mmapped\n");
        return(-1);
      }
      DemuxFilterReset(pGlobal,pDmxfilter);
   }

   pDmxfilter->type   = FILTER_TYPE_BUCKET;
   pDmxfilter->states = FILTER_STAT_SET;

   /*----------------------------------------------------------------------------+
   |  Allocate Channel for Bucket Queue
   +----------------------------------------------------------------------------*/
   DemuxAcq.channel_type   = XP_CHANNEL_TYPE_BUCKET;
   DemuxAcq.pid            = XP_CHANNEL_NULL_PID;
   DemuxAcq.unload_type    = pDmxfilter->bucket_para.unloader.unloader_type;
   DemuxAcq.bthreshold     = pDmxfilter->bucket_para.unloader.threshold;
   DemuxAcq.queue_size     = pDmxfilter->buffer.ulSize;
   if(pDmxfilter->flags & FILTER_FLAG_UNBUFFERED)
     DemuxAcq.notify_data_fn = NULL;
   else
     DemuxAcq.notify_data_fn = DemuxPESCallback;

   if (DemuxChannelAcquire(pGlobal,pDmxfilter,&DemuxAcq)) {
      printk("DemuxFilterBucketSet(): Error acquiring channel for bucket queue\n");
      return(-EMFILE);
   }

   if(pDmxfilter->flags & FILTER_FLAG_UNBUFFERED)
   {
     xp_osi_interrupt_channel_notify(pGlobal,
                                      XP_INTERRUPT_NOTIFY_ADD,
                                      pDmxfilter->chid,
				      XP_INTERRUPT_QSTAT_RPI|XP_INTERRUPT_QSTAT_BTI|XP_INTERRUPT_QSTAT_FP,
				      (XP_INTERRUPT_CHANNEL_FN)DemuxCallback_nb);
   }
   return(0);
}
       
/*----------------------------------------------------------------------------+
|  DemuxFilterPESDecoderSet
|
|  DESCRIPTION: Set the PES filter parameters when directly outputting PES
|               data to the Transport audio and video unloaders.
+----------------------------------------------------------------------------*/
static int DemuxFilterPESDecoderSet(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   int             rc=0;
   UINT            chid=ILLEGAL_CHANNEL;
   XP_CHANNEL_TYPE ctype;


   PDEBUG("DemuxFilterPESDecoderSet(): Entering\n");

   /*----------------------------------------------------------------------------+
   | Deliver Audio/video/PCR PES data to hardware decoder
   | Initially, allocate channel and set PID manually
   +----------------------------------------------------------------------------*/
   if (pDmxfilter->chid  == ILLEGAL_CHANNEL    ||
       pDmxfilter->states < FILTER_STAT_START) {

      if (pDmxfilter->states >= FILTER_STAT_SET) {
         if(pDmxfilter->flags & FILTER_FLAG_MMAPPED)
         {
           printk("DemuxFilterPESDecoderSet(): Error - filter is mmapped\n");
           return(-1);
         }
         DemuxFilterReset(pGlobal,pDmxfilter);
      }

      pDmxfilter->type   = FILTER_TYPE_PES;
      pDmxfilter->pid    = pDmxfilter->pes_para.pid;
      pDmxfilter->states = FILTER_STAT_SET;

      switch (pDmxfilter->pes_para.pesType) {
         case DMX_PES_VIDEO:
            ctype = XP_CHANNEL_TYPE_VIDEO;
            chid  = XP_CHANNEL_VIDEO;
            break;
         case DMX_PES_AUDIO:
            ctype = XP_CHANNEL_TYPE_AUDIO;
            chid  = XP_CHANNEL_AUDIO;
            break;
         case DMX_PES_PCR:
            pDmxfilter->chid = 0x1001;      //vitual PID
            return(0);
            break;
         default:
            printk("DemuxFilterPESDecoderSet(): Illegal PES Type\n");
            return(-EINVAL);
            break;
      }

      os_get_mutex(hMutex);
      if (pDmxfilter->chid == ILLEGAL_CHANNEL) {
         if (xp_osi_channel_get_available(pGlobal,ctype) == 0) {
             xp_osi_channel_free(pGlobal,chid);
         }

         if ((rc = xp_osi_channel_allocate(pGlobal,ctype)) >= 0) {
            pDmxfilter->chid = rc;
         } else {
            os_release_mutex(hMutex);
            printk("DemuxFilterPESDecoderSet(): Error allocating PES channel\n");
            return(-1);
         }
      }

      xp_osi_channel_set_pid(pGlobal,(short)pDmxfilter->chid,
                             (unsigned short)pDmxfilter->pid);
      os_release_mutex(hMutex);

   } else {
      /*-------------------------------------------------------------------------+
      | Deliver Audio/video/PCR PES data to hardware decoder using automated
      | channel change functions.  This assumes the channel has been allocated.
      +-------------------------------------------------------------------------*/
      pDmxfilter->type   = FILTER_TYPE_PES;
      pDmxfilter->pid    = pDmxfilter->pes_para.pid;
      pDmxfilter->states = FILTER_STAT_START_CC;

      switch (pDmxfilter->pes_para.pesType) {
         case DMX_PES_VIDEO:
            xp_osi_video_cchan_auto(pGlobal,pDmxfilter->pid);
            break;
         case DMX_PES_AUDIO:
            xp_osi_audio_cchan_auto(pGlobal,pDmxfilter->pid);
            break;
         case DMX_PES_PCR:
            xp_osi_pcr_cchan_auto(pGlobal,pDmxfilter->pid);
            break;
         default:
            printk("DemuxFilterPESDecoderSet(): Illegal PES Type\n");
            return(-EINVAL);
            break;
      }
   }

   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterPESMemorySet
|
|  DESCRIPTION: Set the PES filter parameters when outputting PES data to
|               the sport queues.
+----------------------------------------------------------------------------*/
static int DemuxFilterPESMemorySet(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   DEMUX_CHANNEL_ACQUIRE_TYPE DemuxAcq;
   XP_CONFIG_VALUES           Config;
   XP_CHANNEL_TYPE            ctype;
   UINT                       chid=ILLEGAL_CHANNEL;


   PDEBUG("DemuxFilterPESMemorySet(): Entering\n");

   /*----------------------------------------------------------------------------+
   | Deliver PES data to Transport Queues
   +----------------------------------------------------------------------------*/
   if (pDmxfilter->states >= FILTER_STAT_SET) {
       if(pDmxfilter->flags & FILTER_FLAG_MMAPPED)
       {
         printk("DemuxFilterPESMemorySet(): Error - filter is mmapped\n");
         return(-1);
       }
       DemuxFilterReset(pGlobal,pDmxfilter);
   }

   pDmxfilter->type   = FILTER_TYPE_PES;
   pDmxfilter->pid    = pDmxfilter->pes_para.pid;
   pDmxfilter->states = FILTER_STAT_SET;

   switch (pDmxfilter->pes_para.pesType) {
      case DMX_PES_VIDEO:
         ctype = XP_CHANNEL_TYPE_VIDEO;
         chid  = XP_CHANNEL_VIDEO;
         os_get_mutex(hMutex);
         xp_osi_get_config_values(pGlobal,&Config);
         Config.vpu = 1;                //force all video packet to memory unloader
         xp_osi_set_config_values(pGlobal,&Config);
         os_release_mutex(hMutex);
         break;
      case DMX_PES_AUDIO:
         ctype = XP_CHANNEL_TYPE_AUDIO;
         chid  = XP_CHANNEL_AUDIO;
         os_get_mutex(hMutex);
         xp_osi_get_config_values(pGlobal,&Config);
         Config.apu = 1;                //force all audio packet to memory unloader
         xp_osi_set_config_values(pGlobal,&Config);
         os_release_mutex(hMutex);
         break;
      case DMX_PES_PCR:
         return(0);
         break;
      case DMX_PES_TELETEXT:
         ctype = XP_CHANNEL_TYPE_TELETEXT;
         chid  = XP_CHANNEL_TELETEXT;
         break;
      case DMX_PES_SUBTITLE:
         ctype = XP_CHANNEL_TYPE_SUBTITLE;
         chid  = XP_CHANNEL_SUBTITLE;
         break;
      case DMX_PES_OTHER:
         ctype = XP_CHANNEL_TYPE_PES;
         chid  = ILLEGAL_CHANNEL;
         break;
      default:
         printk("DemuxFilterPESMemorySet(): Illegal PES Type\n");
         return(-EINVAL);
         break;
   }

   if (pDmxfilter->chid == ILLEGAL_CHANNEL) {
      os_get_mutex(hMutex);
      if (xp_osi_channel_get_available(pGlobal,ctype) == 0) {
          xp_osi_channel_free(pGlobal,chid);
      }
      os_release_mutex(hMutex);

      DemuxAcq.channel_type   = ctype;
      DemuxAcq.pid            = pDmxfilter->pid;
      DemuxAcq.unload_type    = pDmxfilter->pes_para.unloader.unloader_type;
      DemuxAcq.bthreshold     = pDmxfilter->pes_para.unloader.threshold;
      if(pDmxfilter->flags & FILTER_FLAG_UNBUFFERED)
         DemuxAcq.notify_data_fn = NULL;
      else
         DemuxAcq.notify_data_fn = DemuxPESCallback;
      DemuxAcq.queue_size     = 0;

      if (pDmxfilter->pes_para.output == OUT_MEMORY) {
         DemuxAcq.queue_size  = pDmxfilter->buffer.ulSize;
      }
      if (DemuxChannelAcquire(pGlobal,pDmxfilter,&DemuxAcq)) {
         printk("DemuxFilterPESMemorySet(): Error allocating PES channel\n");
         return(-EMFILE);
      }
      if(pDmxfilter->flags & FILTER_FLAG_UNBUFFERED)
      {
         xp_osi_interrupt_channel_notify(pGlobal,
                                         XP_INTERRUPT_NOTIFY_ADD,
                                         pDmxfilter->chid,
	   			         XP_INTERRUPT_QSTAT_RPI | XP_INTERRUPT_QSTAT_BTI,
	                                 (XP_INTERRUPT_CHANNEL_FN)DemuxCallback_nb);
      }
   } else {
      os_get_mutex(hMutex);
      xp_osi_channel_set_pid(pGlobal,(short)pDmxfilter->chid,
                             (unsigned short)pDmxfilter->pid);
      os_release_mutex(hMutex);
   }

   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterTSSet
|
|  DESCRIPTION: Set the TS out mode
+----------------------------------------------------------------------------*/
static int DemuxFilterTSSet(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter)

{
   DEMUX_CHANNEL_ACQUIRE_TYPE DemuxAcq;


   PDEBUG("DemuxFilterTSSet(): Entering\n");

   if(pDmxfilter->pDemuxDev->users > 1) {
      printk("DemuxFilterTSSet(): Error - Too many filters opened!\n");
      return(-1);
   }

   if (pDmxfilter->states >= FILTER_STAT_SET) {
      if(pDmxfilter->flags & FILTER_FLAG_MMAPPED)
      {
        printk("DemuxFilterTSSet(): Error - filter is mmapped\n");
        return(-1);
      }
      DemuxFilterReset(pGlobal,pDmxfilter);
   }

   pDmxfilter->type   = FILTER_TYPE_TS;
   pDmxfilter->states = FILTER_STAT_SET;
   xp_osi_set_parse_bypass_mode(pGlobal);

   /*----------------------------------------------------------------------------+
   |  Allocate Channel for Transport Stream
   +----------------------------------------------------------------------------*/
   DemuxAcq.channel_type   = XP_CHANNEL_TYPE_SUBTITLE;
   DemuxAcq.pid            = XP_CHANNEL_NULL_PID;
   DemuxAcq.unload_type    = XP_CHANNEL_UNLOAD_TRANSPORT;
   DemuxAcq.bthreshold     = 255;
   DemuxAcq.queue_size     = pDmxfilter->buffer.ulSize;
   if(pDmxfilter->flags & FILTER_FLAG_UNBUFFERED)
     DemuxAcq.notify_data_fn = NULL;
   else
     DemuxAcq.notify_data_fn = DemuxPESCallback;

   if (DemuxChannelAcquire(pGlobal,pDmxfilter,&DemuxAcq)) {
      printk("DemuxFilterTSSet(): Error acquiring channel for Transport Stream\n");
      return(-ENOMEM);
   }

   if(pDmxfilter->flags & FILTER_FLAG_UNBUFFERED)
   {
     xp_osi_interrupt_channel_notify(pGlobal,
                                      XP_INTERRUPT_NOTIFY_ADD,
                                      pDmxfilter->chid,
				      XP_INTERRUPT_QSTAT_RPI | XP_INTERRUPT_QSTAT_BTI,
				      (XP_INTERRUPT_CHANNEL_FN)DemuxCallback_nb);
   }

   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxSetBufferSize
|
|  DESCRIPTION: Set size of internel circular buffer.
+----------------------------------------------------------------------------*/
static int DemuxSetBufferSize(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter,
   ULONG            size)

{
   PDEBUG("DemuxSetBufferSize(): Entering\n");

   if (pDmxfilter->states >= FILTER_STAT_START)
   {
      DemuxFilterStop(pGlobal,pDmxfilter);
   }

   if (pDmxfilter->buffer.plData)
   {
      vfree(pDmxfilter->buffer.plData);
   }

   pDmxfilter->buffer.plData  = 0;
   pDmxfilter->buffer.ulSize  = size;
   pDmxfilter->buffer.ulRead  = 0;
   pDmxfilter->buffer.ulWrite = -1;

   if (pDmxfilter->states == FILTER_STAT_READY)
   {
      if((pDmxfilter->flags & FILTER_FLAG_UNBUFFERED) == 0)
      {
        pDmxfilter->buffer.plData = vmalloc(pDmxfilter->buffer.ulSize);
        if (!pDmxfilter->buffer.plData)
        {
           return(-ENOMEM);
        }
      }	
   }

   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxSetACPM
+----------------------------------------------------------------------------*/
static int DemuxSetACPM(
   GLOBAL_RESOURCES *pGlobal,
   ULONG            arg)

{
   XP_CONFIG_VALUES Config;


   PDEBUG("DemuxSetACPM():Entering - uDeviceIndex=%d  arg=%lu\n",pGlobal->uDeviceIndex, arg);

   switch(pGlobal->uDeviceIndex)
   {
      case 0:
         os_get_mutex(hMutex);
         xp_osi_get_config_values(pGlobal,&Config);
         if (arg != 0)
         {
            Config.acpm = 1;
         } else {
            Config.acpm = 0;
         }

         xp_osi_set_config_values(pGlobal,&Config);
         os_release_mutex(hMutex);
         break;

       default:
         return(-1);
         break;
   }

   return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxSetVCPM
+----------------------------------------------------------------------------*/
static int DemuxSetVCPM(
   GLOBAL_RESOURCES *pGlobal,
   ULONG            arg)

{
   XP_CONFIG_VALUES Config;


   PDEBUG("DemuxSetVCPM(): Entering\n");

   switch(pGlobal->uDeviceIndex)
   {
      case 0:
         os_get_mutex(hMutex);
         xp_osi_get_config_values(pGlobal,&Config);
         if (arg != 0)
         {
            Config.vcpm = 1;
         } else {
            Config.vcpm = 0;
         }

         xp_osi_set_config_values(pGlobal,&Config);
         os_release_mutex(hMutex);
         break;

      default:
         return(-1);
         break;
   }

   return(0);
}

/*----------------------------------------------------------------------------+
|  demux_filter_get_queue
+-----------------------------------------------------------------------------+
|  DESCRIPTION: This function returns the current read pointer and write 
|  pointer offsets for the demux queue.  
+----------------------------------------------------------------------------*/
static int demux_filter_get_queue(
   struct file      *file,
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter,
   queue_para       *qpara)

{
  UINT32             flag;
  ULONG pread;
  ULONG pwrite;
  QUEUE_PTR        pQue;     /* channel information          */

  PDEBUG1("demux_filter_get_queue(): Entering\n");

  qpara->readptr = 0;
  qpara->writeptr = 0;
    
  while(1)
  {
    if (!pDmxfilter || pDmxfilter->states < FILTER_STAT_START) 
    {
      printk("demux_filter_get_queue(): filter not started");
      return(-EINVAL);
    }
    
    if(pDmxfilter->chid == ILLEGAL_CHANNEL)
    {
      printk("demux_filter_get_queue(): ILLEGAL CHANNEL");
      return(-EINVAL);
    }

    if((pDmxfilter->flags & FILTER_FLAG_UNBUFFERED) == 0)
    {
      printk("demux_filter_get_queue(): filter not in unbuffered mode");
      return(-EINVAL);
    }

    flag = os_enter_critical_section();
    
    pread = pDmxfilter->buffer.ulRead;      
    pwrite = pDmxfilter->buffer.ulWrite;

    if(pwrite == -1)                          /* initial value is set to -1 */
      pwrite = 0;
      
//      printk("ppBQueue=%08X ppEQueue=%08X pread=%08X pwrite=%08X\n",
//              ppBQueue,ppEQueue,pread,pwrite);
	    
    if(pread != pwrite)        
    {
      os_leave_critical_section(flag);
      break;
    }
    
    // If the filter is opened in non-block mode, return
    if (file->f_flags & O_NONBLOCK)
    {
      os_leave_critical_section(flag);
//      printk("demux_filter_get_queue(): -EWOULDBLOCK");
      return(-EWOULDBLOCK);
    }

    // sleep upon filter's queue, waiting data available
    if (pDmxfilter->para.timeout > 0)
    {
      if(0 == interruptible_sleep_on_timeout(&pDmxfilter->buffer.queue,
                                     (UINT)(pDmxfilter->para.timeout/10)))
      {				      
        os_leave_critical_section(flag);
//        printk("demux_filter_get_queue(): -ETIMEDOUT");
        return(-ETIMEDOUT);					  
      }
    } else 
       interruptible_sleep_on(&pDmxfilter->buffer.queue);
    
    os_leave_critical_section(flag);
    if (signal_pending(current))
    {
      printk("demux_filter_get_queue(): -EINTR");
      return(-EINTR);
    }
      
  }

  /*--------------------------------------------------------------------+
  |  Get the queue range, and the address range of the data
  +--------------------------------------------------------------------*/

//  printk("pread = %8.8x ",(int)pread);
//  printk("pwrite = %8.8x ",(int)pwrite);
//  printk("ppBQueue = %8.8x ",(int)ppBQueue);
//  printk("ppEQueue = %8.8x\n",(int)ppEQueue);

  /*--------------------------------------------------------------------+
  |  make sure the address range is within the bounds of the queue
  +--------------------------------------------------------------------*/
  pQue = &pGlobal->QueueInfo.XpQueueChData[pDmxfilter->chid];
  if(pQue->hMem == 0)
  {
    printk("demux_filter_get_queue(): queue not allocated");
    return(-EINVAL);
  }

  if((pread >= pQue->ulQueueSize) || (pwrite >= pQue->ulQueueSize))
  {
    PDEBUG("XP_ERROR_QUEUE_ADDRESS1\n");
    printk("demux_filter_get_queue(): invalid pread(%d) or pwrite(%d)\n",(int)pread,(int)pwrite);
    pQue->Errors.uwQueueAddress++;
    return(-EINVAL);
  }

  /*--------------------------------------------------------------------+
  |  Check if queue is not active, then we'll skip the new record
  |  which must have arrived in the hardware when we changed the state
  |  to DISABLED.
  +--------------------------------------------------------------------*/
  if(pQue->State != XP_QUEUE_STATUS_ENABLED)
  {
    printk("pChannel->Errors.uwQueueDisabled\n");
    pQue->Errors.uwQueueDisabled++;
    return(-EINVAL);
  }

  qpara->readptr = pread;
  qpara->writeptr = pwrite;
    
  return(0);
}

/*----------------------------------------------------------------------------+
|  demux_filter_set_readptr
+-----------------------------------------------------------------------------+
|  DESCRIPTION: This function sets the read pointer for the specified demux 
|  queue.  
+----------------------------------------------------------------------------*/
static int demux_filter_set_readptr(
   struct file      *file,
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter,
   ULONG            rp)

{
  QUEUE_PTR pQue;     /* channel information          */
  int flag;

  PDEBUG1("demux_filter_set_readptr(): Entering\n");
  
  if (!pDmxfilter || pDmxfilter->states < FILTER_STAT_START) 
  {
    printk("demux_filter_set_readptr(): filter not started");
    return(-EINVAL);
  }
    
  if(pDmxfilter->chid == ILLEGAL_CHANNEL)
  {
    printk("demux_filter_set_readptr(): ILLEGAL CHANNEL");
    return(-EINVAL);
  }
    
  if((pDmxfilter->flags & FILTER_FLAG_UNBUFFERED) == 0)
  {
    printk("demux_filter_get_queue(): filter not in unbuffered mode");
    return(-EINVAL);
  }

  pQue = &pGlobal->QueueInfo.XpQueueChData[pDmxfilter->chid];
  if(pQue->hMem == 0)
  {
    printk("demux_filter_set_readptr(): queue not allocated");
    return(-EINVAL);
  }
  
  if(rp > pQue->ulQueueSize)
  {
    printk("demux_filter_set_readptr(): read pointer outside of queue\n");
    return(-EINVAL);
  }
  
  if(rp == pQue->ulQueueSize)
    pDmxfilter->buffer.ulRead = 0;
  else
    pDmxfilter->buffer.ulRead = rp;
    
  if(rp == 0)
    rp = pQue->ulQueueSize;
    
  rp = (rp+pQue->ulQueueAddr) & 0x00ffff00;
 
  flag = os_enter_critical_section();
  pQue->ppReadAddr = (void *)(rp + pQue->ulQueueAddr);
  xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,XP_QCONFIGB_REG_RPTR, 
                                     pDmxfilter->chid, rp);

  os_leave_critical_section(flag);
  
  return(0);
}

/*----------------------------------------------------------------------------+
|  DemuxFilterSetFlags
+----------------------------------------------------------------------------*/
int DemuxFilterSetFlags(
   GLOBAL_RESOURCES *pGlobal,
   DEMUX_FILTER     *pDmxfilter,
   int flags)
{

  if(flags & FILTER_FLAG_NONBUFFERED)
    pDmxfilter->flags |= FILTER_FLAG_UNBUFFERED;
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
|  demux_filter_open
+-----------------------------------------------------------------------------+
|  DESCRIPTION: This Function will init the demux hardware and return a
|               filter handler.
+----------------------------------------------------------------------------*/
int demux_filter_open(
   DEMUX_DEVICE *pDemuxDev,
   UINT         uDeviceIndex,
   struct inode *inode,
   struct file  *filp)

{
   int i;
   int rc;
   GLOBAL_RESOURCES *pGlobal;
   DEMUX_FILTER     *pDmxfilter;


   PDEBUG("demux_filter_open(): Entering\n");

   //If the hardware hasn't been initilized, init it
   if (uAlreadyInit == 0)
   {
      for(i = 0; i < MAX_XP_NUM; i++)
      {
         XpGlobal[i].uDeviceIndex = i;
         pDemux_dev[i] = NULL;
      }

      uAlreadyInit++;
      hMutex = os_create_mutex();

      rc = os_install_irq(XP_IRQ,IRQ_DEFAULT_TRIG,(void*)&xp_isr, NULL);

      if (rc)
      {
        printk("demux_filter_open(): Error - Invalid IRQ number\n");
        return(-EINVAL);
      }

      for(i = 0; i<MAX_XP_NUM; i++)
      {
         XpGlobal[i].uReservedBufSize = DEFAULT_RESERVED_BUF_SIZE;
         rc = xp_osi_init(&XpGlobal[i]);

         if(rc)
         {
            printk("demux_filter_open(): Error - during Transport %d init\n",i);
            uAlreadyInit = 0;
            os_disable_irq(XP_IRQ);
            os_uninstall_irq(XP_IRQ);
            return(rc);
         }
      }
   }

   pGlobal = &XpGlobal[uDeviceIndex];
   pDemux_dev[uDeviceIndex] = pDemuxDev;

   //see if there is more filters available
   if (pDemuxDev->users >= pDemuxDev->uFilterNum)
   {
      printk("demux_filter_open(): Error - No more filters available\n");
      return(-EMFILE);
   }

   if (!pDemuxDev->uAlreadyInit)
   {
      pDemuxDev->uAlreadyInit++;
      if (pDemuxDev->uFilterNum > MAX_DEMUX_FILTER_NUM)
      {
         pDemuxDev->uFilterNum = MAX_DEMUX_FILTER_NUM;
      }

      for (i = 0; i < pDemuxDev->uFilterNum; i++)
      {
         pDemuxDev->filter[i].states = FILTER_STAT_FREE;
	 pDemuxDev->filter[i].flags = 0;
	 pDemuxDev->filter[i].async_queue = NULL;
         pDemuxDev->filter[i].ulNotifySize = 0;
         pDemuxDev->filter[i].pDemuxDev = pDemuxDev;
         pDemuxDev->filter[i].buffer.plData = 0;
         pDemuxDev->filter[i].buffer.ulSize = DEFAULT_BUF_SIZE;
         pDemuxDev->filter[i].buffer.count = 0;
         pDemuxDev->filter[i].buffer.ulRead = 0;
         pDemuxDev->filter[i].buffer.ulWrite = -1;
         pDemuxDev->filter[i].buffer.error = 0;
         pDemuxDev->filter[i].para.pid = -1;
         init_waitqueue_head(&pDemuxDev->filter[i].buffer.queue);
      }

      os_get_mutex(hMutex);
      for(i=0; i<MAX_DEMUX_CHANNEL_NUM; i++)
      {
         pDemuxDev->chid[i].chid  = (short)ILLEGAL_CHANNEL;
         pDemuxDev->chid[i].inuse = 0;
         pDemuxDev->chid[i].state = XP_CHANNEL_UNUSED;
      }
      os_release_mutex(hMutex);
   }

   for (i = 0; i< pDemuxDev->uFilterNum;i++)
   {
      if (pDemuxDev->filter[i].states == FILTER_STAT_FREE)
      {
         break;
      }
   }

   if (i == pDemuxDev->uFilterNum)
   {
      printk("demux_filter_open(): No more filters to allocate\n");
      return(-EMFILE);
   }

   //store the available filter to file private section
   pDmxfilter         = &pDemuxDev->filter[i];
   filp->private_data = pDmxfilter;

   pDmxfilter->chid   = ILLEGAL_CHANNEL;
   pDmxfilter->fid    = ILLEGAL_FILTER;
   pDmxfilter->type   = 0;
   pDmxfilter->states = FILTER_STAT_ALLOC;
   pDmxfilter->flags = 0;
   pDmxfilter->async_queue = NULL;

   os_get_mutex(hMutex);

   //increase the user number
   pDemuxDev->users++;
   os_release_mutex(hMutex);

   MOD_INC_USE_COUNT;
   return(0);
}

/*----------------------------------------------------------------------------+
|  demux_filter_release
+-----------------------------------------------------------------------------+
|  DESCRIPTION: This function will reset the filter, and free the resources
|               allocated if there is no user is using the driver
+----------------------------------------------------------------------------*/
int demux_filter_release(
   UINT         uDeviceIndex,
   struct inode *inode,
   struct file  *file)

{
   int               i;
   int               users = 0;
   GLOBAL_RESOURCES *pGlobal;
   DEMUX_DEVICE     *pDemuxDev;
   DEMUX_FILTER     *pDmxfilter;


   PDEBUG("demux_filter_release(): Entering\n");

   //get the filter from the file private section, which is determined by process
   pDmxfilter = (DEMUX_FILTER *) file->private_data;
   pGlobal    = &XpGlobal[uDeviceIndex];
   pDemuxDev  = pDemux_dev[uDeviceIndex];

   fasync_helper(-1, file, 0, &pDmxfilter->async_queue);

   if (!pDmxfilter && pDemuxDev->uDeviceIndex != uDeviceIndex)
   {
      return(-EBADF);
   }

   DemuxFilterReset(pGlobal,pDmxfilter);

   if (pDmxfilter->buffer.plData)
   {
      void *mem = pDmxfilter->buffer.plData;
      pDmxfilter->buffer.plData = 0;
      vfree(mem);
   }

   pDmxfilter->states = FILTER_STAT_FREE;
   os_get_mutex(hMutex);

   //decrease the user umber
   pDmxfilter->pDemuxDev->users--;

   for(i = 0; i<MAX_XP_NUM; i++)
   {
      if(pDemux_dev[i] != NULL)
      users += pDemux_dev[i]->users;
   }

   //If there is no user left, free the resources and IRQ
   if (!users)
   {
      uAlreadyInit = 0;

      for(i = MAX_XP_NUM-1; i >=0; i--)
      {
         xp_osi_interrupt_de_init(&XpGlobal[i]);
         xp_osi_queue_de_init(&XpGlobal[i]);
      }

      os_disable_irq(XP_IRQ);
      os_uninstall_irq(XP_IRQ);
   }

   os_release_mutex(hMutex);

   MOD_DEC_USE_COUNT;
   return(0);
}

/*----------------------------------------------------------------------------+
|  demux_filter_poll
+-----------------------------------------------------------------------------+
|  DESCRIPTION: This function implemented the stantard Poll system call.
+----------------------------------------------------------------------------*/
unsigned int demux_filter_poll(
   UINT                     uDeviceIndex,
   struct file              *file,
   struct poll_table_struct *wait)

{
   DEMUX_FILTER     *pDmxfilter;


   PDEBUG1("demux_filter_poll(): Entering\n");
   //get the filter from the file private section, which is determined by process
   pDmxfilter = (DEMUX_FILTER *) file->private_data;

   if (!pDmxfilter)
   {
      return(-EINVAL);
   }

   if (pDmxfilter->states < FILTER_STAT_START)
   {
      return(0);
   }

   if (pDmxfilter->buffer.count > 0)
   {
      return(POLLIN | POLLRDNORM | POLLPRI);
   }

   //wait the queue be wake up
   poll_wait(file, &pDmxfilter->buffer.queue, wait);

   if (pDmxfilter->buffer.count > 0)
   {
      return(POLLIN | POLLRDNORM | POLLPRI);
   }

   return(0);
}

/*----------------------------------------------------------------------------+
|  demux_filter_read
+-----------------------------------------------------------------------------+
|  DESCRIPTION: This function is used to transfer data from internel buffer
|               to the user space.
+----------------------------------------------------------------------------*/
ssize_t demux_filter_read(
   UINT        uDeviceIndex,
   struct file *file,
   char        *buf,
   size_t      count,
   loff_t      *ppos)

{
   int                i;
   UINT32             flag;
   ULONG              BErem;
   ULONG              temp;
   UCHAR              *data;
   DEMUX_FILTER       *pDmxfilter;
   SECTION_HEADER_PTR SectionHeader;


   PDEBUG1("demux_filter_read(): Entering\n");

   /*-------------------------------------------------------------------------+
   | Get the filter from the file private section, which is process dependent
   +-------------------------------------------------------------------------*/
   pDmxfilter = (DEMUX_FILTER *) file->private_data;

   if (!pDmxfilter)
   {
      return(-EBADF);
   }

   if (!pDmxfilter->buffer.plData)
   {
      return(0);
   }

   /*-------------------------------------------------------------------------+
   | If the filter is opened in non-block mode, return
   +-------------------------------------------------------------------------*/
   if ((file->f_flags & O_NONBLOCK) && (pDmxfilter->buffer.count <= 0))
   {
      return(-EWOULDBLOCK);
   }

   /*-------------------------------------------------------------------------+
   | Sleep upon filter's queue, waiting data available
   +-------------------------------------------------------------------------*/
   flag = os_enter_critical_section();

   if (pDmxfilter->buffer.count <= 0)
   {
      if (pDmxfilter->para.timeout > 0)
      {
         interruptible_sleep_on_timeout(&pDmxfilter->buffer.queue,
         (UINT)(pDmxfilter->para.timeout/10));
      } else {
         interruptible_sleep_on(&pDmxfilter->buffer.queue);
      }

      os_leave_critical_section(flag);

      if (signal_pending(current))
      {
         return(-1);
      }
   }

   os_leave_critical_section(flag);

   if (pDmxfilter->buffer.count <= 0)
   {
      return(-ETIMEDOUT);
   }

   if (pDmxfilter->type == FILTER_TYPE_SEC)
   {
      data = (UCHAR*)MALLOC(sizeof(struct mpeg_section_header));
      if (data == NULL)
      {
        return(XP_ERROR_INTERNAL);
      }

      temp = pDmxfilter->buffer.ulRead;

      for(i=0; i<sizeof(struct mpeg_section_header); i++)
      {
         data[i] = pDmxfilter->buffer.plData[temp];
         temp = (temp+1)%pDmxfilter->buffer.ulSize;
      }

      SectionHeader = (SECTION_HEADER_PTR)(data);
      if(count < SectionHeader->sectionLength + 3)
      {
         FREE(data);
         return(-EFAULT);
      }

      count = SectionHeader->sectionLength + 3;
      FREE(data);

      /*----------------------------------------------------------------------+
      | Transfer data from the internel circular buffer to user space
      +----------------------------------------------------------------------*/
      for (i = 0; i < count; i++)
      {
         flag = os_enter_critical_section();
         pDmxfilter->buffer.count--;
         os_leave_critical_section(flag);

         put_user(pDmxfilter->buffer.plData[pDmxfilter->buffer.ulRead], buf++);
         pDmxfilter->buffer.ulRead = (pDmxfilter->buffer.ulRead + 1) % pDmxfilter->buffer.ulSize;
      }
   } else {
      /*----------------------------------------------------------------------+
      | If the count user specified is smaller than recieved size, return
      +----------------------------------------------------------------------*/
      os_get_mutex(hMutex);
      flag = os_enter_critical_section();

      if (count >= pDmxfilter->buffer.count) {
         count = pDmxfilter->buffer.count;
      }
      os_leave_critical_section(flag);

      if (count == 0) {
         os_release_mutex(hMutex);
         return(-EFAULT);
      }

      /*----------------------------------------------------------------------+
      | Transfer data from the internel circular buffer to user space
      +----------------------------------------------------------------------*/
      BErem = pDmxfilter->buffer.ulSize - pDmxfilter->buffer.ulRead;
      if (BErem < count) {
         copy_to_user(buf,&pDmxfilter->buffer.plData[pDmxfilter->buffer.ulRead],BErem);
         copy_to_user(&buf[BErem],&pDmxfilter->buffer.plData[0],count-BErem);
      } else {
         copy_to_user(buf,&pDmxfilter->buffer.plData[pDmxfilter->buffer.ulRead],count);
      }

      flag = os_enter_critical_section();
      pDmxfilter->buffer.ulRead = (pDmxfilter->buffer.ulRead+count) %
                                   pDmxfilter->buffer.ulSize;
      pDmxfilter->buffer.count  = pDmxfilter->buffer.count - count;
      os_leave_critical_section(flag);
      os_release_mutex(hMutex);
   }

   return(count);
}

/*----------------------------------------------------------------------------+
|  demux_filter_ioctl
+----------------------------------------------------------------------------*/
int demux_filter_ioctl(
   UINT         uDeviceIndex,
   struct inode *inode,
   struct file  *filp,
   unsigned int cmd,
   ULONG        arg)

{
   GLOBAL_RESOURCES *pGlobal;
   DEMUX_DEVICE     *pDemuxDev;
   DEMUX_FILTER     *pDmxfilter;


   PDEBUG("demux_filter_ioctl(): Entering - cmd=0x%x\n", cmd);

   //get the filter from the file private section, which is determined by process
   pDmxfilter = (DEMUX_FILTER *) filp->private_data;
   pGlobal    = &XpGlobal[uDeviceIndex];
   pDemuxDev  = pDemux_dev[uDeviceIndex];

   if (!pDmxfilter && pDemuxDev->uDeviceIndex != uDeviceIndex)
   {
      return(-EINVAL);
   }

   switch (cmd)
   {
      case DEMUX_START:
         if (pDmxfilter->states < FILTER_STAT_SET)
         {
            return(-EINVAL);
         }
         return DemuxFilterStart(pGlobal,pDmxfilter);
         break;

      case DEMUX_FILTER_SET_FLAGS:
         if (pDmxfilter->states != FILTER_STAT_ALLOC)
         {
           return(-EINVAL);
         }
         return DemuxFilterSetFlags(pGlobal,pDmxfilter,(int)arg);
         break;

      case DEMUX_STOP:
         if (pDmxfilter->states < FILTER_STAT_START)
         {
           return(-EINVAL);
         }
         return DemuxFilterStop(pGlobal,pDmxfilter);
         break;

      case DEMUX_FILTER_SET:
         //Get the Table section filter parameters from user space
         if (copy_from_user(&pDmxfilter->para, (void *) arg,
             sizeof(struct demux_filter_para)))
         {
            return(-EFAULT);
         }
         return DemuxFilterSectionSet(pGlobal,pDmxfilter);
         break;

      case DEMUX_FILTER_PES_SET:
         //Get the PES filter parameters from user space
         if (copy_from_user(&pDmxfilter->pes_para, (void *)arg,
                            sizeof(struct demux_pes_para))) {
            return(-EFAULT);
         }

         if (pDmxfilter->pes_para.output == OUT_DECODER &&
             pGlobal->uDeviceIndex == 0) {
            return DemuxFilterPESDecoderSet(pGlobal,pDmxfilter);
         } else if (pDmxfilter->pes_para.output == OUT_MEMORY   ||
                    pDmxfilter->pes_para.output == OUT_NOTHING) {
            return DemuxFilterPESMemorySet(pGlobal,pDmxfilter);
         }
         break;

      case DEMUX_FILTER_TS_SET:
         return DemuxFilterTSSet(pGlobal,pDmxfilter);
         break;

      case DEMUX_SET_BUFFER_SIZE:
         return DemuxSetBufferSize(pGlobal,pDmxfilter, arg);
         break;

      case DEMUX_SELECT_SOURCE:
         return DemuxSelectSource(pGlobal,(STREAM_SOURCE)arg);
         break;

      case DEMUX_SET_ACPM:
         return DemuxSetACPM(pGlobal,arg);
         break;

      case DEMUX_SET_VCPM:
         return DemuxSetVCPM(pGlobal,arg);
         break;

      case DEMUX_GET_FILTER_NUM:
      {
         INT filter_number_in_demux;

         os_get_mutex(hMutex);
         filter_number_in_demux = xp_osi_filter_get_available(pGlobal,DEFAULT_FILTER_LENGTH);
         os_release_mutex(hMutex);

         if(copy_to_user((void *) arg,
            &filter_number_in_demux,sizeof(filter_number_in_demux)))
         {
            return(-EFAULT);
         }
         return(0);
         break;
      }

      case DEMUX_FILTER_BUCKET_SET:
         if (copy_from_user(&pDmxfilter->bucket_para, (void *) arg,
             sizeof(struct demux_bucket_para)))
         {
            return(-EFAULT);
         }
         return DemuxFilterBucketSet(pGlobal,pDmxfilter);
         break;

      case DEMUX_SET_DEFAULT_FILTER_LENGTH:
         DEFAULT_FILTER_LENGTH = arg;
         return(0);
         break;

      case DEMUX_REGISTER_STC:
         if (xp_osi_clk_set_stc_compare(pGlobal,arg))
         {
            return(-EINVAL);
         }
         if (xp_osi_clk_set_stc_event_notify(pGlobal,DemuxStcEvent))
         {
            return(-EINVAL);
         }
         break;

     case DEMUX_RELEASE_STC_EVENT:
         return xp_osi_clk_release_stc_event(pGlobal);
         break;

     case DEMUX_GET_STC_EVENT:
         if (stc_notify_fn == NULL)
         {
            return(-EFAULT);
         }

         interruptible_sleep_on(&stc_WaitQ);

         if (copy_to_user((void *) arg, &pGlobal->stc_event,sizeof(pGlobal->stc_event)))
         {
            return(-EFAULT);
         }
         break;

      case DEMUX_GET_CURRENT_STC:
      {
         STC_TYPE stc_type;
         xp_osi_clk_get_current_stc(pGlobal,&stc_type);
         if(copy_to_user((void *)arg, &stc_type, sizeof(STC_TYPE)))
         {
            return(-EFAULT);
         }
         break;
      }
      
      case DEMUX_FILTER_GET_QUEUE:
      {
         queue_para qpara;
         int rc;
	 	 
	 rc = demux_filter_get_queue(filp,pGlobal,pDmxfilter,&qpara);
         if(copy_to_user((void *)arg, &qpara, sizeof(queue_para)))
         {
	    printk("copy_to_user failed\n");
            return(-EFAULT);
         }
	 return(rc);
         break;
      }
      
      case DEMUX_FILTER_SET_READPTR:
      {
         int rc;
 
 
	 rc = demux_filter_set_readptr(filp,pGlobal,pDmxfilter,arg);
	 return(rc);
         break;
      }
      default:
         return(-EINVAL);
         break;
   }

   return(0);
}

static void vma_open(struct vm_area_struct *vma)
{
  return;
}

static void vma_close(struct vm_area_struct *vma)
{
  DEMUX_FILTER *pDmxfilter;
  
  pDmxfilter = vma->vm_private_data;
  pDmxfilter->flags &= ~FILTER_FLAG_MMAPPED;
  return;
}


int demux_filter_mmap(UINT uDeviceIndex, struct file *file, struct vm_area_struct *vma)
{
  GLOBAL_RESOURCES *pGlobal;
  DEMUX_FILTER *pDmxfilter;
  QUEUE_PTR pQue;     /* channel information          */
  unsigned long size;
  
  pDmxfilter = (DEMUX_FILTER *)file->private_data;
  pGlobal    = &XpGlobal[uDeviceIndex];
  pQue = &pGlobal->QueueInfo.XpQueueChData[pDmxfilter->chid];

  if (pDmxfilter->flags & FILTER_FLAG_MMAPPED)
  {
    printk("demux_filter_mmap: error, filter already mapped\n");
    return(-1);
  }
  if (pDmxfilter->states != FILTER_STAT_SET) 
  {
    printk("demux_filter_mmap: error, state != FILTER_STAT_SET\n");
    return(-1);
  }
  
  if(pQue->hMem == 0)
    return(-1);
      
  if(vma->vm_pgoff != 0)
    return(-1);
    
  size = vma->vm_end - vma->vm_start;
  if(size != pDmxfilter->buffer.ulSize)
      return(-1);
      
  vma->vm_page_prot.pgprot |= _PAGE_NO_CACHE;     // map without caching
  if(remap_page_range(vma->vm_start, 
                      os_get_physical_address(pQue->hMem),  
                      size, vma->vm_page_prot))
    return -1;

  vma->vm_private_data = pDmxfilter;
  pDmxfilter->flags |= FILTER_FLAG_MMAPPED;
  vma->vm_ops = &my_vm_ops;
  return(0);  
}
  

