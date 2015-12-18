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
| Author:    Maciej P. Tyrlik
| Component: xp
| File:      xp_pcr.c
| Purpose:   Manage PCR.
| Changes:
|
| Date:      Author   Comment:
| ---------  ------   --------
| 16-Dec-99  tjc      Moved from XP component.
| 01-Sep-01  LGH      Ported to Linux
+----------------------------------------------------------------------------*/
/* The necessary header files */


//delete header file related to linux ,lingh
#include <linux/config.h>

#include <linux/version.h>



#ifdef MODVERSIONS

#include <linux/modversions.h>

#endif



#define  __NO_VERSION__

#include <linux/module.h>

#include <linux/kernel.h>

#include <linux/types.h>



#include "xp_osi_global.h"

#include "xp_atom_reg.h"

/*----------------------------------------------------------------------------+
| Local Defines
+----------------------------------------------------------------------------*/
#define XP_PCR_ONE_SHOT           0
#define XP_PCR_CONTINUOUS         1

GLOBAL_RESOURCES *pGlobal;

/*----------------------------------------------------------------------------+
| Static Variables
+----------------------------------------------------------------------------*/
static int        xp_pcr_callback_registered = 0;
static int        xp_pcr_flags               = 0;
static int        xp_pcr_num                 = 0;
static unsigned   xp_pcr_tshift          = 0;   //changed in Jan.10/2002

/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| XXXX   XX   XX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX
|  XX    XXX  XX   X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX
|  XX    XXXX XX     XX     XX X     XX  XX  XXXX XX   XX  XX   XX
|  XX    XX XXXX     XX     XXXX     XXXXX   XX XXXX   XX  XX   XX
|  XX    XX  XXX     XX     XX X     XX XX   XX  XXX   XXXXXX   XX
|  XX    XX   XX     XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX
| XXXX   XX   XX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|  pcr_av_sync
+----------------------------------------------------------------------------*/
static void pcr_av_sync(stc_t *stc)
{

   ULONG flag;

   xp_pcr_num++;

   flag = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   |  Write Audio STC and Enable Audio Synchronization
   +-------------------------------------------------------------------------*/
   xp_atom_a_hw_sync_off();
   if (xp_atom_a_hw_write_stc(stc)==0)  //lingh changed
   {
       xp_atom_a_hw_sync_on();
   }
   else
   {
       xp_atom_a_hw_sync_off();
    }

   /*-------------------------------------------------------------------------+
   |  Write Video STC and Enable Video Synchronization
   +-------------------------------------------------------------------------*/
   (void)xp_atom_v_hw_write_stc(stc);
   xp_atom_v_hw_sync_on();

   os_leave_critical_section(flag);

   return;
}

/*----------------------------------------------------------------------------+
|  pcr_stc_inthdl
+----------------------------------------------------------------------------*/
static void pcr_stc_inthdl(
       GLOBAL_RESOURCES *pGlobal0,
       unsigned short xpid,
       unsigned long interrupt)

{
   stc_t         stc;
   short         rc;
   ULONG flag;


   flag = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   |  Get STC, Add time for Decoder Delay, and Invoke Call Back Function
   +-------------------------------------------------------------------------*/
   rc = xp_osi_clk_get_stc_high(pGlobal0,(unsigned long *) &stc.bits_32_1);

   if (rc == 0)
   {
      stc.bit_0 = 0;
      stc.bits_32_1 += xp_pcr_tshift;
      pcr_av_sync(&stc);
   }

   os_leave_critical_section(flag);
}

/*----------------------------------------------------------------------------+
|  pcr_cont
+----------------------------------------------------------------------------*/
static void pcr_cont(void)

{
   unsigned long crit = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   |  Set-up Call Back for STC_LOAD and PCR Interrupts
   +-------------------------------------------------------------------------*/
   if (xp_pcr_callback_registered && !(xp_pcr_flags & XP_PCR_CONTINUOUS))
   {
      xp_osi_interrupt_notify(pGlobal,XP_INTERRUPT_NOTIFY_DELETE,
                          XP_INTERRUPT_IR_STCL, (PFS)pcr_stc_inthdl);

      xp_osi_interrupt_notify(pGlobal,XP_INTERRUPT_NOTIFY_ADD,
         XP_INTERRUPT_IR_STCL | XP_INTERRUPT_IR_PCR, (PFS)pcr_stc_inthdl);
      xp_pcr_flags |= XP_PCR_CONTINUOUS;
   }

   os_leave_critical_section(crit);
}

/*----------------------------------------------------------------------------+
|  pcr_once
+----------------------------------------------------------------------------*/
static void pcr_once(void)

