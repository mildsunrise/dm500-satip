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
| File      :  ttx_defs.h
| Purpose   :  Teletext Driver
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 22-Sep-03  TJC  Modified
+----------------------------------------------------------------------------*/
#include <ttx.h>
#include "ttx_mbuf.h"

/*----------------------------------------------------------------------------+
| GENERAL DEFINES
+----------------------------------------------------------------------------*/
#define GET_INT16(p)            ((*(p)<<8) + *(p+1))
#define GET_INT32(p)            ((GET_INT16(p)<<16) + GET_INT16(p+2))


/*----------------------------------------------------------------------------+
| PTS-STC DEFINES
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Intentinal PTS delay
| In some Teletext stream, PTS embeded in PES header may reach STC before
| receiving complete PES packet. That's TS packets close to end of PES packet
| cannot be composed to PES packet.  To rescue such a condition, PTS sent from
| network is intentionally delayed.  Since frame interval of PAL TV is 20 ms,
| this delay value is set to this.  Packet arriving beyond this margin may be
| discarded.
+----------------------------------------------------------------------------*/
#define TTX_PTS_RECEIVE_DELAY   (900*2)          /* 40ms in 45KHz clock      */

/*----------------------------------------------------------------------------+
| Constants for STC <-> PTS match
| If delta of PTS and STC falls into between TTX_PTS_MATCH_MIN and
| TTX_PTS_MATCH_MAX and field id is opposite to current scan, waiting teletext
| data will be written to VBI buffer.  If it falls into between
| TTX_PTS_OUTBOUND_LOW and TTX_PTS_OUTBOUND_HIGH, waiting teletext data keeps
| waiting for future VSYNC.  Othewise waiting teletext data is discarded.
+----------------------------------------------------------------------------*/
#define TTX_PTS_MATCH_MIN       -1350            /* -30ms in 45KHz clock     */
#define TTX_PTS_MATCH_MAX       1350             /* 30ms in 45KHz clock      */
#define TTX_PTS_OUTBOUND_LOW    -450             /* -10ms in 45KHz clock     */
#define TTX_PTS_OUTBOUND_HIGH   9000             /* 200ms in 45KHz clock     */

/*----------------------------------------------------------------------------+
| Currently whole of teletext list is statically allocated at initialization
+----------------------------------------------------------------------------*/
#define MAX_TTX_LIST_SIZE       32

/*----------------------------------------------------------------------------+
| MPEG-2 TS PACKET DEFINES
+----------------------------------------------------------------------------*/
#define TS_PACKET_SIZE          188
#define TS_ERR_MASK             0x80             /* byte 1                   */
#define TS_PUSI_MASK            0x40             /* byte 1                   */
#define TS_PRIORITY_MASK        0x20             /* byte 1                   */
#define TS_PID_MASK             0x1fff           /* byte 1-2                 */
#define TS_AFC_MASK             0x30             /* byte 3                   */
#define TS_AFC_PL_ONLY          0x10             /* payload only             */
#define TS_AFC_AF_ONLY          0x20             /* adaptation field only    */
#define TS_CC_MASK              0x0f             /* byte 3                   */

#define PES_ID                  0x000001BD       /* byte 0-3: start code     */
                                                 /* ..and stream ID          */
#define PES_PTS_DTS_FLAG_MASK   0xC0             /* byte 7                   */
#define PES_PTS_DTS_FLAG_PTS    0x80             /* only PTS exist           */
#define PES_PTS_DTS_FLAG_PDTS   0xC0             /* both of PTS & DTS exist  */
#define PES_HD_LENGTH_TTX       0x24             /* byte 8: fixed to 0x24    */

#define PES_TTX_DATA_ID_START   0x10             /* this range is EBU data   */
#define PES_TTX_DATA_ID_END     0x1F
#define PES_TTX_UNIT_ID_START   0x02             /* this range is EBU data   */
#define PES_TTX_UNIT_ID_END     0x03


/*----------------------------------------------------------------------------+
| DATA STRUCTURES
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| m_data in MBUF Format
+----------------------------------------------------------------------------*/
typedef struct {
    void          *ttx_ctrl;                     /* pointer to ttx control   */
    unsigned int  pts;                           /* 32 bits PTS (45KHz)      */
    unsigned char buf[188];                      /* 188 bytes TS packet      */
} ttx_mbuf_t, *Ttx_mbuf_t;

/*----------------------------------------------------------------------------+
| PES Packet Data
+----------------------------------------------------------------------------*/
typedef struct {
    unsigned char    ctrl;
    unsigned char    framing_code;
    unsigned char    ma_pa[2];
    unsigned char    data[40];
} ttx_data_field_t, *Ttx_data_field_t;

typedef struct {
    unsigned char    data_unit_id;
    unsigned char    data_length;
    ttx_data_field_t data_field;
} ttx_data_t, *Ttx_data_t;

typedef struct {
    unsigned char    data_id;
    ttx_data_t       ttx_data[1];               /* one or more in PES packet */
} pes_data_t, *Pes_data_t;

/*----------------------------------------------------------------------------+
| Teletext List Structure
+----------------------------------------------------------------------------*/
typedef struct
{
    unsigned short pid;
    unsigned char  lang_code[3];
    unsigned char  type_mag;
    unsigned char  page;
} ttx_list_entry, *Ttx_list_entry;

/*----------------------------------------------------------------------------+
| Teletext PID Control Block
+----------------------------------------------------------------------------*/
typedef struct {
    Mbuf               last_chain;  /* last mbuf chain (first mbuf with PTS) */
    Mbuf               last_mb;     /* last mbuf fragment with last PTS value*/
    unsigned int       last_pts;    /* last PTS value                        */
} ttx_ctrl_entry, *Ttx_ctrl_entry;

typedef struct {
    unsigned int in_pkt;                         /* incoming packets         */
    unsigned int in_pusi;                        /* incoming PUSI            */
    unsigned int out_pusi;                       /* outgoing PUSI            */
    unsigned int no_pusi;                        /* no PUSI found in packet  */
    unsigned int trash_pkt;                      /* packet going into trash  */
    unsigned int inbound_pes;                    /* PES processed by PTS cmp */
    unsigned int outbound_pes;                   /* PES discarded by PTS cmp */
} ttx_stax_t, *Ttx_stax_t;

/*----------------------------------------------------------------------------+
| Structure to keep driver status and parameters
+----------------------------------------------------------------------------*/
typedef struct {
    int            ioenabled;                    /* init flag                */
    unsigned int   mem_size;                     /* allocated memory size    */
    void*          mem_ptr;                      /* allocated memory         */
    Ttx_list_entry ttx_list;                     /* Teletext list            */
    Ttx_ctrl_entry ttx_ctrl;                     /* Teletext pid cntl block  */
    unsigned int   num_ttx;                      /* no of subscribed ttx pids*/
    Mbuf           pes_queue;                    /* VSYNC waiting queue      */
    Mbuf           done_queue;                   /* pckts processed in VSYNC */
    ttx_stax_t     stax;                         /* statistics counters      */
    unsigned int   num_vbi_lines;                /* No. of teletext VBI lines*/
    unsigned char  *vbi0_addr;                   /* Even field VBI buffer ptr*/
    unsigned char  *vbi1_addr;                   /* Odd field VBI buffer ptr */
} ttx_stat_t, *Ttx_stat_t;

extern Ttx_stat_t ttxstat;


/*----------------------------------------------------------------------------+
| PROTOTYPE DEFINITIONS
+----------------------------------------------------------------------------*/
int ttx_vbi_init(void);
int ttx_vbi_term(void);
