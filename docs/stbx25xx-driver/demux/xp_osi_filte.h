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
| File      :  xp_filter.h
| Purpose   :  xp_filter declarations and prototypes
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
|            IG   Created
| 30-Sep-01  LGH   Ported to Linux
+----------------------------------------------------------------------------*/

#ifndef XP_FILTER_H

#define XP_FILTER_H



/*----------------------------------------------------------------------------+

|  Defines

+----------------------------------------------------------------------------*/

#define XP_FILTER_ILLEGAL      -1           /* illegal filter                */

#define XP_FILTER_MAX_BLOCKS   64           /* maximum filter blocks in hw   */



/*----------------------------------------------------------------------------+

|  Type Declarations

+----------------------------------------------------------------------------*/

typedef enum filter_control_type

{

    XP_FILTER_CONTROL_ENABLE,

    XP_FILTER_CONTROL_DISABLE

} XP_FILTER_CONTROL_TYPE;



typedef enum xp_filter_short

{

    XP_FILTER_SHORT_DEFAULT,                /* process table using default   */

    XP_FILTER_SHORT_MISS,                   /* process table as filter miss  */

    XP_FILTER_SHORT_HIT                     /* process table as filter hit   */

} XP_FILTER_SHORT;



typedef enum xp_filter_status

{

    XP_FILTER_UNUSED,                       /* allocated but not defined     */

    XP_FILTER_DEFINED,                      /* allocated and defined         */

    XP_FILTER_ENABLED,                      /* filter block is being used    */

    XP_FILTER_DISABLED                      /* filter block not in use       */

} XP_FILTER_STATUS;



typedef enum xp_filter_pending

{

    PENDING_NONE,                           /* no pending operations         */

    PENDING_DELETE,                         /* pending DEL filter from chan  */

    PENDING_FREE                            /* pending DEL, then FREE filter */

} XP_FILTER_PENDING;



typedef struct xp_filter_error_type

{

    USHORT uwShortTable;              /* number of short tables        */

    USHORT uwFilterDisabled;          /* hw "hit" on a disabled filter */

    USHORT uwFilterFreed;             /* hw "hit" on freed filter      */

    USHORT uwFilterMatch;             /* filter "hit" was a miss       */

    USHORT uwFilterMatchWord;         /* matchword says invalid filter */

} XP_FILTER_ERROR_TYPE, *XP_FILTER_ERROR_PTR;



short xp_osi_filter_init(GLOBAL_RESOURCES *pGlobal);

void xp_osi_filter_process_pending(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId);

void xp_osi_filter_process_table_data(GLOBAL_RESOURCES *pGlobal, SHORT wChannelId,

                                      UCHAR *plBa, UCHAR *plEa, UCHAR *plBq,

                                      UCHAR *plEq, SHORT wSectionFilter,

                                      XP_CHANNEL_NOTIFY_FN notify_fn);

SHORT xp_osi_filter_valid(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId);



//APIs

SHORT xp_osi_filter_add_to_channel(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,SHORT wFilterId);

SHORT xp_osi_filter_allocate(GLOBAL_RESOURCES *pGlobal, ULONG ulLength);

SHORT xp_osi_filter_control(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId,XP_FILTER_CONTROL_TYPE cmd);

SHORT xp_osi_filter_delete_from_channel(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,SHORT wFilterId);

SHORT xp_osi_filter_free(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId);

SHORT xp_osi_filter_free_channel(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId);

SHORT xp_osi_filter_get_available(GLOBAL_RESOURCES *pGlobal,USHORT uwFilterLength);

SHORT xp_osi_filter_get_errors(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId,

                               XP_FILTER_ERROR_PTR pErrors);

SHORT xp_osi_filter_reset_errors(GLOBAL_RESOURCES *pGlobal,SHORT wChannelId);

SHORT xp_osi_filter_set(GLOBAL_RESOURCES *pGlobal,SHORT wFilterId,USHORT uwLength,

                        UCHAR *pData,UCHAR *pMask,UCHAR *pPolarity);

#endif

