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
| Author:    Ian Govett
| Component: xp
| File:      xp_osi_queue.h
| Purpose:   Transport queue defines and prototypes.
| Changes:
|
| Date:      Author  Comment:
| ---------  ------  --------
| 04-May-01  TJC     Updates for Pallas.
| 30-Sep-01  LGH     Ported to Linux
+----------------------------------------------------------------------------*/

#ifndef XP_QUEUE_H

#define XP_QUEUE_H



/*----------------------------------------------------------------------------+

| Type Declarations

+----------------------------------------------------------------------------*/

typedef enum xp_queue_control_type

{

    XP_QUEUE_CONTROL_ENABLE,

    XP_QUEUE_CONTROL_DISABLE,

    XP_QUEUE_CONTROL_RESET

} XP_QUEUE_CONTROL_TYPE;



typedef enum xp_queue_mode_type

{

    XP_QUEUE_MODE_SKIP,                 /* data section should be skipped   */

    XP_QUEUE_MODE_LOCK                  /* data section is locked           */

} XP_QUEUE_MODE_TYPE;



typedef enum xp_queue_status

{

    XP_QUEUE_STATUS_UNDEFINED,          /* queue is not defined             */

    XP_QUEUE_STATUS_ENABLED  ,          /* queue is enabled for use by hw   */

    XP_QUEUE_STATUS_RPI      ,          /* an RPI error occurred            */

    XP_QUEUE_STATUS_DISABLED            /* queue is disabled                */

} XP_QUEUE_STATUS;



typedef struct xp_queue_error_type

{

    USHORT uwNoData;              /* interrupt but no data to process */

    USHORT uwQueueDisabled;      /* interrupts after queue disabled  */

    USHORT uwQueueAddress;       /* invalid regions to unlock        */

    USHORT uwProcessRpi;         /* error processing RPI             */

    USHORT uwRegionNotFound;    /* region not found to unlock       */

    USHORT uwInternal;            /* internal errors                  */

    USHORT uwReadPointer;        /* read pointer occurred, lost data */

    USHORT uwCrc32;               /* crc errors occurred, lost data   */

    USHORT uwSectionLength;      /* table section length errors      */

    USHORT uwCc;                  /* continuity counter errors        */

    USHORT uwOverlap;             /* lock regions overlap             */

} XP_QUEUE_ERROR_TYPE, *XP_QUEUE_ERROR_PTR;



//External

SHORT xp_osi_queue_init(GLOBAL_RESOURCES *pGlobal);

SHORT xp_osi_queue_get_status(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                              XP_QUEUE_STATUS *pStatus);

void xp_osi_queue_process_data(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                               XP_CHANNEL_UNLOAD_TYPE UnloadType, XP_CHANNEL_NOTIFY_FN notify_fn);

short xp_osi_queue_process_interrupt(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                                     ULONG ulInterrupt);



//APIs

SHORT xp_osi_queue_reset_errors(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId);

SHORT xp_osi_queue_allocate(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, ULONG ulQueueSize);

SHORT xp_osi_queue_control(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                           XP_QUEUE_CONTROL_TYPE Cmd);

SHORT xp_osi_queue_get_config(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                              UCHAR **pBQueue, UCHAR **pEQueue);

SHORT xp_osi_queue_free(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId);

SHORT xp_osi_queue_get_errors(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                              XP_QUEUE_ERROR_PTR pErrors);

SHORT xp_osi_queue_get_size(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId, ULONG *pQueueSize);

SHORT xp_osi_queue_lock_data(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                             XP_QUEUE_MODE_TYPE Mode, UCHAR *ppBAddr, UCHAR *ppEAddr);

SHORT xp_osi_queue_unlock(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                          SHORT wBIndex, SHORT wEIndex);

SHORT xp_osi_queue_unlock_data(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                               UCHAR *ppBAddr, ULONG ulLength);

void xp_osi_queue_de_init(GLOBAL_RESOURCES *pGlobal);
#endif

