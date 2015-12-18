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
|   Author    :  Ian Govett
|   Component :  xp
|   File      :  xp.c
|   Purpose   :  Initialization functions
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  -------------------------------------------------------
|   15-Jan-98       Created
|   04-May-01       Updated for Pallas
|   30-Sep-01  LGH  Combined codes of 3 devices, ported to Linux
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
|                   Hardware and Software initialization
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
|
|   The following functions provide initialization routines for both the
|   transport demux driver, and the hardware.  xp0_init() is called to
|   initialize the driver and transport hardware.  The calling program must
|   supply the amount of memory reserved for the transport demux queues.
|   The space may be allocated on the heap (using malloc) or allocated via
|   a predetermined memory map.  The queue address must be aligned on a
|   16 MByte boundary if the pre-assigned address is used (queue_mode=0).
|
|   The error codes for all transport demux errors are mapped to a text
|   description using the xp0_get_error_msg() function.
|
+----------------------------------------------------------------------------*/

#include <linux/config.h>
#include <linux/version.h>

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif

#define  __NO_VERSION__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include "hw/hardware.h"
#include "xp_osi_global.h"
#include "xp_atom_reg.h"

/*----------------------------------------------------------------------------+
|  Static Variables
+----------------------------------------------------------------------------*/
static ULONG ulSavedCtlReg;
/*----------------------------------------------------------------------------+
|  Configuration Values
+----------------------------------------------------------------------------*/
static XP_CONFIG_VALUES  xp0_configv;
static XP_CONFIG_VALUES  xp0_configv_reset = {
                                            /* CONFIG REGISTER 1 VALUES      */
      0,                                    /*    denbl                      */
      0,                                    /*    vpu                        */
      0,                                    /*    apu                        */
      0,                                    /*    tstoe                      */
      0,                                    /*    tsclkp                     */
      0,                                    /*    tsdp                       */
      0,                                    /*    tssp                       */
      0,                                    /*    tsep                       */
      0,                                    /*    tsvp                       */
      0,                                    /*    tssm                       */
      0,                                    /*    syncd                      */
      0,                                    /*    bbmode                     */
      2,                                    /*    syncl                      */
                                            /* CONFIG REGISTER 2 VALUES      */
      0,                                    /*    ved                        */
      0,                                    /*    acpm                       */
      0,                                    /*    vcpm                       */
      1,                                    /*    mwe                        */
      1,                                    /*    salign                     */
      0,                                    /*    atsed                      */
      0,                                    /*    atbd                       */
      0,                                    /*    accd                       */
      0,                                    /*    vtsed                      */
      0,                                    /*    vtbd                       */
      0,                                    /*    vccd                       */
                                            /* CONFIG REGISTER 3 VALUES      */
#ifndef __DRV_FOR_VESTA__
      XP_CONFIG3_INSEL_CI0,                 /*    insel                      */
#else
      0,
#endif

                                            /* CONTROL REGISTER 1 VALUES     */
      0,                                    /*    sbe                        */
      0,                                    /*    pbe                        */
      1,                                    /*    senbl                      */
      0,                                    /*    sdop                       */
                                            /* PACKET BUF LEVEL REG VALUES   */
      0x8,                                  /*    qpthres                    */
      0x4,                                  /*    apthres                    */
      0x8                                   /*    vpthres                    */
};


