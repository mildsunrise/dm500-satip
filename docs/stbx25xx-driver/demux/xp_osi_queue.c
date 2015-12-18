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
|
|   Author    :  Ian Govett
|   Component :  xp
|   File      :  xp_osi_queue.c
|   Purpose   :  Queue Management
|   Changes   :
|   Date       By   Comments
|   ---------  ---  -----------------------------------------------------
|   15-Jan-98       Created
|   30-Sep-01   LGH Ported to linux
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|                        DRAM Queue Management
+-----------------------------------------------------------------------------+
|
|   The following functions provide queue management services for the
|   transport driver.  The xp0_queue_allocate() function allocates a region
|   of space (queue_size) for a specific channel, and xp0_queue_free()
|   disables the queue, and releases the space.  Since the transport has a
|   fixed amount of memory available, the space is partitioned
|   among all the channels in use.  The driver maintains a free list for
|   the management of this limited resource.
|       The state of the queue is enabled, disabled, or reset using the
|   xp0_queue_control() function.  The queue must be enabled before any data
|   can be written to the queue.       When the queue is allocated, the
|   queue state is disabled.  A queue reset is used to reset the write
|   pointer in hardware to the bottom of the queue.  The queue disable
|   operation writes the "stops" register to notify the hardware.

|       The hardware notifies the driver when data is available using the

|   DATA_AVAILABLE interrupt.                  The application registers

|   a function to call when data is available for a specific channel.  The

|   interrupt routine forwards the DATA_AVAILABLE interrupt to the demux

|   thread to process the request.  The demux thread then calls the user

|   callback function when data is available.  The callback function

|   provides a structure containing the channel_id, filter_match word

|   starting address of the data in the queue, and the length (in bytes)

|   of the data.  The memory region described with the notification

|   is "locked" until the application "releases" the memory region using

|   the xp0_queue_unlock_data().

|       A read pointer is used by the hardware to lock memory regions from

|   being overwritten.  Basically, there is a circular queue which contains

|   addresses for the top and bottom of the queue, a read pointer, and a

|   write pointer (ie. writeStart in the hardware spec.)  The

|   addresses used by the hardware will not write beyond the address

|   specified by the read_addr.  The write_start pointer contains the

|   starting location for the data region delivered with the next interrupt.

|       Using the queue_bottom, queue_top, read pointer, and write pointer,

|   the driver manages the movement of the read pointer.  As records

|   are "unlocked" by the application, the driver advances the read pointer

|   (read_addr).

|       This driver does not require sections to be unlocked in the order

|   they are delivered, however, the region being unlocked must be the same

|   size as the region delivered to the application.  The region is locked

|   when is is delivered to the application, and unlocked (and removed)

|   when the application unlocks the region.

|       An application may "unregister" a notification function but never

|   stop the data (channel or queue disable).  In this case, the hardware

|   continues to save the data until either (1) the application registers

|   a notification function, or (2) a read pointer error occurs.

|

|   The queue management functions support memory management within each

|   queue.  Since table sections may be unlocked in any order, memory

|   fragmentation may occur.  The read pointer interrupt is delivered to

|   the driver when the write pointer value would match the read pointer

|   value.  If a notification function is registered, the queue management

|   routines relocate the read and write pointers and restarts the channel.

|   The read and write pointers are relocated as follows.  The driver finds

|   the largest available region within the queue for the channel.  If the *

|   region is large enough (>256) the read and write pointers are updated,

|   and the channel is restarted.

|

+----------------------------------------------------------------------------*/

#if 0

/* The necessary header files */

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

#include "xp_osi_global.h"

#include "xp_atom_reg.h"

/*----------------------------------------------------------------------------+

| Local Defines

+----------------------------------------------------------------------------*/

#define QUEUE_BLOCK_SIZE            4096

#define QUEUE_REGION                0xff000000



#define XPORT_BANK_REGISTER     0x00000000

#define REGION_PLB_BYTE_OFFSET  ((unsigned long) XPORT_BANK_REGISTER)

#define REGION_PLB              (XPORT_BANK_REGISTER >> 2)



/*----------------------------------------------------------------------------+

| Define macros to generate the addresses to access DRAM

| These macros convert between a word index, and a byte address

+----------------------------------------------------------------------------*/

#define MAKE_PLB_ADDR(addr)     ((unsigned char *) (addr | pGlobal->QueueInfo.ulXpQueueBAddr))

#define MAKE_PLB_INDEX(addr)    (((unsigned long) addr) & (~pGlobal->QueueInfo.ulXpQueueBAddr))



//Internal

static SHORT update_read_pointer(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                                 UCHAR *ppAddr);

static void process_read_pointer(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId);

static SHORT unlock_data(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,

                         UCHAR *ppBAddr,UCHAR *ppEAddr);

static short queue_valid(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId);

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

|  update_read_pointer

+----------------------------------------------------------------------------*/

static SHORT update_read_pointer(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                                 UCHAR *ppAddr)

