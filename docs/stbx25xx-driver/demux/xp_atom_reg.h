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
| Author:    Paul Gramann
| Component: xp
| File:      xp_atom_reg.h
| Purpose:   Transport defines.
|
| Changes:
| Date:      Author  Comment:
| ---------  ------  --------
| 05-Apr-99  PAG     Created
| 04-May-99  PAG     Placed in library
| 1m-May-99  MPT     Changed "pack" pragma
| 20-Jan-00  TJC     Updated for Vesta.
| 30-Apr-01  TJC     Updated for Pallas.
| 01-Sep-01  LGH     Combine the three devices dcr definition, ported to Linux
+----------------------------------------------------------------------------*/

#ifndef XP_REG_H
#define XP_REG_H

#include <hw/hardware.h>

#pragma pack(1)
/*----------------------------------------------------------------------------+
|  Register Defines
+----------------------------------------------------------------------------*/
#ifndef __DRV_FOR_VESTA__       // for vulcan and pallas ...
#define XP_CONFIG3_INSEL_CI0     0     /* input select to CIO Interface      */

#ifdef __DRV_FOR_PALLAS__
#define XP_CONFIG3_INSEL_CI1     1     /* input select to CI1 Interface      */
#define XP_CONFIG3_INSEL_1394    2     /* input select to 1394 Interface     */
#endif
#endif
//Only make sense to device XP0
#define XP_CONFIG3_INSEL_PVR     3     /* input select to PVR mode           */

/*----------------------------------------------------------------------------+
|  CONFIGURATION 1 REGISTER
|  Location Address: 0x0000
+----------------------------------------------------------------------------*/
typedef struct xp_config1_type {
    //The following 4 fields make bo sense to XP0
    unsigned tsrc        : 1;          /* Auxiliary Port 1394 Compatiability */
    unsigned axclkp      : 1;          /* 1=Rising edge of AX_CLOCK,0=falling*/
    unsigned axced       : 3;          /* Auxilliary Clock Edge Delay        */
    unsigned aec         : 3;          /* Auxilliary Extend Clock            */

    unsigned denbl       : 1;          /* 1=descrambler enabled, 0=disabled  */
//the following 5 fields only make sense to XP0
    unsigned res2        : 2;
    unsigned vpu         : 1;          /* 1=force vid packets also to Q31/BQ */
    unsigned apu         : 1;          /* 1=force aud packets also to Q30/BQ */
    unsigned apwma       : 1;          /* 1=adjust     PWM, 0=no adjusted    */
    unsigned tstoe       : 1;          /* 1=TS timeout enable, 0=disabled    */
//the following 5 fields make no senses to XP0
    unsigned axdp        : 1;          /* 1=AX_DATA inverted, 0=unmodified   */
    unsigned res3        : 1;
    unsigned axsp        : 1;          /* 1=start polarity low act,0=high    */
    unsigned axep        : 1;          /* 1=error polarity low act,0=high    */
    unsigned axdvp       : 1;          /* 1=data val polarity low act,0=high */

    unsigned tsclkp      : 1;          /* 1=data latched on clock FE, 0=RE   */
    unsigned tsdp        : 1;          /* 1=CI_DATA inverted, 0=not inverted */

#ifdef __DRV_FOR_PALLAS__
    unsigned tssp        : 1;          /* 1=CI_PACKET_START low act, 0=high  */
    unsigned tsep        : 1;          /* 1=CI_DATA_ERROR low active,0=high  */
#else
    unsigned res4        : 2;
#endif

    unsigned tsvp        : 1;          /* 1=CI_DATA_ENABLE low input,0=high  */

#ifdef __DRV_FOR_PALLAS__
    unsigned tssm        : 1;          /* 1=CI_PACKET_START used, 0=not used */
#else
    unsigned res5        : 1;
#endif

    unsigned syncd       : 2;          /* no. of syncbytes +1 dropped        */
    unsigned bbmode      : 1;          /* 1=bit mode, 0=byte mode            */
    unsigned syncl       : 3;          /* no. of sync bytes+1 before in sync */

} XP_CONFIG1_REG, *XP_CONFIG1_REGP;

/*----------------------------------------------------------------------------+
|  CONTROL1 REGISTER
|  Location Address: 0x0001
+----------------------------------------------------------------------------*/
typedef struct xp_control1_type {
    unsigned res1        :25;
    unsigned sbe         : 1;          /* 1=bypass, 0=scan for 0x47 in stream*/
    unsigned pbe         : 1;          /* 1=parser bypass enabled, 0=disabled*/
    unsigned res2        : 1;
    unsigned senbl       : 1;          /* 1=bit mode, 0=byte mode            */
    unsigned sdop        : 1;          /* 1=disable packet sync, 0=no action */
    unsigned res3        : 1;
    unsigned swrst       : 1;          /* 1=reset transport                  */
} XP_CONTROL1_REG, *XP_CONTROL1_REGP;

