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
|       COPYRIGHT   I B M   CORPORATION 1999
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Tony J. Cerreto
| Component: XP
| File:      xp_atom_pcr.h
| Purpose:   Common definitions for xp_atom_pcr.c functions.
|
| Changes:
| Date:      Author  Comment:
| ---------  ------  --------
| 19-Dec-99  TJC     Created.
| 10-Oct-01  LGH     Ported to Linux
+----------------------------------------------------------------------------*/

#ifndef XP_DCR
#define XP_DCR
#include <hw/hardware.h>
#include <os/os-io.h>


#define    XPT0_LR    0x0180
#define    XPT0_DT    0x0181
#define    XPT0_IR    0x0182

#ifdef     __DRV_FOR_PALLAS__

#define    XPT1_LR    0x0190
#define    XPT1_DT    0x0191
#define    XPT1_IR    0x0192

#define    XPT2_LR    0x02d0
#define    XPT2_DT    0x02d1
#define    XPT2_IR    0x02d2

#endif

/*----------------------------------------------------------------------------+
|  DCR Addresses for Location, Data Transfer, and Interrupt Registers
+----------------------------------------------------------------------------*/

#define XP_DCR_ADDR_LR               0x0180      /* Location Register        */
#define XP_DCR_ADDR_DT               0x0181      /* Data Register            */
#define XP_DCR_ADDR_IR               0x0182      /* Interrupt Register       */

/*----------------------------------------------------------------------------+
|  General Configuration Registers
+----------------------------------------------------------------------------*/
#define XP_DCR_ADDR_CONFIG1          0x0000      /* Configuration 1 Register */
#define XP_DCR_ADDR_CONTROL1         0x0001      /* Control 1 Register       */
#define XP_DCR_ADDR_FESTAT           0x0002      /* Front-End Status Reg     */
#define XP_DCR_ADDR_FEIMASK          0x0003      /* Front-End Int Mask Reg   */

#ifndef __DRV_FOR_VESTA__       // for vulcan and pallas ...
#define XP_DCR_ADDR_CONFIG3          0x0006      /* Configuration 3 Register */
#endif


/*----------------------------------------------------------------------------+
|  Clock Recovery Registers
+----------------------------------------------------------------------------*/
#define XP_DCR_ADDR_PCRHI            0x0010      /* PCR High Register        */
#define XP_DCR_ADDR_PCRLOW           0x0011      /* PCR Low Register         */
#define XP_DCR_ADDR_LSTCHI           0x0012      /* Latched STC High Reg     */
#define XP_DCR_ADDR_LSTCLOW          0x0013      /* Latched STC Low Reg      */
#define XP_DCR_ADDR_STCHI            0x0014      /* Running STC High Reg     */
#define XP_DCR_ADDR_STCLOW           0x0015      /* Running STC Low Reg      */
#define XP_DCR_ADDR_PWM              0x0016      /* PWM Register             */
#define XP_DCR_ADDR_PCRSTCT          0x0017      /* PCR-STC Threshold Reg    */
#define XP_DCR_ADDR_PCRSTCD          0x0018      /* PCR-STC Delta Reg        */
#define XP_DCR_ADDR_STCCOMP          0x0019      /* STC Compare Reg          */
#define XP_DCR_ADDR_STCCMPD          0x001A      /* STC Compare Disarm Reg   */



/*----------------------------------------------------------------------------+

|  Descrambler Status Registers

+----------------------------------------------------------------------------*/

#define XP_DCR_ADDR_DSSTAT           0x0048      /* Descrambler Status Reg   */

#define XP_DCR_ADDR_DSIMASK          0x0049      /* Descrambler Int Mask Reg */



/*----------------------------------------------------------------------------+

|  Additional PID Registers

+----------------------------------------------------------------------------*/

#define XP_DCR_ADDR_VCCHNG           0x01F0      /* Video Channel Change Reg */

#define XP_DCR_ADDR_ACCHNG           0x01F1      /* Audio Channel Change Reg */