{

    SHORT wRc;

    ULONG ulIndex;

    QUEUE_PTR pChannel;

    UINT32  flag;



    /*------------------------------------------------------------------------+

    |  Convert address to an index aligned on a 256 byte boundary

    |  The hardware requires the read pointer to not wrap onto the first

    |  word of the queue

    +------------------------------------------------------------------------*/

    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];

    ulIndex = MAKE_PLB_INDEX(ppAddr) & 0xffffff00;



    //phisical address

    if(ulIndex == pChannel->ulQueueAddr)

    {

        ulIndex += pChannel->ulQueueSize;

    }



    /*------------------------------------------------------------------------+

    |  Now update the read pointer.  if there was a read pointer error which

    |  occurred then reset, and restart the queue

    +------------------------------------------------------------------------*/

    flag = os_enter_critical_section();

    xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,XP_QCONFIGB_REG_RPTR, wChannelId, ulIndex);

    os_leave_critical_section(flag);



    if(pChannel->wRpiStatus)

    {

        pChannel->wRpiStatus = 0;

        wRc = xp_osi_channel_restart(pGlobal,wChannelId);

        PDEBUG("xp_osi_channel_restart return\n");

        if(wRc)

        {

            return(wRc);

        }

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|  process_read_pointer

+----------------------------------------------------------------------------*/

static void process_read_pointer(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId)

{

    short i;

    ULONG ulData;

    UCHAR *ppBAddr;

    UCHAR *ppEAddr;

    ULONG ulFreeSize;         /* region of locked data                */

    ULONG ulMaxSize;          /* largest region of unlocked space     */

    UCHAR *ppBFree;           /* start address of the free space      */

    UCHAR *ppEFree;           /* ending address of the free space     */

    ULONG ulBIndex;           /* word index of starting address       */

    ULONG ulEIndex;           /* word index of ending address         */

    ULONG ulBQueue;           /* starting index of the queue          */

    ULONG ulEQueue;           /* ending index of the queue            */

    QUEUE_PTR pChannel;

    UINT32  flag;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



    /*------------------------------------------------------------------------+

    |  Make sure there is at least one lock record

    |  Move hardware read pointer to the address last processed.

    +------------------------------------------------------------------------*/

    if(pChannel->wLockInuse <= 0)

    {

        update_read_pointer(pGlobal,wChannelId, pChannel->ppProcessAddr);

        pChannel->Errors.uwProcessRpi++;

        return;

    }



    /*------------------------------------------------------------------------+

    |  Find the largest block of space, so we can move the hardware write

    |  pointer, and restart the queue. To find the largest block, we have to

    |  look at the collection of locked records get the starting address of

    |  the first lock record and the ending address of the last lock record

    |  to calculate any free space across the queue wrap.

    +------------------------------------------------------------------------*/

    else

    {

        ulBQueue = pChannel->ulQueueAddr;

        ulEQueue = ulBQueue + pChannel->ulQueueSize;

        ppBAddr = pChannel->pLockData[0].ppBAddr;

        ppEAddr = pChannel->pLockData[pChannel->wLockInuse-1].ppEAddr;



        /*--------------------------------------------------------------------+

        |  Calculate the total region locked.  Factor in the possible

        |  queue wrap.

        +--------------------------------------------------------------------*/

        if(ppBAddr <= ppEAddr)

        {

            ulFreeSize = pChannel->ulQueueSize - ((unsigned long) (ppEAddr - ppBAddr));

        }

        else

        {

            ulFreeSize = ((unsigned long) (ppBAddr - ppEAddr));

        }



        ulMaxSize = ulFreeSize;

        ppBFree = ppEAddr;

        ppEFree = ppBAddr;



        /*--------------------------------------------------------------------+

        |  Now scan the skip records to see if any are larger regions

        +--------------------------------------------------------------------*/

        for(i=0; i<pChannel->wLockInuse-1; i++)

        {

            ulFreeSize = pChannel->pLockData[i+1].ppBAddr -

                        pChannel->pLockData[i].ppEAddr;



            if(ulFreeSize > ulMaxSize)

            {

                ulMaxSize = ulFreeSize;

                ppBFree = pChannel->pLockData[i].ppEAddr;

                ppEFree = pChannel->pLockData[i+1].ppBAddr;

            }

        }



        ulBIndex = MAKE_PLB_INDEX(ppBFree);

        ulEIndex = MAKE_PLB_INDEX(ppEFree);



        /*--------------------------------------------------------------------+

        |  Make sure the address range is inside the queue bounds

        +--------------------------------------------------------------------*/

        if((ulBIndex < ulBQueue) || (ulBIndex > ulEQueue))

        {

            pChannel->Errors.uwProcessRpi++;

            return;

        }

        if((ulEIndex < ulBQueue) || (ulEIndex > ulEQueue))

        {

            pChannel->Errors.uwProcessRpi++;

            return;

        }



        /*--------------------------------------------------------------------+

        |  The read pointer is on a 256 byte boundary, so if the available

        |  size is less, then leave the interrupt on, otherwise use the

        |  available space

        +--------------------------------------------------------------------*/

        if(ulMaxSize >= 256)

        {

            /*----------------------------------------------------------------+

            |  Convert to a an index, and adjust the start address to align

            |  on a word boundary

            +----------------------------------------------------------------*/

            i = ulBIndex % 4;

            if(i)

            {

                ulBIndex += (4 - i);

            }

            if(ulBIndex == ulEQueue)

            {

                ulBIndex = ulBQueue;

            }



            /*----------------------------------------------------------------+

            |  Write the "stops" register to stop any "in-process" packets

            |  from being delivered, and stop the read/write cycle in hw.

            +----------------------------------------------------------------*/

            ulData = (1 << (31 - wChannelId));



            flag = os_enter_critical_section();



            xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_QSTOPS, ulData);



            /*----------------------------------------------------------------+

            |  Update hardware registers for writeStart & writeNow

            +----------------------------------------------------------------*/

            xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,XP_QSTATD_REG_WSTART,

                                           wChannelId, ulBIndex);

            xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,XP_QSTATB_REG_WPTR,

                                           wChannelId, ulBIndex);

            os_leave_critical_section(flag);



            /*----------------------------------------------------------------+

            |  Re-write the read pointer, and restart the queue

            +----------------------------------------------------------------*/

            update_read_pointer(pGlobal,wChannelId, ppEFree);

/*          update_read_pointer(wChannel_Id, channel->process_addr); */



            /*----------------------------------------------------------------+

            |  Change the read pointer in the driver, and clear the read

            |  pointer indicator

            +----------------------------------------------------------------*/

            pChannel->ppProcessAddr = MAKE_PLB_ADDR(ulBIndex);

            pChannel->ppReadAddr    = MAKE_PLB_ADDR(ulBIndex);

            pChannel->ppLockAddr    = ppEFree;

        }

    }

}



/*----------------------------------------------------------------------------+

|  unlock_data

+----------------------------------------------------------------------------*/

