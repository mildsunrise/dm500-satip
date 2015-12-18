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
| Author    :  Ian Govett
| Component :  xp
| File      :  xp_chann.h
| Purpose   :  xp_chann declarations and prototypes
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
|            IG   Created
| 30-Sep-01  LGH  Ported to Linux
+----------------------------------------------------------------------------*/

#ifndef XP_CHANNEL_H

#define XP_CHANNEL_H

/*----------------------------------------------------------------------------+

|  There are 32 channels available (0..31).  Audio, Video, Subtitle &

|  teletext are reserved to specific channels

|  NOTE:  if CHANNEL_COUNT changes, update "xp_interrupt.h"

+----------------------------------------------------------------------------*/

#define XP_CHANNEL_NULL_PID    0x1FFF       /* pid unused                    */

#define XP_CHANNEL_SUBTITLE         0       /* dedicated channel for subtit  */

#define XP_CHANNEL_TELETEXT         1       /* dedicated channel for teletxt */

#define XP_CHANNEL_PES_MIN          2       /* lower bound for "other" PES   */

#define XP_CHANNEL_PES_MAX         29       /* upper bound for "other" PES   */

#define XP_CHANNEL_AUDIO           30       /* dedicated audio channel       */

#define XP_CHANNEL_VIDEO           31       /* dedicated video channel       */

#define XP_CHANNEL_COUNT           32       /* number of available channels  */

#define XP_CHANNEL_CC_AUDIO       100       /* audio channel change register */

#define XP_CHANNEL_CC_VIDEO       101       /* audio channel change register */

#define XP_CHANNEL_ILLEGAL         -1       /* Illegal channel               */



/*----------------------------------------------------------------------------+

|  Define the channel control, status, and descramble

+----------------------------------------------------------------------------*/

typedef enum channel_control_type

{

    XP_CHANNEL_CONTROL_ENABLE,

    XP_CHANNEL_CONTROL_DISABLE,

    XP_CHANNEL_CONTROL_RESET

} XP_CHANNEL_CONTROL_TYPE;



typedef enum channel_status

{

    XP_CHANNEL_UNUSED  =0,                  /* record is available for alloc */

    XP_CHANNEL_ENABLED =1,                  /* channel currently active      */

    XP_CHANNEL_DISABLED=2                   /* channel defined but disabled  */

} XP_CHANNEL_STATUS;



typedef enum channel_descramble

{

    XP_DESCRAMBLE_TS   =0,                  /* descramble transport packets  */

    XP_DESCRAMBLE_PES  =1,                  /* descramble PES portion of pckt*/

    XP_DESCRAMBLE_OFF  =2                   /* no descrambling required      */

} XP_CHANNEL_DESCRAMBLE;



/*----------------------------------------------------------------------------+

|  Define the channel unload types available

+----------------------------------------------------------------------------*/

typedef enum xp_channel_unload_type

{

    XP_CHANNEL_UNLOAD_TRANSPORT            = 0x0,

    XP_CHANNEL_UNLOAD_ADAPTATION           = 0x1,

    XP_CHANNEL_UNLOAD_ADAPTATION_PRIVATE   = 0x2,

    XP_CHANNEL_UNLOAD_PAYLOAD              = 0x3,

    XP_CHANNEL_UNLOAD_PAYLOAD_AND_BUCKET   = 0x4,

    XP_CHANNEL_UNLOAD_BUCKET               = 0x5,

    XP_CHANNEL_UNLOAD_PSI                  = 0x8,

    XP_CHANNEL_UNLOAD_FILTER_PSI           = 0x9,

    XP_CHANNEL_UNLOAD_PSI_CRC              = 0xa,

    XP_CHANNEL_UNLOAD_FILTER_PSI_CRC       = 0xb,

    XP_CHANNEL_UNLOAD_PSI_BUCKET           = 0xc,

    XP_CHANNEL_UNLOAD_FILTER_PSI_BUCKET    = 0xd,

    XP_CHANNEL_UNLOAD_PSI_CRC_BUCKET       = 0xe,

    XP_CHANNEL_UNLOAD_FILTER_PSI_CRC_BUCKET= 0xf,

    XP_CHANNEL_UNLOAD_UNDEFINED            = 0x10

} XP_CHANNEL_UNLOAD_TYPE;



/*----------------------------------------------------------------------------+

|  Define the type of channels available in the transport

+----------------------------------------------------------------------------*/

typedef enum xp_channel_type

{

    XP_CHANNEL_TYPE_VIDEO   =0,

    XP_CHANNEL_TYPE_AUDIO   =1,

    XP_CHANNEL_TYPE_SUBTITLE=2,

    XP_CHANNEL_TYPE_TELETEXT=3,

    XP_CHANNEL_TYPE_BUCKET  =4,

    XP_CHANNEL_TYPE_PES     =5              /* table data, etc               */

} XP_CHANNEL_TYPE;



typedef struct xp_channel_notify_data

{

    void *pGlobal;

    SHORT wChannelId;                       /* channel which has data avail  */

    ULONG ulMatchWord;               /* bit mask of filters "hit"     */

    UCHAR *plData;                    /* pointer to starting address   */

    ULONG ulLength;                   /* number of bytes available     */

} XP_CHANNEL_NOTIFY_DATA;



typedef void (*XP_CHANNEL_NOTIFY_FN)(XP_CHANNEL_NOTIFY_DATA *);



SHORT xp_osi_channel_get_status(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                                XP_CHANNEL_STATUS *pStatus);

SHORT xp_osi_channel_restart(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId);

SHORT xp_osi_channel_valid(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId);

//APIs

SHORT xp_osi_channel_init(GLOBAL_RESOURCES *pGlobal);

SHORT xp_osi_channel_allocate(GLOBAL_RESOURCES *pGlobal, XP_CHANNEL_TYPE type);

SHORT xp_osi_channel_control(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                             XP_CHANNEL_CONTROL_TYPE Cmd);

SHORT xp_osi_channel_free(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId);

SHORT xp_osi_channel_get_available(GLOBAL_RESOURCES *pGlobal, XP_CHANNEL_TYPE Type);

SHORT xp_osi_channel_get_key(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                             XP_CHANNEL_DESCRAMBLE *pDescram, USHORT *pKeyId);

SHORT xp_osi_channel_get_pid(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, USHORT *pPid);

SHORT xp_osi_channel_get_unload_type(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                                     XP_CHANNEL_UNLOAD_TYPE *pUnloadType);

SHORT xp_channel_set_key(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                         XP_CHANNEL_DESCRAMBLE Descram, USHORT uwKeyId);

SHORT xp_osi_channel_set_notification_fn(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                                         XP_CHANNEL_NOTIFY_FN notify_fn);

SHORT xp_osi_channel_set_pid(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId, USHORT uwPid);

SHORT xp_osi_channel_set_unload_type(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,

                                     XP_CHANNEL_UNLOAD_TYPE UnloadType);

#endif