#ifndef __DRV_FOR_VULCAN__
#define XP_DCR_ADDR_AXENABLE         0x01FE      /* Aux PID Enables Register */
#endif

#define XP_DCR_ADDR_PCRPID           0x01FF      /* PCR PID Register         */



/*----------------------------------------------------------------------------+

|  Back-End Configuration and Control Registers

+----------------------------------------------------------------------------*/

#define XP_DCR_ADDR_CONFIG2          0x1000      /* Configuration 2 Register */

#define XP_DCR_ADDR_PBUFLVL          0x1002      /* Packet Buffer Level Reg  */

#define XP_DCR_ADDR_INTMASK          0x1003      /* Transport Int Mask Reg   */

#define XP_DCR_ADDR_PLBCNFG          0x1004      /* PLB Configuration Reg    */

#define XP_DCR_ADDR_QSTATMASK        0x1005      /* Queue Status Mask Reg    */



/*----------------------------------------------------------------------------+

|  Back-End Status and Interrupt Mask Registers

+----------------------------------------------------------------------------*/

#define XP_DCR_ADDR_QINT             0x1010      /* Queues Interrupt Reg     */

#define XP_DCR_ADDR_QINTMSK          0x1011      /* Queues Interrupt Mask Reg*/

#define XP_DCR_ADDR_ASTATUS          0x1012      /* Audio Status Reg         */

#define XP_DCR_ADDR_AINTMSK          0x1013      /* Audio Interrupt Mask Reg */

#define XP_DCR_ADDR_VSTATUS          0x1014      /* Video Status Reg         */

#define XP_DCR_ADDR_VINTMSK          0x1015      /* video Interrupt Mask Reg */



/*----------------------------------------------------------------------------+

|  Queues Configuration and Control Registers

+----------------------------------------------------------------------------*/

#define XP_DCR_ADDR_QBASE            0x1020      /* Queue Base Register      */

#define XP_DCR_ADDR_BUCKET1Q         0x1021      /* Bucket 1 Queue Register  */

#define XP_DCR_ADDR_QADDRST          0x1023      /* Queue Address Stack Reg  */

#define XP_DCR_ADDR_QSTOPS           0x1024      /* Queue Stops Register     */

#define XP_DCR_ADDR_QRESETS          0x1025      /* Queue Resets Register    */

#define XP_DCR_ADDR_SFCHNG           0x1026      /* Section Filter Change Reg*/

#define XP_DCR_ADDR_STR0CFG          0x1028      /* Search String 0 Config   */

#define XP_DCR_ADDR_STR0             0x1029      /* Search String 0 Register */

#define XP_DCR_ADDR_STR0MSK          0x102A      /* Search String 0 Mask     */

#define XP_DCR_ADDR_STR1CFG          0x102B      /* Search String 1 Config   */

#define XP_DCR_ADDR_STR1             0x102C      /* Search String 1 Register */

#define XP_DCR_ADDR_STR1MSK          0x102D      /* Search String 1 Mask     */



/*----------------------------------------------------------------------------+

|  Queue, Descrambler, and Filter Base Addresses for Location Map

+----------------------------------------------------------------------------*/

#define XP_DCR_ADDR_BASE_PID         0x0100      /* DCR address base PID     */

#define XP_DCR_ADDR_BASE_QCONFIGAB   0x2200      /* Base Queue Config AB Reg */

#define XP_DCR_ADDR_BASE_FILTER      0x2300      /* Base Filter Registers    */

#define XP_DCR_ADDR_BASE_DEKEY       0x2500      /* Base Descrambler Key Reg */

#define XP_DCR_ADDR_BASE_QSTAT       0x2600      /* Base Queue Status Reg    */

#define XP_DCR_ADDR_BASE_QSTATABCD   0x2800      /* Base Queue Stat ABCD Reg */



/*----------------------------------------------------------------------------+

|   Type Declarations

+----------------------------------------------------------------------------*/
typedef struct stc                               /* Copy from decod.h        */
{
        ULONG bits_32_1;
        ULONG bit_0;
} stc_t;

