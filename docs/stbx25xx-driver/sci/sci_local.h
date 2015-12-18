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
| File:      sci_drv_local.h
| Purpose:   Smart Card Interface device driver local header file.
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/13/2001  MAL, Zongwei Liu  Added EMV2000 defines and TX timeout define.
|                               Added the define of SCI_DRV_MODES.
+----------------------------------------------------------------------------*/

#ifndef _sci_local_h_
#define _sci_local_h_

#include "sci/sci_global.h"
#include "os/os-types.h"

/* board-specific defines */
#define ACTIVE_HIGH                 1
#define ACTIVE_LOW                  0
#define SCI_CLASS                   SCI_CLASS_A /* operating class of SCI */

/* base address and interrupt defined */
#define SCI_BASE_ADDRESS_0          0x40020000
#define SCI_INTERRUPT_LEVEL_0       IRQ_SC0
#define SCI_INTERRUPT_MASK_0        (1<<(31-SCI_INTERRUPT_LEVEL_0))

#define SCI_BASE_ADDRESS_1          0x40070000
#define SCI_INTERRUPT_LEVEL_1       IRQ_SC1
#define SCI_INTERRUPT_MASK_1        (1<<(31-SCI_INTERRUPT_LEVEL_1))

#define SCI_IO_SIZE                 4096

/* Some registers for SCI */
#define CICCR       0x0030
#define CICSEL3     0x0035
#define UICSR       0x0040

#define RXLEN_MAX                   0x1FF
#define DIVISOR_MAX                 0xFF

/* register offsets from base address */
#define SCBUFFS                     0x00
#define SCCTL0                      0x04
#define SCCTL1                      0x05
#define SCINTEN0                    0x06
#define SCINTEN1                    0x07
#define SCINT0                      0x08
#define SCINT1                      0x09

#define SCSTAT                      0x0A
#define SCBMR                       0x0B
#define SCCLK_CNT0                  0x0C
#define SCCTL3                      0x0D
#define SCETU0                      0x0E
#define SCETU1                      0x0F
#define SCRXLEN                     0x10
#define SCTXLEN                     0x12
#define SCCWT0                      0x14
#define SCCWT1                      0x15
#define SCEGT                       0x16
#define SCBWT0                      0x18
#define SCBWT1                      0x19
#define SCBWT2                      0x1A
#define SCBWT3                      0x1B

/* register bit defines */
/* SCCTL0 */
#define RST                         0x80
#define VCC                         0x40
#define CLKEN                       0x20
#define IO_EN                       0x10
#define DEACT                       0x08
#define DCONV                       0x04
#define SCRST                       0x02
#define SDFLT                       0x01

/* SCCTL1 */
#define ERRDET                      0x80
#define PRTCL                       0x40
#define FIFO1                       0x20
#define FIFO2                       0x10
#define RFIFOR                      0x08
#define TFIFOR                      0x04
#define FLUSH                       0x02
#define FIFO0                       0x01

/* SCINTEN0 */
#define ENBOI                       0x8000
#define ENREI                       0x4000
#define ENTUI                       0x2000
#define ENCHI                       0x1000
#define ENTSI                       0x0800
#define ENRFI                       0x0400
#define ENRXLI                      0x0200
#define ENCDI                       0x0100

/* SCINTEN1 */
#define ERFTHI                      0x0080
#define ETFHI                       0x0040
#define ERUI                        0x0020

#define EPARI                       0x0010
#define ELTXI                       0x0008
#define EFRXI                       0x0004

/* SCINT0 */
#define BOI                         0x8000      /* Buffer Overflow */
#define REI                         0x4000      /* Receive End */
#define TUI                         0x2000      /* Transmit Underrun */
#define CHI                         0x1000      /* Checks Failed */
#define TSI                         0x0800      /* TS Character Invalid */
#define RFI                         0x0400      /* Reset Failed */
#define RXLEN                       0x0200      /* Receive Length Error */
#define CDI                         0x0100      /* Card Detection */

