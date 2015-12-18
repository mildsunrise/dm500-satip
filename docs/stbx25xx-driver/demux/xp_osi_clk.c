/*----------------------------------------------------------------------------+
|     This source code has been made available to you by IBM on an AS-IS
|     basis.  Anyone receiving this source is licensed under IBM
|     copyrights to use it in any way he or she deems fit, including
|     copying it, modifying it, compiling it, and redistributing it either
|     with or without modifications.  No license under IBM patents or
|     patent applications is to be implied by the copyright license.
|
|     Any user of this software should understand that IBM cannot provide
|     technical support for this software and will not be responsible for
|     any consequences resulting from the use of this software.
|
|     Any person who transfers this source code or any derivative work
|     must include the IBM copyright notice, this paragraph, and the
|     preceding two paragraphs in the transferred software.
|
|     COPYRIGHT   I B M   CORPORATION 1999
|     LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|
| DESCRIPTION :  The set of functions allow the application to process
|                the clock recovery interrupts.
|
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author    :  Andy Anderson, Ian Govett
| Component :  xp
| File      :  xp_clk.c
| Purpose   :  Clock Recovery.
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 15-Jan-98  IG   Created
| 30-Sep-01  LGH  Ported to Linux
| 01-aug-02  LGH  FIx the bug in PCR interrupt
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+

|                              Clock Recovery

+-----------------------------------------------------------------------------+

|

|   The clock recovery is implemented by making use of both the hardware

|   (auto-PWM), and software.  The software algorithm provides fast and

|   accurate control necessary to quickly match the clock rates.  The

|   hardware algorithm (auto-PWM) makes much smaller adjustments to

|   "maintain" the proper clock rate.  The software algorithm is activated

|   when the PCR PID is updated or when the STC & PCR differ by a

|   programmable value.

|   Once this difference is below a pre-determined threshold, the software

|   algorithm shuts off.  At this time, the hardware algorithm (auto-PWM)

|   continues to adjust the clock rate based on the current STC and

|   arriving PCRs.  When the threshold is exceeded, the software algorithm

|   is activated via a PCR type interrupt.  Using this technique, the

|   number of clock recovery interrupts is greatly reduced.

|

|   The clock recovery algorithm make use of two types of hardware

|   interrupts:  STC_LOAD, and PCR.  An interrupt handler is registered

|   when the clock recovery algorithm is started.  The hardware makes use

|   of the PWM register to control the clock rate, and a threshold register

|   to control when interrupts should be generated for the software

|   algorithm.

|

|   The following functions provide access to clock recovery algorithms

|   available for the transport demux driver.  The clock recovery

|   algorithms are enabled and disabled using the xp0_clk_start() and

|   xp0_clk_stop() functions.  The xp0_clk_set_pid() function designates the

|   PID value of the transport packets containing the PCR's.  The

|   xp0_clk_get_stc_high() function provides the caller with the current

|   value of the STC register which may be used by the decoder device

|   driver for the decoders.

|

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



#include "xp_osi_global.h"

#include "xp_atom_reg.h"


XP_STC_NOTIFY_FN stc_notify_fn = NULL;


/*----------------------------------------------------------------------------+

| Local Defines

+----------------------------------------------------------------------------*/

#define PWM_CLAMP               50       /* Sets the maximum rate of change  */

                                         /* of the PWM value                 */

#define STC_LOAD_PWM_CLAMP      100      /* Sets the maximum rate of change  */

                                         /* just after the PWM value is read */

#define PCRS_HIGH_GAIN          10       /* Number of PCRs after the STC is  */

                                         /* loaded which will use            */

                                         /* STC_LOAD_PWM_CLAMP               */

#define RATE_GAIN               4        /* The calculated rate difference   */

                                         /* is divided by RATE_GAIN ^ 2      */

#define DELTA_GAIN              4        /* The calculated delta             */

                                         /* is divided by DELTA_GAIN ^ 2     */

