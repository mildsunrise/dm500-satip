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
| Author:    Lin Guo Hui
| Component: PVR
| File:      pvr_atom_reg.h
| Purpose:   PVR register defines.
|
| Changes:
| Date:      Author  Comment:
| ---------  ------  --------
| 10-Oct-01  LGH     Creat
+----------------------------------------------------------------------------*/

#ifndef PCR_REG_H
#define PVR_REG_H

#pragma pack(1)
/*----------------------------------------------------------------------------+
|  Register Defines
+----------------------------------------------------------------------------*/
#define PVR_PLAYBACK_MODE_SEL   1
#define PVR_PLAYBACK_LINE_MODE  2
#define PVR_PLAYBACK_START_DMA  1
#define PVR_STAT_BUF_NOT_EMPTY  2
#define PVR_STAT_BUF_FULL       1

#define PVR_INT_EN      0x80
#define PVR_INT_MASK        0x01
/*----------------------------------------------------------------------------+
|  CONFIGURATION REGISTER
|  Location Address: 0x02c0
+----------------------------------------------------------------------------*/
typedef struct PVR_config_type
{
    unsigned res        :31;
    unsigned PVRCONFI   :1;     //PVR playback mode select, 1: select 0: not select
} PVR_CONFIG_REG, *PVR_CONFIG_REGP;

/*----------------------------------------------------------------------------+
|  PVR START REGISTER
|  Location Address: 0x02c1
+----------------------------------------------------------------------------*/
typedef struct PVR_start_type
{

    unsigned res        :30;
    unsigned line       :1;     //0: PVR playback word mode(4-byte transfer)
                                //1: PVR playback line mode(32-byte transfer)

    unsigned start      :1;     //PVR start DMA, 0: no transfer 1:start transfer

} PVR_START_REG, *PVR_START_REGP;

/*------------------------------------------------------------------------------+
| PVR play back transfer count
| Location Address: 0x2c7
+-------------------------------------------------------------------------------*/
typedef struct PVR_playback_count
{
    unsigned res        :16;
    unsigned count      :16;
} PVR_COUNT_REG, *PVR_COUNT_REGP;

#pragma pack()
#endif