static SHORT unlock_data(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,

                         UCHAR *ppBAddr,UCHAR *ppEAddr)

{

    short i;

    short j;

    SHORT wIndex;

    QUEUE_PTR pChannel;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



    if((pChannel->wLockInuse == 0) || (pChannel->pLockData == NULL))

    {

        return(XP_ERROR_QUEUE_ADDRESS);

    }



    /*------------------------------------------------------------------------+

    |  Find the index which contains both the start and ending addresses

    +------------------------------------------------------------------------*/

    for(wIndex=0; wIndex < pChannel->wLockInuse; wIndex++)

    {

        if((ppBAddr == pChannel->pLockData[wIndex].ppBAddr) ||

           (ppEAddr == pChannel->pLockData[wIndex].ppEAddr))

        {

            break;

        }

    }



    /*------------------------------------------------------------------------+

    |  Check if we couldn't find the block to unlock remove the lock entry

    |  if we've an exact match

    +------------------------------------------------------------------------*/

    if(wIndex == pChannel->wLockInuse)

    {

        pChannel->Errors.uwRegionNotFound++;

        return(XP_ERROR_QUEUE_ADDRESS);

    }



    if((ppBAddr == pChannel->pLockData[wIndex].ppBAddr) && (ppEAddr == pChannel->pLockData[wIndex].ppEAddr))

    {

        for(i=wIndex, j=i+1; j<pChannel->wLockInuse; i++, j++)

        {

            pChannel->pLockData[i] = pChannel->pLockData[j];

        }

        pChannel->wLockInuse--;

    }

    else if(ppBAddr == pChannel->pLockData[wIndex].ppBAddr)

    {

        pChannel->pLockData[wIndex].ppBAddr = ppEAddr;

    }

    else

    {

        pChannel->pLockData[wIndex].ppEAddr = ppBAddr;

    }



    /*------------------------------------------------------------------------+

    |  If the record being unlocked matches the current read pointer in

    |  hardware, then we have to move the read pointer

    +------------------------------------------------------------------------*/

    if(pChannel->ppLockAddr == ppBAddr)

    {

        /*--------------------------------------------------------------------+

        |  If there are no more lock records, then use the most recent

        |  region processed

        +--------------------------------------------------------------------*/

        if(pChannel->wLockInuse == 0)

        {

            update_read_pointer(pGlobal,wChannelId, pChannel->ppProcessAddr);

        }



        /*--------------------------------------------------------------------+

        |  If there is another entry in the list, then use address of the

        |  next entry, otherwise use the address in the beginning of the list

        +--------------------------------------------------------------------*/

        else

        {

            if(wIndex < pChannel->wLockInuse)

            {

                pChannel->ppLockAddr = pChannel->pLockData[wIndex].ppBAddr;

            }

            else

            {

                pChannel->ppLockAddr = pChannel->pLockData[0].ppBAddr;

            }

            update_read_pointer(pGlobal,wChannelId, pChannel->ppLockAddr);

        }

    }



    /*------------------------------------------------------------------------+

    |  if we're in an RPI condition, then try to find a new region to get

    |  going again

    +------------------------------------------------------------------------*/

    else if(pChannel->wRpiStatus)

    {

        process_read_pointer(pGlobal,wChannelId);

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|  queue_valid

+----------------------------------------------------------------------------*/

static short queue_valid(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId)

{

    if(pGlobal->QueueInfo.XpQueueChData[wChannelId].State == XP_QUEUE_STATUS_UNDEFINED)

    {

        return(XP_ERROR_QUEUE_UNDEFINED);

    }

    return(0);

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


void xp_osi_queue_de_init(GLOBAL_RESOURCES *pGlobal)
{
    if(pGlobal->gpPMRoot != NULL)
    {
        __pm_alloc_physical_deinit(pGlobal->gpPMRoot);
        os_free_physical(pGlobal->hRoot);
        pGlobal->gpPMRoot = NULL;
        pGlobal->hRoot = NULL;
    }
}


/*----------------------------------------------------------------------------+

|  xp0_queue_init

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_init(GLOBAL_RESOURCES *pGlobal)

{
    short i;
    short rc=0;
    ULONG ulAddr;
    UINT32  flag;
    MEM_HANDLE_T hRoot;
    UINT uPhyAddr;
    void *pLogicalAddr;
    /*------------------------------------------------------------------------+
    |  Free any space from a previous initialization
    +------------------------------------------------------------------------*/

    if(pGlobal->QueueInfo.uXpQueueInited)

    {
        for(i=0; i<XP_CHANNEL_COUNT; i++)
        {
            if(pGlobal->QueueInfo.XpQueueChData[i].pLockData)
            {
                FREE(pGlobal->QueueInfo.XpQueueChData[i].pLockData);
            }
        }
    }

    pGlobal->QueueInfo.uXpQueueInited = 1;



    /*------------------------------------------------------------------------+

    |  Setup the channels

    +------------------------------------------------------------------------*/

    for(i=0; i<XP_CHANNEL_COUNT; i++)

    {

        pGlobal->QueueInfo.XpQueueChData[i].State       = XP_QUEUE_STATUS_UNDEFINED;

        pGlobal->QueueInfo.XpQueueChData[i].ulQueueAddr  = 0;

        pGlobal->QueueInfo.XpQueueChData[i].ulQueueSize  = 0;

        pGlobal->QueueInfo.XpQueueChData[i].wLockAlloc  = 0;

        pGlobal->QueueInfo.XpQueueChData[i].wLockInuse  = 0;

        pGlobal->QueueInfo.XpQueueChData[i].pLockData   = NULL;

        pGlobal->QueueInfo.XpQueueChData[i].hMem    = NULL;

        xp_osi_queue_reset_errors(pGlobal,i);

    }



    /*------------------------------------------------------------------------+
    |  If the address is to be used, then make sure the address is
    |  properly aligned
    +------------------------------------------------------------------------*/
//lingh added in 5/29/2002

    __os_alloc_physical_heap_walk();
    hRoot = os_alloc_physical_justify(pGlobal->uReservedBufSize, 0x400000);
    __os_alloc_physical_heap_walk();

    if(hRoot == NULL )
    {
        return -1;
    }

    pGlobal->hRoot = hRoot;

    uPhyAddr = os_get_physical_address(hRoot);
    pLogicalAddr = os_get_logical_address(hRoot);


    ulAddr = uPhyAddr;

    pGlobal->gpPMRoot = __pm_alloc_physical_init(uPhyAddr, pLogicalAddr,
        pGlobal->uReservedBufSize,256);
    __os_alloc_physical_heap_walk();

    if(pGlobal->gpPMRoot == NULL)
    {
        os_free_physical(hRoot);
        return -1;
    }
    /*------------------------------------------------------------------------+
    |  We have an aligned address, so save the address onto the freelist,
    |  and setup the hardware base address
    +------------------------------------------------------------------------*/

    pGlobal->QueueInfo.ulXpQueueBAddr = ulAddr & 0xff000000;



    flag = os_enter_critical_section();

    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_QBASE, pGlobal->QueueInfo.ulXpQueueBAddr);

    os_leave_critical_section(flag);



    pGlobal->QueueInfo.ulXpQueueBAddr |= REGION_PLB_BYTE_OFFSET;



    return(rc);

}





/*----------------------------------------------------------------------------+

|  xp0_queue_get_status

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_get_status(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                              XP_QUEUE_STATUS *pStatus)



{

    short rc;



//    xp_os_semaphore_wait();



    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc == 0)

    {

        *pStatus = pGlobal->QueueInfo.XpQueueChData[wChannelId].State;

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_process_data

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  define a queue for data unloaded on this channel

|

|  PROTOTYPE  :  short xp0_queue_process_data(

|                short wChannel_Id,

|                CHANNEL_UNLOAD_TYPE unload_type,

|                XP_CHANNEL_NOTIFY_FN notify_fn)

|

|  ARGUMENTS  :  wChannel_Id    -  the channel id as returned from the

|                                 xp0_channel_allocate().

|                unload_type   -  type of data being unloaded

|

|  RETURNS    :

|

|  COMMENTS   :  This function determines the amount of data to process.

|                The starting address of the data to process is the

|                address where we last read (read_addr).  The ending

|                address is acquired from hardware.

|                    If the unload type is for tables, then the tables are

|                determined and the application is notified for each

|                completed table.

|

+----------------------------------------------------------------------------*/

void xp_osi_queue_process_data(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                               XP_CHANNEL_UNLOAD_TYPE UnloadType, XP_CHANNEL_NOTIFY_FN notify_fn)

{

    short rc;

    SHORT wSectionFilter;       /* 1=filtering enabled, 0=no filtering  */

    ULONG ulWriteStart;         /* hardware write pointer               */

    UCHAR *ppBQueue=NULL;       /* starting address of the queue        */

    UCHAR *ppEQueue=NULL;       /* ending address of the queue          */

    UCHAR *ppBAddr=NULL;        /* start address of the data            */

    UCHAR *ppEAddr=NULL;        /* end address of the data              */



    UCHAR *plBQueue=NULL;       /* starting address of the queue        */

    UCHAR *plEQueue=NULL;       /* ending address of the queue          */

    UCHAR *plBAddr=NULL;        /* start address of the data            */

    UCHAR *plEAddr=NULL;        /* end address of the data              */





    ULONG ulLength;             /* ending address of the data           */

    XP_CHANNEL_NOTIFY_DATA Info;    /* notification data                    */

    QUEUE_PTR pChannel;

    UINT32  flag;





    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



//    xp_os_semaphore_wait();



    /*------------------------------------------------------------------------+

    |  Make sure the channel is still allocated

    +------------------------------------------------------------------------*/

    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc == 0)

    {

        flag = os_enter_critical_section();

        rc = xp_atom_dcr_read_register_channel(pGlobal->uDeviceIndex,XP_QSTATD_REG_WSTART,

                                       wChannelId, &ulWriteStart);

        os_leave_critical_section(flag);

    }

    if(rc == 0)

    {

        /*--------------------------------------------------------------------+

        |  Get the queue range, and the address range of the data

        +--------------------------------------------------------------------*/

        ppBAddr  = pChannel->ppReadAddr;

        ppEAddr  = MAKE_PLB_ADDR(ulWriteStart);

        ppBQueue = MAKE_PLB_ADDR(pChannel->ulQueueAddr);

        ppEQueue = ppBQueue + pChannel->ulQueueSize;

    PDEBUG("ppBAddr = %8.8x\n",ppBAddr);
    PDEBUG("ppEAddr = %8.8x\n",ppEAddr);
    PDEBUG("ppBQueue = %8.8x\n",ppBQueue);
    PDEBUG("ppEQueue = %8.8x    \n",ppEQueue);

        /*--------------------------------------------------------------------+

        |  make sure the address range is within the bounds of the queue

        +--------------------------------------------------------------------*/

        if((ppBAddr < ppBQueue) || (ppEAddr > ppEQueue))

        {
    PDEBUG("XP_ERROR_QUEUE_ADDRESS1\n");

            pChannel->Errors.uwQueueAddress++;

            rc = XP_ERROR_QUEUE_ADDRESS;

        }



        /*--------------------------------------------------------------------+

        |  Abort if there is no data

        +--------------------------------------------------------------------*/

        else if(ppBAddr == ppEAddr)

        {
    PDEBUG("XP_ERROR_QUEUE_ADDRESS2\n");

            pChannel->Errors.uwNoData++;

            rc = XP_ERROR_QUEUE_ADDRESS;

        }

    }

    if(rc == 0)

    {

        /*--------------------------------------------------------------------+

        |  Check if queue is not active, then we'll skip the new record

        |  which must have arrived in the hardware when we changed the state

        |  to DISABLED.

        +--------------------------------------------------------------------*/

        if(pChannel->State != XP_QUEUE_STATUS_ENABLED)

        {
    PDEBUG("pChannel->Errors.uwQueueDisabled\n");

            pChannel->Errors.uwQueueDisabled++;

        }

        else

        {

            /*----------------------------------------------------------------+

            |  Calculate the number of bytes, and adjust the read pointer.

            |  We must also check if a queue wrap occurred.

            +----------------------------------------------------------------*/
        PDEBUG("ppBAddr = %8.8x\n",ppBAddr);
        PDEBUG("ppEAddr = %8.8x\n",ppEAddr);
        PDEBUG("ppBQueue = %8.8x\n",ppBQueue);
        PDEBUG("ppEQueue = %8.8x        \n",ppEQueue);

            if(ppBAddr <= ppEAddr)

            {
    PDEBUG("ppBAddr <= ppEAddr\n");

                ulLength = ppEAddr - ppBAddr;

            }

            else

            {
        PDEBUG("2\n");

                ulLength = (ppEQueue - ppBAddr) + (ppEAddr - ppBQueue);

            }

       PDEBUG("uulLength = %8.8x\n",ulLength);


            /*----------------------------------------------------------------+

            |  If the application has requested notification, then process

            |  the data, otherwise the hardware will continue to advance the

            |  writeStart until a read pointer error occurs or the application

            |  sets the notify function.

            +----------------------------------------------------------------*/

            if(notify_fn)

            {

                /*------------------------------------------------------------+

                |  For table unloading, process each table section

                |  The sectionFilter = 0 if the unload type does NOT require

                |  filtering, 1=filtering tables

                +------------------------------------------------------------*/

                if(UnloadType >= XP_CHANNEL_UNLOAD_PSI)

                {

                    wSectionFilter = (UnloadType % 2);

                    plBQueue = (UCHAR*)os_get_logical_address(pChannel->hMem);

                    plEQueue = plBQueue + pChannel->ulQueueSize;

                    plBAddr = plBQueue+(ULONG)ppBAddr - (ULONG)pChannel->ulQueueAddr;

                    plEAddr = plBQueue+(ULONG)ppEAddr - (ULONG)pChannel->ulQueueAddr;



                    xp_osi_filter_process_table_data(pGlobal,wChannelId, plBAddr, plEAddr,

                              plBQueue, plEQueue, wSectionFilter, notify_fn);

                }



                /*------------------------------------------------------------+

                |  Keep track of the record locked call the application to

                |  notify of data available

                +------------------------------------------------------------*/

                else

                {

                    Info.pGlobal        = pGlobal;

                    Info.wChannelId     = wChannelId;

                    Info.ulMatchWord    = 0;

                    Info.plData         = (UCHAR*)((ULONG)os_get_logical_address(pChannel->hMem)

                        +(ULONG)ppBAddr - (ULONG)pChannel->ulQueueAddr);

                    Info.ulLength       = ulLength;



                    xp_osi_queue_lock_data(pGlobal,wChannelId, XP_QUEUE_MODE_LOCK,

                                       ppBAddr, ppEAddr);

                    (notify_fn)(&Info);

                }



                /*------------------------------------------------------------+

                |  Update the read pointer after processing the data

                +------------------------------------------------------------*/

                pGlobal->QueueInfo.XpQueueChData[wChannelId].ppReadAddr = ppEAddr;

            }

        }

    }

//    xp_os_semaphore_signal();

}



