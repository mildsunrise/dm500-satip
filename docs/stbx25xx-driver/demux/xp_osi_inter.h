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
/*-----------------------------------------------------------------------------+
| Author:    Ian Govett
| Component: xp
| File:      xp_inter.h
| Purpose:   Transport interrupt defines and prototypes.
| Changes:
|
| Date:      Author  Comment:
| ---------  ------  --------
| 04-May-01  TJC     Updates for Pallas.
| 30-Sep-01  LGH     Ported to Linux
+-----------------------------------------------------------------------------*/

#ifndef XP_INTERRUPT_H

#define XP_INTERRUPT_H

/*----------------------------------------------------------------------------+

|  Local Defines

+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+

|  There are 32 channels available (0..31), and there are 34 interrupt

|  registers available, 32 channels, Audio, and Video

|  NOTE:  if CHANNEL_COUNT changes in "xp_channel, update the following

+----------------------------------------------------------------------------*/

#define XP_INTERRUPT_AUDIO            32         /* audio interrupts         */

#define XP_INTERRUPT_VIDEO            33         /* video interrupts         */

#define XP_INTERRUPT_COUNT            34         /* total interrupts avail   */



#define XP_INTERRUPT_NOTIFY_ADD       0          /* add notification funct   */

#define XP_INTERRUPT_NOTIFY_DELETE    1          /* remove notification funct*/



/*----------------------------------------------------------------------------+

|  Interrupt Register Mask

+----------------------------------------------------------------------------*/

#define XP_INTERRUPT_IR_FES         0x80000000   /* Front-End Status         */

#define XP_INTERRUPT_IR_QUE         0x40000000   /* Queue Status             */

#define XP_INTERRUPT_IR_AUD         0x20000000   /* Audio Unloader           */

#define XP_INTERRUPT_IR_VID         0x10000000   /* Video Unloader           */

#define XP_INTERRUPT_IR_STCL        0x04000000   /* STC Loaded               */

#define XP_INTERRUPT_IR_PCR         0x02000000   /* PCR Compare              */

#define XP_INTERRUPT_IR_STCC        0x01000000   /* STC Compare              */

#define XP_INTERRUPT_IR_PLBME       0x00800000   /* PLB M_Error              */

#define XP_INTERRUPT_IR_ACCC        0x00200000   /* Audio Channel Chg Cmplt  */

#define XP_INTERRUPT_IR_VCCC        0x00100000   /* Video Channel Chg Cmplt  */

#define XP_INTERRUPT_IR_SFLL        0x00080000   /* Section Filter Error     */

#define XP_INTERRUPT_IR_DS          0x00040000   /* Descrambler Status       */

#define XP_INTERRUPT_IR_QSTK        0x00020000   /* Queue Address Stack      */

#define XP_INTERRUPT_IR_X2INT       0x00000002   /* Transport Demux 2        */

#define XP_INTERRUPT_IR_X1INT       0x00000001   /* Transport Demux 1        */



/*----------------------------------------------------------------------------+

|  Front-End Status Interrupt Masks

+----------------------------------------------------------------------------*/

#define XP_INTERRUPT_FESTAT_FPCR    0x00004000   /* First PCR                */

#define XP_INTERRUPT_FESTAT_SP      0x00002000   /* Short Packet             */

#define XP_INTERRUPT_FESTAT_MPFM    0x00001000   /* Mult PID Filter Matches  */

#define XP_INTERRUPT_FESTAT_QPO     0x00000800   /* Queue Packet Overflow    */

#define XP_INTERRUPT_FESTAT_APO     0x00000400   /* Audio Packet Overflow    */

#define XP_INTERRUPT_FESTAT_VPO     0x00000200   /* Video Packet Overflow    */

#define XP_INTERRUPT_FESTAT_TSE     0x00000100   /* Transport Stream Error   */

#define XP_INTERRUPT_FESTAT_PBO     0x00000020   /* Packet Buffer Overflow   */

#define XP_INTERRUPT_FESTAT_MS      0x00000010   /* Missing Sync             */

#define XP_INTERRUPT_FESTAT_TSHE    0x00000008   /* Transport Stream Hdr Err */

