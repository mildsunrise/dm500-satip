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
|   Author    :  Ian Govett
|   Component :  xp
|   File      :  pvr_atom_dcr.h
|   Purpose   :  definition of PVR DCR access
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  --------------------------------------------------------
|   30-Sep-01  LGH  create
|
+----------------------------------------------------------------------------*/

#ifndef PVR_DCR
#define PVR_DCR

#include <os/os-io.h>

//DCR addres of the reg of PVR
#define PVR_CONFIG_DCR      0x02c0
#define PVR_START_DCR       0x02c1
#define PVR_ADDR_DCR        0x02c4
#define PVR_COUNT_DCR       0x02c7
#define PVR_INT_DCR         0x02ca
#define PVR_STAT_DCR        0x02cb
#define PVR_INT_MASK_DCR    0x02ce

#endif
