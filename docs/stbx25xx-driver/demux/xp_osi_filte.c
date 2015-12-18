/*----------------------------------------------------------------------------+
|   This source code has been made available to you by IBM on an AS-IS
|   basis.  Anyone receiving this source is licensed under IBM
|   copyrights to use it in any way he or she deems fit, including
|   copying it, modifying it, compiling it, and redistributing it either
|   with or without modifications.  No license under IBM patents or
|   patent applications is to be implied by the copyright license.
|
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
|   Author    :  Ian Govett
|   Component :  xp0
|   File      :  xp0_filte.c
|   Purpose   :  Section filter management
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  ------------------------------------------------------
|   15-Jan-98  IG   Created
|   04-May-01  IG   Updated for Pallas
|   30-Sep-01  LGH  Ported to Linux, combined codes 0f 3 devices
|   29-Oct-02  LGH  Add negative filter support for VESTA
+----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------+

|            Section Filter Management

+-----------------------------------------------------------------------------+

|

|   The following functions provide section filter management for the demux

|   driver.  Section filters are allocated for a user-specified number of

|   bytes using the xp0_filter_allocate() function.  The caller is returned

|   a unique filter_id which is used with subsequent operations on the

|   filter.  Section filters are defined or modified using the

|   xp0_filter_set() function.  This function uses the data, mask, and

|   polarity fields to define the filter characteristics.  The caller

|   provides an array of bytes to specify each of the data, mask, and

|   polarity fields.  These fields are mapped to hardware registers (bytes

|   2 & 3 are skipped by the hardware).

|

|   It is possible for section filters to be longer than the table section

|   present in the stream.  The function xp0_filter_short(), can be used to

|   define the match characteristic (hit, or miss) when the section filter

|   length exceeds the section length.  By default, short filters are

|   considered a filter match.

|

|   Section filters are added to a channel using

|   xp0_filter_add_to_channel().  function which returns a unique match_id

|   number.  Multiple filters may be assigned to the same channel (max of

|   32 filters per channel), each with a unique match_id.  The match_id is

|   a component of the match word discussed in the next section.  The

|   hardware link registers are automatically updated by this function when

|   filters are added.

|

|   The xp0_filter_delete_from_channel() removes the filter from the channel.

|   As each filter is removed, the hardware links are automatically updated

|   by the function.  The channel is disabled if the last filter is removed

|   from a channel defined with section filtering.

|

|   Table sections are processed and delivered by the transport demux

|   driver.  The application registers a callback function using the

|   xp0_channel_set_notification_fn().  This callback function is provided

|   the location (address in memory) of the table section, length of the

|   data, wChannelId, and a match word.  The match word is a 32-bit

|   unsigned value whose bit positions indicate which filters matched.  The

|   match_id value (returned from xp0_filter_add_to_channel()) described

|   above is a bit position in the match word.  Using the match word, an

|   application can quickly determine which filters matched the table

|   section.  Note:  the bit positions in the match word are number 0..31

|   where bit 0 is the most significant (left-most) bit, and bit 31 is the

|   least significant bit.

|

|   For example, suppose three filters are allocated (xp0_filter_allocate),

|   defined (xp0_filter_set), and added (xp0_filter_add_to_channel) to a

|   channel.  The filter id's returned from xp0_filter_allocate(), and the

|   match id's returned from xp0_filter_add_to_channel() could be:

|

|       filter_id      match_id

|       -----------------------

|          23         0

|          18         2

|          36         8

|

|   The application may need to maintain a relationship between the filter

|   id's and match id's for each channel.

|

|   Now suppose a table section is delivered to the application with a

|   match word of 0x80800000, then filter id number 23, and 36 were a "hit".

|   Or suppose the match word as 0xA0000000, then filter id's 23, and 18

|   were a "hit".

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

#include <linux/types.h>

#include <linux/slab.h>

#include <linux/interrupt.h>

#endif


#include "xp_osi_global.h"

#include "xp_atom_reg.h"







/*----------------------------------------------------------------------------+

| Local Defines

+----------------------------------------------------------------------------*/

#define FILTER_ID_UNUSED   XP_FILTER_MAX_BLOCKS

#define PSI_HEADER_LENGTH  3



/*----------------------------------------------------------------------------+

| Local Type Declarations

+----------------------------------------------------------------------------*/

typedef struct psi_header_type {

    unsigned table_id:8;         /* table type               */

    unsigned syntax_ind:1;       /* section syntax indicator         */

    unsigned private_ind:1;      /* private indicator on type private    */

    unsigned reserved_1:2;       /*                      */

    unsigned sectionLength:12;       /* length of the remaining data         */

    unsigned reserved_2:8;

} PSI_HEADER_TYPE, *PSI_HEADER_PTR;



//Internal

static SHORT reserve_hw_blocks(GLOBAL_RESOURCES *pGlobal, USHORT uwBlockCount, UCHAR  *pBlocks);

static void clear_hw_block(GLOBAL_RESOURCES *pGlobal,SHORT wIndex);

static void init_filter_links(GLOBAL_RESOURCES *pGlobal,FILTER_TYPE *pFilter);

static void update_links(GLOBAL_RESOURCES *pGlobal, SHORT wId1, SHORT wId2);

static void write_dram_filter(GLOBAL_RESOURCES *pGlobal, FILTER_TYPE *pFilter, USHORT uwCount,

                              ULONG ulData, ULONG ulMask, ULONG ulControl, ULONG ulPolarity);

static SHORT filter_match(GLOBAL_RESOURCES *pGlobal,FILTER_PTR pFilter,

                          XP_CHANNEL_NOTIFY_DATA *pInfo,UCHAR *plBQueue,

                          UCHAR *plEQueue);

static void hw_filter_set(GLOBAL_RESOURCES *pGlobal,FILTER_TYPE *pFilter);

static short hw_filter_add(GLOBAL_RESOURCES *pGlobal, FILTER_CHANNEL_PTR pChannel,

                           SHORT wChannelId, FILTER_PTR pF2);

static short hw_filter_delete(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,FILTER_PTR pF2);

static void free_filter(GLOBAL_RESOURCES *pGlobal, SHORT wFilterId);

static SHORT find_filter(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,SHORT wFilterId,

                         SHORT *pId);

static void delete_filter(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, SHORT wFilterId,short i);

static void process_section_change(GLOBAL_RESOURCES *pGlobal);

static SHORT filter_get_pending(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId,

                                XP_FILTER_PENDING *pPending);



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

| reserve_hw_blocks

+----------------------------------------------------------------------------*/

static SHORT reserve_hw_blocks(GLOBAL_RESOURCES *pGlobal, USHORT uwBlockCount,      /* number of blocks requested       */

                               UCHAR  *pBlocks)       /* array of block id's returned         */



{

    unsigned short i;

    USHORT uwCount;    /* number of blocks reserved            */



    /*------------------------------------------------------------------------+

    | Find one of the words with a bit available

    +------------------------------------------------------------------------*/

    for(i=0, uwCount=0; (i < XP_FILTER_MAX_BLOCKS) && (uwCount < uwBlockCount); i++)

    {

    if(pGlobal->FilterInfo.XpFilterHwData[i].inuse == 0)

        {

        pGlobal->FilterInfo.XpFilterHwData[i].inuse = 1;

        pBlocks[uwCount] = i;

        uwCount++;

    }

    }



    /*------------------------------------------------------------------------+

    |  Free the reserved bits which were allocated

    +------------------------------------------------------------------------*/

    if (uwCount < uwBlockCount)

    {

       for(i=0; i<uwCount; i++)

       {

       pGlobal->FilterInfo.XpFilterHwData[pBlocks[i]].inuse = 0;

       }

       return(XP_ERROR_FILTER_UNAVAILABLE);

    }



    return(0);

}



/*----------------------------------------------------------------------------+

| clear_hw_block

+----------------------------------------------------------------------------*/

static void clear_hw_block(GLOBAL_RESOURCES *pGlobal,SHORT wIndex)