#define LOW_THRESHOLD           32       /* Lower Delta Threshold for the    */

                                         /* s/w clock recovery algorithm     */

#define HIGH_THRESHOLD          256      /* Upper Delta Threshold for the    */

                                         /* s/w clock recovery algorithm     */

#define DECODE_DELAY ((unsigned long) 10)/* decode delay                     */



//Internal

static void pcr_interrupt(GLOBAL_RESOURCES *pGlobal,ULONG ulInterrupt);





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
|  pcr_interrupt
+-----------------------------------------------------------------------------+
|
|  FUNCTION    :  pcr_interrupt
|
|  DESCRIPTION :  This function is called by an interrupt service
|                 routine whenever an PCR or STC is processed by hardware
|
|  COMMENTS    :  This function is processed in the interrupt context
|                 and must be careful to only use functions which are
|                 safe within the context of an interrupt.  Do NOT use
|                 any I/O instructions such as printf() or gets(), etc.
|
|                 Portions of this function could be shared with the
|                 other interrupt routine.  The question is how to best
|                 optimize the interrupt routines and the overhead
|                 of function calls.
|
|                 The other option which is better is to integrate both
|                 functions and setup so that the single function is
|                 called in either case
|
+----------------------------------------------------------------------------*/

static void pcr_interrupt(GLOBAL_RESOURCES *pGlobal,ULONG ulInterrupt)
{
    unsigned        overflow;         /* Delta Overflow bit                  */
    unsigned long   delta;            /* Delta or Delta Magnitude            */
    unsigned long   tdelta;

    short           rate_adjust;      /* Adjustment to PWM based on frequency*/
    short           pwm_adjust;       /* The amount to adjust the pwm        */
    unsigned long   pwm;              /* Current PWM value                   */
    unsigned long   new_pwm;          /* Updated PWM value                   */
    short           soft_overflow;    /* software algorithm overflow         */
    long            delta_pcr;        /* Difference between the current      */
    long            delta_stc;        /* and previous values                 */

    long            temp1;
    long            temp2;

    long            value_adjust;     /* Adjustment to PWM based on delta    */

    unsigned long   stc_high;
    unsigned long   stc_low;

    unsigned long   pcr_high;
    unsigned long   pcr_low;

    unsigned long   stc;
    unsigned long   pcr;

    XP_PCRSTCD_REGP p_delta;
    UINT32          flag;

    /*------------------------------------------------------------------------+
   |  Read the STC, PCR, and delta values
    +------------------------------------------------------------------------*/

    flag = os_enter_critical_section();
    pcr_high = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRHI);
    pcr_low  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRLOW);

    stc_high = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_LSTCHI);
    stc_low  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_LSTCLOW);

    tdelta   = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRSTCD);
    os_leave_critical_section(flag);



    p_delta  = (XP_PCRSTCD_REGP)(void *) &tdelta;

    delta    = p_delta->delta;

    overflow = p_delta->ovfl;

    if(overflow)
    {
        delta = ~0;
    }

    /*------------------------------------------------------------------------+
    |   Make sure the clock recovery registers are consistent
    |   Later the value of Incons_data can be returned as
    |   part of the status of the driver.
    |   This is a candidate to be removed later.
    +------------------------------------------------------------------------*/

    flag = os_enter_critical_section();
    if(pcr_low != xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRLOW))
    {
        pGlobal->ClkInfo.XpClkInconsData++;
        return;
    }
    os_leave_critical_section(flag);

    /*------------------------------------------------------------------------+
    |  Track number of PCRS Since STC loaded
    +------------------------------------------------------------------------*/

    if(ulInterrupt & XP_INTERRUPT_IR_STCL)
    {
        pGlobal->ClkInfo.XpClkNpcrs = 0;                        /* Initialize to 0         */
        if(stc_notify_fn != NULL)
            (*stc_notify_fn)(pGlobal,STC_DISCONTINUITY);
    }



    if(pGlobal->ClkInfo.XpClkNpcrs <= PCRS_HIGH_GAIN)
    {         /* Still in the high gain  */
        pGlobal->ClkInfo.XpClkNpcrs++;
    }



    /*------------------------------------------------------------------------+
    |   Normalize the STC and PCR
    |   This drops the upper 10 bits of both the PCR and the STC
    |   This also detects if either the PCR or STC has wrapped
    |   and other has not.  In either case the software algorithm
    |   doesn't run.
    +------------------------------------------------------------------------*/

    stc = (((stc_high & 0x003FFFFF) * 2) + ((stc_low & 0x200) >> 9)) * 300;
    stc = stc + (stc_low & 0x1FF);

    pcr = (((pcr_high & 0x003FFFFF) * 2) + ((pcr_low & 0x200) >> 9)) * 300;
    pcr = pcr + (pcr_low & 0x1FF);


    /*------------------------------------------------------------------------+
    |  Check if the upper 10 bits are the same.
    |  The software clock recovery algorithm is not used if
    |  soft_overflow occurs.
    |  Also check if hardware overflow has occured and treat
    |  like a software overflow - pag 7/16/99
    +------------------------------------------------------------------------*/

    if(overflow || (stc_high & 0xFFC00000) != (pcr_high & 0xFFC00000))
    {
        stc = stc_high & 0xFFC00000;
        pcr = pcr_high & 0xFFC00000;

        soft_overflow = 1;

        pGlobal->ClkInfo.XpClkErrs++;                  /* Increment the global error count */

        /*--------------------------------------------------------------------+
        |  if there were two consecutive errors, force a PCR reload
        +--------------------------------------------------------------------*/

        if(pGlobal->ClkInfo.wXpClkPrevErr == 1)
        {
            flag = os_enter_critical_section();
            xp_osi_clk_set_pid(pGlobal,xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRPID));
            os_leave_critical_section(flag);

            pGlobal->ClkInfo.wXpClkPrevErr = 0;
        }
        else
        {
            pGlobal->ClkInfo.wXpClkPrevErr = 1;
        }
    }
    else
    {
        soft_overflow   = 0;
        pGlobal->ClkInfo.wXpClkPrevErr = 0;
    }

    /*------------------------------------------------------------------------+
    |   Run the software algorithm if there are no errors
    |   Adjust auto PWM if the delta is large
    +------------------------------------------------------------------------*/
    if(soft_overflow == 0)
    {
        delta_pcr = pcr - pGlobal->ClkInfo.XpClkPrevPcr;
        delta_stc = stc - pGlobal->ClkInfo.XpClkPrevStc;

        temp1 = delta_pcr - delta_stc;
        temp2 = 27000000 / delta_pcr;

        rate_adjust = (temp1 * temp2) >> RATE_GAIN;
//lingh changed it
        value_adjust = (pcr - stc) >> DELTA_GAIN;

        pwm_adjust = rate_adjust + value_adjust;

        /*--------------------------------------------------------------------+
        |  Once the delta is small enough increase the threshold
        |  and use only autopwm until this function is called
        |  again, then increase the threshold.
        +--------------------------------------------------------------------*/

        if((delta < LOW_THRESHOLD) && (pGlobal->ClkInfo.lXpClkPrevDelta < LOW_THRESHOLD))
        {
            pGlobal->ClkInfo.uwXpClkThreshold = HIGH_THRESHOLD;

            flag = os_enter_critical_section();
            xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRSTCT, pGlobal->ClkInfo.uwXpClkThreshold);
            os_leave_critical_section(flag);

        }

        else if(pGlobal->ClkInfo.uwXpClkThreshold != 0)
        {
            pGlobal->ClkInfo.uwXpClkThreshold = 0;

            flag = os_enter_critical_section();
            xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRSTCT, pGlobal->ClkInfo.uwXpClkThreshold);
            os_leave_critical_section(flag);
        }

        pGlobal->ClkInfo.XpClkPrevStc   = stc;
        pGlobal->ClkInfo.XpClkPrevPcr   = pcr;
        pGlobal->ClkInfo.lXpClkPrevDelta = delta;

        /*--------------------------------------------------------------------+
        |  Restrict the rate of change of the PWM value.
        |  For the first few PCRs received after the STC is
        |  loaded the clamp value is larger so that the local
        |  clock moves closer to the multiplexor clock quickly.
        |  After that reduce use a smaller clamp value.
        +--------------------------------------------------------------------*/

        if (pGlobal->ClkInfo.XpClkNpcrs <= PCRS_HIGH_GAIN)
        {           /* clamp adjustment */
            if(pwm_adjust > STC_LOAD_PWM_CLAMP)
            {
                pwm_adjust = STC_LOAD_PWM_CLAMP;
            }
            else if(pwm_adjust < -STC_LOAD_PWM_CLAMP)
            {
                pwm_adjust = -STC_LOAD_PWM_CLAMP;
            }
        }
        else
        {
            if(pwm_adjust > PWM_CLAMP)
            {
                pwm_adjust = PWM_CLAMP;
            }
            else if(pwm_adjust < -PWM_CLAMP)
            {
                pwm_adjust = -PWM_CLAMP;
            }
        }

        flag = os_enter_critical_section();
        pwm     = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_PWM);
        os_leave_critical_section(flag);

        new_pwm = pwm + pwm_adjust;

        /*--------------------------------------------------------------------+
        |  PWM range is actually a signed 12 bit number
        |  whose range is 0x800 - 0x7ff.  So after calculating
        |  the new pwm, make sure we don't exceed the above bounds
        +--------------------------------------------------------------------*/

        if((pwm <= 0x07ff) && (new_pwm > 0x07ff) && (pwm_adjust > 0))
        {
            new_pwm = 0x07ff;
        }

        else if((pwm >= 0x0800) && (new_pwm < 0x0800) && (pwm_adjust < 0))
        {
            new_pwm = 0x0800;
        }

        flag = os_enter_critical_section();
        xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PWM, new_pwm); /* Update the PWM register  */
        os_leave_critical_section(flag);
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
|  xp0_clk_get_stc_high
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  allocates a channel for the requested type
|
|  PROTOTYPE  :  short xp0_clk_get_stc_high(
|                unsigned long *p_stc_high)
|
|  ARGUMENTS  :  p_stc_high    -  the upper 32 bits of the stc, returned
|
|  RETURNS    :  zero if the stc is valid, or non-zero if invalid
|
|  ERRORS     :  XP_ERROR_INVALID_STC
|
|  COMMENTS   :  The stc value is determined if it is valid if a recent
|                PCR value was loaded.  Note: During a channel change,
|                there is an interval between the time the new PCR pid is
|                written, and the arrival of the first PCR where the STC,
|                could be considered either valid or invalid.  This
|                function assumes the STC is valid for this duration.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_clk_get_stc_high(GLOBAL_RESOURCES *pGlobal,ULONG *pStcHigh)
{
    short rc;
    short i;
    unsigned long stc_high=0;
    unsigned long lstc_high=0;
    unsigned long lstc_high2=0;
    unsigned long diff;
    UINT32  flag;

    /*------------------------------------------------------------------------+
    |  Do not use xp0_os_semaphore_wait() since called from an interrupt
    +------------------------------------------------------------------------*/
    /*------------------------------------------------------------------------+
    |  Get STC and Latched STC values.  Since the hardware could
    |  receive a PCR at any time, we re-read the STC Latched Register
    |  twice to make sure the value is still the same.
    +------------------------------------------------------------------------*/
    flag = os_enter_critical_section();

    for(i=0; i<2; i++)
    {
        lstc_high  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_LSTCHI);
        stc_high   = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_STCHI);
        lstc_high2 = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_LSTCHI);
        if(lstc_high == lstc_high2) {
            break;
        }
    }
    os_leave_critical_section(flag);

    /*------------------------------------------------------------------------+
    |  Compare the Two Latched STC Values Read, A PCR must have been
    |  Received if their Different
    +------------------------------------------------------------------------*/
    rc = (lstc_high != lstc_high2) ? XP_ERROR_INVALID_STC : 0;

    /*------------------------------------------------------------------------+
    |   The STC is considered valid if a recent PCR has been received.
    |   Each PCR received is loaded into the STC Latched register.  So, we
    |   compare the values of the STC Latched and the STC registers whose
    |   difference should be within the expected arrival rate of PCR's.
    |
    |   The calculation is as follows:
    |       STC clock is 27 MHZ, PCR arrival rate=100ms
    |       The upper 32 bits of the STC are modulo 600
    |
    |       difference    =  2.7E6 / 600
    |                     =  4500
    |
    |   Add a 1% margin so we don't hit the edge condition exactly
    |   and are somewhat tolerant of non-compliant streams.
    |
    |      4500 x  1.01  = 4545
    |
    |   STC is valid if | STC - STCHOLD | < 4545
    |
    |   Note: We have to account for the STC wrapping.
    +------------------------------------------------------------------------*/
    /*------------------------------------------------------------------------+
    |  Since these are both unsigned, there is not a Wrapping Issue
    +------------------------------------------------------------------------*/
    if (rc == 0)
    {
       diff = stc_high - lstc_high;
       if (diff > 4545)
       {
          rc = XP_ERROR_INVALID_STC;
       }
    }

    if (rc == 0 && pGlobal->ClkInfo.XpClkNpcrs == 0)
    {
       rc = XP_ERROR_INVALID_STC;
    }
    *pStcHigh = stc_high;

    return(rc);
}