static XP_CONFIG_VALUES  xp1_configv;
static XP_CONFIG_VALUES  xp1_configv_reset = {
                                            /* CONFIG REGISTER 1 VALUES      */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    tsclkp                     */
      0,                                    /*    tsdp                       */
      0,                                    /*    tssp                       */
      0,                                    /*    tsep                       */
      0,                                    /*    tsvp                       */
      0,                                    /*    tssm                       */
      0,                                    /*    syncd                      */
      0,                                    /*    bbmode                     */
      2,                                    /*    syncl                      */
                                            /* CONFIG REGISTER 2 VALUES      */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      1,                                    /*    mwe                        */
      1,                                    /*    salign                     */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
                                            /* CONFIG REGISTER 3 VALUES      */
#ifndef __DRV_FOR_VESTA__
      XP_CONFIG3_INSEL_CI0,                 /*    insel                      */
#else
      0,
#endif

                                            /* CONTROL REGISTER 1 VALUES     */
      0,                                    /*    sbe                        */
      0,                                    /*    pbe                        */
      1,                                    /*    senbl                      */
      0,                                    /*    sdop                       */
                                            /* PACKET BUF LEVEL REG VALUES   */
      0x0,                                  /*    resv                       */
      0x0,                                  /*    resv                       */
      0x0                                   /*    resv                       */
};


static XP_CONFIG_VALUES  xp2_configv;
static XP_CONFIG_VALUES  xp2_configv_reset = {
                                            /* CONFIG REGISTER 1 VALUES      */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    tsclkp                     */
      0,                                    /*    tsdp                       */
      0,                                    /*    tssp                       */
      0,                                    /*    tsep                       */
      0,                                    /*    tsvp                       */
      0,                                    /*    tssm                       */
      0,                                    /*    syncd                      */
      0,                                    /*    bbmode                     */
      2,                                    /*    syncl                      */
                                            /* CONFIG REGISTER 2 VALUES      */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      1,                                    /*    mwe                        */
      1,                                    /*    salign                     */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
      0,                                    /*    resv                       */
                                            /* CONFIG REGISTER 3 VALUES      */
#ifndef __DRV_FOR_VESTA__
      XP_CONFIG3_INSEL_CI0,                 /*    insel                      */
#else
      0,
#endif
                                            /* CONTROL REGISTER 1 VALUES     */
      0,                                    /*    sbe                        */
      0,                                    /*    pbe                        */
      1,                                    /*    senbl                      */
      0,                                    /*    sdop                       */
                                            /* PACKET BUF LEVEL REG VALUES   */
      0x0,                                  /*    resv                       */
      0x0,                                  /*    resv                       */
      0x0                                   /*    resv                       */
};


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
|  trans_get_configure
+----------------------------------------------------------------------------*/
static void trans_get_configure(GLOBAL_RESOURCES *pGlobal)
{
    XP_CONFIG1_REG     config1;
    XP_CONFIG2_REG     config2;
#ifndef __DRV_FOR_VESTA__
    XP_CONFIG3_REG     config3;
#endif
    XP_CONTROL1_REG    control1;
    XP_PBUFLVL_REG     pbuflvl;
    XP_CONFIG_VALUES   *pXpConfigv = NULL;
    UINT32             flag;


    /*-----------------------------------------------------------------------+
    |  Get Configuration Values for specified Transport
    +------------------------------------------------------------------------*/
    switch(pGlobal->uDeviceIndex)
    {
      case 0:
          pXpConfigv = &xp0_configv;
          break;
      case 1:
          pXpConfigv = &xp1_configv;
          break;
      case 2:
          pXpConfigv = &xp2_configv;
          break;
      default:
          break;
    }

    /*-----------------------------------------------------------------------+
    |  Read Registers
    +------------------------------------------------------------------------*/
    flag = os_enter_critical_section();
    *(unsigned *)(void *)&config1  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG1);
    *(unsigned *)(void *)&config2  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG2);
#ifndef __DRV_FOR_VESTA__
    *(unsigned *)(void *)&config3  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG3);
#endif
    *(unsigned *)(void *)&control1 = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONTROL1);
    *(unsigned *)(void *)&pbuflvl  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_PBUFLVL);
    os_leave_critical_section(flag);

    /*-----------------------------------------------------------------------+
    |  Set Registers According to Stored Settings
    +------------------------------------------------------------------------*/
    pXpConfigv->vpu     = config1.vpu;
    pXpConfigv->apu     = config1.apu;
    pXpConfigv->tstoe   = config1.tstoe;
    pXpConfigv->tsclkp  = config1.tsclkp;
    pXpConfigv->tsdp    = config1.tsdp;
