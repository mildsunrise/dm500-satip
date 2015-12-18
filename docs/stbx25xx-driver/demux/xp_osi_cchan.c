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
|
|   Author    :  Ian Govett and Paul Gramann
|   Component :  xp
|   File      :  xp_cchan.c
|   Purpose   :  Channel change functions.
|   Changes   :
|   Date       By   Comments
|   ---------  ---  ------------------------------------------------------
|   27-Apr-98       Created
|   22-Jun-99  PAG  Added stat and clear stat commands
|   01-Feb-00  RLB  Removed printf's from pes_error.
|   02-Jun-01  TJC  Substituted "demux_" calls for "xp_" calls.
|   04-Jun-01  TJC  Moved from "menus" to "xp" folder.
|   30-Sep-01  LGH  ported to Linux
|   10-Jan-02  LGH  Added xp_osi_sync_stop when pcr = 0x1fff
+----------------------------------------------------------------------------*/
/* The necessary header files */

#include <linux/config.h>

#include <linux/version.h>



#ifdef MODVERSIONS

#include <linux/modversions.h>

#endif



#define  __NO_VERSION__

#include <linux/module.h>

#include <linux/kernel.h>

#include <linux/types.h>

#include  <vid/vid_types.h>

#include "xp_osi_global.h"

#include "xp_atom_reg.h"

GLOBAL_RESOURCES *pGlobal=NULL;

extern XP_STC_NOTIFY_FN stc_notify_fn;
SEMAPHORE_T     sema;

void hardware_cchan (USHORT audio_pid,USHORT video_pid,USHORT pcr_pid);
void hardware_audio_cchan(USHORT audio_pid);
void hardware_video_cchan(USHORT video_pid);
void hardware_pcr_cchan(USHORT pcr_pid);

extern void vid_atom_set_sfm(vidsfm_t sfm);

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
|  hardware_cchan
+----------------------------------------------------------------------------*/
void hardware_cchan (USHORT audio_pid,
                            USHORT video_pid,
                            USHORT pcr_pid)
{
    if(pGlobal == NULL)
        return;

    hardware_audio_cchan(audio_pid);
    hardware_video_cchan(video_pid);
    hardware_pcr_cchan(pcr_pid);
    return;
}

void hardware_audio_cchan(USHORT audio_pid)
{
    if(pGlobal == NULL)
        return;
    pGlobal->ChanChanInfo.xp_cchan_audchan_pending = 1;
    pGlobal->ChanChanInfo.xp_cchan_audpid          = audio_pid;


    xp_osi_channel_set_pid(pGlobal,XP_CHANNEL_CC_AUDIO, audio_pid);

    return;
}

void hardware_video_cchan(USHORT video_pid)
{
    if(pGlobal == NULL)
        return;
    pGlobal->ChanChanInfo.xp_cchan_vidchan_pending = 1;

    pGlobal->ChanChanInfo.xp_cchan_vidpid          = video_pid;
    vid_atom_set_sfm(VID_SFM_FIRST_ONLY);
    xp_osi_channel_set_pid(pGlobal,XP_CHANNEL_CC_VIDEO, video_pid);

    return;
}

void hardware_pcr_cchan(USHORT pcr_pid)
{
    if(pGlobal == NULL)
        return;

    if(stc_notify_fn != NULL)
        (*stc_notify_fn)(pGlobal,STC_REMOVED);

    pGlobal->ChanChanInfo.xp_cchan_pcrpid          = pcr_pid;

    xp_osi_clk_set_pid(pGlobal,pcr_pid);
    return;
}