/* SCINT1 */
#define RFTHI                       0x0080      /* Receive FIFO Threshold */
#define TFHI                        0x0040      /* Transmit FIFO Threshold */
#define RUI                         0x0020      /* Read Underflow */
#define PARI                        0x0010      /* Parity Error */
#define LTXI                        0x0008      /* Last Transmit */
#define FRXI                        0x0004      /* First Receive */

/* SCSTAT */
#define VALIDB                      0xC0
#define TSTAT                       0x30
#define RSTAT                       0x0C
#define DET                         0x02
#define RPEND                       0x01

/* SCBMR */
#define SCH0                        0x80
#define RCH0                        0x40
#define SCH1                        0x20
#define RCH1                        0x10
#define RFFT                        0x08
#define TFFT                        0x04

/* SCCTL3 */
#define CRESET                      0x80
#define NDEACT                      0x40
#define POLDET                      0x20
#define POLVCC                      0x10
#define CLKSTP                      0x08
#define AUTO                        0x04
#define SCIEMV                      0x02

/* SCETU0 */
#define CNT0BUSY                    0x80

/* ATR parameter defines by standard */
#define SCI_ATR_T                   0
#define SCI_ATR_F                   4500000
#define SCI_ATR_ETU                 372
#define SCI_ATR_WWT                 9600
#define SCI_ATR_CWT                 8203
#define SCI_ATR_BWT                 15371
#define SCI_ATR_EGT                 0
#define SCI_ATR_CHECK               1
#define SCI_ATR_P                   5
#define SCI_ATR_I                   50
#define SCI_ATR_U                   SCI_CLASS_A

/* total ATR time EMV2000-Book1 only */
#define SCI_ATR_AWT                 20160

/* minimum time, t1 (between SCCLK active and SCRST active) ISO/IEC 7816 only */
#define SCI_ISO_T1_MIN              400
/* minimum time, t1 (between SCCLK active and SCRST active) EMV2000-Book1 only */
#define SCI_EMV_T1_MIN              40000
/* maximum time in ms to block in synchronous mode for read or write */
#define SCI_TIMEOUT_MS              150000

typedef enum
{
    COLD,
    WARM
}
SCI_RESET_TYPE;

typedef enum
{
    SCI_STATE_INIT = 0,
    SCI_STATE_DEAC,
    SCI_STATE_RX,
    SCI_STATE_TX
}
SCI_STATE;

typedef struct
{
    ULONG           detect_polarity;
    ULONG           vcc_polarity;
    ULONG           emv_supported;
}
SCI_DRV_MODES;

/* SCI control block */
typedef struct
{
    ULONG           sci_base_address;
    ULONG           sci_interrupt_mask;
    ULONG           sci_interrupt_level;

    /* SCI register addresses */
    volatile ULONG  *scbuffs;
    volatile UCHAR  *scctl0;
    volatile UCHAR  *scctl1;
    volatile USHORT *scinten;
    volatile USHORT *scint;
    volatile UCHAR  *scstat;
    volatile UCHAR  *scbmr;
    volatile UCHAR  *scclk_cnt0;
    volatile UCHAR  *scctl3;
    volatile USHORT *scetu;
    volatile USHORT *scrxlen;
    volatile USHORT *sctxlen;
    volatile USHORT *sccwt;
    volatile UCHAR  *scegt;
    volatile ULONG  *scbwt;

    SCI_STATE       state;
    INT             error;
    ULONG           waiting;
    //MUTEX_T         mutex;
    ULONG           read_flags;
    ULONG           rx_complete;
    UCHAR           buffer[SCI_BUFFER_SIZE];
    UCHAR           *p_read;
    UCHAR           *p_write;
    ULONG           first_rx;
    SCI_MODES       sci_modes;
    SCI_PARAMETERS  sci_parameters;
    UCHAR           write_buf[SCI_BUFFER_SIZE];
    UCHAR           read_buf[SCI_BUFFER_SIZE];
    ULONG           driver_inuse;
    SCI_ATR_STATUS  atr_status;
    INT             bytes_expected;
}
SCI_CONTROL_BLOCK;

#endif /* _sci_local_h_ */