/*----------------------------------------------------------------------------+

|  xp0_clk_init

+----------------------------------------------------------------------------*/

SHORT xp_osi_clk_init(GLOBAL_RESOURCES *pGlobal)

{

    pGlobal->ClkInfo.XpClkInstalled = 0;



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

|  xp0_clk_get_errors

+----------------------------------------------------------------------------*/

void xp_osi_clk_get_errors(GLOBAL_RESOURCES *pGlobal,XP_CLK_STATUS_PTR pClkStatus)



{

    pClkStatus->Incons_data = pGlobal->ClkInfo.XpClkInconsData;

    pClkStatus->Errors      = pGlobal->ClkInfo.XpClkErrs;

}



/*----------------------------------------------------------------------------+
|  xp0_clk_set_pid
+-----------------------------------------------------------------------------+
|
|  DESCRIPTION:  define the PID value which contains the PCR's
|
|  PROTOTYPE  :  void xp0_clk_set_pid(
|                unsigned short pid)
|
|  ARGUMENTS  :  pid      -  PID value containing PCRs.
|
|  RETURNS    :
|
|  ERRORS     :
|
|  COMMENS    :  This function is called to set the pid which contains
|                the PCR information used for clock synchronization.
|                When the PCR pid is written, the hardware will load the
|                next PCR value into the stc register.  This action
|                generates an XP_INTERRUPT_IR_STCL interrupt.
|
+----------------------------------------------------------------------------*/
void xp_osi_clk_set_pid(GLOBAL_RESOURCES *pGlobal,USHORT uwPid)
{
    unsigned reg;
    XP_PCRPID_REGP  p_reg;
    UINT32  flag;
    /*------------------------------------------------------------------------+
    |  Do not use xp0_os_semaphore_wait() since called from an interrupt
    +------------------------------------------------------------------------*/
    p_reg = (XP_PCRPID_REG *)(void *) &reg;
    p_reg->res1     = 0;
    p_reg->pidv     = uwPid;

    flag = os_enter_critical_section();
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRPID, reg);
    os_leave_critical_section(flag);

    pGlobal->ClkInfo.XpClkNpcrs = 0;                          /* Initialize to 0           */

}