/*----------------------------------------------------------------------------+
| aud_cc_done
+----------------------------------------------------------------------------*/
void aud_cc_done()
{
   /*-------------------------------------------------------------------------+
   | Disable Audio Synchronization
   +-------------------------------------------------------------------------*/
    if(pGlobal == NULL)
        return;
    xp_atom_disable_aud_sync();
   /*-------------------------------------------------------------------------+
   | Complete Pending Channel Change and Force Audio STC Update
   +-------------------------------------------------------------------------*/
   pGlobal->ChanChanInfo.xp_cchan_audchan_pending = 0;

   if ((pGlobal->ChanChanInfo.xp_cchan_vidchan_pending==0)
       && (pGlobal->ChanChanInfo.xp_cchan_next!=0))
   {
       if(pGlobal->ChanChanInfo.xp_cchan_next_audpid != 0x1fff)
       {
           hardware_audio_cchan(pGlobal->ChanChanInfo.xp_cchan_next_audpid);
           pGlobal->ChanChanInfo.xp_cchan_next_audpid = 0x1fff;
       }
       if(pGlobal->ChanChanInfo.xp_cchan_next_vidpid != 0x1fff)
       {
           hardware_video_cchan(pGlobal->ChanChanInfo.xp_cchan_next_vidpid);
           pGlobal->ChanChanInfo.xp_cchan_next_vidpid = 0x1fff;
       }

       if(pGlobal->ChanChanInfo.xp_cchan_next_pcrpid != 0x1fff)
       {
           hardware_pcr_cchan(pGlobal->ChanChanInfo.xp_cchan_next_pcrpid);
           pGlobal->ChanChanInfo.xp_cchan_next_pcrpid = 0x1fff;
       }
       pGlobal->ChanChanInfo.xp_cchan_next=0;
   }

   xp_osi_pcr_sync();
}

/*----------------------------------------------------------------------------+
| vid_cc_done
+----------------------------------------------------------------------------*/
void vid_cc_done()
{
    if(pGlobal == NULL)
        return;
 /*-------------------------------------------------------------------------+
   | Disable Video Synchronization
   +-------------------------------------------------------------------------*/
    xp_atom_disable_vid_sync();
   /*-------------------------------------------------------------------------+
   | Complete Pending Channel Change
   +-------------------------------------------------------------------------*/

   pGlobal->ChanChanInfo.xp_cchan_vidchan_pending = 0;
   if ((pGlobal->ChanChanInfo.xp_cchan_audchan_pending!=0) && (xp_atom_a_hw_cc_inprogress()==0))
   {
      pGlobal->ChanChanInfo.xp_cchan_audchan_pending=0;
   }
   if ((pGlobal->ChanChanInfo.xp_cchan_audchan_pending==0) &&
       (pGlobal->ChanChanInfo.xp_cchan_next!=0))
   {
       if(pGlobal->ChanChanInfo.xp_cchan_next_audpid != 0x1fff)
       {
           hardware_audio_cchan(pGlobal->ChanChanInfo.xp_cchan_next_audpid);
           pGlobal->ChanChanInfo.xp_cchan_next_audpid = 0x1fff;
       }
       if(pGlobal->ChanChanInfo.xp_cchan_next_vidpid != 0x1fff)
       {
           hardware_video_cchan(pGlobal->ChanChanInfo.xp_cchan_next_vidpid);
           pGlobal->ChanChanInfo.xp_cchan_next_vidpid = 0x1fff;
       }

       if(pGlobal->ChanChanInfo.xp_cchan_next_pcrpid != 0x1fff)
       {
           hardware_pcr_cchan(pGlobal->ChanChanInfo.xp_cchan_next_pcrpid);
           pGlobal->ChanChanInfo.xp_cchan_next_pcrpid = 0x1fff;
       }
       pGlobal->ChanChanInfo.xp_cchan_next=0;
   }

   /*-------------------------------------------------------------------------+
   | Unmute if Necessary and Force Video STC Update
   +-------------------------------------------------------------------------*/
   xp_osi_pcr_sync();
}

/*----------------------------------------------------------------------------+
| trans_aud_cc_done
+----------------------------------------------------------------------------*/
static void trans_aud_cc_done(GLOBAL_RESOURCES *pGlobal0, SHORT wChannelId, ULONG ulInterrupt)

{
   if(pGlobal0 == NULL || pGlobal0->uDeviceIndex != 0)
       return ;

   pGlobal->ChanChanInfo.xp_cchan_audsig++;
}

/*----------------------------------------------------------------------------+
| trans_vid_cc_done
+----------------------------------------------------------------------------*/
static void trans_vid_cc_done(GLOBAL_RESOURCES *pGlobal0, SHORT wChannelId, ULONG ulInterrupt)