{

    if((wIndex >= 0) && (wIndex < XP_FILTER_MAX_BLOCKS))

    {

    pGlobal->FilterInfo.XpFilterHwData[wIndex].inuse = 0;

    }

}



/*----------------------------------------------------------------------------+

|  init_filter_links

+----------------------------------------------------------------------------*/

static void init_filter_links(GLOBAL_RESOURCES *pGlobal,FILTER_TYPE *pFilter)

{

    unsigned short i;

    unsigned b1, b2;        /* block numbers in the link            */



    /*------------------------------------------------------------------------+

    |  Update the link chains

    +------------------------------------------------------------------------*/

    b1 = pFilter->cHwBlockId[0];

    pGlobal->FilterInfo.XpFilterHwData[b1].prev = b1;



    for(i=0; i<pFilter->uwHwBlockCount-1; i++)

    {

    b2 =  pFilter->cHwBlockId[i+1];

    pGlobal->FilterInfo.XpFilterHwData[b1].next    = b2;

    pGlobal->FilterInfo.XpFilterHwData[b2].prev    = b1;

    pGlobal->FilterInfo.XpFilterHwData[b1].endOfColumn = 1;

    b1 = b2;

    }



    pGlobal->FilterInfo.XpFilterHwData[b1].next        = b1;

    pGlobal->FilterInfo.XpFilterHwData[b1].endOfColumn = 1;

}



/*----------------------------------------------------------------------------+

|  update_links

+----------------------------------------------------------------------------*/

static void update_links(GLOBAL_RESOURCES *pGlobal, SHORT wId1, SHORT wId2)

{

    short next;

    UINT32  flag;



    next = pGlobal->FilterInfo.XpFilterHwData[wId1].next;

    /*------------------------------------------------------------------------+

    |  If the last node hasn't been reached, then copy the forward chain

    |  (id1.next) to id2.next.  Update the hardware

    +------------------------------------------------------------------------*/

    if(next != wId1)

    {

    pGlobal->FilterInfo.XpFilterHwData[wId2].next = next;

    pGlobal->FilterInfo.XpFilterHwData[next].prev = wId2;



        flag = os_enter_critical_section();

    xp_atom_dcr_write_filter_link(pGlobal->uDeviceIndex,

            wId2, pGlobal->FilterInfo.XpFilterHwData[wId2].next,

        pGlobal->FilterInfo.XpFilterHwData[wId2].endOfColumn);

        os_leave_critical_section(flag);

    }



    /*------------------------------------------------------------------------+

    |  Now update id1.next to point to id2.  Update the hardware

    +------------------------------------------------------------------------*/

    pGlobal->FilterInfo.XpFilterHwData[wId1].next    = wId2;

    pGlobal->FilterInfo.XpFilterHwData[wId2].prev    = wId1;

    pGlobal->FilterInfo.XpFilterHwData[wId1].endOfColumn = 0;



    flag = os_enter_critical_section();

    xp_atom_dcr_write_filter_link(pGlobal->uDeviceIndex,

        wId1, pGlobal->FilterInfo.XpFilterHwData[wId1].next,

    pGlobal->FilterInfo.XpFilterHwData[wId1].endOfColumn);

    os_leave_critical_section(flag);



}



/*----------------------------------------------------------------------------+

|  write_dram_filter

+----------------------------------------------------------------------------*/

static void write_dram_filter(GLOBAL_RESOURCES *pGlobal,

                              FILTER_TYPE *pFilter,            /* filter information               */

                              USHORT uwCount,       /* filter block index           */

                              ULONG ulData,

                              ULONG ulMask,

                              ULONG ulControl,

                              ULONG ulPolarity)

{

    short filterId;         /* filter block number in hardware      */

    XP_FILTER_CONTROL_REG *pControl;

    UINT32  flag;



    pControl = (XP_FILTER_CONTROL_REG *)(void *) &ulControl;

    /*------------------------------------------------------------------------+

    |  Determine the next filter block number to setup the links in hardware

    |  Preserve the enabled state if we're already enabled or we're not

    |  in the first filter block

    +------------------------------------------------------------------------*/

    filterId = pFilter->cHwBlockId[uwCount];



    if((pFilter->state == XP_FILTER_ENABLED) || (uwCount > 0))

    {

    pControl->enbl = 1;

    }

    else

    {

    pControl->enbl = 0;

    }



    /*------------------------------------------------------------------------+

    |  Retain current value of endOfColumn during a "Live" filter_set

    |  operation old: pControl->ncol  = 1;

    |  Write the filter block

    +------------------------------------------------------------------------*/

    pControl->sfid    = pFilter->wMatchId;

    pControl->ncol    = pGlobal->FilterInfo.XpFilterHwData[filterId].endOfColumn;

    pControl->nfilt   = pGlobal->FilterInfo.XpFilterHwData[filterId].next;



    flag = os_enter_critical_section();

    xp_atom_dcr_write_dram_filter(pGlobal->uDeviceIndex,filterId,ulData,ulMask,ulControl,ulPolarity);

    os_leave_critical_section(flag);



}



/*----------------------------------------------------------------------------+

|  filter_match

+----------------------------------------------------------------------------*/

static SHORT filter_match(GLOBAL_RESOURCES *pGlobal,FILTER_PTR pFilter,

                          XP_CHANNEL_NOTIFY_DATA *pInfo,UCHAR *plBQueue,

                          UCHAR *plEQueue)

{

    short match;

    unsigned long i;

    unsigned long length;

    unsigned char *s;



    length = (pInfo->ulLength < pFilter->uwLength) ? pInfo->ulLength : pFilter->uwLength;



    for(i=0, s=pInfo->plData, match=1; i<length && match; i++, s++)

    {

    if(s == plEQueue)

        {

        s = plBQueue;

    }



    if((i != 1) && (i != 2))

        {

        match = ((*s ^ pFilter->cData[i]) & pFilter->cMask[i]) ? 0 : 1;

/*      if(filter->polarity[i]) {

        match = !match;

        }  */

    }

    }



    return(match);

}



/*----------------------------------------------------------------------------+

|  hw_filter_set

+----------------------------------------------------------------------------*/

static void hw_filter_set(GLOBAL_RESOURCES *pGlobal,FILTER_TYPE *pFilter)

{

    unsigned short i, j;

    unsigned short hw_count;        /* index into the hw_block_id       */

    unsigned long data;         /* data portion of filter           */

    unsigned long mask;         /* mask for the filter          */

    //added in 4/2002
    unsigned long polarity;

    unsigned long control;      /* filter control               */

    unsigned char *pData;

    unsigned char *pMask;

    unsigned char *pPolarity;



    pData    = (unsigned char *) &data;

    pMask    = (unsigned char *) &mask;

    pPolarity = (unsigned char *) &polarity;



    control = 0;

    data    = 0;

    mask    = 0;

    for(i=0, j=0, hw_count=0; i<pFilter->uwLength; i++)

    {

    /*--------------------------------------------------------------------+

    |  Skip bytes 1 & 2,  add the byte to the word

    |  When all four bytes of the word have been written, update hardware

    +--------------------------------------------------------------------*/

    if((i == 1) || (i == 2))

        {

        continue;

    }



    pData[j] = pFilter->cData[i];

    pMask[j] = pFilter->cMask[i];

        pPolarity[j] = pFilter->cPolarity[i];



    j++;

    if(j == 4)

        {

        write_dram_filter(pGlobal, pFilter, hw_count, data, mask, control, polarity);

        hw_count++;

        j       = 0;

        control = 0;

    }

    }



    /*------------------------------------------------------------------------+

    |  if there was not a multiple of 4 bytes to filter, then turn off the

    |  remaining bytes and write the last filter

    +------------------------------------------------------------------------*/

    if(j) {

    for(; j<4; j++)

        {

        pData[j] = 0;

        pMask[j] = 0;

            pPolarity[j] = 0xff;

    }

    write_dram_filter(pGlobal,pFilter, hw_count, data, mask, control, polarity);

    }



}



/*----------------------------------------------------------------------------+

|  hw_filter_add

+----------------------------------------------------------------------------*/

