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
| Author:    Maciej P. Tyrlik
| Component: xp
| File:      xp_osi_global.h
| Purpose:   Global resources declaration
| Changes:
|
| Date:      Author  Comment:
| ---------  ------  --------
| 30-Sep-01  LINGH    Create
+----------------------------------------------------------------------------*/

#ifndef XP_OSI_GlOBAL_H

#define XP_OSI_GLOBAL_H





#include <os/os-generic.h>

#include <os/os-types.h>

#include <os/pm-alloc.h>

#include <os/os-interrupt.h>



typedef  struct global_resources_t GLOBAL_RESOURCES;



#include "xp_osi.h"

#include "xp_osi_chann.h"

#include "xp_osi_queue.h"

#include "xp_osi_filte.h"

#include "xp_osi_inter.h"

#include "xp_osi_clk.h"

#include "xp_atom_dcr.h"

#include "xp_osi_key.h"

#include "xp_osi_cchan.h"

#include "xp_osi_pcr.h"

#include "pvr_osi.h"

#include "hw/hardware.h"



#include <os/os-sync.h>





#define XP_IRQ IRQ_XPORT



typedef struct channel_info_type

{

    XP_CHANNEL_STATUS       Status;

    USHORT                  uwPid;

    XP_CHANNEL_UNLOAD_TYPE  UnloadType;

    XP_CHANNEL_NOTIFY_FN    notify_fn;

    XP_CHANNEL_DESCRAMBLE   Descram;

    USHORT                  uwKeyId;

} CHANNEL_INFO_TYPE;



typedef struct chann_related_info_t

{

    SHORT                   wXpChannelBucketId;

    CHANNEL_INFO_TYPE       XpChannelData[32];

} CHANN_RELATED_INFO;





typedef struct filter_type

{

    XP_FILTER_STATUS state;

    XP_FILTER_PENDING pending;

    SHORT wSoftFilter;

    SHORT wChannelId;

    SHORT wMatchId;

    USHORT uwLength;

    USHORT uwHwBlockCount;

    UCHAR  cHwBlockId[XP_FILTER_MAX_BLOCKS];

    UCHAR  cData[XP_FILTER_MAX_BLOCKS];

    UCHAR  cMask[XP_FILTER_MAX_BLOCKS];

    UCHAR  cPolarity[XP_FILTER_MAX_BLOCKS];

} FILTER_TYPE, *FILTER_PTR;





typedef struct hw_filter_type

{

    unsigned inuse:1;

    unsigned endOfColumn:1;

    unsigned next:6;

    unsigned prev:6;

} HW_FILTER_TYPE, *HW_FILTER_PTR;



typedef struct filter_channel_type

{

    SHORT wInuse;

    SHORT wCount;

    SHORT wFilterId[XP_CHANNEL_COUNT];

    SHORT wFirstHwFilterId;

    SHORT wPendingRequest;

    XP_FILTER_SHORT ShortFilter;

    XP_FILTER_ERROR_TYPE Errors;

} FILTER_CHANNEL_TYPE, *FILTER_CHANNEL_PTR;





typedef struct filter_related_info_t

{

    FILTER_TYPE             *pXpFilterData[XP_FILTER_MAX_BLOCKS];

    HW_FILTER_TYPE          XpFilterHwData[XP_FILTER_MAX_BLOCKS];

    FILTER_CHANNEL_TYPE     XpFilterChData[XP_CHANNEL_COUNT];

    XP_FILTER_SHORT         XpFilterDefaultShort;

    ULONG                   ulXpFilterSectionChange;

} FILTER_RELATED_INFO;





typedef struct interrupt_notify_type

{

    ULONG ulMask;

    PFS notify_fn;

} XP_INTERRUPT_NOTIFY_TYPE, *INTERRUPT_NOTIFY_PTR;



typedef struct interrupt_info_type

{

    SHORT wNotifyCount;

    SHORT wNotifyAlloc;

    INTERRUPT_NOTIFY_PTR pNotify;

} INTERRUPT_INFO_TYPE, *INTERRUPT_INFO_PTR;





typedef struct inter_related_info_t

{

    unsigned                XpInterChanMask;

    unsigned                XpInterPriMask;

    INTERRUPT_INFO_TYPE     XpInterStatInt;

    INTERRUPT_INFO_TYPE     XpInterPriInt;

    INTERRUPT_INFO_TYPE     XpInterChanInt[34];

}   INTER_RELATED_INFO;



typedef struct clk_related_info_t

{

        unsigned    XpClkInconsData;

        unsigned    XpClkNpcrs;

        unsigned    XpClkErrs;

        unsigned    XpClkInstalled;

        SHORT       wXpClkPrevErr;

        LONG        lXpClkPrevDelta;

        unsigned    XpClkPrevPcr;

        unsigned    XpClkPrevStc;

        USHORT      uwXpClkThreshold;

} CLK_RELATED_INFO;



typedef struct queue_lock_type_t

{

  UCHAR *ppBAddr;

  UCHAR *ppEAddr;

} QUEUE_LOCK_TYPE, *QUEUE_LOCK_PTR;





typedef struct queue_type

{

    XP_QUEUE_STATUS     State;

    MEM_HANDLE_T        hMem;

    ULONG               ulQueueAddr;

    ULONG               ulQueueSize;

    UCHAR               *ppProcessAddr;

    UCHAR               *ppLockAddr;

    UCHAR               *ppReadAddr;

    SHORT               wRpiStatus;

    SHORT               wLockAlloc;

    SHORT               wLockInuse;

    QUEUE_LOCK_TYPE     *pLockData;

    XP_QUEUE_ERROR_TYPE Errors;

} QUEUE_TYPE, *QUEUE_PTR;





typedef struct queue_related_info_t

{

    QUEUE_TYPE      XpQueueChData[XP_CHANNEL_COUNT];

    ULONG           ulXpQueueBAddr;

    UINT            uXpQueueInited;

} QUEUE_RELATED_INFO;


typedef struct chan_chan_related_info_t
{
    int xp_cchan_audchan_pending;
    SHORT  xp_cchan_audchan;
    USHORT xp_cchan_audpid;
    ULONG  xp_cchan_audsig;
    int    xp_cchan_next;
    USHORT xp_cchan_next_audpid;
    USHORT xp_cchan_next_vidpid;
    USHORT xp_cchan_next_pcrpid;
    USHORT xp_cchan_pcrpid;
    SHORT  xp_cchan_vidchan;
    SHORT  xp_cchan_vidchan_pending;
    USHORT xp_cchan_vidpid;
    ULONG  xp_cchan_vidsig;
    int    xp_cchan_vid_oflw;
    int    xp_cchan_aud_oflw;
    int    xp_cchan_def_oflw;
} CHAN_CHAN_RELATED_INFO;

struct global_resources_t

{

    UINT                    uDeviceIndex;

    CHANN_RELATED_INFO      ChannInfo;

    QUEUE_RELATED_INFO      QueueInfo;

    FILTER_RELATED_INFO     FilterInfo;

    INTER_RELATED_INFO      InterInfo;

    CLK_RELATED_INFO        ClkInfo;

    CHAN_CHAN_RELATED_INFO  ChanChanInfo;

    //lingh added in 5/29/2002
    UINT                    uReservedBufSize;
    void                *gpPMRoot;
    MEM_HANDLE_T            hRoot;

    XP_STC_EVENT        stc_event;

};



#if defined(DEBUG)

#define PDEBUG(fmt, args...)    printk(__FILE__ ":" __FUNCTION__":" fmt,  ##args)

#else

#define PDEBUG(fmt, args...)

#endif



#endif