{
   if(pGlobal0 == NULL || pGlobal0->uDeviceIndex != 0)
       return;

   pGlobal->ChanChanInfo.xp_cchan_vidsig++;
}

/*----------------------------------------------------------------------------+
| trans_audvid_cc_done
+----------------------------------------------------------------------------*/
static void trans_audvid_cc_done(GLOBAL_RESOURCES *pGlobal0, SHORT wChannelId, ULONG ulInterrupt)

{
    if(pGlobal0 == NULL || pGlobal0->uDeviceIndex != 0)
        return;

   xp_osi_pcr_sync();
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
| xp0_cchan_init
+----------------------------------------------------------------------------*/
SHORT xp_osi_cchan_init(GLOBAL_RESOURCES *pGlobal0)

{
   int   rc;

   pGlobal = pGlobal0;

   if(pGlobal == NULL || pGlobal->uDeviceIndex != 0)
       return -1;

   pGlobal->ChanChanInfo.xp_cchan_audchan         = XP_ERROR_CHANNEL_INVALID;
   pGlobal->ChanChanInfo.xp_cchan_audchan_pending = 0;
   pGlobal->ChanChanInfo.xp_cchan_audpid          = 0x1fff;
   pGlobal->ChanChanInfo.xp_cchan_audsig          = 0;
   pGlobal->ChanChanInfo.xp_cchan_next            = 0;
   pGlobal->ChanChanInfo.xp_cchan_next_audpid     = 0x1fff;
   pGlobal->ChanChanInfo.xp_cchan_next_vidpid     = 0x1fff;
   pGlobal->ChanChanInfo.xp_cchan_next_pcrpid     = 0x1fff;
   pGlobal->ChanChanInfo.xp_cchan_pcrpid          = 0x1fff;
   pGlobal->ChanChanInfo.xp_cchan_vidchan         = XP_ERROR_CHANNEL_INVALID;
   pGlobal->ChanChanInfo.xp_cchan_vidchan_pending = 0;
   pGlobal->ChanChanInfo.xp_cchan_vidpid          = 0x1fff;
   pGlobal->ChanChanInfo.xp_cchan_vidsig          = 0;
   pGlobal->ChanChanInfo.xp_cchan_vid_oflw        = 0;
   pGlobal->ChanChanInfo.xp_cchan_aud_oflw        = 0;
   pGlobal->ChanChanInfo.xp_cchan_def_oflw        = 0;
   /*-------------------------------------------------------------------------+
   |  Set-up Audio and Video Channel Change Complete Interrupt Handlers
   +-------------------------------------------------------------------------*/
    //lingh delete
   /*-------------------------------------------------------------------------+
   |  Set-up Transport Audio/Video Channel Change Complete Interrupt Handlers
   +-------------------------------------------------------------------------*/
   if ((rc = xp_osi_interrupt_notify (pGlobal,XP_INTERRUPT_NOTIFY_ADD,
             XP_INTERRUPT_IR_ACCC, (PFS)trans_aud_cc_done)) != 0)
   {

      return(rc);
   }

   if ((rc = xp_osi_interrupt_notify (pGlobal,XP_INTERRUPT_NOTIFY_ADD,
             XP_INTERRUPT_IR_VCCC, (PFS)trans_vid_cc_done)) != 0)
   {
      return(rc);
   }

   if ((rc = xp_osi_interrupt_notify (pGlobal,XP_INTERRUPT_NOTIFY_ADD,
             XP_INTERRUPT_IR_ACCC | XP_INTERRUPT_IR_VCCC,
             (PFS)trans_audvid_cc_done)) != 0)
   {
      return(rc);
   }

   sema = os_create_semaphore(1);

   return(0);
}


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
| xp0_cchan_auto
+----------------------------------------------------------------------------*/
void xp_osi_video_cchan_auto(GLOBAL_RESOURCES *pGlobal0,unsigned short vpid)

{
   unsigned long   crit;


   /*-------------------------------------------------------------------------+
   | Issue a Manual Channel Change When Audio and Video not Playing
   +-------------------------------------------------------------------------*/
   if ((vpid == 0x1fff) )
   {
      xp_osi_cchan_stop_video();
      xp_osi_cchan_start_video(vpid);
   /*-------------------------------------------------------------------------+
   | Use Hardware Channel Change if Audio and Video is Already Playing
   +-------------------------------------------------------------------------*/
   }
   else
   {
      crit = os_enter_critical_section();

      if (!pGlobal->ChanChanInfo.xp_cchan_vidchan_pending)
      {
         hardware_video_cchan(vpid);

      }
      /*----------------------------------------------------------------------+
      |  Channel change is still pending, set the waiting flag
      +----------------------------------------------------------------------*/
      else
      {

          //lingh added for test
          os_wait_semaphore(sema,100);

          hardware_video_cchan(vpid);
          pGlobal->ChanChanInfo.xp_cchan_vidchan_pending = 0;
          pGlobal->ChanChanInfo.xp_cchan_next        = 1;
          pGlobal->ChanChanInfo.xp_cchan_next_vidpid = vpid;
      }
      os_leave_critical_section(crit);
   }

   return;
}


void xp_osi_audio_cchan_auto(GLOBAL_RESOURCES *pGlobal0,unsigned short apid)

{
    unsigned long   crit;

   /*-------------------------------------------------------------------------+
   | Issue a Manual Channel Change When Audio and Video not Playing
   +-------------------------------------------------------------------------*/
   if ((apid == 0x1fff))
   {

      xp_osi_cchan_stop_audio();
      xp_osi_cchan_start_audio(apid);

   /*-------------------------------------------------------------------------+
   | Use Hardware Channel Change if Audio and Video is Already Playing
   +-------------------------------------------------------------------------*/
   }
   else
   {
      crit = os_enter_critical_section();

      if (!pGlobal->ChanChanInfo.xp_cchan_audchan_pending)
      {
          hardware_audio_cchan(apid);

      }
      /*----------------------------------------------------------------------+
      |  Channel change is still pending, set the waiting flag
      +----------------------------------------------------------------------*/
      else
      {
          os_wait_semaphore(sema,100);

          hardware_audio_cchan(apid);
          pGlobal->ChanChanInfo.xp_cchan_audchan_pending = 0;
          pGlobal->ChanChanInfo.xp_cchan_next        = 1;
          pGlobal->ChanChanInfo.xp_cchan_next_audpid = apid;
      }

      os_leave_critical_section(crit);
   }

   return;
}

void xp_osi_pcr_cchan_auto(GLOBAL_RESOURCES *pGlobal0,unsigned short ppid)

{
    hardware_pcr_cchan(ppid);
   /*-------------------------------------------------------------------------+
   | Enable Synchronization
   +-------------------------------------------------------------------------*/
   if (ppid == 0x1fff)
   {
      xp_osi_pcr_sync_stop();       //added in Jan.10/2002
      (void)xp_atom_a_hw_sync_off();
      (void)xp_atom_v_hw_sync_off();
   }
   else
   {
      xp_osi_pcr_sync_start();
   }

   return;
}


/*----------------------------------------------------------------------------+
|  xp0_cchan_start_audio
+----------------------------------------------------------------------------*/
int xp_osi_cchan_start_audio(unsigned short pid)
{
   return(0);
}

/*----------------------------------------------------------------------------+
|  xp0_cchan_start_video
+----------------------------------------------------------------------------*/
int xp_osi_cchan_start_video(unsigned short pid)
{
   return(0);
}

/*----------------------------------------------------------------------------+
|  xp0_cchan_start_pcr
+----------------------------------------------------------------------------*/
void xp_osi_cchan_start_pcr(unsigned short pid)
{
}

/*----------------------------------------------------------------------------+
|  xp0_cchan_stop_audio
+----------------------------------------------------------------------------*/
int xp_osi_cchan_stop_audio(void)
{

   return(0);
}

/*----------------------------------------------------------------------------+
|  xp0_cchan_stop_video
+----------------------------------------------------------------------------*/
int xp_osi_cchan_stop_video(void)
{

   return(0);
}

/*----------------------------------------------------------------------------+
|  xp0_cchan_stop_pcr
+----------------------------------------------------------------------------*/
void xp_osi_cchan_stop_pcr(void)
{
}