static short hw_filter_add(GLOBAL_RESOURCES *pGlobal, FILTER_CHANNEL_PTR pChannel,     /* channel information          */

                           SHORT wChannelId, FILTER_PTR pF2)              /* new filter to add            */

{

    unsigned short i;

    short    id1;           /* current hw index number          */

    short    id2;           /* current hw index number          */

    short    prev;          /* save the previous id.            */

    UINT32  flag;



    /*------------------------------------------------------------------------+

    |  Set the match_id for the filters

    +------------------------------------------------------------------------*/

    for(i=0; i<pF2->uwHwBlockCount; i++)

    {

        flag = os_enter_critical_section();

    xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,

                                XP_FILTER_CONTROL_REG_SFID,

                pF2->cHwBlockId[i], pF2->wMatchId);

        os_leave_critical_section(flag);

    }



    /*------------------------------------------------------------------------+

    |  If there are no filters defined to the channel, then just set the

    |  first filter id.

    +------------------------------------------------------------------------*/

    if(pChannel->wInuse == 0)

    {

    id2 = pF2->cHwBlockId[0];

    pChannel->wFirstHwFilterId = id2;



        flag = os_enter_critical_section();

    xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,

            XP_QCONFIGB_REG_FSF, wChannelId, id2);

        os_leave_critical_section(flag);



    }

    else

    {

    /*--------------------------------------------------------------------+

    |  Traverse the chain, and at each endOfColumn, move both pointers

    |  (id1, id2) to the next column until we reach the end of either

    |  column.

    +--------------------------------------------------------------------*/

    id1 = pChannel->wFirstHwFilterId;

    id2 = pF2->cHwBlockId[0];



    while(pGlobal->FilterInfo.XpFilterHwData[id1].next != id1)

        {

      if (pGlobal->FilterInfo.XpFilterHwData[id1].endOfColumn)

          {

          if(pGlobal->FilterInfo.XpFilterHwData[id2].next == id2)

              {

          break;

          }



          id2 = pGlobal->FilterInfo.XpFilterHwData[id2].next;

      }



      id1 = pGlobal->FilterInfo.XpFilterHwData[id1].next;

    }



    /*--------------------------------------------------------------------+

    |  Now traverse backwards adding nodes to the hardware links

    +--------------------------------------------------------------------*/

    while (pGlobal->FilterInfo.XpFilterHwData[id1].prev != id1)

        {

      if (pGlobal->FilterInfo.XpFilterHwData[id1].endOfColumn)

          {

          prev = pGlobal->FilterInfo.XpFilterHwData[id2].prev;

          update_links(pGlobal,id1, id2);

          id2 = prev;

      }

      id1 = pGlobal->FilterInfo.XpFilterHwData[id1].prev;

    }



    if (pGlobal->FilterInfo.XpFilterHwData[id1].endOfColumn)

        {

        update_links(pGlobal,id1, id2);

    }

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|  hw_filter_delete

+----------------------------------------------------------------------------*/

static short hw_filter_delete(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId, FILTER_PTR pF2)              /* filter to delete             */

{

    unsigned short     i;

    short          id;      /* current filter_id            */

    short          next;        /* next filter_id in the list       */

    short          prev=0;      /* previous filter_id in the list       */

    FILTER_CHANNEL_PTR pChannel;     /* channel information          */

    UINT32  flag;



    pChannel = &pGlobal->FilterInfo.XpFilterChData[wChannelId];



    /*------------------------------------------------------------------------+

    |  Check if this is the first filter defined

    +------------------------------------------------------------------------*/

    id = pF2->cHwBlockId[0];

    i  = 0;

    if(pChannel->wFirstHwFilterId == id)

    {

        flag = os_enter_critical_section();

    xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,

                                   XP_QCONFIGB_REG_FSF, wChannelId,

                   pGlobal->FilterInfo.XpFilterHwData[id].next);

        os_leave_critical_section(flag);



    next = pGlobal->FilterInfo.XpFilterHwData[id].next;

    /*--------------------------------------------------------------------+

    |  Move the 1st filter for the channel to the next node

    +--------------------------------------------------------------------*/

    pChannel->wFirstHwFilterId = next;

    pGlobal->FilterInfo.XpFilterHwData[next].prev = next;

    i++;

    }



    for(; i<pF2->uwHwBlockCount; i++)

    {

    id = pF2->cHwBlockId[i];

    next = pGlobal->FilterInfo.XpFilterHwData[id].next;

    prev = pGlobal->FilterInfo.XpFilterHwData[id].prev;



    pGlobal->FilterInfo.XpFilterHwData[next].prev = prev;

    pGlobal->FilterInfo.XpFilterHwData[prev].next = next;

    if(pGlobal->FilterInfo.XpFilterHwData[id].endOfColumn)

        {

        pGlobal->FilterInfo.XpFilterHwData[prev].endOfColumn = 1;

    }



    /*--------------------------------------------------------------------+

    |  Update the hardware links

    +--------------------------------------------------------------------*/

    flag = os_enter_critical_section();

        xp_atom_dcr_write_filter_link(pGlobal->uDeviceIndex,

                                prev, pGlobal->FilterInfo.XpFilterHwData[prev].next,

                 pGlobal->FilterInfo.XpFilterHwData[prev].endOfColumn);

        os_leave_critical_section(flag);

    }



    /*------------------------------------------------------------------------+

    |  Check if the last node removed is also the last node in the list.

    |  If so, then change the previous node to be the last node.

    |  Update the hardware links

    +------------------------------------------------------------------------*/

    if(pGlobal->FilterInfo.XpFilterHwData[id].next == id)

    {

    pGlobal->FilterInfo.XpFilterHwData[prev].next = prev;



        flag = os_enter_critical_section();

    xp_atom_dcr_write_filter_link(pGlobal->uDeviceIndex,

                                prev, pGlobal->FilterInfo.XpFilterHwData[prev].next,

                  pGlobal->FilterInfo.XpFilterHwData[prev].endOfColumn);

        os_leave_critical_section(flag);

    }



    return(0);



}



/*----------------------------------------------------------------------------+

|  free_filter

+----------------------------------------------------------------------------*/

static void free_filter(GLOBAL_RESOURCES *pGlobal, SHORT wFilterId)

{

    unsigned short i;

    FILTER_TYPE    *pFilter;



    pFilter = pGlobal->FilterInfo.pXpFilterData[wFilterId];



    for(i=0; i<pFilter->uwHwBlockCount; i++)

    {

    clear_hw_block(pGlobal,pFilter->cHwBlockId[i]);

    }



    FREE(pFilter);

    pGlobal->FilterInfo.pXpFilterData[wFilterId] = NULL;

}



/*----------------------------------------------------------------------------+

|  find_filter

+----------------------------------------------------------------------------*/

static SHORT find_filter(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,SHORT wFilterId,

                         SHORT *pId)                /* array index in channel array     */

{

    short i;

    FILTER_CHANNEL_PTR pChannel;



    pChannel = &pGlobal->FilterInfo.XpFilterChData[wChannelId];

    /*------------------------------------------------------------------------+

    |  Find the filter_id in the channel array

    +------------------------------------------------------------------------*/

    for(i=0; i<pChannel->wCount; i++)

    {

    if(pChannel->wFilterId[i] == wFilterId)

        {

        break;

    }

    }



    if(i == pChannel->wCount)

    {

    return(XP_ERROR_FILTER_NOT_ASSIGN);

    }

    *pId = i;



    return(0);

}



/*----------------------------------------------------------------------------+

|  delete_filter

+----------------------------------------------------------------------------*/

static void delete_filter(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, SHORT wFilterId,short i)