/*----------------------------------------------------------------------------+

|  xp0_queue_process_interrupt

+----------------------------------------------------------------------------*/

short xp_osi_queue_process_interrupt(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                                     ULONG ulInterrupt)

{

    short rc;

    QUEUE_PTR pChannel;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



//    xp_os_semaphore_wait();



    /*------------------------------------------------------------------------+

    |  Make sure the channel is still allocated

    +------------------------------------------------------------------------*/

    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc == 0)

    {

        /*--------------------------------------------------------------------+

        |  Try to recover from a read pointer error

        +--------------------------------------------------------------------*/

        if(ulInterrupt & XP_INTERRUPT_QSTAT_RPI)

        {

            pChannel->Errors.uwReadPointer++;

            pChannel->wRpiStatus = 1;

            process_read_pointer(pGlobal,wChannelId);

        }

        if(ulInterrupt & XP_INTERRUPT_QSTAT_CRCE)

        {

            pChannel->Errors.uwCrc32++;

        }

        if(ulInterrupt & XP_INTERRUPT_QSTAT_TSLE)

        {

            pChannel->Errors.uwSectionLength++;

        }

        if(ulInterrupt & XP_INTERRUPT_QSTAT_PSE)

        {

            pChannel->Errors.uwCc++;

        }

    }