#define XP_INTERRUPT_FESTAT_EI      0x00000004   /* Error Input              */

#define XP_INTERRUPT_FESTAT_SLOST   0x00000002   /* Sync Lost                */

#define XP_INTERRUPT_FESTAT_SLOCK   0x00000001   /* Sync Locked              */



/*----------------------------------------------------------------------------+

|  Queue Status Interrupt Masks

+----------------------------------------------------------------------------*/

#define XP_INTERRUPT_QSTAT_RPI      0x00008000   /* Read Pointer             */

#define XP_INTERRUPT_QSTAT_BTI      0x00004000   /* Boundary Threshold       */

#define XP_INTERRUPT_QSTAT_TSC      0x00002000   /* Table Section/Pckt Cmplt */

#define XP_INTERRUPT_QSTAT_CRCE     0x00001000   /* CRC32 Error              */

#define XP_INTERRUPT_QSTAT_TSLE     0x00000800   /* Table Section Length Err */

#define XP_INTERRUPT_QSTAT_SFUC     0x00000400   /* Section Filter Upd Cmplt */

#define XP_INTERRUPT_QSTAT_FP       0x00000200   /* First Packet             */

#define XP_INTERRUPT_QSTAT_AFLE     0x00000100   /* Adaptation Field Len Err */

#define XP_INTERRUPT_QSTAT_TSP      0x00000080   /* Transport Scrambling     */

#define XP_INTERRUPT_QSTAT_PSE      0x00000040   /* PID Stream Error         */

#define XP_INTERRUPT_QSTAT_PUSIP    0x00000020   /* Payload Unit Start Indic */

#define XP_INTERRUPT_QSTAT_AFP      0x00000010   /* Adapt Field Present      */

#define XP_INTERRUPT_QSTAT_AFPDP    0x00000008   /* Adapt Field Private Data */

#define XP_INTERRUPT_QSTAT_DIP      0x00000004   /* Discontinuity Indic      */

#define XP_INTERRUPT_QSTAT_SPP      0x00000002   /* Splicing Point           */

#define XP_INTERRUPT_QSTAT_RAIP     0x00000001   /* Random Access Indic      */



/*----------------------------------------------------------------------------+

|  Type Definitions

+----------------------------------------------------------------------------*/

typedef void (*PFS)(void *pGlobal,ULONG ulInterrupt);

typedef void (*XP_INTERRUPT_CHANNEL_FN)(void *pGlobal,SHORT wChannelId,ULONG ulInterrupt);



typedef enum xp_interrupt_control_type

{

    XP_INTERRUPT_CONTROL_ENABLE ,                /* enable ints for pid      */

    XP_INTERRUPT_CONTROL_DISABLE,                /* disable ints for pid     */

    XP_INTERRUPT_CONTROL_RESET                   /* reset ints for the pid   */

} XP_INTERRUPT_CONTROL_TYPE;



//External

void *xp_osi_task(QUEUE_T *pQueue);

void xp_osi_interrupt();

void xp1_interrupt(GLOBAL_RESOURCES *pXp0Global, ULONG ulXp0Int);

void xp2_interrupt(GLOBAL_RESOURCES *pXp0Global, ULONG ulXp0Int);

SHORT xp_osi_interrupt_init(GLOBAL_RESOURCES *pGlobal);

SHORT xp_osi_interrupt_channel_control(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,

                                       XP_INTERRUPT_CONTROL_TYPE cmd);

SHORT xp_osi_interrupt_channel_free(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId);



//APIS

SHORT xp_osi_interrupt_channel_notify(GLOBAL_RESOURCES *pGlobal,SHORT wCmd,SHORT wChannelId,

                                      ULONG ulIntMask,XP_INTERRUPT_CHANNEL_FN notify_fn);

SHORT xp_osi_interrupt_notify(GLOBAL_RESOURCES *pGlobal,SHORT wCmd,ULONG ulIntMask,

                              PFS notify_fn);

SHORT xp_osi_interrupt_status_notify(GLOBAL_RESOURCES *pGlobal,SHORT wCmd,ULONG ulIntMask,

                                     PFS notify_fn);
int xp_osi_interrupt_de_init(GLOBAL_RESOURCES *pGlobal);
#endif

