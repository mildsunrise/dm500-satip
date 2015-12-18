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
| Author    :  Katsuyuki Sugita
| Component :  ttx
| File      :  ttx_drv.c
| Purpose   :  Teletext Driver
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 22-Sep-03  TJC  Modified
+----------------------------------------------------------------------------*/
#include <linux/config.h>
#include <linux/version.h>
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#define  __NO_VERSION__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <stdio.h>
#include <ttx.h>
#include <asm/uaccess.h>
#include "os/os-generic.h"
#include "powerpc.h"
#include "ttx_defs.h"
#include "ttx_mbuf.h"


/*----------------------------------------------------------------------------+
| Local Defines
+----------------------------------------------------------------------------*/
#define DENC_CR1_TELETEXT_EN    0x10000000
#define TTXREQ_START            319
#define TTXREQ_END              1727
#define TTX_ODD_FIELD_START     6
#define TTX_EVEN_FIELD_START    7
#define XP_CHANNEL_NULL_PID     0x1fff

/*----------------------------------------------------------------------------+
| Static Variables
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| Global Variables
+----------------------------------------------------------------------------*/
ttx_stat_t ttx_stat = {
    0,               /* ioenabled: iniz flag                                 */
    0,               /* mem_size: allocated memory size                      */
    NULL,            /* mem_ptr: allocated memory                            */
    NULL,            /* ttx_list: Teletext list                              */
    NULL,            /* ttx_ctrl: Teletext pid control block                 */
    0,               /* num_ttx: number of subscribed ttx pid currently      */
    NULL,            /* pes_queue: VSYNC waiting queue                       */
    NULL,            /* done_queue: packets which processed in VSYNC         */
    {                /* stax: statistics counters                            */
        0, 0, 0, 0, 0, 0
    },
    0,               /* num_vbi_lines: VBI line count allocated for teletext */
    NULL,            /* vbi0_addr: VBI buffer pointer for even field         */
    NULL             /* vbi1_addr: VBI buffer pointer for odd field          */
};
Ttx_stat_t ttxstat = &ttx_stat;

/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/

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
|  ttx_on
|  Setup internal DENC to enable teletext processing.
+----------------------------------------------------------------------------*/
static int ttx_on(void)
{

  if (!ttxstat->ioenabled) {
     return(E_TTX_NOT_INITIALIZED);
  }

  powerpcMtdenc0_trr(((TTXREQ_START&0x7ff) << 16) | (TTXREQ_END&0x7ff));

  powerpcMtdenc0_tosr(((TTX_ODD_FIELD_START&0x1ff) << 16) |
                      ((TTX_ODD_FIELD_START+ttxstat->num_vbi_lines)&0x1ff));
  powerpcMtdenc0_tesr(((TTX_EVEN_FIELD_START&0x1ff)<<16) |
                      ((TTX_EVEN_FIELD_START+ttxstat->num_vbi_lines)&0x1ff));
  powerpcMtdenc0_cr1(powerpcMfdenc0_cr1() | DENC_CR1_TELETEXT_EN);

  return(0);
}

/*----------------------------------------------------------------------------+
|  ttx_off
|  Setup internal DENC to disable teletext processing.
+----------------------------------------------------------------------------*/
static int ttx_off(void)
{

  if (!ttxstat->ioenabled) {
      return(E_TTX_NOT_INITIALIZED);
  }

  powerpcMtdenc0_cr1(powerpcMfdenc0_cr1() & (~DENC_CR1_TELETEXT_EN));
  powerpcMtdenc0_trr(0);
  powerpcMtdenc0_tosr(0);
  powerpcMtdenc0_tesr(0);

  return(0);
}

/*----------------------------------------------------------------------------+
|  extract_pts
+-----------------------------------------------------------------------------+
|  PES packet header contains PTS formed in ISO 13818-1.
|  Return top 32 bits of PTS out of 33 bits.
|  Thus this represents unit of 45Khz. (90Khz by 33 bits)
+----------------------------------------------------------------------------*/
static unsigned int extract_pts(unsigned char *p)
{
  unsigned int tmp;

  tmp  = (unsigned int)(*p++ & 0x0E) << 28;    /* bit 32..30 */
  tmp |= (unsigned int)(*p++)        << 21;    /* bit 29..22 */
  tmp |= (unsigned int)(*p++ & 0xFE) << 13;    /* bit 21..15 */
  tmp |= (unsigned int)(*p++)        <<  6;    /* bit 14.. 7 */
  tmp |= (unsigned int)(*p++ & 0xFC) >>  2;    /* bit  6.. 1 */

  return(tmp);
}