{

    FILTER_CHANNEL_PTR pChannel;

    FILTER_TYPE        *pFilter;



    pChannel = &pGlobal->FilterInfo.XpFilterChData[wChannelId];

    pFilter = pGlobal->FilterInfo.pXpFilterData[wFilterId];



    /*------------------------------------------------------------------------+

    |  If this is the last filter defined to the channel, turn off the queue

    +------------------------------------------------------------------------*/

    if(pChannel->wInuse == 1)

    {

    xp_osi_filter_control(pGlobal,pChannel->wFirstHwFilterId,

               XP_FILTER_CONTROL_DISABLE);

    }

    else

    {

    hw_filter_delete(pGlobal,wChannelId, pFilter);

    }



    /*------------------------------------------------------------------------+

    |  Re-initialize the software links for the standalone filter remove

    |  the filter association with the channel

    +------------------------------------------------------------------------*/

    init_filter_links(pGlobal,pFilter);



    pChannel->wFilterId[i] = FILTER_ID_UNUSED;



    if(pChannel->wInuse > 0)

     pChannel->wInuse--;



    pFilter->wChannelId = XP_CHANNEL_COUNT;

    pFilter->pending    = PENDING_NONE;

}



/*----------------------------------------------------------------------------+

|  check_filters

+----------------------------------------------------------------------------*/

static void check_filters(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                          XP_CHANNEL_NOTIFY_DATA *pInfo, UCHAR *plBQueue,

                          UCHAR *plEQueue)

{

    short          i;

    short          skip=0;

    SHORT          wFilterId;

    short          errorMatchWord=0;

    unsigned long      mask;

    XP_FILTER_SHORT    shortFilter;

    FILTER_CHANNEL_PTR pChannel;



    pChannel = &pGlobal->FilterInfo.XpFilterChData[wChannelId];

    for (i=0, mask=0x80000000; i<32; i++, mask>>=1)

    {

    if (mask & pInfo->ulMatchWord)

        {

       /*-----------------------------------------------------------------+

       |  Check for an invalid matchword

       +-----------------------------------------------------------------*/

       if(i >= pChannel->wInuse)

           {

           errorMatchWord++;

           skip = 1;

       }

       else

           {

           wFilterId = pChannel->wFilterId[i];

           if(wFilterId == FILTER_ID_UNUSED)

               {

           pChannel->Errors.uwFilterFreed++;

           skip = 1;

           }

           else if (pGlobal->FilterInfo.pXpFilterData[wFilterId]->state!=XP_FILTER_ENABLED)

               {



           pChannel->Errors.uwFilterDisabled++;

           skip = 1;

           }



           /*-------------------------------------------------------------+

           |  Check for filter_length > table length

           |  Determine how to treat short filters

           +-------------------------------------------------------------*/

           else if(pGlobal->FilterInfo.pXpFilterData[wFilterId]->uwLength > pInfo->ulLength)

               {

           pChannel->Errors.uwShortTable++;

           shortFilter = (pChannel->ShortFilter==XP_FILTER_SHORT_DEFAULT)

                 ? pGlobal->FilterInfo.XpFilterDefaultShort : pChannel->ShortFilter;



           if(shortFilter == XP_FILTER_SHORT_MISS)

                   {

               skip = 1;

           }

           }

           else if(pGlobal->FilterInfo.pXpFilterData[wFilterId]->wSoftFilter)

               {

           /*---------------------------------------------------------+

           |  Software filter to check the hardware, Reset the

           |  software filtering after the first valid data arrives

           +---------------------------------------------------------*/

           if (filter_match(pGlobal,pGlobal->FilterInfo.pXpFilterData[wFilterId],

               pInfo, plBQueue, plEQueue) == 1)

                   {

               pGlobal->FilterInfo.pXpFilterData[wFilterId]->wSoftFilter = 0;

               skip = 0;

           }

                   else

                   {

               pChannel->Errors.uwFilterMatch++;

               skip = 1;

           }

           }

               else

               {

           skip = 0;

           }

       }



       if(skip)

           {



           pInfo->ulMatchWord &= ~mask;

       }

    }

    }



    if(errorMatchWord)

    {

    pChannel->Errors.uwFilterMatchWord++;

    }

}



/*----------------------------------------------------------------------------+

|  process_section_change

+----------------------------------------------------------------------------*/

static void process_section_change(GLOBAL_RESOURCES *pGlobal)



{

    short i;

    unsigned long changes;

    unsigned long mask;

    UINT32  flag;



    flag = os_enter_critical_section();

    changes = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_SFCHNG);

    os_leave_critical_section(flag);



    if (changes != pGlobal->FilterInfo.ulXpFilterSectionChange)

    {

       for (i=0, mask=0x80000000; i<32; i++, mask>>=1)

       {

       if((pGlobal->FilterInfo.ulXpFilterSectionChange & mask) && !(changes & mask))

           {

           xp_osi_filter_process_pending(pGlobal,i);

       }

       }

    }

}



/*----------------------------------------------------------------------------+

|  filter_get_state

+----------------------------------------------------------------------------*/

static SHORT filter_get_state(

GLOBAL_RESOURCES *pGlobal,

SHORT wFilterId,

XP_FILTER_STATUS *pState)        /* state of this filter block       */

{

    short rc;



    rc = xp_osi_filter_valid(pGlobal,wFilterId);

    if(rc == 0)

    {

    *pState = pGlobal->FilterInfo.pXpFilterData[wFilterId]->state;

    }

    return(rc);

}



/*----------------------------------------------------------------------------+

|  filter_get_pending

+----------------------------------------------------------------------------*/

static SHORT filter_get_pending(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId,

                                XP_FILTER_PENDING *pPending)         /* state of this filter block       */