#ifdef __DRV_FOR_PALLAS__
    pXpConfigv->tssp    = config1.tssp;
    pXpConfigv->tsep    = config1.tsep;
    pXpConfigv->tssm    = config1.tssm;
#endif
    pXpConfigv->tsvp    = config1.tsvp;
    pXpConfigv->syncd   = config1.syncd;
    pXpConfigv->bbmode  = config1.bbmode;
    pXpConfigv->syncl   = config1.syncl;

    pXpConfigv->ved     = config2.ved;
    pXpConfigv->acpm    = config2.acpm;
    pXpConfigv->vcpm    = config2.vcpm;
    pXpConfigv->mwe     = config2.mwe;
    pXpConfigv->salign  = config2.salign;
    pXpConfigv->atsed   = config2.atsed;
    pXpConfigv->atbd    = config2.atbd;
    pXpConfigv->accd    = config2.accd;
    pXpConfigv->vtsed   = config2.vtsed;
    pXpConfigv->vtbd    = config2.vtbd;
    pXpConfigv->vccd    = config2.vccd;

#ifndef __DRV_FOR_VESTA__
    pXpConfigv->insel   = config3.insel;
#endif

    pXpConfigv->sbe     = control1.sbe;
    pXpConfigv->pbe     = control1.pbe;
    pXpConfigv->sdop    = control1.sdop;

    pXpConfigv->qpthres = pbuflvl.qpthres;
    pXpConfigv->apthres = pbuflvl.apthres;
    pXpConfigv->vpthres = pbuflvl.vpthres;

}

/*----------------------------------------------------------------------------+
|  trans_set_configure
+----------------------------------------------------------------------------*/
static void trans_set_configure(GLOBAL_RESOURCES *pGlobal)
{
    XP_CONFIG1_REG     config1;
    XP_CONFIG2_REG     config2;
#ifndef __DRV_FOR_VESTA__
    XP_CONFIG3_REG     config3;
#endif
    XP_CONTROL1_REG    control1;
    XP_PBUFLVL_REG     pbuflvl;
    XP_CONFIG_VALUES   *pXpConfigv = NULL;
    UINT32         flag;


    /*-----------------------------------------------------------------------+
    |  Get Configuration Values for specified Transport
    +------------------------------------------------------------------------*/
    switch(pGlobal->uDeviceIndex)
    {
      case 0:
          pXpConfigv = &xp0_configv;
          break;
      case 1:
          pXpConfigv = &xp1_configv;
          break;
      case 2:
          pXpConfigv = &xp2_configv;
          break;
      default:
          break;
    }

    /*-----------------------------------------------------------------------+
    |  Read Registers
    +------------------------------------------------------------------------*/
    flag = os_enter_critical_section();
    *(unsigned *)(void *)&config1  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG1);
    *(unsigned *)(void *)&config2  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG2);
#ifndef __DRV_FOR_VESTA__
    *(unsigned *)(void *)&config3  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG3);
#endif
    *(unsigned *)(void *)&control1 = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONTROL1);
    *(unsigned *)(void *)&pbuflvl  = xp_atom_dcr_read(pGlobal->uDeviceIndex,XP_DCR_ADDR_PBUFLVL);
    os_leave_critical_section(flag);

    /*-----------------------------------------------------------------------+
    |  Set Registers According to Stored Settings
    +------------------------------------------------------------------------*/
    config1.vpu      = pXpConfigv->vpu;
    config1.apu      = pXpConfigv->apu;
    config1.tstoe    = pXpConfigv->tstoe;
    config1.tsclkp   = pXpConfigv->tsclkp;
    config1.tsdp     = pXpConfigv->tsdp;
#ifdef __DRV_FOR_PALLAS__
    config1.tssp     = pXpConfigv->tssp;
    config1.tsep     = pXpConfigv->tsep;
    config1.tssm     = pXpConfigv->tssm;
