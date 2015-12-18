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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author: Ling shao
| File:   vid_int.c
| Purpose: video decoder atom layer interrupt function PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01     create                                                                                          SL
| 21-Oct-02     exported contents of "Video Interrupt Register" so other
|               drivers can install interrupt handlers that service it.         BJC
+----------------------------------------------------------------------------*/
#include <os/helper-queue.h>
#include <os/os-io.h>
#include <os/drv_debug.h>
#include <os/os-interrupt.h>
#include <linux/sched.h>
#include <linux/module.h>
#include "vid_atom.h"
#include "vid_atom_local.h"
#include "vid_osi.h"
#include "vid_osd.h"

/*----------------------------------------------------------------------------+
| Static Declarations
+----------------------------------------------------------------------------*/
static int             __ACC_PENDING = 0;
#if 1
static unsigned int    vid_atom_notify_ndx = 0;
static VID_NOTIFY      vid_atom_notify[VID_NOTIFY_MAX];
#endif

/*----------------------------------------------------------------------------+
| External Declarations
+----------------------------------------------------------------------------*/
extern void                   aud_atom_set_stc(STC_T *pData);
extern int                    vid_cc_done();
extern volatile int           __PS_SF_COMPLETE;
extern volatile int           __FF_COMPLETE;
extern volatile int           __FF_PENDING;
extern volatile int           __CC_PENDING;
extern volatile int           __CC_COMPLETE;
extern volatile int           __VID_PLAY_PENDING;
extern volatile VIDPLAYSTATUS vid_play_stat;
extern int                    video_open_count;

/*----------------------------------------------------------------------------+
| Global Declarations
+----------------------------------------------------------------------------*/
/*
 * Declare and export a variable to save the video decoder interrupt
 * status register. -- BJC 102102
 */
unsigned int                  stb_vid_int_status;

