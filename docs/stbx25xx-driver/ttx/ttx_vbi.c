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
| File      :  ttx_vbi.c
| Purpose   :  Teletext VBI Driver
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
#include <os/os-interrupt.h>
#include <ttx.h>
#include "../vid/vid_atom.h"
#include "../vid/vid_osd.h"
#include "powerpc.h"
#include "ttx_mbuf.h"
#include "ttx_defs.h"

/*----------------------------------------------------------------------------+
| Local Defines
+----------------------------------------------------------------------------*/
#define DRAM_VBI_POINTER        0x00000004
#define DENC_TRR_FIELD_ID       0x70000000
#define TTX_READIN              0xaaaa
#define TTX_LINE_START          7
#define VBI_NBYTES_PER_LINE     48
#define VBI_ALLOC_UNIT          1440
#define ODD                     1
#define EVEN                    0

/*----------------------------------------------------------------------------+
| Static Variables
+----------------------------------------------------------------------------*/
extern VDEC         _videoDecoder;
extern unsigned int stb_vid_int_status;


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
|  get_field
+----------------------------------------------------------------------------*/
static int get_field(Mbuf mb)
{
  int              field_parity;
  unsigned char    *data_ptr;
  Ttx_mbuf_t       ttx_mbuf;
  Ttx_data_t       data;
  Ttx_data_field_t data_field;


  ttx_mbuf = (Ttx_mbuf_t)DATA_PTR(mb);

  /*------------------------------------------------------------------------+
  | Check PES packet header exist
  +------------------------------------------------------------------------*/
  if (ttx_mbuf->pts == 0) {
      data_ptr = ttx_mbuf->buf + 4;
  } else {
      data_ptr = ttx_mbuf->buf + 4 + 46;
  }

  data         = (Ttx_data_t)data_ptr;
  data_field   = &data->data_field;
  field_parity = (data_field->ctrl&0x20)>>5;

  return(field_parity);
}

/*----------------------------------------------------------------------------+
|  write_vbi
+----------------------------------------------------------------------------*/
static int write_vbi(unsigned char *vbi_base, Mbuf mb)
{
  Ttx_mbuf_t    ttx_mbuf;
  unsigned char *addr;


  /*--------------------------------------------------------------------------+
  | Process all MBUFS
  +--------------------------------------------------------------------------*/
  while (mb != NULL) {
     unsigned char *data_ptr;
     int           line_count;
     int           byte_count;

     ttx_mbuf = (Ttx_mbuf_t)DATA_PTR(mb);

     /*----------------------------------------------------------------------+
     | Check PES packet header exist
     +----------------------------------------------------------------------*/
     if (ttx_mbuf->pts == 0) {
         line_count = 4;
         data_ptr = ttx_mbuf->buf + 4;
     } else {
         line_count = 3;
         data_ptr = ttx_mbuf->buf + 4 + 46;
     }

     while (line_count-- > 0) {
        Ttx_data_t       data;
        Ttx_data_field_t data_field;
        int              field_parity;
        int              line_offset;
        unsigned char    *src;

        data         = (Ttx_data_t)data_ptr;
        data_field   = &data->data_field;
        field_parity = (data_field->ctrl&0x20)>>5;

        /*--------------------------------------------------------------------+
        | Calc buffer address for this line
        +--------------------------------------------------------------------*/
        line_offset = (data_field->ctrl&0x1f);
        addr = vbi_base + (line_offset - TTX_LINE_START)*VBI_NBYTES_PER_LINE;
        byte_count = 45;

        /*--------------------------------------------------------------------+
        | Write TTX read-in pattern
        +--------------------------------------------------------------------*/
        *(unsigned short *)addr = (unsigned short)TTX_READIN;
        addr       += 2;
        byte_count -= 2;

        /*--------------------------------------------------------------------+
        | Write TTX data
        +--------------------------------------------------------------------*/
        src = &data_field->framing_code;
        memcpy(addr,src,byte_count);
        data_ptr += sizeof(ttx_data_t);
     }

     mb = mb->m_pnext;
  }

  return(0);
}