#endif
    config1.tsvp     = pXpConfigv->tsvp;
    config1.syncd    = pXpConfigv->syncd;
    config1.bbmode   = pXpConfigv->bbmode;
    config1.syncl    = pXpConfigv->syncl;

    config2.ved      = pXpConfigv->ved;
    config2.acpm     = pXpConfigv->acpm;
    config2.vcpm     = pXpConfigv->vcpm;
    config2.mwe      = pXpConfigv->mwe;
    config2.salign   = pXpConfigv->salign;
    config2.atsed    = pXpConfigv->atsed;
    config2.atbd     = pXpConfigv->atbd;
    config2.accd     = pXpConfigv->accd;
    config2.vtsed    = pXpConfigv->vtsed;
    config2.vtbd     = pXpConfigv->vtbd;
    config2.vccd     = pXpConfigv->vccd;

#ifndef __DRV_FOR_VESTA__
    config3.insel    = pXpConfigv->insel;
#endif

    control1.sbe     = pXpConfigv->sbe;
    control1.pbe     = pXpConfigv->pbe;
    control1.sdop    = pXpConfigv->sdop;

    pbuflvl.qpthres  = pXpConfigv->qpthres;
    pbuflvl.apthres  = pXpConfigv->apthres;
    pbuflvl.vpthres  = pXpConfigv->vpthres;

    /*-----------------------------------------------------------------------+
    |  Write Registers with Control 1 Register, Bit SE, Set to 0
    +------------------------------------------------------------------------*/
    control1.senbl   = 0;
    flag = os_enter_critical_section();
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG1,  *(unsigned *)(void *)&config1);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG2,  *(unsigned *)(void *)&config2);
#ifndef __DRV_FOR_VESTA__
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG3,  *(unsigned *)(void *)&config3);
#endif
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONTROL1, *(unsigned *)(void *)&control1);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PBUFLVL,  *(unsigned *)(void *)&pbuflvl);
    os_leave_critical_section(flag);

    /*-----------------------------------------------------------------------+
    |  Set Control 1 Register, Bit SE, According to Settings
    +------------------------------------------------------------------------*/
    control1.senbl   = pXpConfigv->senbl;
    flag = os_enter_critical_section();
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONTROL1, *(unsigned *)(void *)&control1);
    os_leave_critical_section(flag);
}

/*----------------------------------------------------------------------------+
|  trans_reset
+----------------------------------------------------------------------------*/
static short trans_reset(GLOBAL_RESOURCES *pGlobal)
{

    short i = 0;
    int j;
    XP_CONTROL1_REG control1;
    UINT32 flag;

    /*------------------------------------------------------------------------+
    |  Init configuration vector to reset value
    +------------------------------------------------------------------------*/
    switch(pGlobal->uDeviceIndex)
    {

      case 0:
        memcpy(&xp0_configv, &xp0_configv_reset, sizeof(XP_CONFIG_VALUES));
        break;
      case 1:
        memcpy(&xp1_configv, &xp1_configv_reset, sizeof(XP_CONFIG_VALUES));
        break;
      case 2:
        memcpy(&xp2_configv, &xp2_configv_reset, sizeof(XP_CONFIG_VALUES));
        break;
      default:
        break;
    }

    /*------------------------------------------------------------------------+
    |  Soft Reset Transport
    +------------------------------------------------------------------------*/
    memset(&control1, 0, sizeof(control1));
    control1.swrst = 1;
    flag = os_enter_critical_section();
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONTROL1, *(unsigned *)(void *)&control1);

    while (xp_atom_dcr_read(pGlobal->uDeviceIndex, XP_DCR_ADDR_CONTROL1) != 0)
    {
      for(j=0;j<100000;j++)
        ;

      if (++i > 200)
      {
        os_leave_critical_section(flag);
        return(-1);
      }
    }

    /*-----------------------------------------------------------------------+
    |  Clear all registers that are at an unpredictable state after reset
    +------------------------------------------------------------------------*/
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG1,   0);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONTROL1,  0);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_FEIMASK,   0);