/*----------------------------------------------------------------------------+
|  process_ttx_pes
+-----------------------------------------------------------------------------+
|  Parse received TS packet as PES
|  Received mbuf is queued in ttxstat->pes_queue and it will processed by
|  ttx_vsync_handler() which called as by video decorder's VBI START
|  interrupt notification.
+----------------------------------------------------------------------------*/
static int process_ttx_pes(Mbuf mb)
{
  unsigned char    data8;
  unsigned short   data16;
  unsigned int     data32;
  unsigned int     pusi = 0;
  Ttx_list_entry   ttx_list = ttxstat->ttx_list;
  Ttx_ctrl_entry   ttx_ctrl = ttxstat->ttx_ctrl;
  Ttx_mbuf_t       ttx_mbuf;
  kernel_state     st;

  /*--------------------------------------------------------------------------+
  | MBUF includes pointer to ttx_ctrl
  +--------------------------------------------------------------------------*/
  ttx_mbuf = (Ttx_mbuf_t)DATA_PTR(mb);

  /*--------------------------------------------------------------------------+
  |  Check TS packet level
  |  Note that at this moment, ttx_list may be no longer valid (ttx pid
  |  has been already removed) It is detected by verifing TS PID in TS
  |  packet header with ttx_list entry.
  +--------------------------------------------------------------------------*/
  data16 = ((ttx_mbuf->buf[1]<<8) | ttx_mbuf->buf[2]) & TS_PID_MASK;
  if (data16 != ttx_list->pid) {               /* ttx list still valid ?     */
      ttxstat->stax.trash_pkt++;
      ttx_m_free(mb);
      return(-1);
  }

  data8 = ttx_mbuf->buf[1];
  if (data8 & TS_ERR_MASK) {                   /* TS error ?                 */
      ttxstat->stax.trash_pkt++;
      ttx_m_free(mb);
      return(-1);
  }

  if (data8 & TS_PUSI_MASK) {                  /* PUSI ?                     */
      pusi = 1;
  }

  data8 = ttx_mbuf->buf[3] & TS_AFC_MASK;;
  if (data8 != TS_AFC_PL_ONLY) {               /* AFC is ok ?                */
      ttxstat->stax.trash_pkt++;
      ttx_m_free(mb);
      return(-1);
  }

  /*--------------------------------------------------------------------------+
  |  Check PES packet header level
  +--------------------------------------------------------------------------*/
  ttx_mbuf->pts = 0;
  if (pusi) {
     /*-----------------------------------------------------------------------+
     |  TS packet starts PES header
     |  PES Start Code Prefix & Stream ID
     +-----------------------------------------------------------------------*/
     data32 = *((unsigned int *)(ttx_mbuf->buf+4));
     if (data32 != PES_ID) {
         ttxstat->stax.trash_pkt++;
         ttx_m_free(mb);
         return(-1);
     }

     /*-----------------------------------------------------------------------+
     |  PES header length ok ?
     +-----------------------------------------------------------------------*/
     data8 = ttx_mbuf->buf[4+8];
     if (data8 != PES_HD_LENGTH_TTX) {
         ttxstat->stax.trash_pkt++;
         ttx_m_free(mb);
         return(-1);
     }

     /*-----------------------------------------------------------------------+
     | Extract PTS if exists
     | In some Teletext stream, PTS embeded in PES header may reach STC
     | before receiving complete PES packet. That's TS packets close
     | to end of PES packet cannot be composed to PES packet.
     | To rescue such a condition, PTS sent from network is
     | intentionally delayed.  Since frame interval of PAL TV is 20 ms,
     | this delay value is set to this. Packet arriving beyond this
     | margin may be discarded.
     +-----------------------------------------------------------------------*/
     data8 = ttx_mbuf->buf[4+7] & PES_PTS_DTS_FLAG_MASK;
     if ((data8 == PES_PTS_DTS_FLAG_PTS) ||
         (data8 == PES_PTS_DTS_FLAG_PDTS)) {
         ttx_mbuf->pts = extract_pts(ttx_mbuf->buf+4+9);

         ttx_mbuf->pts     += TTX_PTS_RECEIVE_DELAY;
         ttx_ctrl->last_pts = ttx_mbuf->pts;
     }

     /*-----------------------------------------------------------------------+
     | Check PES packet data level
     | For efficiency (to avoid extracting only valid data units to other
     | buffer), only data id is checked.
     +-----------------------------------------------------------------------*/
     data8 = ttx_mbuf->buf[4+45];
     if ((data8 < PES_TTX_DATA_ID_START) ||
         (data8 > PES_TTX_DATA_ID_END))  {
         ttxstat->stax.trash_pkt++;
         ttx_m_free(mb);
         return(-1);
     }
     ttxstat->stax.in_pusi++;
  } else {
     ttxstat->stax.no_pusi++;
  }

  /*--------------------------------------------------------------------------+
  |  Now MBUF is validated
  |  No PTS, so chain it to previous MBUF chain.
  +--------------------------------------------------------------------------*/
  if (ttx_mbuf->pts == 0) {
      ttx_m_cat(ttx_ctrl->last_mb, mb);
  } else {
      if (ttx_ctrl->last_chain) {
          ttxstat->stax.out_pusi++;

          st = beginCriticalSection();
          ttx_m_enq(&ttxstat->pes_queue, ttx_ctrl->last_chain);
          endCriticalSection(st);
      }

      ttx_ctrl->last_chain = mb;
  }

  /*--------------------------------------------------------------------------+
  |  If later MBUF does not have PTS, it will be chained to this.
  +--------------------------------------------------------------------------*/
  ttx_ctrl->last_mb = mb;

  /*--------------------------------------------------------------------------+
  |  If the packets which already processed by VSYNC handler, free them
  +--------------------------------------------------------------------------*/
  st = beginCriticalSection();
  if (ttxstat->done_queue) {
      ttx_m_free_q(&ttxstat->done_queue);
  }
  endCriticalSection(st);

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
|  ttx_stats_get
+----------------------------------------------------------------------------*/
void ttx_stats_get(unsigned long tstat_ptr)

{
  copy_to_user((ttx_stax_t *)tstat_ptr, &ttxstat->stax, sizeof(ttx_stax_t));
}

/*----------------------------------------------------------------------------+
|  ttx_stats_clear
+----------------------------------------------------------------------------*/
void ttx_stats_clear(void)

{
  memset(&ttxstat->stax, 0, sizeof(ttx_stax_t));
}

/*----------------------------------------------------------------------------+
|  ttx_insert_pid
|  Start PID subscription for teletext driver.
+----------------------------------------------------------------------------*/
int ttx_insert_pid(unsigned short pid)
{
  int ttx_list_size         = 1;
  Ttx_ctrl_entry   ttx_ctrl = ttxstat->ttx_ctrl;
  Ttx_list_entry   ttx_list = ttxstat->ttx_list;


  if (!ttxstat->ioenabled) {
      return(E_TTX_NOT_INITIALIZED);
  }

  /*--------------------------------------------------------------------------+
  |  Look down to end of entry
  +--------------------------------------------------------------------------*/
  ttxstat->num_ttx = 0;
  while (ttx_list_size-- > 0) {
      ttx_ctrl->last_chain = ttx_ctrl->last_mb = NULL;
      ttx_ctrl->last_pts = 0;
      ttx_list->pid = pid;

      ttxstat->num_ttx++;
      ttx_ctrl++;
      ttx_list++;
  }

  return(0);
}

/*----------------------------------------------------------------------------+
|  ttx_delete_pid
|  Stop PID subscription for teletext driver.
+----------------------------------------------------------------------------*/
int ttx_delete_pid(void)
{
  kernel_state   st;
  Ttx_ctrl_entry ttx_ctrl = ttxstat->ttx_ctrl;
  Ttx_list_entry ttx_list = ttxstat->ttx_list;


  if (!ttxstat->ioenabled) {
      return(E_TTX_NOT_INITIALIZED);
  }

  /*--------------------------------------------------------------------------+
  | Remove all PIDs
  +--------------------------------------------------------------------------*/
  while (ttxstat->num_ttx) {
      ttx_ctrl->last_mb = NULL;
      ttx_ctrl->last_pts = 0;
      ttx_m_free_p(ttx_ctrl->last_chain);
      ttx_ctrl->last_chain = NULL;
      ttx_list->pid = XP_CHANNEL_NULL_PID;

      ttxstat->num_ttx--;
      ttx_list++;
      ttx_ctrl++;
  }

  st = beginCriticalSection();
  ttx_m_free_q(&ttxstat->done_queue);
  ttx_m_free_q(&ttxstat->pes_queue);
  endCriticalSection(st);

  return(0);
}

/*----------------------------------------------------------------------------+
|  ttx_write
+----------------------------------------------------------------------------*/
int ttx_write(void *buffer, size_t length)

{
  int            err;
  Ttx_ctrl_entry ttx_ctrl = ttxstat->ttx_ctrl;
  Mbuf           cur_mb;
  Ttx_mbuf_t     ttx_mbuf;


  /*--------------------------------------------------------------------------+
  |  Set Next Buffer
  +--------------------------------------------------------------------------*/
  while (length >= TS_PACKET_SIZE) {
     cur_mb = ttx_m_getn(sizeof(ttx_mbuf_t), &err);
     if (cur_mb == NULL) {
         return(E_TTX_NOMEM);
     }

     ttx_mbuf = (Ttx_mbuf_t)DATA_PTR(cur_mb);
     copy_from_user(ttx_mbuf->buf, buffer, TS_PACKET_SIZE);

     ttxstat->stax.in_pkt++;
     ttx_mbuf->ttx_ctrl = ttx_ctrl;
     cur_mb->m_type = MBT_DATA;

     process_ttx_pes(cur_mb);

     length -= TS_PACKET_SIZE;
     buffer += TS_PACKET_SIZE;
  }

  /*--------------------------------------------------------------------------+
  |  Buffer Length Should be Multiple of Transport Packet Size
  +--------------------------------------------------------------------------*/
  if (length > 0) {
     return(E_TTX_BAD_LENGTH);
  }

  return(0);
}

/*----------------------------------------------------------------------------+
|  ttx_initialize
|  Initialize teletext driver.
+----------------------------------------------------------------------------*/
int ttx_initialize(void)
{
  int           err;
  unsigned char *addr;


  if (ttxstat->ioenabled) {
      return(0);
  }

  /*--------------------------------------------------------------------------+
  | Allocate & init memory
  +--------------------------------------------------------------------------*/
  ttxstat->mem_size = (sizeof(ttx_list_entry) +
                       sizeof(ttx_ctrl_entry))* MAX_TTX_LIST_SIZE;
  if ((ttxstat->mem_ptr = MALLOC(ttxstat->mem_size)) == NULL) {
      return(E_TTX_NOMEM);
  }

  addr = ttxstat->mem_ptr;
  ttxstat->ttx_list = (Ttx_list_entry)addr;
  addr += sizeof(ttx_list_entry)*MAX_TTX_LIST_SIZE;

  ttxstat->ttx_ctrl = (Ttx_ctrl_entry)addr;
  addr += sizeof(ttx_ctrl_entry)*MAX_TTX_LIST_SIZE;

  ttxstat->num_ttx = 0;

  /*--------------------------------------------------------------------------+
  | Initialize Video decorder and DENC for teletext
  +--------------------------------------------------------------------------*/
  if ((err = ttx_vbi_init()) != 0) {
      FREE(ttxstat->mem_ptr);
      return(err);
  }

  ttxstat->ioenabled = 1;

  /*--------------------------------------------------------------------------+
  | Setup DENC to enable teletext processing
  +--------------------------------------------------------------------------*/
  ttx_insert_pid(0x1fff);
  ttx_on();

  return(0);
}

/*----------------------------------------------------------------------------+
|  ttx_terminate
|  Terminate teletext driver.
+----------------------------------------------------------------------------*/
int ttx_terminate(void)
{

  if (!ttxstat->ioenabled) {
      return(0);
  }

  /*--------------------------------------------------------------------------+
  | Disable teletext function of DENC
  +--------------------------------------------------------------------------*/
  ttx_off();

  /*--------------------------------------------------------------------------+
  | Disable teletext function of video decorder
  +--------------------------------------------------------------------------*/
  ttx_vbi_term();

  /*--------------------------------------------------------------------------+
  | Free the allocated memory.
  +--------------------------------------------------------------------------*/
  FREE(ttxstat->mem_ptr);
  ttxstat->ioenabled = 0;

  return(0);
}
