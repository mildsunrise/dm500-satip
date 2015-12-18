/*-----------------------------------------------------------------------------+
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
|       COPYRIGHT   I B M   CORPORATION 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------+
| Author:    Mike Lepore
| Component: sci
| File:      sci_prot.h
| Purpose:   Smart Card protocol layer PUBLIC API header file.
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 12/03/2001  MAL, Zongwei Liu  Added temporary RX buffer to control block and 
|                               check_incoming_data() prototype.
+----------------------------------------------------------------------------*/

#ifndef _sci_prot_h_
#define _sci_prot_h_

/* I-block PCB defines */
#define I_NS                        0x40
#define I_M                         0x20

/* R-block PCB defines */
#define R_NR                        0x10
#define R_EDC_ERROR                 0x01
#define R_ERROR                     0x02

/* S-block PCB defines */
#define S_RESYNCH_REQUEST           0xC0
#define S_RESYNCH_RESPONSE          0xE0
#define S_IFS_REQUEST               0xC1
#define S_IFS_RESPONSE              0xE1
#define S_ABORT_REQUEST             0xC2
#define S_ABORT_RESPONSE            0xE2
#define S_WTX_REQUEST               0xC3
#define S_WTX_RESPONSE              0xE3

/* ATR parameter defines */
#define SC_ATR_T                    0
#define SC_ATR_FI                   1
#define SC_ATR_DI                   1
#define SC_ATR_II                   1
#define SC_ATR_PI1                  5
#define SC_ATR_PI2                  50
#define SC_ATR_WI                   10
#define SC_ATR_XI                   0
#define SC_ATR_UI                   1
#define SC_ATR_N                    0
#define SC_ATR_CWI                  13
#define SC_ATR_BWI                  4
#define SC_ATR_IFSC                 32
#define SC_ATR_IFSD                 32
#define SC_ATR_CHECK                1
#define SC_ATR_RETRY                3
#define SC_RX_TEMP_BUF_SIZE         259

/* protocol state */
typedef struct
{
    unsigned char ATR[SC_MAX_ATR_SIZE];
    unsigned char atr_size;
    unsigned char *p_historical;
    unsigned char historical_size;
    unsigned char TCK_present;
    unsigned char firstT;
    unsigned char currentT;
    unsigned char NS;
    unsigned char IFSD;
    unsigned char proposed_IFSD;
    SC_PARAMETERS sc_parameters;
    unsigned char temp_buf[SC_RX_TEMP_BUF_SIZE];
}
SC_CONTROL_BLOCK;

/* ATR character types */
typedef enum
{
    TS,
    T0,
    TA,
    TB,
    TC,
    TD,
    TK,
    TCK
}
I_BYTES;

/* T=1 block types */
typedef enum
{
    I,
    R,
    S
}
BLOCKS;

/* private prototypes */
static SCI_ERROR process_body(unsigned char *p_body,
                              unsigned long L,
                              unsigned char **pp_data,
                              unsigned short *p_Lc, unsigned long *p_Le);

static SCI_ERROR sc_t0_command(unsigned long sc_id,
                               unsigned char *p_header,
                               unsigned char *p_body,
                               unsigned char *p_end_sequence,
                               unsigned long direction);

static SCI_ERROR sc_t1_command(unsigned long sc_id,
                               unsigned char *p_capdu,
                               unsigned char *p_rapdu,
                               unsigned long *p_length);

#endif /*_sci_prot_h_*/