#ifndef __DRV_FOR_VESTA__
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG3,   0);
#endif

    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PCRSTCT,   0);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_STCCOMP,   0);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_DSIMASK,   0);

#ifndef __DRV_FOR_VULCAN__
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_AXENABLE,  0);
#endif

    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONFIG2,   0);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PBUFLVL,   0);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_INTMASK,   0);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_PLBCNFG,   0x00030010);
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_QSTATMASK, 0);

    for(i=0; i < XP_CHANNEL_COUNT; i++) {
      xp_atom_dcr_reset_queue(pGlobal->uDeviceIndex,i);
    }

    os_leave_critical_section(flag);

    /*-----------------------------------------------------------------------+
    |  Configure XP Registers
    +------------------------------------------------------------------------*/

    trans_set_configure(pGlobal);
    return(0);
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
/*----------------------------------------------------------------------------+
|  xp0_init
+----------------------------------------------------------------------------*/
short xp_osi_init(GLOBAL_RESOURCES *pGlobal)            /* memory available for DRAM queues     */
{
    short rc = 0;

    PDEBUG("Entering into xp_os_init\n");

    os_disable_irq(XP_IRQ);

    rc = trans_reset(pGlobal);

    if(rc == 0)
    {
      rc = xp_osi_interrupt_init(pGlobal);
    }

    if(rc == 0)
    {
      rc = xp_osi_channel_init(pGlobal);
    }

    if(rc == 0)
    {
      rc = xp_osi_queue_init(pGlobal);
    }

    if(rc == 0)
    {
      rc = xp_osi_filter_init(pGlobal);
    }

    if(rc == 0)
    {
      os_enable_irq(XP_IRQ);
    }

    if(pGlobal->uDeviceIndex == 0)
    {
      xp_osi_pcr_delay((45000/1000) * 60 * -1);
      if(rc == 0)
      {
        rc = xp_osi_clk_init(pGlobal);
      }
      PDEBUG("After xp_osi_clk_init(pGlobal), rc = %d\n",rc);
      if(rc == 0)
      {
        rc = xp_osi_clk_start(pGlobal);
      }
      if(rc == 0)
      {
        rc = xp_osi_cchan_init(pGlobal);
      }
      if(rc == 0)
      {
        rc = xp_osi_pcr_init(pGlobal);
      }
    }


    return(rc);

}

/*----------------------------------------------------------------------------+
|  xp0_get_config_values
+----------------------------------------------------------------------------*/
void xp_osi_get_config_values (GLOBAL_RESOURCES *pGlobal,XP_CONFIG_VALUES *configv_parm)
{

    XP_CONFIG_VALUES   *pXpConfigv = NULL;


    trans_get_configure(pGlobal);

    switch(pGlobal->uDeviceIndex)
    {
      case 0:
        pXpConfigv = &xp0_configv;
        break;
      case 1:
        pXpConfigv = &xp1_configv;
        break;
      case 2:
        pXpConfigv = &xp2_configv;
        break;
      default:
        break;
    }

    memcpy(configv_parm, pXpConfigv, sizeof(XP_CONFIG_VALUES));
}

/*----------------------------------------------------------------------------+
|  xp0_set_config_values
+----------------------------------------------------------------------------*/
void xp_osi_set_config_values (GLOBAL_RESOURCES *pGlobal, XP_CONFIG_VALUES *configv_parm)
{
    XP_CONFIG_VALUES   *pXpConfigv = NULL;


    switch(pGlobal->uDeviceIndex)
    {
      case 0:
        pXpConfigv = &xp0_configv;
        break;
      case 1:
        pXpConfigv = &xp1_configv;
        break;
      case 2:
        pXpConfigv = &xp2_configv;
        break;
      default:
        break;
    }

    memcpy(pXpConfigv, configv_parm, sizeof(XP_CONFIG_VALUES));
    trans_set_configure(pGlobal);
}