//    xp_os_semaphore_signal();



    return(rc);

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

|  xp0_queue_allocate

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  define a queue for data delivered on this channel

|

|  PROTOTYPE  :  short xp0_queue_allocate(

|                short wChannel_Id,

|                unsigned long queue_size

|

|  ARGUMENTS  :  wChannel_Id    -  the channel id as returned from the

|                                 xp0_channel_allocate().

|                queue_size    -  minimum size of queue in bytes

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel id is not defined

|                XP_ERROR_OUT_OF_SPACE     -  space is not available for a

|                                          a queue of the requested size

|

|  COMMENTS   :  defines a queue for use by the channel specified.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_allocate(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, ULONG ulQueueSize)

{

    short rc;

    UCHAR *ppAddr;

    unsigned long u;

    ULONG ulQueueTop;

    ULONG ulQueueBottom;

    QUEUE_PTR pChannel;

    UINT32  flag;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



//    xp_os_semaphore_wait();



    rc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(rc == 0)

    {

        if(queue_valid(pGlobal,wChannelId) == 0)

        {

            rc = XP_ERROR_QUEUE_DEFINED;

        }

    }



    if(rc == 0)

    {

        u = ulQueueSize % QUEUE_BLOCK_SIZE;

        if(u)

        {

            ulQueueSize += (QUEUE_BLOCK_SIZE - u);

        }



        if(pChannel->wLockAlloc > 0)

        {

            FREE(pChannel->pLockData);

        }


//lingh
        pChannel->hMem = pm_alloc_physical(pGlobal->gpPMRoot,ulQueueSize);

        if(pChannel->hMem == NULL)
            return -1;

    PDEBUG("pm_alloc_physical, phy = %x, logical = %x\n",os_get_physical_address(pChannel->hMem),os_get_logical_address(pChannel->hMem));


        ppAddr = MAKE_PLB_ADDR(os_get_physical_address(pChannel->hMem));



        pChannel->ulQueueAddr       = os_get_physical_address(pChannel->hMem);

        pChannel->ulQueueSize       = ulQueueSize;

        pChannel->wLockInuse        = 0;

        pChannel->wLockAlloc        = 0;

        pChannel->pLockData         = NULL;

        pChannel->ppLockAddr        = NULL;

        pChannel->ppProcessAddr     = ppAddr;



        xp_osi_queue_reset_errors(pGlobal,wChannelId);



        /*--------------------------------------------------------------------+

        |  Write the new queue definition, and initialize the queue state

        |  to disabled

        +--------------------------------------------------------------------*/

        ulQueueBottom = pChannel->ulQueueAddr / QUEUE_BLOCK_SIZE;

        ulQueueTop    = (pChannel->ulQueueAddr +  pChannel->ulQueueSize) / QUEUE_BLOCK_SIZE;



        flag = os_enter_critical_section();

        xp_atom_dcr_init_queue(pGlobal->uDeviceIndex,wChannelId, ulQueueTop, ulQueueBottom);

        os_leave_critical_section(flag);



        pChannel->State = XP_QUEUE_STATUS_DISABLED;



        /*--------------------------------------------------------------------+

        |  Reset the queue.  This will cause the hardware to set the

        |  read_address = queue_bottom

        +--------------------------------------------------------------------*/

        rc = xp_osi_queue_control(pGlobal,wChannelId, XP_QUEUE_CONTROL_RESET);

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_control

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  controls the state of the queue

|

|  PROTOTYPE  :  short xp0_queue_control(

|                short wChannel_Id,

|                XP_QUEUE_CONTROL_TYPE cmd)

|

|  ARGUMENTS  :  wChannel_Id    -  the channel id as returned from the

|                                 xp0_channel_allocate().

|                cmd           -  XP_QUEUE_CONTROL_ENABLE  -  enable queue

|                                 XP_QUEUE_CONTROL_DISABLE -  disable queue

|                                 XP_QUEUE_CONTROL_RESET   -  reset the queue

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel id is not defined

|                XP_ERROR_INTERNAL         -  internal driver error

|

|  COMMENTS   :  controls the current queue state.  XP_QUEUE_CONTROL_DISABLE

|                prevents the delivery of new data.  Existing data in the

|                queue is undisturbed.  XP_QUEUE_CONTROL_ENABLE restarts a

|                disabled queue.  XP_QUEUE_CONTROL_RESET disables the queue,

|                resets the read/write pointers, and unlocks all locked

|                sections for the channel.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_control(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, XP_QUEUE_CONTROL_TYPE Cmd)

{

    short rc;

    ULONG ulData;

    QUEUE_PTR pChannel;

    UINT32  flag;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



//    xp_os_semaphore_wait();



    rc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(rc == 0)

    {

        rc = queue_valid(pGlobal,wChannelId);

    }

    if(rc == 0)

    {

        switch(Cmd)

        {

        case XP_QUEUE_CONTROL_ENABLE:



            flag = os_enter_critical_section();

            xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,XP_QCONFIGB_REG_ENBL,

                                                     wChannelId, 1);

            os_leave_critical_section(flag);



            pGlobal->QueueInfo.XpQueueChData[wChannelId].State = XP_QUEUE_STATUS_ENABLED;



            rc = xp_osi_channel_restart(pGlobal,wChannelId);

            break;



            /*----------------------------------------------------------------+

            |  Write the "stops" register to stop any "in-process" packets

            |  from being delivered. Clear any pending filter operations

            |  Clear the interrupts to get rid of any latent interrupts

            +----------------------------------------------------------------*/

        case XP_QUEUE_CONTROL_DISABLE:

            ulData = (1 << (31 - wChannelId));



            flag = os_enter_critical_section();

            xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_QSTOPS, ulData);

            os_leave_critical_section(flag);



            xp_osi_filter_process_pending(pGlobal,wChannelId);



            rc = xp_osi_interrupt_channel_control(pGlobal,wChannelId,

                                 XP_INTERRUPT_CONTROL_RESET);



            flag = os_enter_critical_section();

            xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,XP_QCONFIGB_REG_ENBL,

                wChannelId, 0);

            os_leave_critical_section(flag);



            pChannel->State = XP_QUEUE_STATUS_DISABLED;

            break;





            /*----------------------------------------------------------------+

            |  Reset the queue, and set the read_addr to the queue bottom.

            |  Remove all the skip and lock records

            +----------------------------------------------------------------*/



        case XP_QUEUE_CONTROL_RESET:

            if(pChannel->State != XP_QUEUE_STATUS_DISABLED)

            {

                rc = xp_osi_queue_control(pGlobal,wChannelId, XP_QUEUE_CONTROL_DISABLE);

            }



            ulData = (1 << (31 - wChannelId));



            flag = os_enter_critical_section();

            xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_QRESETS, ulData);

            os_leave_critical_section(flag);



            pChannel->ppReadAddr    =   MAKE_PLB_ADDR(pChannel->ulQueueAddr);



            pChannel->ppProcessAddr =   MAKE_PLB_ADDR(pChannel->ulQueueAddr);



            pChannel->wLockInuse = 0;

            pChannel->ppLockAddr  = NULL;

            break;



        default:

            break;

        }

    }