/*----------------------------------------------------------------------------+
|  xp0_clk_start
+-----------------------------------------------------------------------------+
|
|  FUNCTION    :  xp0_clk_start
|
|  DESCRIPTION :  This function is called to start using the clock
|                 recovery algorithms (software/hardware combination)
|                 provided with the driver.
|
|  COMMENTS    :  The function xp0_clk_stop() is used to turn off the
|                 default clock recovery routines.
|
|  NOTES       :  xp0_clk_start() is called during driver initialization.
|
+----------------------------------------------------------------------------*/

SHORT xp_osi_clk_start(GLOBAL_RESOURCES *pGlobal)
{
    UINT32  flag;

    if(pGlobal->ClkInfo.XpClkInstalled == 0)
    {
        pGlobal->ClkInfo.XpClkInstalled = 1;
        pGlobal->ClkInfo.XpClkInconsData = 0;
        pGlobal->ClkInfo.XpClkErrs = 0;
        pGlobal->ClkInfo.XpClkNpcrs = PCRS_HIGH_GAIN;

        /*--------------------------------------------------------------------+
        |  Setup the stcThreshold to be 0 and Setup the Interrupt
        |  Notification function to Process each stcThreshhold
        |  Turn on AutoPWM
        +--------------------------------------------------------------------*/

        flag = os_enter_critical_section();
        xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRSTCT, 0);
        os_leave_critical_section(flag);



        xp_osi_interrupt_notify(
            pGlobal,
            XP_INTERRUPT_NOTIFY_ADD,
            XP_INTERRUPT_IR_PCR | XP_INTERRUPT_IR_STCL,
            (PFS)pcr_interrupt);

        flag = os_enter_critical_section();
        xp_atom_dcr_write_register(pGlobal->uDeviceIndex,XP_CONFIG1_REG_APWMA, 1);
        os_leave_critical_section(flag);
    }

    return(0);
}