//The following 3 function are added by lingh for TS recoder
/*----------------------------------------------------------------------------+
|  xp0_osi_set_parse_bypass_mode
+----------------------------------------------------------------------------*/
void xp_osi_set_parse_bypass_mode(GLOBAL_RESOURCES *pGlobal)
{
    short             i = 0;
    XP_CONTROL1_REG   control1;
    int               j;
    UINT32            flag;
    ULONG             reg;
    int rc = 0;

    /*------------------------------------------------------------------------+
    |  Soft Reset Transport
    +------------------------------------------------------------------------*/
    memset(&control1, 0, sizeof(control1));
    control1.swrst = 1;

    flag = os_enter_critical_section();
    xp_atom_dcr_write(pGlobal->uDeviceIndex,XP_DCR_ADDR_CONTROL1, *(unsigned *)(void *)&control1);
    while (xp_atom_dcr_read(pGlobal->uDeviceIndex, XP_DCR_ADDR_CONTROL1) != 0)
    {
      for(j=0;j<1000000;j++)
        ;

      if (++i > 200)
      {
        os_leave_critical_section(flag);
        return;
      }
    }
    control1.swrst = 0;

    if(rc == 0)
    {
      xp_osi_queue_de_init(pGlobal);
      rc = xp_osi_queue_init(pGlobal);
    }


    PDEBUG("After xp_osi_queue_init(pGlobal), rc= %d\n",rc);

    if(rc == 0)
    {
      rc = xp_osi_filter_init(pGlobal);
    }

    reg = xp_atom_dcr_read(pGlobal->uDeviceIndex, XP_DCR_ADDR_INTMASK) & 0xc9c3ffff;
    xp_atom_dcr_write(pGlobal->uDeviceIndex, XP_DCR_ADDR_INTMASK, reg);
    reg = xp_atom_dcr_read(pGlobal->uDeviceIndex, XP_DCR_ADDR_QSTATMASK) & 0x0000e200;
    xp_atom_dcr_write(pGlobal->uDeviceIndex, XP_DCR_ADDR_QSTATMASK, reg);
    reg = xp_atom_dcr_read(pGlobal->uDeviceIndex, XP_DCR_ADDR_FEIMASK) & 0x00006ff7;
    xp_atom_dcr_write(pGlobal->uDeviceIndex, XP_DCR_ADDR_FEIMASK, reg);
    os_leave_critical_section(flag);
}

/*----------------------------------------------------------------------------+
|  xp0_osi_start_parse_bypass
+----------------------------------------------------------------------------*/
void xp_osi_start_parse_bypass(GLOBAL_RESOURCES *pGlobal)
{
    XP_CONTROL1_REG    control1;
    UINT32             flag;
    ULONG              reg;

    flag = os_enter_critical_section();
    reg = xp_atom_dcr_read(pGlobal->uDeviceIndex, XP_DCR_ADDR_CONTROL1);
    control1 = *(XP_CONTROL1_REG *)(void *)&reg;
    ulSavedCtlReg = reg;
    control1.pbe = 1;
    control1.senbl = 1;
    xp_atom_dcr_write(pGlobal->uDeviceIndex, XP_DCR_ADDR_CONTROL1, *(unsigned *)(void *)&control1);
    os_leave_critical_section(flag);
}

/*----------------------------------------------------------------------------+
|  xp0_osi_stop_parse_bypass
+----------------------------------------------------------------------------*/
void xp_osi_stop_parse_bypass(GLOBAL_RESOURCES *pGlobal)
{
    XP_CONTROL1_REG    control1;
    UINT32             flag;

    flag = os_enter_critical_section();
    control1 = *(XP_CONTROL1_REG*)(void *)&ulSavedCtlReg;

    xp_atom_dcr_write(pGlobal->uDeviceIndex, XP_DCR_ADDR_CONTROL1, *(unsigned *)(void *)&control1);
    os_leave_critical_section(flag);
}
