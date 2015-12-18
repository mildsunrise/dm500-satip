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
| File      :  xp_clk.h
| Purpose   :  xp_clk declarations and prototypes
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
|            IG   Created
| 30-Sep-01  LGH  Ported to Linux
+----------------------------------------------------------------------------*/

#ifndef XP_CLK_H

#define XP_CLK_H

typedef enum _XP_STC_EVENT
{
    STC_FIRED = 0,
    STC_DISCONTINUITY,
    STC_PRESENT,
    STC_REMOVED,
    STC_FATAL_ERROR
} XP_STC_EVENT;

typedef struct _STC_TYPE
{
    ULONG base_time1;
    ULONG base_time2;
} STC_TYPE;

typedef void (*XP_STC_NOTIFY_FN)(GLOBAL_RESOURCES *pGlobal,XP_STC_EVENT stc_event);

/*----------------------------------------------------------------------------+

| Type Definitions

+----------------------------------------------------------------------------*/

typedef struct xp_clk_status_type

{

    unsigned Incons_data;                   /* Inconsistent data was read    */

    unsigned Errors;                        /* Number of errors detected     */

} XP_CLK_STATUS_TYPE, *XP_CLK_STATUS_PTR;



//External

SHORT xp_osi_clk_get_stc_high(GLOBAL_RESOURCES *pGlobal,ULONG *pStcHigh);

SHORT xp_osi_clk_init(GLOBAL_RESOURCES *pGlobal);



//APIs

void xp_osi_clk_get_errors(GLOBAL_RESOURCES *pGlobal,XP_CLK_STATUS_PTR pClkStatus);

void xp_osi_clk_set_pid(GLOBAL_RESOURCES *pGlobal,USHORT uwPid);

SHORT xp_osi_clk_start(GLOBAL_RESOURCES *pGlobal);

void xp_osi_clk_stop(GLOBAL_RESOURCES *pGlobal);

int xp_osi_clk_set_stc_compare(GLOBAL_RESOURCES *pGlobal,ULONG stc_high);
int xp_osi_clk_set_stc_event_notify(GLOBAL_RESOURCES *pGlobal,XP_STC_NOTIFY_FN notify_fn);
int xp_osi_clk_release_stc_event(GLOBAL_RESOURCES *pGlobal);
void stc_interrupt(GLOBAL_RESOURCES *pGlobal,ULONG ulInterrupt);
void first_pcr_interrupt(GLOBAL_RESOURCES *pGlobal,ULONG ulInterrupt);

void xp_osi_clk_get_current_stc(GLOBAL_RESOURCES *pGlobal,STC_TYPE *stc_type);
#endif