typedef enum xp_dcr_register_type {

    XP_PID_FILTER_REG_PIDV           ,           /* PIDV   in PID_FILTER_REG */

    XP_QCONFIGB_REG_DTYPE            ,           /* DTYPE  in QCONFIGB_REG   */

    XP_QCONFIGB_REG_ENBL             ,           /* ENBL   in QCONFIGB_REG   */

    XP_QCONFIGA_REG_BTHRES           ,           /* BTHRES in QCONFIGA_REG   */

    XP_FILTER_CONTROL_REG_ENBL       ,           /* ENBL   in FILTER_CONTROL */

    XP_FILTER_CONTROL_REG_NFILT      ,           /* NFILT  in FILTER_CONTROL */

    XP_QCONFIGB_REG_FSF              ,           /* FSF    in QCONFIGB_REG   */

    XP_QCONFIGB_REG_RPTR             ,           /* RPTR   in QCONFIGB_REG   */

    XP_FILTER_CONTROL_REG_SFID       ,           /* SFID   in FILTER_CONTROL */

    XP_BUCKET1Q_REG_BVALID           ,           /* BVALID in BUCKET1Q_REG   */

    XP_BUCKET1Q_REG_INDX             ,           /* INDX   in BUCKET1Q_REG   */

    XP_QSTATD_REG_WSTART             ,           /* WSTART in QSTATD_REG     */

    XP_QSTATB_REG_WPTR               ,           /* WPTR   in QSTATB_REG     */

    XP_CONFIG1_REG_APWMA                         /* APWMA  in XP_CONFIG1_REG */

} XP_DCR_REGISTER_TYPE;



ULONG xp_atom_dcr_read(UINT uDeviceIndex,ULONG ulAddress);

void xp_atom_dcr_write(UINT uDeviceIndex,ULONG ulAddress,ULONG ulData);

void xp_atom_dcr_init_queue(UINT uDeviceIndex,SHORT wChannelId,ULONG ulQueueTop,ULONG ulQueueBottom);

ULONG xp_atom_dcr_read_interrupt(UINT uDeviceIndex);

void xp_atom_dcr_read_queue_config(UINT uDeviceIndex,SHORT wChannelId,ULONG *ppQueueTop,ULONG *ppQueueBottom);

void xp_atom_dcr_reset_queue(UINT uDeviceIndex,SHORT wChannelId);

void xp_atom_dcr_write_dram_filter(UINT uDeviceIndex,SHORT wFilterId,

                                   ULONG ulData,ULONG ulMask,ULONG ulControl,ULONG ulPolarity);

SHORT xp_atom_dcr_write_filter_link(UINT uDeviceIndex,SHORT wFilterId,

                                    ULONG ulNextFilterId,ULONG ulEndOfColumn);

SHORT xp_atom_dcr_write_register(UINT uDeviceIndex,XP_DCR_REGISTER_TYPE regtype,ULONG ulData);



//APIS

ULONG xp_atom_dcr_read(UINT uDeviceIndex,ULONG ulAddress);

SHORT xp_atom_dcr_read_register_channel(UINT uDeviceIndex,XP_DCR_REGISTER_TYPE regtype,

                                        SHORT wId,ULONG *pData);

void xp_atom_dcr_write(UINT uDeviceIndex,ULONG ulAddress,ULONG ulData);

SHORT xp_atom_dcr_write_register_channel(UINT uDeviceIndex,XP_DCR_REGISTER_TYPE regtype,

                                         SHORT wId,ULONG ulData);

// AV related
void xp_atom_disable_aud_sync();

void xp_atom_disable_vid_sync();

int xp_atom_a_hw_cc_inprogress();

void xp_atom_a_hw_sync_off();

void xp_atom_a_hw_sync_on();

void xp_atom_v_hw_sync_off();

void xp_atom_v_hw_sync_on();

int xp_atom_a_hw_write_stc(stc_t *data);

int xp_atom_v_hw_write_stc(stc_t *data);


#endif