{
   unsigned long crit = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   |  Set-up Call Back for STC_LOAD Interrupts Only
   +-------------------------------------------------------------------------*/
   if (xp_pcr_callback_registered && (xp_pcr_flags & XP_PCR_CONTINUOUS))
   {
      xp_osi_interrupt_notify(pGlobal,XP_INTERRUPT_NOTIFY_DELETE,
         XP_INTERRUPT_IR_STCL | XP_INTERRUPT_IR_PCR, (PFS)pcr_stc_inthdl);

      xp_osi_interrupt_notify(pGlobal,XP_INTERRUPT_NOTIFY_ADD,
         XP_INTERRUPT_IR_STCL, (PFS)pcr_stc_inthdl);
      xp_pcr_flags &= ~XP_PCR_CONTINUOUS;
   }

   os_leave_critical_section(crit);
}

/*----------------------------------------------------------------------------+
| XXXXXXX  XXX XXX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX
|  XX   X   XX XX    X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX
|  XX X      XXX       XX     XX X     XX  XX  XXXX XX   XX  XX   XX
|  XXXX       X        XX     XXXX     XXXXX   XX XXXX   XX  XX   XX
|  XX X      XXX       XX     XX X     XX XX   XX  XXX   XXXXXX   XX
|  XX   X   XX XX      XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX
| XXXXXXX  XXX XXX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
|   XX     XXXXXX    XXXXXX    XXXXX
|  XXXX    XX   XX     XX     XX   XX
| XX  XX   XX   XX     XX      XX
| XX  XX   XXXXX       XX        XX
| XXXXXX   XX          XX         XX
| XX  XX   XX          XX     XX   XX
| XX  XX   XX        XXXXXX    XXXXX
+----------------------------------------------------------------------------*/
//xp_pcr_init
int xp_osi_pcr_init(GLOBAL_RESOURCES *pGlobal0)
{
    pGlobal = pGlobal0;
    if(pGlobal == NULL || pGlobal->uDeviceIndex)
        return -1;
    return 0;
}

/*----------------------------------------------------------------------------+
|  xp0_pcr_delay
+----------------------------------------------------------------------------*/
void xp_osi_pcr_delay(unsigned long delay)

{
   xp_pcr_tshift = (unsigned) delay;
}

/*----------------------------------------------------------------------------+
|  xp0_pcr_sync
+----------------------------------------------------------------------------*/
void xp_osi_pcr_sync(void)

{
   stc_t         stc;
   short         rc;
   unsigned long crit;


   crit = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   |  Get STC, Add time for Decoder Delay, and Synchronize Audio/Video
   +-------------------------------------------------------------------------*/
   rc = xp_osi_clk_get_stc_high(pGlobal,(unsigned long *) &stc.bits_32_1);

   if (rc == 0)
   {
      stc.bit_0 = 0;
      stc.bits_32_1 += xp_pcr_tshift;
      pcr_av_sync(&stc);
   }

   os_leave_critical_section(crit);
}

/*----------------------------------------------------------------------------+
|  xp0_pcr_sync_start
+----------------------------------------------------------------------------*/
void xp_osi_pcr_sync_start(void)

{
   unsigned long crit;
   unsigned long interrupts;


   crit = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   |  Use PCR to Immediately Sync Audio and Video
   +-------------------------------------------------------------------------*/
   xp_osi_pcr_sync();

   /*-------------------------------------------------------------------------+
   |  Install the PCR/STC Interrupt Handler
   +-------------------------------------------------------------------------*/
   if (!xp_pcr_callback_registered)
   {
       interrupts = XP_INTERRUPT_IR_STCL;
       if (xp_pcr_flags & XP_PCR_CONTINUOUS)
       {
           interrupts |= XP_INTERRUPT_IR_PCR;
       }

       xp_osi_interrupt_notify(pGlobal,XP_INTERRUPT_NOTIFY_ADD,
                           interrupts, (PFS)pcr_stc_inthdl);
       xp_pcr_callback_registered = 1;
   }

   os_leave_critical_section(crit);
   return;

}

/*----------------------------------------------------------------------------+
|  xp0_pcr_sync_stop
+----------------------------------------------------------------------------*/
void xp_osi_pcr_sync_stop(void)

{
   unsigned long crit;
   unsigned long interrupts;


   crit = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   |  Remove the Previous Registered Call Back Function and Uninstall
   |  the PCR/STC Interrupt Handler
   +-------------------------------------------------------------------------*/
   if (xp_pcr_callback_registered)
   {
       interrupts = XP_INTERRUPT_IR_STCL;
       if (xp_pcr_flags & XP_PCR_CONTINUOUS)
       {
          interrupts |= XP_INTERRUPT_IR_PCR;
       }

       xp_osi_interrupt_notify(pGlobal,XP_INTERRUPT_NOTIFY_DELETE,
                           interrupts, (PFS)pcr_stc_inthdl);
       xp_pcr_callback_registered = 0;
   }

   os_leave_critical_section(crit);
   return;
}