/*----------------------------------------------------------------------------+
| Exported Variables
+----------------------------------------------------------------------------*/
EXPORT_SYMBOL_NOVERS(stb_vid_int_status);
#if 0
EXPORT_SYMBOL_NOVERS(vid_atom_ttx_add_notify);
EXPORT_SYMBOL_NOVERS(vid_atom_ttx_del_notify);
#endif

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
| vid_atom_process_int
+----------------------------------------------------------------------------*/
void vid_atom_process_int(UINT uIrq, unsigned long reg)
{

   /*------------------------------------------------------------------------+
   | Auto channel change done
   +------------------------------------------------------------------------*/
   if ((reg & DECOD_HOST_INT_CHAN_CHAN) != 0)
   {
       vid_cc_done();
       __ACC_PENDING = 1;
       vid_atom_set_irq_mask(vid_atom_get_irq_mask() | DECOD_HOST_MASK_PSTART);
       PDEBUG("channel change done\n");
   }

   //saved new PTS
   if((reg & DECOD_HOST_INT_SAVED_PTS) != 0)
   {
       STC_T stc;
       //video master
       if( (MF_DCR(VID_CHIP_CTRL) &
           (DECOD_CHIP_CONTROL_VID_MAS | DECOD_CHIP_CONTROL_AUD_MAS))
           == DECOD_CHIP_CONTROL_VID_MAS )
       {
           vid_atom_get_stc(&stc);
           aud_atom_set_stc(&stc);
       }
   }

   /*------------------------------------------------------------------------+
   | Block read complete.
   +------------------------------------------------------------------------*/
   if ((reg & DECOD_HOST_INT_BLOCK_READ) != 0)
   {
       //schedule a task for this interrupt
       TASK_MSG msg;
       msg.uMsgType = VID_MSG_BLOCK_READ;  //message type
       msg.ulPara1 = reg;                  //interrupt status
       msg.ulPara2 = 0;
       os_call_irq_task(uIrq, &msg);
   }

   /*------------------------------------------------------------------------+
   | Sequence Start Information
   +------------------------------------------------------------------------*/
   if (( reg & DECOD_HOST_INT_SS) != 0)
   {
       PDEBUG("sequence start\n");
   }

   if (( reg & DECOD_HOST_INT_SEND) != 0)
   {
       TASK_MSG msg;
       msg.uMsgType = VID_MSG_SEQUENCE_END;  //message type
       msg.ulPara1 = reg;                  //interrupt status
       msg.ulPara2 = 0;
       os_call_irq_task(uIrq, &msg);
   }

   /*------------------------------------------------------------------------+
   | Picture Start Information
   +------------------------------------------------------------------------*/
   if(( reg & DECOD_HOST_INT_PSTART) != 0)
   {

       PDEBUG("pic start\n");
       if(__VID_PLAY_PENDING)
       {
         if ((reg & DECOD_HOST_INT_FF_STATUS) ==0)
         {
           vid_atom_set_sfm(VID_SFM_NORMAL);
           __VID_PLAY_PENDING = 0;
         }
         else if(vid_play_stat.picture_start == 0)
             vid_play_stat.freeze = 1;
          vid_play_stat.picture_start = 1;
       }
       if ((reg & DECOD_HOST_INT_FF_STATUS) ==0)
       {
         if(__CC_PENDING)
         {
           vid_atom_set_sfm(VID_SFM_NORMAL);
            __CC_COMPLETE = 1;
           __CC_PENDING = 0;
           vid_osd_cc_done();
         }
         if(__ACC_PENDING)
         {
           __ACC_PENDING = 0;
           vid_atom_set_sfm(VID_SFM_NORMAL);
         }
       }
       else
       {
         if(__FF_PENDING)
         {
           __FF_COMPLETE = 1;
           __FF_PENDING = 0;
         }
       }
       if((__ACC_PENDING | __VID_PLAY_PENDING | __CC_PENDING | __FF_PENDING) == 0)
         vid_atom_set_irq_mask(vid_atom_get_irq_mask() & ~DECOD_HOST_MASK_PSTART);
   }

   /*------------------------------------------------------------------------+
   | Picture Start Status
   +------------------------------------------------------------------------*/
   if((reg & DECOD_HOST_INT_PS_STATUS) != 0)
   {
       __PS_SF_COMPLETE = 1;              //defined in vid_osi
   }

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
| vid_atom_irq_handle
| The QLR SB bit does not work in Current implementation.
+----------------------------------------------------------------------------*/
void vid_atom_irq_handle(UINT uIrq, void *pData)
{
    int           i;
    unsigned int  reg;


    /*------------------------------------------------------------------------+
    | When a video decoder interrupt occurs, this will be the first installed
    | handler to service it.  Save and export the interrupt status so that
    | other drivers may install their own handlers for this interrupt.
    | (The hardware status register is cleared after being read by the first
    |  interrupt handler.) -- BJC 102102
    +------------------------------------------------------------------------*/
    reg = MF_DCR(VID_HOST_INT);
    stb_vid_int_status = reg;
    PDEBUG("\n[IRQ = %d, reg = 0x%8.8lx\n", uIrq, reg);

    /*------------------------------------------------------------------------+
    | Process Video Interrupts
    +------------------------------------------------------------------------*/
    if (video_open_count == 0) {
       return;
    }

    /*------------------------------------------------------------------------+
    | Process Video Interrupts
    +------------------------------------------------------------------------*/
    vid_atom_process_int(uIrq, reg);

    /*------------------------------------------------------------------------+
    | Call Registered Notify functions
    +------------------------------------------------------------------------*/
    for(i=0; i<vid_atom_notify_ndx; i++)
    {
      if(reg & vid_atom_notify[i].mask)
      {
        if(vid_atom_notify[i].fn)
           (*vid_atom_notify[i].fn)(reg);
      }
    }

    PDEBUG("\n]\n");
    return;
}

#if 0
/*----------------------------------------------------------------------------+
|   XX     XXXXXX    XXXXXX    XXXXX
|  XXXX    XX   XX     XX     XX   XX
| XX  XX   XX   XX     XX      XX
| XX  XX   XXXXX       XX        XX
| XXXXXX   XX          XX         XX
| XX  XX   XX          XX     XX   XX
| XX  XX   XX        XXXXXX    XXXXX
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| vid_atom_ttx_add_notify
| NOTE: FOR USE WITH TTX DRIVER ONLY!!!
+----------------------------------------------------------------------------*/
int vid_atom_ttx_add_notify(void(*fn)(unsigned int intreq),unsigned int mask)
{
  int state;
  int i;

  if(mask & ~VID_NOTIFY_VALID_MASK || fn == 0)
    return(-1);

  state = os_enter_critical_section();

  /*--------------------------------------------------------------------------+
  | Test if notify function is already installed
  | If it is, "OR" the new mask with mask already installed
  +--------------------------------------------------------------------------*/
  for(i=0; i<vid_atom_notify_ndx; i++)
  {
    if(vid_atom_notify[i].fn == fn)
    {
      vid_atom_notify[i].mask |= mask;
      vid_atom_set_irq_mask(mask | vid_atom_get_irq_mask());
      os_leave_critical_section(state);
      return(0);                          /* max no notify functions reached */
    }
  }

  /*--------------------------------------------------------------------------+
  | see if maximum number of notify functions has been reached
  +--------------------------------------------------------------------------*/
  if(i >= VID_NOTIFY_MAX)
  {
    os_leave_critical_section(state);
    return(-1);                           /* max no notify functions reached */
  }

  /*--------------------------------------------------------------------------+
  | Add new Notify Function to end of list
  +--------------------------------------------------------------------------*/
  vid_atom_notify[i].fn = fn;
  vid_atom_notify[i].mask = mask;
  vid_atom_set_irq_mask(mask | vid_atom_get_irq_mask());
  vid_atom_notify_ndx++;

  os_leave_critical_section(state);
  return(0);
}

/*----------------------------------------------------------------------------+
| vid_atom_ttx_del_notify
| NOTE: FOR USE WITH TTX DRIVER ONLY!!!  Assumes all IRQs Removed were
|       EXCLUSIVELY added using the function vid_atom_ttx_add_notify().
+----------------------------------------------------------------------------*/
int vid_atom_ttx_del_notify(void(*fn)(unsigned int intreq),unsigned int mask)
{
  int          i,j;
  int          rc;
  unsigned int state;
  unsigned int irq_removed=0;


  state = os_enter_critical_section();
  rc = -1;

  /*--------------------------------------------------------------------------+
  | search for the notify record
  +--------------------------------------------------------------------------*/
  for(i=0; i<vid_atom_notify_ndx && rc!=0; i++)
  {
    if((vid_atom_notify[i].fn == fn) &&
      ((vid_atom_notify[i].mask & mask) != 0))
    {
      rc = 0;
      irq_removed = vid_atom_notify[i].mask;
      vid_atom_notify[i].mask &= ~mask;         /* reset specified mask bits */
      irq_removed ^= vid_atom_notify[i].mask;   /* find irq removed          */

      if(vid_atom_notify[i].mask == 0)        /* if zero, remove the handler */
      {
        for(j=i+1; j<vid_atom_notify_ndx; j++,i++)
        {
          vid_atom_notify[i].fn   = vid_atom_notify[j].fn;
          vid_atom_notify[i].mask = vid_atom_notify[j].mask;
        }
        vid_atom_notify[i].fn   = 0;
        vid_atom_notify[i].mask = 0;
        vid_atom_notify_ndx--;
      }
    }
  }

  /*--------------------------------------------------------------------------+
  | Calculate new Interrupt Mask
  +--------------------------------------------------------------------------*/
  if (rc == 0)
  {
    mask = vid_atom_get_irq_mask() & (~irq_removed);
    for(i=0; i<vid_atom_notify_ndx; i++)
    {
      mask |= vid_atom_notify[i].mask;
    }

    vid_atom_set_irq_mask(mask);
  }

  os_leave_critical_section(state);
  return(rc);
}
#endif