/*----------------------------------------------------------------------------+
|  CONFIGURATION 3 REGISTER
|  Location Address: 0x0006
+----------------------------------------------------------------------------*/
typedef struct xp_config3_type {
    unsigned res1        :21;
    unsigned insel       : 2;          /* Input Sel:0=CI0,1=CI1,2=1394,3=PVR */
//make no sense to XP0
    unsigned api         : 5;          /* Auxiliary Packet Interval          */
    unsigned dmode       : 1;          /* Descrambler Mode                   */
    unsigned res3        : 3;
} XP_CONFIG3_REG, *XP_CONFIG3_REGP;

/*----------------------------------------------------------------------------+
|  PWM REGISTER
|  Location Address: 0x0016
+----------------------------------------------------------------------------*/
typedef struct xp_pwm_type {
    unsigned res1        :20;
    unsigned pwm         :12;          /* Pulse width modulator              */
} XP_PWM_REG;

/*----------------------------------------------------------------------------+
|  PCR-STC THRESHOLD REGISTER
|  Location Address: 0x0017
+----------------------------------------------------------------------------*/
typedef struct xp_pcrstct_type {
    unsigned res1        :14;
    unsigned pcrt        :18;          /* PCR Threshold                      */
} XP_PCRSTCT_REG;

/*----------------------------------------------------------------------------+
|  PCR-STC DELTA REGISTER
|  Location Address: 0x0018
+----------------------------------------------------------------------------*/
typedef struct xp_pcrstcd_type {
    unsigned res1        :12;
    unsigned ovfl        : 1;          /* overflow                           */
    unsigned sign        : 1;          /* sign, 0=PCR < STC, 1=PCR > STC     */
    unsigned delta       :18;          /* delta between latched PCR and STC  */
} XP_PCRSTCD_REG, *XP_PCRSTCD_REGP;