//    xp_os_semaphore_signal();

   PDEBUG("xp_osi_channel_restart return\n");



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_free

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  define a queue for data unloaded on this channel

|

|  PROTOTYPE  :  short xp0_queue_free(

|                short wChannel_Id)

|

|  ARGUMENTS  :  wChannel_Id    -  the channel id as returned from the

|                                 xp0_channel_allocate().

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel id is not defined

|                XP_ERROR_QUEUE_UNDEFINED  -  queue is not currently defined

|

|  COMMENTS   :  frees the queue allocated to the channel.  All locked

|                regions are released.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_free(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId)

{

    short rc;

    QUEUE_PTR pChannel;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];





//    xp_os_semaphore_wait();



    /*------------------------------------------------------------------------+

    |  Check that the wChannel_Id is valid and the queue is currently defined.

    |  Then disable the queue

    +------------------------------------------------------------------------*/

    rc = xp_osi_queue_control(pGlobal,wChannelId, XP_QUEUE_CONTROL_DISABLE);



    if(rc == 0 && pChannel->hMem != NULL)

    {

        //lingh
        pm_free_physical(pGlobal->gpPMRoot,pChannel->hMem);



    }



    pChannel->ulQueueAddr = 0;

    pChannel->ulQueueSize = 0;

    pChannel->State      = XP_QUEUE_STATUS_UNDEFINED;

        /*--------------------------------------------------------------------+

        |  Remove all the skip and lock records

        +--------------------------------------------------------------------*/

    pChannel->wLockInuse = 0;

    pChannel->ppLockAddr  = NULL;



    if(pChannel->wLockAlloc > 0)

    {

        FREE(pChannel->pLockData);

        pChannel->pLockData = NULL;

        pChannel->wLockAlloc = 0;

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_get_config

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_get_config(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                              UCHAR **pBQueue, UCHAR **pEQueue)           /* queue range is exclusive             */

{

    short rc;

    UCHAR *plBQueue;

    UCHAR *plEQueue;

    QUEUE_PTR pChannel;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



//    xp_os_semaphore_wait();



    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc == 0)

    {

        plBQueue = os_get_logical_address(pChannel->hMem);

        plEQueue = plBQueue + pChannel->ulQueueSize;



        *pBQueue = plBQueue;

        *pEQueue = plEQueue;

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_get_errors

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_get_errors(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                              XP_QUEUE_ERROR_PTR pErrors)

{

    short rc;

    QUEUE_PTR pChannel;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];





    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc)

    {

        return(rc);

    }



    memcpy(pErrors, &pChannel->Errors, sizeof(XP_QUEUE_ERROR_TYPE));



    return(0);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_get_size

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_get_size(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId, ULONG *pQueueSize)

{

    short rc;

    QUEUE_PTR pChannel;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



//    xp_os_semaphore_wait();



    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc == 0)

    {

        *pQueueSize = pChannel->ulQueueSize;

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_lock_data

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  add a skip or lock record to the channel data

|

|  PROTOTYPE  :  static short xp0_queue_lock_data(

|                short wChannel_Id,

|                XP_QUEUE_MODE_TYPE mode,

|                unsigned char *b_addr,

|                unsigned char *e_addr)

|

|  ARGUMENTS  :  wChannel_Id    -  the channel id as returned from the

|                                 xp0_channel_allocate().

|                mode          -  LOCK the record or SKIP the record

|                b_addr        -  starting address of the region

|                e_addr        -  ending address+1 of the region

|

|  RETURNS    :

|

|  COMMENTS   :  This function processes requests to lock or skip records

|                The SKIP records only update the address of the last

|                data address processed.  LOCK records are added in sorted

|                order to the lock list. mine whether the end of the

|                this region is the same as the start of the region to

|                reserve and the mode is equivalent.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_lock_data(

GLOBAL_RESOURCES *pGlobal,

SHORT wChannelId,                        /* channel to lock region in        */

XP_QUEUE_MODE_TYPE Mode,                 /* LOCK or SKIP                     */

UCHAR *ppBAddr,                   /* start of region to lock          */

UCHAR *ppEAddr)                   /* end of region to lock            */

{

    short rc=0;

    SHORT wIndex;                         /* location to add new node         */

    short i, j;

    SHORT wLockAlloc;

    QUEUE_LOCK_TYPE *pLockData;

    QUEUE_PTR pChannel;



//    xp_os_semaphore_wait();

    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



    /*------------------------------------------------------------------------+

    |  Save the address of the most recent data processed. This gets used

    |  when the last 'lock' record is unlocked and determines where the

    |  hardware read pointer is moved.

    +------------------------------------------------------------------------*/

    pChannel->ppProcessAddr = ppEAddr;



    /*------------------------------------------------------------------------+

    |  If we've used all the available records allocate more space

    +------------------------------------------------------------------------*/

    if(Mode == XP_QUEUE_MODE_LOCK)

    {

        if(pChannel->wLockInuse >= pChannel->wLockAlloc)

        {

            if (pChannel->wLockAlloc == 0)

            {

                wLockAlloc = 128 / sizeof (QUEUE_LOCK_TYPE);

                pLockData = MALLOC (wLockAlloc * sizeof (QUEUE_LOCK_TYPE));



                if(pLockData == NULL)

                {

                    pChannel->Errors.uwInternal++;

                    rc = XP_ERROR_INTERNAL;

                }

                else

                {

                    pChannel->pLockData  = pLockData;

                    pChannel->wLockAlloc = wLockAlloc;

                }

            }

            else

            {                   /* we can not realloc it */

                pChannel->Errors.uwInternal++;

                rc = XP_ERROR_INTERNAL;

            }

        }



        if(rc == 0)

        {

            if(pChannel->wLockInuse == 0)

            {

                pChannel->ppLockAddr = ppBAddr;

                wIndex = 0;

            }



            /*----------------------------------------------------------------+

            |  Find the location to insert a new node.  Since the list is

            |  in sorted order, the node which wraps is always the last one

            |  in the list

            +----------------------------------------------------------------*/

            else

            {

                for(wIndex=0; wIndex<pChannel->wLockInuse; wIndex++)

                {

                    if(ppBAddr < pChannel->pLockData[wIndex].ppBAddr)

                    {

                        break;

                    }

                }

                if(wIndex < pChannel->wLockInuse)

                {    /* 05/16/98 */

                    /*--------------------------------------------------------+

                    |  Shift all the records down to make room for the node

                    |  to insert at index location 'i'.

                    +--------------------------------------------------------*/

                    for(i=pChannel->wLockInuse, j=i-1; i>wIndex; i--, j--)

                    {

                        pChannel->pLockData[i] = pChannel->pLockData[j];

                    }

                }

            }

            pChannel->pLockData[wIndex].ppBAddr = ppBAddr;

            pChannel->pLockData[wIndex].ppEAddr = ppEAddr;

            pChannel->wLockInuse++;

        }

    }



    /*------------------------------------------------------------------------+

    |  If there are no locked records, then move the read pointer to the

    |  end of the skip

    +------------------------------------------------------------------------*/

    else if(pChannel->wLockInuse == 0)

    {

        update_read_pointer(pGlobal,wChannelId, ppEAddr);

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_reset_errors

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_reset_errors(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId)

{

    short rc;

    QUEUE_PTR pChannel;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc)

    {

        return(rc);

    }



    memset(&pChannel->Errors, 0,  sizeof(XP_QUEUE_ERROR_TYPE));



    return(0);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_unlock

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_unlock(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                          SHORT wBIndex, SHORT wEIndex)

{

    short rc;

    short i;

    ULONG ulBytes;

    QUEUE_PTR pChannel;



    pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



    for(i=wEIndex; i>=wBIndex; i--)

    {

        if(pChannel->pLockData[i].ppBAddr <= pChannel->pLockData[i].ppEAddr)

        {

            ulBytes = pChannel->pLockData[i].ppEAddr -

                    pChannel->pLockData[i].ppBAddr;

        }

        else

        {

            ulBytes = pChannel->ulQueueSize -

                    (pChannel->pLockData[i].ppBAddr -

                    pChannel->pLockData[i].ppEAddr);

        }



        if ((rc = xp_osi_queue_unlock_data(pGlobal,wChannelId,

                  pChannel->pLockData[i].ppBAddr, ulBytes)) != 0)

        {

           return(rc);

        }

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|  xp0_queue_unlock_data

+-----------------------------------------------------------------------------+

|

|   DESCRIPTION:  define a queue for data unloaded on this channel

|

|   PROTOTYPE  :  short xp0_queue_unlock_data(

|                 short wChannel_Id,

|                 unsigned char *buffer,

|                 unsigned long length)

|

|   ARGUMENTS  :  wChannel_Id    -  the channel id as returned from the

|                                  xp0_channel_allocate().

|                 buffer        -  address of data being unlocked

|                 length        -  number of bytes to unlock

|

|   RETURNS    :  0 if successful, nonzero otherwise.

|

|   COMMENTS   :  This function notifies the driver when the application

|                 has finished using the space (buffer [ 0..length-1].

|                 The buffer address always observes the data bounaries

|                 provided during the notification phase.  Locked sections

|                 may be unlocked in any order.

|                     NOTE:  locked regions are not released when a queue

|                 is disabled but are released upon a queue reset.  A queue

|                 may be disabled, and re-enabled without disturbing any

|                 locked sections.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_queue_unlock_data(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                               UCHAR *ppBAddr, ULONG ulLength)

{

    short rc;

    UCHAR *ppEAddr;

    ULONG ulAddr;

    ULONG ulBQueue=0;

    ULONG ulEQueue=0;

    QUEUE_PTR pChannel=NULL;          /* pointer to channel record            */



//    xp_os_semaphore_wait();



    rc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(rc == 0)

    {

        pChannel = &pGlobal->QueueInfo.XpQueueChData[wChannelId];



        /*--------------------------------------------------------------------+

        |  Check if queue is currently active

        +--------------------------------------------------------------------*/

        switch(pChannel->State)

        {

            case XP_QUEUE_STATUS_ENABLED:

            case XP_QUEUE_STATUS_DISABLED:

            case XP_QUEUE_STATUS_RPI:

                break;

            case XP_QUEUE_STATUS_UNDEFINED:

                rc = XP_ERROR_QUEUE_UNDEFINED;

                break;

            default:

                rc = XP_ERROR_INTERNAL;

                break;

        }

    }



    /*------------------------------------------------------------------------+

    |  if we don't have any locked record, then we cannot unlock any records

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

        if(pChannel->wLockInuse == 0)

        {

            rc = XP_ERROR_QUEUE_ADDRESS;

        }

        else if(ulLength == 0)

        {

            rc = XP_ERROR_QUEUE_REGION_SIZE;

        }

    }



    /*------------------------------------------------------------------------+

    |  Determine the start and end address of the space being freed

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

        ulAddr    = (ULONG)ppBAddr;         //lingh

        ulBQueue = pChannel->ulQueueAddr;

        ulEQueue = ulBQueue + pChannel->ulQueueSize;





    if(ulAddr < ulBQueue)

        {

            pChannel->Errors.uwQueueAddress++;

            rc = XP_ERROR_QUEUE_ADDRESS;

        }

    }



    if(rc == 0)

    {

        if(ulLength > pChannel->ulQueueSize)

        {

            pChannel->Errors.uwQueueAddress++;

            rc = XP_ERROR_QUEUE_ADDRESS;

        }

    }



    /*------------------------------------------------------------------------+

    |  Adjust address to end of space to unlock.  Watch for the queue wrap.

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

        ulAddr += ulLength;

        if(ulAddr >= ulEQueue)

        {

            ulAddr = (unsigned long) ulBQueue + (ulAddr - ulEQueue);

        }

        ppEAddr = MAKE_PLB_ADDR(ulAddr);



        rc = unlock_data(pGlobal,wChannelId, ppBAddr, ppEAddr);

    }



//    xp_os_semaphore_signal();



    return(rc);

}

