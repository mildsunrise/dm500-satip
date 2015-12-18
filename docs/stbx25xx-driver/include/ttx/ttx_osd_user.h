/*----------------------------------------------------------------------------+
|
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
|       COPYRIGHT 2003   I B M   CORPORATION
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|
|  Author    :  Tony J. Cerreto
|  File      :  ttx_osd_user.h
|  Purpose   :  Linux User defines
|  Changes   :
|  Date         Comments
|  ----------------------------------------------------------------------
|  25-Sep-03    Created
+----------------------------------------------------------------------------*/
#ifndef TTX_OSD_USER_H
#define TTX_OSD_USER_H

/*----------------------------------------------------------------------------+
| Defines
+----------------------------------------------------------------------------*/
#pragma pack(1)
#define TTX_IOC_MAGIC                   't'
#define TTX_START                       _IO(TTX_IOC_MAGIC,1)
#define TTX_STOP                        _IO(TTX_IOC_MAGIC,2)
#define TTX_STATS_GET                   _IO(TTX_IOC_MAGIC,3)
#define TTX_STATS_CLEAR                 _IO(TTX_IOC_MAGIC,4)
#pragma pack()

/*----------------------------------------------------------------------------+
| Type Declarations
+----------------------------------------------------------------------------*/
typedef struct {
    unsigned int in_pkt;                         /* incoming packets         */
    unsigned int in_pusi;                        /* incoming PUSI            */
    unsigned int out_pusi;                       /* outgoing PUSI            */
    unsigned int no_pusi;                        /* no PUSI found in packet  */
    unsigned int trash_pkt;                      /* packet going into trash  */
    unsigned int inbound_pes;                    /* PES processed by PTS cmp */
    unsigned int outbound_pes;                   /* PES discarded by PTS cmp */
} TTX_STATISTICS;


#endif