/*----------------------------------------------------------------------------+
|  PCRPID REGISTER
|  Location Address: 0x01FF
+----------------------------------------------------------------------------*/
typedef struct xp_pcrpid_type {
    unsigned res1        :19;
    unsigned pidv        :13;          /* pid value                          */
} XP_PCRPID_REG, *XP_PCRPID_REGP;
/*----------------------------------------------------------------------------+
|  CONFIGURATION 2 REGISTER
|  Location Address: 0x1000
+----------------------------------------------------------------------------*/
typedef struct xp_config2_type {
//ONLY make sens to XP0
    unsigned res1        :12;
    unsigned ved         : 1;          /* 1=vid unloader clear errors,0=send */
    unsigned res2        : 3;
    unsigned acpm        : 1;          /* 1=aud CPM mode disabled,0=enabled  */
    unsigned vcpm        : 1;          /* 1=vid CPM mode disabled,0=enabled  */
    unsigned res3        : 4;
    unsigned mwe         : 1;          /* 1=match word enabled,   0=disabled */
    unsigned salign      : 1;          /* 1=table sect word align,0=byte     */
//the following 8 fields only make sens to XP0
    unsigned res4        : 1;
    unsigned atsed       : 1;          /* 1=aud TS errs disabled, 0=enabled  */
    unsigned atbd        : 1;          /* 1=aud time base disable,0=enabled  */
    unsigned accd        : 1;          /* 1=aud chan chg disabled,0=enabled  */
    unsigned res5        : 1;
    unsigned vtsed       : 1;          /* 1=vid TS errs disabled, 0=enabled  */
    unsigned vtbd        : 1;          /* 1=vid time base disable,0=enabled  */
    unsigned vccd        : 1;          /* 1=vid chan chg disabled,0=enabled  */
} XP_CONFIG2_REG, *XP_CONFIG2_REGP;
/*----------------------------------------------------------------------------+
|  PACKET BUFFER LEVEL REGISTER
|  Location Address: 0x1002
+----------------------------------------------------------------------------*/
typedef struct xp_pbuflvl_type {
//the following 4 fields only make sense to XP0
    unsigned qpthres     : 4;          /* queue packet threshold             */
    unsigned apthres     : 4;          /* audio packet threshold             */
    unsigned vpthres     : 4;          /* video packet threshold             */
    unsigned res1        : 4;
    unsigned mlvl        : 4;          /* maximum packets seen in FIFO       */
    unsigned res2        : 2;
    unsigned cvp         :10;          /* bits for current packets           */
} XP_PBUFLVL_REG;
/*----------------------------------------------------------------------------+
|  BUCKET1 QUEUE REGISTER
|  Location Address: 0x1021
+----------------------------------------------------------------------------*/
#ifdef __DRV_FOR_PALLAS__
typedef struct xp_bucket1q_type {
    unsigned res1        :23;
    unsigned bqdt        : 1;          /* 1=packet delivered, 0=hdr/adapt    */
    unsigned res2        : 2;
    unsigned bvalid      : 1;          /* 1=bucket pid valid                 */
    unsigned indx        : 5;          /* bucket queue pid index             */
} XP_BUCKET1Q_REG, *XP_BUCKET1Q_REGP;
#else
typedef struct xp_bucket1q_type {
    unsigned res1        :19;
    unsigned bvalid      : 1;          /* 1=bucket pid valid                 */
    unsigned res2        : 3;
    unsigned bqdt        : 1;          /* 1=packet delivered, 0=hdr/adapt    */
    unsigned res3        : 2;
    unsigned indx        : 6;          /* bucket queue pid index             */
} XP_BUCKET1Q_REG, *XP_BUCKET1Q_REGP;
#endif
/*----------------------------------------------------------------------------+
|  FILTER REGISTERS
|  Location Address: 0x0100-0x011F
+----------------------------------------------------------------------------*/
typedef struct xp_pid_filter_type {
    /*------------------------------------------------------------------------+
    |  Bits 0-13 are valid for PIDS 24-27 only
    +------------------------------------------------------------------------*/
    unsigned dteic       : 1;          /* disable Transport Err indic check  */
    unsigned dafcc       : 1;          /* disable Adaptation Field Cnt check */
    unsigned ddpc        : 1;          /* disable Duplicate Packet Check     */
    unsigned tei         : 1;          /* comp to Transport Error indic      */
    unsigned pusi        : 1;          /* comp to Payload Unit Start indic   */
    unsigned tpi         : 1;          /* comp to Transport Polarity indic   */
    unsigned tsc         : 2;          /* comp to Transport Scrambling Cntl  */
    unsigned afc         : 2;          /* comp to Adaptation Field Cntl      */
    unsigned ccnt        : 4;          /* comp to Continuity Count           */
    /*------------------------------------------------------------------------+
    |  Bits 14-31 are valid for all PID registers
    +------------------------------------------------------------------------*/
    unsigned denbl       : 1;          /* 1=process packet by descrambler    */
    unsigned pesl        : 1;          /* 0=ts level, 1=PES descrambling     */
    unsigned kid         : 3;          /* indicates which key set to use     */
    unsigned pidv        :13;          /* pid value                          */
} XP_PID_FILTER_REG, *XP_PID_FILTER_REGP;
/*----------------------------------------------------------------------------+
|  QUEUE CONFIGA REGISTER
|  Location Address: 0x2200-0x223E (EVN)
+----------------------------------------------------------------------------*/
typedef struct xp_qconfiga_type {
    unsigned enda        :12;          /* address of top of queue            */
    unsigned starta      :12;          /* address of bottom of queue         */
    unsigned bthres      : 8;          /* num of blocks before an interrupt  */
} XP_QCONFIGA_REG, *XP_QCONFIGA_REGP;
/*----------------------------------------------------------------------------+
|  QUEUE CONFIGB REGISTER
|  Location Address: 0x2201-0x223F (ODD)
+----------------------------------------------------------------------------*/
typedef struct xp_qconfigb_type {
    unsigned rptr        :16;          /* address of the read pointer        */
    unsigned res1        : 1;
    unsigned scpc        : 1;          /* 1=generate interrupt for each table*/
    unsigned fsf         : 6;          /* first DRAM filter number           */
//make no sense to XP0
    unsigned bsel        : 2;          /* Bucket Que: 0=B1,1=B2,2=B1,3=B1/B2 */
    unsigned apus        : 1;          /* 1=wait for PUS first               */
    unsigned enbl        : 1;          /* 0=off, 1=use DRAM queues           */
    unsigned dtype       : 4;          /* types to dump                      */
} XP_QCONFIGB_REG, *XP_QCONFIGB_REGP;
/*----------------------------------------------------------------------------+
|  QUEUE STATUSB REGISTER
|  Location: 0x2801-0x287D (BY-4)
+----------------------------------------------------------------------------*/
typedef struct xp_qstatb_type {
    unsigned res         : 8;
    unsigned wptr        :24;          /* write pointer                      */
} XP_QSTATB_REG, *XP_QSTATB_REGP;
/*----------------------------------------------------------------------------+
|  QUEUE STATUSC REGISTER
|  Location: 0x2802-0x287E (BY-4)
+----------------------------------------------------------------------------*/
typedef struct xp_qstatc_type {
    unsigned crce        :32;          /* crc32 for current table section    */
} XP_QSTATC_REG, *XP_QSTATC_REGP;
/*----------------------------------------------------------------------------+
|  QUEUE STATUSD REGISTER
|  Location: 0x2803-0x287F (BY-4)
+----------------------------------------------------------------------------*/
typedef struct xp_qstatd_type {
    unsigned res1        : 8;
    unsigned wstart      :24;          /* write start of table section       */
} XP_QSTATD_REG, *XP_QSTATD_REGP;
/*----------------------------------------------------------------------------+
|  FILTER BLOCK CONTROL REGISTER
|  Location: 0x2380-0x23FC (BY-4)
+----------------------------------------------------------------------------*/
typedef struct xp_filter_control_type {
    unsigned res1        : 3;
    unsigned sfid        : 5;          /* group Id. of the filter block      */
    unsigned res2        :14;
    unsigned enbl        : 1;          /* 1=filter enabled, 0=disabled       */
    unsigned ncol        : 1;          /* 1=end of this column               */
    unsigned res3        : 2;
    unsigned nfilt       : 6;          /* next filter number                 */
} XP_FILTER_CONTROL_REG, *XP_FILTER_CONTROL_REGP;
#pragma pack()
#endif