/*----------------------------------------------------------------------------+
|  xp0_clk_stop
+-----------------------------------------------------------------------------+
|
|  FUNCTION    :  xp0_clk_stop
|
|  DESCRIPTION :  This function is called to stop using the clock
|                 recovery algorithms (software/hardware combination)
|                 provided with the driver.
|
|  COMMENTS    :  Clock recovery functions may be re-started using the
|                 xp0_clk_start() function.
|
+----------------------------------------------------------------------------*/

void xp_osi_clk_stop(GLOBAL_RESOURCES *pGlobal)
{
    UINT32  flag;


    if(pGlobal->ClkInfo.XpClkInstalled)
    {
        pGlobal->ClkInfo.XpClkInstalled = 0;

        /*--------------------------------------------------------------------+
        |  Setup Interrupt notification function to process each
        |  stcThreshhold.  Turn off AutoPWM
        +--------------------------------------------------------------------*/
        xp_osi_interrupt_notify(pGlobal,XP_INTERRUPT_NOTIFY_DELETE,
            XP_INTERRUPT_IR_PCR | XP_INTERRUPT_IR_STCL, (PFS)pcr_interrupt);

        flag = os_enter_critical_section();
        xp_atom_dcr_write_register(pGlobal->uDeviceIndex,XP_CONFIG1_REG_APWMA, 0);
        os_leave_critical_section(flag);

        if(stc_notify_fn != NULL)
        {
            xp_osi_interrupt_notify(
                pGlobal,
                XP_INTERRUPT_NOTIFY_DELETE,
                XP_INTERRUPT_IR_STCC,
                (PFS)stc_interrupt);

            xp_osi_interrupt_status_notify(
                pGlobal,
                XP_INTERRUPT_NOTIFY_DELETE,
                XP_INTERRUPT_FESTAT_FPCR,
                (PFS)first_pcr_interrupt);

            stc_notify_fn = NULL;
        }

    }
}