{

    short rc;



    rc = xp_osi_filter_valid(pGlobal,wFilterId);

    if(rc == 0)

    {

    *pPending = pGlobal->FilterInfo.pXpFilterData[wFilterId]->pending;

    }



    return(rc);

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

|  xp0_filter_init

+----------------------------------------------------------------------------*/

short xp_osi_filter_init(GLOBAL_RESOURCES *pGlobal)

{

    memset((void *) pGlobal->FilterInfo.pXpFilterData,  0, sizeof(pGlobal->FilterInfo.pXpFilterData));

    memset((void *) pGlobal->FilterInfo.XpFilterHwData, 0, sizeof(pGlobal->FilterInfo.XpFilterHwData));

    memset((void *) pGlobal->FilterInfo.XpFilterChData, 0, sizeof(pGlobal->FilterInfo.XpFilterChData));



    return(0);

}

/*----------------------------------------------------------------------------+

|  xp0_filter_process_pending

+----------------------------------------------------------------------------*/

void xp_osi_filter_process_pending(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId)            /* channel containing the data      */

{

    short i;

    SHORT wFilterId;

    FILTER_CHANNEL_PTR pChannel;

    FILTER_PTR pFilter;



//    xp_os_semaphore_wait();



    pChannel = &pGlobal->FilterInfo.XpFilterChData[wChannelId];



    /*------------------------------------------------------------------------+

    |  Process the pending operations

    +------------------------------------------------------------------------*/

    if(pChannel->wPendingRequest)

    {

    pChannel->wPendingRequest = 0;



    for(i=0; i<pChannel->wCount; i++)

        {

        wFilterId = pChannel->wFilterId[i];



        if(wFilterId != FILTER_ID_UNUSED)

            {

        pFilter = pGlobal->FilterInfo.pXpFilterData[wFilterId];

        if(pFilter)

                {

            if(pFilter->pending == PENDING_FREE)

                    {

            delete_filter(pGlobal,wChannelId, wFilterId, i);

            free_filter(pGlobal,wFilterId);

            }

            else if(pFilter->pending == PENDING_DELETE)

                    {

            delete_filter(pGlobal,wChannelId, wFilterId, i);

            }

        }

        }

    }

    }



//    xp_os_semaphore_signal();

}



/*----------------------------------------------------------------------------+

|  xp0_filter_process_table_data

|  (Under interrupt control)

+----------------------------------------------------------------------------*/

void xp_osi_filter_process_table_data(

GLOBAL_RESOURCES *pGlobal,

SHORT wChannelId,            /* channel containing the data      */

UCHAR *plBa,               /* starting address of available data   */

UCHAR *plEa,               /* ending address of available data     */

UCHAR *plBq,               /* starting address of the queue        */

UCHAR *plEq,               /* ending address of the queue          */

SHORT wSectionFilter,             /* 1=filtering enabled, 0=no filtering  */

XP_CHANNEL_NOTIFY_FN notify_fn)      /* application function to call         */

{

    short wrapCount;             /* number of queue wraps,for loop check */

    XP_QUEUE_MODE_TYPE mode;         /* LOCK or SKIP                 */

    UCHAR *plByteAddr;        /* bytes address of current table       */

    ULONG *plAddr;         /* address incremented through tables   */

    ULONG *plBAddr;       /* starting word address of the data    */

    ULONG *plEAddr;       /* ending word address of the data      */

    ULONG *plBQueue;          /* starting word address of the queue   */

    ULONG *plEQueue;          /* ending word address of the queue     */



    UCHAR *ppByteAddr;

    UCHAR *ppEAddr;

    UCHAR *ppBQueue;



    PSI_HEADER_TYPE h;           /* word containing the table header     */

    unsigned long length;        /* number of words in current table     */

    unsigned long avail;         /* number of words until a queue wrap   */

    XP_CHANNEL_NOTIFY_DATA Info;     /* notification data for application    */





//    xp_os_semaphore_wait();



    Info.pGlobal = pGlobal;



    /*------------------------------------------------------------------------+

    |  Convert pointers to improve performance of reads

    +------------------------------------------------------------------------*/

    plBQueue = (unsigned long *) plBq;

    plEQueue = (unsigned long *) plEq;

    plBAddr  = (unsigned long *) plBa;

    plEAddr  = (unsigned long *) plEa;



    /*------------------------------------------------------------------------+

    |  Setup the pointer to the table section header

    +------------------------------------------------------------------------*/

    Info.wChannelId = wChannelId;



    /*------------------------------------------------------------------------+

    |  Get the matchword

    |  Read the word containing the table section header and setup the info

    |  structure with the starting address and the length of the table

    +------------------------------------------------------------------------*/

    for(wrapCount=0, plAddr=plBAddr; (plAddr != plEAddr) && (wrapCount < 2); )

    {

    Info.ulMatchWord = *plAddr;

    plAddr++;

    if(plAddr == plEQueue)

        {

        plAddr = plBQueue;

    }



    h = *(PSI_HEADER_PTR)plAddr;

    /*--------------------------------------------------------------------+

    |  Determine the starting address and table length and calculate the

    |  ending (byte) address for use when recording the record lock

    +--------------------------------------------------------------------*/

    plByteAddr   = (unsigned char *) plAddr;

    Info.plData   = plByteAddr;

    Info.ulLength = PSI_HEADER_LENGTH + h.sectionLength;



        ppBQueue = (UCHAR*)os_get_physical_address(pGlobal->QueueInfo.XpQueueChData[wChannelId].hMem);

        ppByteAddr = ppBQueue + (ULONG)Info.plData - (ULONG)plBQueue;





    /*--------------------------------------------------------------------+

    |  Find the end of the table.  We must adjust this pointer based on

    |  where the queue wraps

    +--------------------------------------------------------------------*/

    avail = plEq - plByteAddr;

    if(avail <= Info.ulLength)

        {

        plByteAddr = plBq + (Info.ulLength - avail);

    }

        else

        {

        plByteAddr += Info.ulLength;

    }



        ppEAddr = ppBQueue + (ULONG)plByteAddr - (ULONG)plBQueue;



    mode = XP_QUEUE_MODE_LOCK;



    /*--------------------------------------------------------------------+

    |  if we're filtering, check the filters against the software values.

    |  Set the skip value if the hardware match is overridden.

    +--------------------------------------------------------------------*/

    if(wSectionFilter)

        {

        /*----------------------------------------------------------------+

        |  Check the section filters to be sure they we're disabled by

        |  software if we've overridden all the filter matches, then

        |  skip the table

        +----------------------------------------------------------------*/

        check_filters(pGlobal,wChannelId, &Info, plBq, plEq);



        if(Info.ulMatchWord == 0)

            {

          mode = XP_QUEUE_MODE_SKIP;

        }

    }



    /*--------------------------------------------------------------------+

    |  Calculate the number of words which we need to move to get to

    |  the start of the next table.  Since tables are aligned on word

    |  boundaries, adjust the length to a 4 byte boundary.

    |  Now move the address pointer.  We must adjust this pointer based

    |  on where the queue wraps.

    +--------------------------------------------------------------------*/

    length = (Info.ulLength + 3) / 4;

    avail = plEQueue - plAddr;

    if(avail <= length)

        {

        plAddr = plBQueue + (length - avail);

        wrapCount++;

    }

        else

        {

        plAddr += length;

    }



    /*--------------------------------------------------------------------+

    |  Process the record by locking, and notifying the application.

    +--------------------------------------------------------------------*/

    xp_osi_queue_lock_data(pGlobal,wChannelId, mode, ppByteAddr, ppEAddr);



    if(mode == XP_QUEUE_MODE_LOCK)

        {

        (notify_fn)(&Info);

    }

    }



    /*------------------------------------------------------------------------+

    |  Process any pending filter operations

    +------------------------------------------------------------------------*/

    xp_osi_filter_process_pending(pGlobal,wChannelId);

//    xp_os_semaphore_signal();

}



/*----------------------------------------------------------------------------+

|  xp0_filter_valid

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_valid(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId)

{



    if((wFilterId < 0) || (wFilterId >= XP_FILTER_MAX_BLOCKS))

    {

    return(XP_ERROR_FILTER_INVALID);

    }





    if(pGlobal->FilterInfo.pXpFilterData[wFilterId] == NULL)

    {

    return(XP_ERROR_FILTER_FREE);

    }



    return(0);

}



/*----------------------------------------------------------------------------+

|   XX     XXXXXX    XXXXXX    XXXXX

|  XXXX    XX   XX     XX     XX   XX

| XX  XX   XX   XX     XX      XX

| XX  XX   XXXXX       XX    XX

| XXXXXX   XX          XX     XX

| XX  XX   XX          XX     XX   XX

| XX  XX   XX        XXXXXX    XXXXX

+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+

|  xp0_filter_add_to_channel

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  add table section filter to channel

|

|  PROTOTYPE  :  short xp0_filter_add_to_channel(

|        short channel_id,

|        short filter_id)

|

|  ARGUMENTS  :  channel_id    -  the channel id as returned from the

|                 xp0_channel_allocate().

|        filter_id     -  the filter number as returned from the

|                 xp0_filter_allocate().

|

|  RETURNS    :  match_id if the operation was successful, or a negative

|        value if an error occurs

|

|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel_id is not defined

|        XP_ERROR_FILTER_INVALID   -  one or more filters invalid

|        XP_ERROR_INTERNAL     -  failed allocating space

|

|  COMMENTS   :  This function adds a filter to the channel.  The match

|        id. returned may be used with the match word when data

|        is delivered to the application.  This match id. is a

|        bit position in the match word.

|

|        Filters may be added to an active channel.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_add_to_channel(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,SHORT wFilterId)

{

    short       rc;

    short       i=0;            /* index into channel_data array */

    short       j;

    short       count;

    FILTER_TYPE     *pFilter=NULL;

    FILTER_CHANNEL_TYPE *pChannel=NULL;

//    short       *list;



//    xp_os_semaphore_wait();

    /*------------------------------------------------------------------------+

    |  Verify the channel and filter id's are valid

    +------------------------------------------------------------------------*/

    rc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(rc == 0)

    {
    rc = xp_osi_filter_valid(pGlobal,wFilterId);
    }



    /*------------------------------------------------------------------------+

    |  Check that the filter has been defined

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

    pFilter  = pGlobal->FilterInfo.pXpFilterData[wFilterId];

    pChannel = &pGlobal->FilterInfo.XpFilterChData[wChannelId];

    switch(pFilter->state)

        {

        case XP_FILTER_UNUSED:

            rc = XP_ERROR_FILTER_UNDEFINED;
            break;



        case XP_FILTER_DEFINED:

            break;

        case XP_FILTER_ENABLED:

        case XP_FILTER_DISABLED:

            rc = XP_ERROR_FILTER_INUSE;
            break;

        default:

            rc = XP_ERROR_INTERNAL;
            break;

    }

    }



    if(rc == 0)

    {

    if(pFilter->pending != PENDING_NONE)

        {
        rc = XP_ERROR_FILTER_PENDING;
    }

    }



    /*------------------------------------------------------------------------+

    |  Find an unused index to add the filter to the channel

    +------------------------------------------------------------------------*/

    if (rc == 0)

    {

    for(i=0; i<pChannel->wCount; i++)

        {
        if(pChannel->wFilterId[i] == FILTER_ID_UNUSED)

            {

        break;

        }

    }

    if(i >= 32)

        {

        rc = XP_ERROR_FILTER_MAX;
    }



    /*--------------------------------------------------------------------+

    |  Allocate additional space if there are no available blocks

    |  Allocate/realloc space and increase space by groups of 4

    +--------------------------------------------------------------------*/

    else if(i == pChannel->wCount)

        {

            count = 32;

            for(j=pChannel->wCount;j<count;j++)

            {

                pChannel->wFilterId[j] = FILTER_ID_UNUSED;

            }

            pChannel->wCount = count;





        }

    }



    /*------------------------------------------------------------------------+

    |  Define the new hardware filter links now, add the filter_id to the

    |  channel add the filter_id to the sorted array

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

    pFilter->state      = XP_FILTER_DISABLED;

    pFilter->wChannelId = wChannelId;

    pFilter->wMatchId   = i;



    hw_filter_add(pGlobal,pChannel, wChannelId, pFilter);

    pChannel->wFilterId[i] = wFilterId;

    pChannel->wInuse++;

    }



//    xp_os_semaphore_signal();



    /*------------------------------------------------------------------------+

    |  If there were errors, return the error code

    +------------------------------------------------------------------------*/

    if(rc)

    {

    return(rc);

    }



    return(i);

}