/*----------------------------------------------------------------------------+
|  ttx_vbi_send
+----------------------------------------------------------------------------*/
static void ttx_vbi_send()
{
  int           delta;
  int           field_id;
  int           field_update;
  unsigned char *vbi_base;
  unsigned int  stc, pts;
  Mbuf          mb;


  /*--------------------------------------------------------------------------+
  | Toggle VBI Pointer
  +--------------------------------------------------------------------------*/
  field_id = (powerpcMfdenc0_trr()&DENC_TRR_FIELD_ID) >> 28;

  /*--------------------------------------------------------------------------+
  | Even Field Scan - Prepare writing to odd field (VBI1) buffer
  +--------------------------------------------------------------------------*/
  if (field_id&1) {
      vbi_base = ttxstat->vbi1_addr;
      field_update = ODD;

  /*--------------------------------------------------------------------------+
  | Odd field scan - Prepare writing to even field (VBI0) buffer
  +--------------------------------------------------------------------------*/
  } else {
      vbi_base = ttxstat->vbi0_addr;
      field_update = EVEN;
  }

  memset(vbi_base,0,VBI_ALLOC_UNIT);

  /*--------------------------------------------------------------------------+
  | Get current STC value
  +--------------------------------------------------------------------------*/
  stc = xp0_dcr_read(XP_DCR_ADDR_STCHI);

  while ((mb = ttxstat->pes_queue) != NULL) {
      pts = ((Ttx_mbuf_t)DATA_PTR(mb))->pts;
      delta = pts - stc;

      /*----------------------------------------------------------------------+
      | PTS is matched to STC, write to VBI buffer
      +----------------------------------------------------------------------*/
      if ((TTX_PTS_MATCH_MIN < delta)     &&
         (delta < TTX_PTS_MATCH_MAX)      &&
         (get_field(mb) == field_update)) {
          write_vbi(vbi_base, mb);
          ttx_m_deq(&ttxstat->pes_queue);
          ttx_m_enq(&ttxstat->done_queue, mb);
          ttxstat->stax.inbound_pes++;
          break;

      /*----------------------------------------------------------------------+
      | Do nothing. This data should be processed future VBI_START
      +----------------------------------------------------------------------*/
      } else if ((TTX_PTS_OUTBOUND_LOW < delta)   &&
                 (delta < TTX_PTS_OUTBOUND_HIGH)) {
          break;

      /*----------------------------------------------------------------------+
      | Out of range, discard data and continue loop to process next
      +----------------------------------------------------------------------*/
      } else {
          ttx_m_deq(&ttxstat->pes_queue);
          ttx_m_enq(&ttxstat->done_queue, mb);
          ttxstat->stax.outbound_pes++;
          continue;
      }
  }

  /*--------------------------------------------------------------------------+
  | Set VBI pointer for next VBI scan
  +--------------------------------------------------------------------------*/
  if (field_id&1) {
      powerpcMtvid0_memcntl(powerpcMfvid0_memcntl()&~DRAM_VBI_POINTER);
  } else {
      powerpcMtvid0_memcntl(powerpcMfvid0_memcntl()|DRAM_VBI_POINTER);
  }

  return;
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
| ttx_vsync_handler
| This function is called anytime there is a video decoder interrupt,
| but only VBI Start interrupts are processed.
+----------------------------------------------------------------------------*/
void ttx_vsync_handler()
{
  unsigned long reg;


  reg = stb_vid_int_status;

  if ((reg & DECOD_HOST_MASK_VBI_START) != 0) {
     ttx_vbi_send();
  }

  return;
}

/*----------------------------------------------------------------------------+
|  ttx_vbi_init
|  Setup video decorder to enable teletext processing.
+----------------------------------------------------------------------------*/
int ttx_vbi_init(void)
{
  unsigned int reg;
  unsigned int vdly;


  /*--------------------------------------------------------------------------+
  | Check video mode. This ttx driver supports PAL mode only.
  +--------------------------------------------------------------------------*/
  if (!(powerpcMfvid0_dispm()&DECOD_DISP_MODE_PAL_MODE)) {
      return(E_TTX_BAD_DISP_MODE);
  }

  /*--------------------------------------------------------------------------+
  | Check how many VBI lines were assumed by Video Driver.
  | Set VBI line Count
  +--------------------------------------------------------------------------*/
  ttxstat->num_vbi_lines = VID_VBI_LINES;

  if (ttxstat->num_vbi_lines < 16) {
     return(E_TTX_LESS_VBI_COUNT);
  } else {
     reg = powerpcMfvid0_vbcntl() & (~DECOD_VBI_CTL_VBI_LINE_MASK);
     powerpcMtvid0_vbcntl(reg | ttxstat->num_vbi_lines);
  }

  /*--------------------------------------------------------------------------+
  | Adjust VSYNC delay
  +--------------------------------------------------------------------------*/
  vdly = 23 - ttxstat->num_vbi_lines;
  powerpcMtvid0_dispd((powerpcMfvid0_dispd()&~0xff)|(vdly&0xff));

  /*--------------------------------------------------------------------------+
  | Map VBI Logical Address
  +--------------------------------------------------------------------------*/
  if (vid_osd_map_vbi_laddr((ULONG *)&ttxstat->vbi0_addr,
                            (ULONG *)&ttxstat->vbi1_addr) != 0) {
     return(E_TTX_BAD_VBI_ADDR);
  }

  /*--------------------------------------------------------------------------+
  | Configure TTX control for field 1. All lines for TELETEXT
  +--------------------------------------------------------------------------*/
  powerpcMtvid0_vbcntl(powerpcMfvid0_vbcntl()&~DECOD_VBI_CTL_TTX_INDEX_MASK);
  powerpcMtvid0_ttxcntl(0xffffffff);

  /*--------------------------------------------------------------------------+
  | Configure TTX control for field 2. All lines for TELETEXT
  +--------------------------------------------------------------------------*/
  powerpcMtvid0_vbcntl(powerpcMfvid0_vbcntl()|DECOD_VBI_CTL_TTX_INDEX_MASK);
  powerpcMtvid0_ttxcntl(0xffffffff);

  /*--------------------------------------------------------------------------+
  | Attach to Video Decoder Interrupt Handler to receive VBI Start interrupts
  +--------------------------------------------------------------------------*/
#if 1
  if (request_irq(IRQ_VID, ttx_vsync_handler, SA_SHIRQ,
                  "TTXvbi", &ttx_vbi_init) != 0) {
     vid_osd_unmap_vbi_laddr();
     return(E_TTX_VDC_INT_NOTIFY);
  }
  vid_atom_set_irq_mask(vid_atom_get_irq_mask() | DECOD_HOST_MASK_VBI_START);
#else
  if (vid_atom_ttx_add_notify(ttx_vsync_handler,DECOD_HOST_MASK_VBI_START) != 0) {
     vid_osd_unmap_vbi_laddr();
     return(E_TTX_VDC_INT_NOTIFY);
  }
#endif

  return(0);
}

/*----------------------------------------------------------------------------+
|  ttx_vbi_term
|  Setup video decorder to enable teletext processing.
+----------------------------------------------------------------------------*/
int ttx_vbi_term(void)
{
  unsigned int reg;


  /*--------------------------------------------------------------------------+
  | Remove notification of Video Decoder interrupts for VBI Start
  +--------------------------------------------------------------------------*/
#if 1
  vid_atom_set_irq_mask(vid_atom_get_irq_mask() & (~DECOD_HOST_MASK_VBI_START));
  free_irq(IRQ_VID, &ttx_vbi_init);
#else
  if (vid_atom_ttx_del_notify(ttx_vsync_handler,DECOD_HOST_MASK_VBI_START) != 0) {
     return(E_TTX_VDC_INT_NOTIFY);
  }
#endif

  /*--------------------------------------------------------------------------+
  | Change TTX control to reset value
  +--------------------------------------------------------------------------*/
  powerpcMtvid0_vbcntl(powerpcMfvid0_vbcntl()&~DECOD_VBI_CTL_TTX_INDEX_MASK);
  powerpcMtvid0_ttxcntl(0x00000000);
  powerpcMtvid0_vbcntl(powerpcMfvid0_vbcntl()|DECOD_VBI_CTL_TTX_INDEX_MASK);
  powerpcMtvid0_ttxcntl(0x00000000);

  /*--------------------------------------------------------------------------+
  | Reset number of VBI lines
  +--------------------------------------------------------------------------*/
  ttxstat->num_vbi_lines = 0;
  reg = powerpcMfvid0_vbcntl() & (~DECOD_VBI_CTL_VBI_LINE_MASK);
  powerpcMtvid0_vbcntl(reg | ttxstat->num_vbi_lines);

  /*--------------------------------------------------------------------------+
  | Unmap VBI Logical Address
  +--------------------------------------------------------------------------*/
  vid_osd_unmap_vbi_laddr();

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