int xp_osi_clk_set_stc_compare(GLOBAL_RESOURCES *pGlobal,ULONG stc_high)
{
    UINT32 flag;

    if(pGlobal->uDeviceIndex != 0)
        return -1;

    flag = os_enter_critical_section();
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_STCCOMP,stc_high);
    os_leave_critical_section(flag);

    return 0;
}

int xp_osi_clk_set_stc_event_notify(GLOBAL_RESOURCES *pGlobal,XP_STC_NOTIFY_FN notify_fn)
{

    if(notify_fn == NULL)
        return -1;

    if(stc_notify_fn != NULL)
    {
        xp_osi_interrupt_notify(
            pGlobal,
            XP_INTERRUPT_NOTIFY_DELETE,
            XP_INTERRUPT_IR_STCC,
            (PFS)stc_interrupt);

        xp_osi_interrupt_status_notify(
            pGlobal,
            XP_INTERRUPT_NOTIFY_DELETE,
            XP_INTERRUPT_FESTAT_FPCR,
            (PFS)first_pcr_interrupt);

        stc_notify_fn = NULL;
    }

    stc_notify_fn = notify_fn;

    xp_osi_interrupt_notify(
        pGlobal,
        XP_INTERRUPT_NOTIFY_ADD,
        XP_INTERRUPT_IR_STCC,
        (PFS)stc_interrupt);

    xp_osi_interrupt_status_notify(
        pGlobal,
        XP_INTERRUPT_NOTIFY_ADD,
        XP_INTERRUPT_FESTAT_FPCR,
        (PFS)first_pcr_interrupt);

    return 0;
}