/*----------------------------------------------------------------------------+

|  xp0_filter_allocate

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  allocate space for a table section filter

|

|  PROTOTYPE  :  short xp0_filter_allocate(

|        unsigned short length)

|

|  ARGUMENTS  :  length        -  number of bytes defined to the filter

|

|  RETURNS    :  filter_id if the operation was successful, or a negative

|        value if an error occurs

|

|  ERRORS     :  XP_ERROR_FILTER_UNAVAILABLE -  not enough filters available

|

|  COMMENTS   :  This function allocates space to contain the filter

|        definition, and reserves filter blocks in hardware.  The

|        reserved filter blocks are linked together to save time

|        when the filter is added to a channel.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_allocate(GLOBAL_RESOURCES *pGlobal, ULONG ulLength)

{

    short    rc=0;

    short    i=0;

    short    j=0;

    short    count;

    FILTER_TYPE  *pFilter=NULL;



//    xp_os_semaphore_wait();



    PDEBUG("Entering into xp_osi_filter_allocate\n");

    /*------------------------------------------------------------------------+

    |  Try to process any pending section filter changes.

    +------------------------------------------------------------------------*/

    if(pGlobal->FilterInfo.ulXpFilterSectionChange)

    {

    process_section_change(pGlobal);

    }



    if(ulLength == 0)

    {

    rc = XP_ERROR_FILTER_BAD_LENGTH;

    }



    /*------------------------------------------------------------------------+

    |  Find the index of the filter block

    +------------------------------------------------------------------------*/

    if (rc == 0)

    {

    for (i=0; i<XP_FILTER_MAX_BLOCKS; i++)

        {

        if(pGlobal->FilterInfo.pXpFilterData[i] == NULL)

            {

        break;

        }

    }



    if(i == XP_FILTER_MAX_BLOCKS)

        {

        rc = XP_ERROR_FILTER_UNAVAILABLE;

    }

    }



    /*------------------------------------------------------------------------+

    |  Allocate a new filter, and initialize pointers to NULL

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

    pFilter = (FILTER_TYPE *) MALLOC(sizeof(FILTER_TYPE));

    if(pFilter == NULL)

        {

        rc = XP_ERROR_INTERNAL;

    }

    }



    if (rc == 0)

    {

       pFilter->state        = XP_FILTER_UNUSED;

       pFilter->pending      = 0;

       pFilter->wChannelId   = XP_CHANNEL_COUNT;

       pFilter->uwLength       = ulLength;



       for(j = 0; j<XP_FILTER_MAX_BLOCKS; j++)

       {

           pFilter->cData[j]        = 0;

           pFilter->cMask[j]        = 0;

           pFilter->cPolarity[j]    = 0;

           pFilter->cHwBlockId[j]   = 0;

       }



       pFilter->wSoftFilter  = 0;



       /*--------------------------------------------------------------------+

       |  We skip bytes 1,2 and use bytes 0, 3,4,5,... so hardware filtering

       |  length is always 2 bytes less than the total since bytes 1, & 2

       |  are not filtered Calculate the number of hardware blocks based on

       |  the ceiling of a 4 byte block

       +--------------------------------------------------------------------*/

       if(ulLength < 3)

       {

       count = 1;

       }

       else

       {

       count = ulLength - 2;

       }



       /*--------------------------------------------------------------------+

       |  Allocate space for the block id's, and then reserve these blocks

       +--------------------------------------------------------------------*/

       pFilter->uwHwBlockCount = (count + 3) / 4;

    }



    if(rc == 0)

    {

    rc = reserve_hw_blocks(pGlobal,pFilter->uwHwBlockCount, pFilter->cHwBlockId);

    }

    if(rc == 0)

    {

    init_filter_links(pGlobal,pFilter);

    PDEBUG("i = %d\n",i);

    PDEBUG("pFilter = %x\n",pFilter);

    pGlobal->FilterInfo.pXpFilterData[i] = pFilter;

    }



//    xp_os_semaphore_signal();



    /*------------------------------------------------------------------------+

    |  Clean up any allocate space due to errors

    +------------------------------------------------------------------------*/

    if (rc) {

        if(pFilter)

        {

            FREE(pFilter);

        }

        return(rc);

    }



    /*------------------------------------------------------------------------+

    |  Return the filter id which was setup

    +------------------------------------------------------------------------*/

    return(i);



}



/*----------------------------------------------------------------------------+

|  xp0_filter_control

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  controls the state of the filter

|

|  PROTOTYPE  :  short xp0_filter_control(

|        short filter_id,

|        FILTER_CONTROL_TYPE cmd)

|

|  ARGUMENTS  :  filter_id     -  the filter_id as returned from the

|                 xp0_filter_allocate()

|        cmd           -  XP_FILTER_CONTROL_ENABLE  -  enable filter

|                 XP_FILTER_CONTROL_DISABLE -  disable filter

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel_id is not defined

|

|  COMMENTS   :  xp0_filter_control() is used to enable or disable the

|        filter.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_control(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId,XP_FILTER_CONTROL_TYPE cmd)

{

    short      rc;

    SHORT      wHwFilterId;

    FILTER_PTR pFilter=NULL;

    UINT32  flag;



//    xp_os_semaphore_wait();



    /*------------------------------------------------------------------------+

    |  Verify the channel and filter id's are valid

    +------------------------------------------------------------------------*/

    rc = xp_osi_filter_valid(pGlobal,wFilterId);

    PDEBUG("lingh   ==== rc = %d\n",rc);



    /*------------------------------------------------------------------------+

    |  Make sure the filter was added to a channel

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

    pFilter = pGlobal->FilterInfo.pXpFilterData[wFilterId];

    switch(pFilter->state)

        {

        case XP_FILTER_ENABLED:

        case XP_FILTER_DISABLED:

            break;

        default:

            rc = XP_ERROR_FILTER_NOT_ASSIGN;

            break;

    }

    }



    if(rc == 0)

    {

    if(pFilter->pending == PENDING_FREE)

        {

    PDEBUG("rc = XP_ERROR_FILTER_PENDING\n");

        rc = XP_ERROR_FILTER_PENDING;

    }

    }



    /*------------------------------------------------------------------------+

    |  Get the id number of the first filter block in hardware.  All other

    |  filter blocks should already be enabled

    +------------------------------------------------------------------------*/

    if (rc == 0)

    {

    wHwFilterId = pFilter->cHwBlockId[0];



    switch(cmd)

        {

        case XP_FILTER_CONTROL_ENABLE:



            flag = os_enter_critical_section();

            xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,

                                                XP_FILTER_CONTROL_REG_ENBL,

                          wHwFilterId, 1);

            os_leave_critical_section(flag);



            pFilter->state = XP_FILTER_ENABLED;

            break;

        case XP_FILTER_CONTROL_DISABLE:



            flag = os_enter_critical_section();

            xp_atom_dcr_write_register_channel(pGlobal->uDeviceIndex,

                                            XP_FILTER_CONTROL_REG_ENBL,

                          wHwFilterId, 0);

            os_leave_critical_section(flag);



            pFilter->state = XP_FILTER_DISABLED;

            break;



        default:

            rc = XP_ERROR_INTERNAL;

            break;

        }

    }



//    xp_os_semaphore_signal();

    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_filter_delete_from_channel

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  remove table section filter from channel

|

|  PROTOTYPE  :  short xp0_filter_delete_from_channel(

|        short channel_id,

|        short filter_id)

|

|  ARGUMENTS  :  channel_id    -  the channel id as returned from the

|                 xp0_channel_allocate().

|        filter_id     -  the filter number as returned from the

|                 xp0_filter_allocate().

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel_id is not defined

|        XP_ERROR_FILTER_INVALID   -  one or more filters invalid

|

|  COMMENTS   :  section filters are removed from the channel by updating

|        the links in the hardware filter blocks.  Table sections

|        are not be delivered to the application after the

|        xp0_filter_delete_from_channel() completes.  If the last

|        filter is removed from the channel, and the channel is

|        configured for table section filtering, then the channel

|        is automatically disabled.

|

|        Section filters may be removed from an active channel

|        without disabling the channel.  Since the hardware could

|        be processing a table section split across multiple

|        packets, the hardware filter blocks are not released from

|        the channel until the hardware completes processing the

|        current table section.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_delete_from_channel(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,SHORT wFilterId)

{

    short       rc;

    short       i;

    unsigned long   mask;

    FILTER_TYPE     *pFilter=NULL;

    FILTER_CHANNEL_TYPE *pChannel=NULL;

    XP_CHANNEL_STATUS   channel_status;

    XP_QUEUE_STATUS queue_status;

    UINT32  flag;



//    xp_os_semaphore_wait();



    /*------------------------------------------------------------------------+

    |  Verify the channel and filter id's are valid

    +------------------------------------------------------------------------*/

    rc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(rc == 0)

    {
//  printk("1\n");

    rc = xp_osi_filter_valid(pGlobal,wFilterId);

    }



    if(rc == 0)

    {
//  printk("2\n");

    pChannel = &pGlobal->FilterInfo.XpFilterChData[wChannelId];

    pFilter  = pGlobal->FilterInfo.pXpFilterData[wFilterId];

    if(pFilter->pending != PENDING_NONE)

        {
//      printk("3\n");


        rc = XP_ERROR_FILTER_PENDING;

    }

    }



    /*------------------------------------------------------------------------+

    |  Check that the filter was added to the channel

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {
//    printk("4\n");

    switch(pFilter->state)

        {

        case XP_FILTER_UNUSED:

            rc = XP_ERROR_FILTER_UNDEFINED;
//    printk("5\n");

            break;



        case XP_FILTER_DEFINED:

//    printk("6\n");
            rc = XP_ERROR_FILTER_NOT_ASSIGN;

            break;



        case XP_FILTER_ENABLED:
//    printk("7\n");

            rc = xp_osi_filter_control(pGlobal,wFilterId, XP_FILTER_CONTROL_DISABLE);

            break;



        case XP_FILTER_DISABLED:
//    printk("8\n");

            break;



        default:

//          printk("9\n");
    rc = XP_ERROR_INTERNAL;

            break;

    }

    }



    /*------------------------------------------------------------------------+

    |  Determine if the filter is associated with the channel

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {
//    printk("10\n");


    rc = find_filter(pGlobal,wChannelId, wFilterId, &i);

    }



    /*------------------------------------------------------------------------+

    |  Remove the filter only if the channel or queue is disabled,

    |  otherwise, defer the request until the hardware has finished using

    |  the filter block (ie. moveUp).

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {
//    printk("11\n");


    rc = xp_osi_channel_get_status(pGlobal,wChannelId, &channel_status);

    }





    if(rc == 0)

    {
//    printk("12\n");


    rc = xp_osi_queue_get_status(pGlobal,wChannelId, &queue_status);

    }



    /*------------------------------------------------------------------------+

    |  Signal to the changes register in hardware. Process the request later

    |  if the stream is enabled and the section_change bit was not cleared

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {
//    printk("13\n");


    pFilter->state = XP_FILTER_DEFINED;

    mask = 0x80000000 >> wChannelId;



        flag = os_enter_critical_section();

    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_SFCHNG, mask);



    pGlobal->FilterInfo.ulXpFilterSectionChange = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_SFCHNG);

        os_leave_critical_section(flag);



    if((channel_status == XP_CHANNEL_ENABLED) &&

            (queue_status == XP_QUEUE_STATUS_ENABLED) &&

            (pGlobal->FilterInfo.ulXpFilterSectionChange & mask))

        {

        pFilter->pending = PENDING_DELETE;

        pChannel->wPendingRequest++;

    }

    else

        {
//    printk("14\n");


        delete_filter(pGlobal,wChannelId, wFilterId, i);

    }

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_filter_free

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  release space for a table section filter

|

|  PROTOTYPE  :  short xp0_filter_free(

|        short filter_id)

|

|  ARGUMENTS: :  filter_id     -  the filter number as returned from the

|                 xp0_filter_allocate().

|

|  RETURNS    :  0 if the operation was successful, or a negative value

|        if an error occurs

|

|  ERRORS     :  XP_ERROR_FILTER_INVALID   -  filter_id was not allocated

|

|  COMMENTS   :  This function frees filter blocks allocated to the filter.

|        If the filter was added to a channel using the

|        xp0_filter_add_to_channel(), it will be removed from the

|        before being freed.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_free(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId)

{

    short rc;

    FILTER_TYPE *pFilter=NULL;



//    xp_os_semaphore_wait();



    rc = xp_osi_filter_valid(pGlobal,wFilterId);

    if(rc == 0)

    {

    pFilter = pGlobal->FilterInfo.pXpFilterData[wFilterId];

    switch(pFilter->state)

        {

        case XP_FILTER_UNUSED:

        case XP_FILTER_DEFINED:

            break;

        case XP_FILTER_ENABLED:

        case XP_FILTER_DISABLED:

            rc = XP_ERROR_FILTER_INUSE;

            break;

        default:

            rc = XP_ERROR_INTERNAL;

            break;

    }

    }



    /*------------------------------------------------------------------------+

    |  Process immediately if there are no pending requests.  Otherwise,

    |  defer the request until the hardware has finished using the filter

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

    if(pFilter->pending == PENDING_NONE)

        {

        free_filter(pGlobal,wFilterId);

    }

        else

        {

        pFilter->pending = PENDING_FREE;

    }

    }



//    xp_os_semaphore_signal();



    return(rc);



}



/*----------------------------------------------------------------------------+

|  xp0_filter_free_channel

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  free all filter blocks for a channel

|

|  PROTOTYPE  :  short xp0_filter_free_channel(

|        short channel_id)

|

|  ARGUMENTS  :  channel_id    -  the channel id as returned from the

|                 xp0_channel_allocate().

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_CHANNEL_INVALID  -  channel_id is not defined

|

|  COMMENTS   :  xp0_filter_free() is called to remove each filter assigned

|        to the channel.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_free_channel(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId)

{

    short       rc;

    short       i;

    SHORT       wFilterId;

    FILTER_CHANNEL_TYPE *pChannel;



//    xp_os_semaphore_wait();



    /*------------------------------------------------------------------------+

    |  Verify the channel and filter id's are valid

    +------------------------------------------------------------------------*/

    rc = xp_osi_channel_valid(pGlobal,wChannelId);

    if(rc == 0)

    {

    pChannel = &pGlobal->FilterInfo.XpFilterChData[wChannelId];



    for(i=0; i<pChannel->wCount; i++)

        {

        wFilterId = pChannel->wFilterId[i];

        if(wFilterId != FILTER_ID_UNUSED)

            {

        xp_osi_filter_delete_from_channel(pGlobal,wChannelId, wFilterId);

        xp_osi_filter_free(pGlobal,wFilterId);

        }

    }



    if(pChannel->wCount > 0)

        {

        pChannel->wCount = 0;

//        free(pChannel->filter_id);

        for(i = 0; i<XP_CHANNEL_COUNT;i++)

                pChannel->wFilterId[i] = FILTER_ID_UNUSED;      //lingh changed

    }

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_filter_get_available

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_get_available(GLOBAL_RESOURCES *pGlobal,USHORT uwFilterLength)    /* number of filter bytes requested     */

{

    short rc;

    short i;

    short count;             /* number of 4-byte filter blocks       */

    short length;            /* length adjusted for skip bytes       */

    short filter_blocks;         /* filter blocks used for filter_length */



//    xp_os_semaphore_wait();



    /*------------------------------------------------------------------------+

    |  Try to process any pending section filter changes.

    +------------------------------------------------------------------------*/

    if(pGlobal->FilterInfo.ulXpFilterSectionChange)

    {

    process_section_change(pGlobal);

    }

    rc = (uwFilterLength == 0) ? XP_ERROR_FILTER_BAD_LENGTH : 0;



    if (rc == 0)

    {

    /*--------------------------------------------------------------------+

    |  Find the number of available filter blocks

    +--------------------------------------------------------------------*/

    for(i=0, count=0; i<XP_FILTER_MAX_BLOCKS; i++)

        {

        if(pGlobal->FilterInfo.pXpFilterData[i] == NULL)

            {

        count++;

        }

    }



    /*--------------------------------------------------------------------+

    |  Now determine the number of filter blocks required for the

    |  filter_length requested.  Note:  bytes 1 & 2 are skipped so they

    |  aren't counted now determine the number of filter groups available

    |  using the filter bytes required

    +--------------------------------------------------------------------*/

    length = (uwFilterLength < 3) ? uwFilterLength : uwFilterLength - 2;

    filter_blocks = (length + 3) / 4 ;

    rc = count / filter_blocks;

    }



//    xp_os_semaphore_signal();



    return(rc);

}



/*----------------------------------------------------------------------------+

|  xp0_filter_get_errors

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_get_errors(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,

                               XP_FILTER_ERROR_PTR pErrors)

{

    short rc;



    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc)

    {

    return(rc);

    }



    memcpy(pErrors, &pGlobal->FilterInfo.XpFilterChData[wChannelId].Errors,

       sizeof(XP_FILTER_ERROR_TYPE));



    return(0);

}



/*----------------------------------------------------------------------------+

|  xp0_filter_reset_errors

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_reset_errors(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId)

{

    short rc;



    rc = xp_osi_channel_valid(pGlobal,wChannelId);



    if(rc) {

    return(rc);

    }



    memset(&pGlobal->FilterInfo.XpFilterChData[wChannelId].Errors, 0,

       sizeof(XP_FILTER_ERROR_TYPE));



    return(0);

}



/*----------------------------------------------------------------------------+

|  xp0_filter_set

+-----------------------------------------------------------------------------+

|

|  DESCRIPTION:  define the table section filter

|

|  PROTOTYPE  :  short xp0_filter_set(

|        short filter_id,

|        unsigned short length,

|        unsigned char *data,

|        unsigned char *mask,

|        unsigned char *polarity)

|

|  ARGUMENTS  :  filter_id     -  the filter id as returned from the

|                 xp0_filter_allocate().

|        length        -  number of bytes in the filter data

|        data          -  array of bytes representing the filter

|        mask          -  array of bytes representing the mask

|        polarity      -  array of bytes representing the polarity

|                 of the compare.  A zero bit causes a

|                 compare in positive mode and a one bit

|                 causes a compare in netative mode (this

|                 is opposite to the way the hardware

|                 is set).

|

|  RETURNS    :  0 if successful, or non-zero if an error occurs

|

|  ERRORS     :  XP_ERROR_FILTER_INVALID   -  filter_id was not allocated

|

|  COMMENTS   :  The length field indicates the size of the arrays for the

|        data, mask, and polarity field.  This length must match

|        the length specified when the filter was allocated using

|        the xp0_filter_allocate().  When the length is 0, the

|        length of the allocated filter is used. xp0_filter_set()

|        is normally called after a section filter is allocated

|        to define the data, mask, and polarity

|

|        Filters which are connected to a channel and enabled on

|        an active channel may be re-defined without having to

|        disable the section filter or channel.

|

+----------------------------------------------------------------------------*/

SHORT xp_osi_filter_set(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId,USHORT uwLength,

                        UCHAR *pData,UCHAR *pMask,UCHAR *pPolarity)

{

    short   i,rc;

    FILTER_TYPE *pFilter=NULL;





    PDEBUG(" entering into xp_osi_filter_set\n");

//    xp_os_semaphore_wait();



    rc = xp_osi_filter_valid(pGlobal,wFilterId);

    if(rc == 0)

    {

    PDEBUG("xp_osi_filter_valid success\n");



    if(pData == NULL)

    {

        PDEBUG("rc = XP_ERROR_FILTER_DATA\n");

        rc = XP_ERROR_FILTER_DATA;

    }

    }



    if(rc == 0)

    {

    pFilter = pGlobal->FilterInfo.pXpFilterData[wFilterId];

    if(uwLength && (uwLength != pFilter->uwLength))

    {

        PDEBUG("rc = XP_ERROR_FILTER_BAD_LENGTH\n");



        rc = XP_ERROR_FILTER_BAD_LENGTH;

    }

    }



    if(rc == 0)

    {

    if(pFilter->pending == PENDING_FREE)

    {

        PDEBUG(" XP_ERROR_FILTER_PENDING\n");

        rc = XP_ERROR_FILTER_PENDING;

    }

    }



    /*------------------------------------------------------------------------+

    |  if the mask is not specified, then default to using all the bits

    |  if the polarity is not specified, then default to not using these bits

    +------------------------------------------------------------------------*/

    if(rc == 0)

    {

    memcpy(pFilter->cData, pData, pFilter->uwLength);

    if(pMask == NULL)

        {

        memset(pFilter->cMask, 0xff, pFilter->uwLength);

    }

        else

        {

        memcpy(pFilter->cMask, pMask, pFilter->uwLength);

    }



    if(pPolarity == NULL)

        {

        memset(pFilter->cPolarity, 0xff, pFilter->uwLength);

    }

        else

        {

        for (i=0; i < pFilter->uwLength; ++i)
            {
                pPolarity[i] = ~pPolarity[i];
#ifdef __DRV_FOR_VESTA__
                if(pPolarity[i] != 0xff && pPolarity[i] != 0x00)
                {
                    printk("VESTA can't support bit negative filter\n");
                    return -1;
                }
#endif
            }

        memcpy(pFilter->cPolarity, pPolarity, pFilter->uwLength);

    }



    if(pFilter->state == XP_FILTER_UNUSED)

        {

        pFilter->state = XP_FILTER_DEFINED;

    }



    /*--------------------------------------------------------------------+

    |  if the filter is updated without disabling the filter, then filter

    |  the data from hardware to eliminate the arrival of data from the

    |  old filter.  Update the hardware filter

    +--------------------------------------------------------------------*/

    else if(pFilter->state == XP_FILTER_ENABLED)

        {

        pFilter->wSoftFilter = 1;

    }

    hw_filter_set(pGlobal,pFilter);



    }



//    xp_os_semaphore_signal();



    return(rc);

}