int xp_osi_clk_release_stc_event(GLOBAL_RESOURCES *pGlobal)
{

    if(stc_notify_fn != NULL)
    {
        xp_osi_interrupt_notify(
            pGlobal,
            XP_INTERRUPT_NOTIFY_DELETE,
            XP_INTERRUPT_IR_STCC,
            (PFS)stc_interrupt);

        xp_osi_interrupt_status_notify(
            pGlobal,
            XP_INTERRUPT_NOTIFY_DELETE,
            XP_INTERRUPT_FESTAT_FPCR,
            (PFS)first_pcr_interrupt);

        stc_notify_fn = NULL;
        return 0;
    }
    return -1;
}

void stc_interrupt(GLOBAL_RESOURCES *pGlobal,ULONG ulInterrupt)
{
    ULONG flag;

    flag = os_enter_critical_section();
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_STCCMPD,0xffffffff);
    os_leave_critical_section(flag);

    if(ulInterrupt & XP_INTERRUPT_IR_STCC)
    {
        if(stc_notify_fn != NULL)
        {
            (*stc_notify_fn)(pGlobal,STC_FIRED);
        }
    }
}

void first_pcr_interrupt(GLOBAL_RESOURCES *pGlobal,ULONG ulInterrupt)
{
    if(ulInterrupt & XP_INTERRUPT_IR_STCC)
    {
        if(stc_notify_fn != NULL)
        {
            (*stc_notify_fn)(pGlobal,STC_PRESENT);
        }
    }
}

void xp_osi_clk_get_current_stc(GLOBAL_RESOURCES *pGlobal, STC_TYPE *stc_type_ptr)
{
    ULONG flag;
    ULONG stc_high;
    ULONG stc_low;

    flag = os_enter_critical_section();
    stc_high = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_STCHI);
    stc_low = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_STCLOW);
    os_leave_critical_section(flag);

    stc_type_ptr->base_time1 = ((stc_high & 0xfffffffe) << 1) |
        ((stc_low & 0x00000200) >> 9);
    stc_type_ptr->base_time2 = (stc_high & 0x80000000)>>31;

    return;
}

