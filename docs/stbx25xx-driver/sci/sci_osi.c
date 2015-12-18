/*-----------------------------------------------------------------------------+
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
|       COPYRIGHT   I B M   CORPORATION 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------+
| Author:    Mike Lepore, Zongwei Liu
| Component: sci
| File:      sci_osi.c
| Purpose:   The OS-independency functions of Smart Card Interface
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/13/2001  MAL, Zongwei Liu  Move init, uninit, reset, read, write and irq
|                               handler to the OS-dependent layer.
|                               Merge sci_osi.c and sci_osi_local.c.
| 12/13/2001  MAL, Zongwei Liu  Added EMV2000 support and made several changes
|                               to improve PIO efficiency.
| 04/25/2003  Detrick, Mark     Changed sci_osi_set_modes to print a message
|                               instead of returning an error when DMA mode
|                               (which is not supported) is requested.
+----------------------------------------------------------------------------*/

#include "os/os-types.h"
#include "os/os-sync.h"
#include "hw/hardware.h"

#include "sci_osi.h"
#include "sci_atom.h"

#include "os/drv_debug.h"

extern SCI_CONTROL_BLOCK sci_cb[SCI_NUMBER_OF_CONTROLLERS];
extern ULONG sci_driver_init;
extern SCI_DRV_MODES sci_drv_modes;

/* Local function calls for set parameters */
SCI_ERROR sci_osi_set_para_T (ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);
SCI_ERROR sci_osi_set_para_f (ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);
SCI_ERROR sci_osi_set_para_ETU (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_WWT (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_CWT (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_BWT (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_EGT (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_CLK_p (ULONG sci_id,
                                  SCI_PARAMETERS *p_sci_parameters
);

SCI_ERROR sci_osi_set_para_check (ULONG sci_id,
                                  SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_class (ULONG sci_id,
                                  SCI_PARAMETERS *p_sci_parameters
);

/*****************************************************************************
** Function:    sci_osi_clock_stop
**
** Purpose:     Stop the SCI/Smart Card clock at a given polarity.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
**              SCI_ERROR_CLOCK_STOP_DISABLED: if clock stop is disabled
*****************************************************************************/
SCI_ERROR sci_osi_clock_stop(ULONG sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if(sci_id < SCI_NUMBER_OF_CONTROLLERS)
        {
            if(sci_osi_is_card_activated(sci_id) == 1)
            {
                /* check for clock stop enabled */
                if(sci_cb[sci_id].sci_parameters.clock_stop_polarity !=
                   SCI_CLOCK_STOP_DISABLED)
                {
                    sci_atom_clock_stop(sci_id);
                }
                else
                {
                    rc = SCI_ERROR_CLOCK_STOP_DISABLED;
                }
            }
            else
            {
                rc = SCI_ERROR_CARD_NOT_ACTIVATED;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_clock_start
**
** Purpose:     Start the SCI/Smart Card clock.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osi_clock_start(unsigned long sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if(sci_id < SCI_NUMBER_OF_CONTROLLERS)
        {
            if(sci_osi_is_card_activated(sci_id) == 1)
            {
                /* start the clock */
                sci_atom_clock_start(sci_id);
            }
            else
            {
                rc = SCI_ERROR_CARD_NOT_ACTIVATED;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/****************************************************************************
** Name:        sci_osi_is_card_activated
**
** Purpose:     This determines if the SCI/Smart Card is activated.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     0: card is not present
**              1: card is present
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
****************************************************************************/
SCI_ERROR sci_osi_is_card_activated (ULONG sci_id)
{
    SCI_ERROR rc = 0;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        /* check both the driver state and the h/w state (VCC) */
        if((sci_cb[sci_id].state == SCI_STATE_RX) ||
           (sci_cb[sci_id].state == SCI_STATE_TX))
        {
            /* driver is in activated state, now check h/w */
            if((rc = sci_atom_is_HW_activated(sci_id)) == 0)
            {
                /* h/w is not activated for some reason-      */
                /* deactivate to get driver and h/w "in sync" */
                rc = SCI_ERROR_CARD_NOT_ACTIVATED;
            }
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    PDEBUG("card[%d] exit:returns %d\n", (UINT) sci_id, rc);

    return(rc);
}

/****************************************************************************
** Name:        sci_osi_is_card_present
**
** Purpose:     Determine if a card is present in the reader.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     0: card is not present
**              1: card is present
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
****************************************************************************/
SCI_ERROR sci_osi_is_card_present(ULONG sci_id)
{
    SCI_ERROR rc;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        rc = sci_atom_is_card_present(sci_id);
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    PDEBUG("card[%d] exit:returns %d\n", (UINT) sci_id, rc);

    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_modes
**
** Purpose:     Set the current Smart Card driver modes.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_modes: input pointer to Smart Card modes
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_modes is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_modes(ULONG sci_id, SCI_MODES *p_sci_modes)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if((p_sci_modes != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
        {
            if((p_sci_modes->emv2000 == 0) || (p_sci_modes->emv2000 == 1))
            {
                if((p_sci_modes->emv2000 == 1) && 
                   (sci_drv_modes.emv_supported == 0))
                {
                    sci_cb[sci_id].sci_modes.emv2000 = 0;
                    rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
                }
                else
                {
                    sci_cb[sci_id].sci_modes.emv2000 = p_sci_modes->emv2000;
                }
            }
            else
            {
                rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
            }

            if((p_sci_modes->dma == 0) || (p_sci_modes->dma == 1))
            {
                /*sci_cb[sci_id].sci_modes.dma = p_sci_modes->dma;*/
                /* not yet supported */
                if(p_sci_modes->dma == 1)
                {
                    printk("DMA mode is not supported\n");
                    //rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
                }
            }
            else
            {
                rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
            }

            if((p_sci_modes->man_act == 0) || (p_sci_modes->man_act == 1))
            {
                sci_cb[sci_id].sci_modes.man_act = p_sci_modes->man_act;
            }
            else
            {
                rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
            }

            if (p_sci_modes->rw_mode < 4)
            {
                sci_cb[sci_id].sci_modes.rw_mode = p_sci_modes->rw_mode;
            }
            else
            {
                rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }
    
    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/****************************************************************************
** Function:    sci_osi_get_modes
**
** Purpose:     Retrieve the current Smart Card modes.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              sci_get_modes: output pointer to Smart Card modes
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_modes is zero.
*****************************************************************************/
SCI_ERROR sci_osi_get_modes(ULONG sci_id, SCI_MODES *p_sci_modes)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if((p_sci_modes != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
        {
            p_sci_modes->emv2000 = sci_cb[sci_id].sci_modes.emv2000;
            p_sci_modes->dma     = sci_cb[sci_id].sci_modes.dma;
            p_sci_modes->man_act = sci_cb[sci_id].sci_modes.man_act;
            p_sci_modes->rw_mode = sci_cb[sci_id].sci_modes.rw_mode;
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;

        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_parameters
**
** Purpose:     Set the current Smart Card parameters. This function calls
**              set_parameters(), which is also called internally.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osi_set_parameters(ULONG sci_id, 
                                 SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc=SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if((p_sci_parameters != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
        {
            if(sci_osi_is_card_activated(sci_id) == 1)
            {
                rc = sci_osi_set_para(sci_id, p_sci_parameters);
            }
            else
            {
                rc = SCI_ERROR_CARD_NOT_ACTIVATED;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/****************************************************************************
** Function:    sci_osi_get_parameters
**
** Purpose:     Retrieve the current Smart Card parameters.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: output pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osi_get_parameters(ULONG sci_id, 
                                 SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if((p_sci_parameters != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
        {
            if(sci_atom_is_HW_activated(sci_id) == 1)
            {
                p_sci_parameters->T   = sci_cb[sci_id].sci_parameters.T;
                p_sci_parameters->f   = sci_cb[sci_id].sci_parameters.f;
                p_sci_parameters->ETU = sci_cb[sci_id].sci_parameters.ETU;
                p_sci_parameters->WWT = sci_cb[sci_id].sci_parameters.WWT;
                p_sci_parameters->CWT = sci_cb[sci_id].sci_parameters.CWT;
                p_sci_parameters->BWT = sci_cb[sci_id].sci_parameters.BWT;
                p_sci_parameters->EGT = sci_cb[sci_id].sci_parameters.EGT;
                p_sci_parameters->clock_stop_polarity =
                    sci_cb[sci_id].sci_parameters.clock_stop_polarity;
                p_sci_parameters->check =
                    sci_cb[sci_id].sci_parameters.check;
                p_sci_parameters->P = sci_cb[sci_id].sci_parameters.P;
                p_sci_parameters->I = sci_cb[sci_id].sci_parameters.I;
                p_sci_parameters->U = sci_cb[sci_id].sci_parameters.U;
            }
            else
            {
                rc = SCI_ERROR_CARD_NOT_ACTIVATED;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_tx_start
**
** Purpose:     Enter transmit (tx) state.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              num_bytes: number of bytes to write from out buffer
*****************************************************************************/
void sci_osi_tx_start(ULONG sci_id, ULONG num_bytes)
{
    /* initialize control block values */
    sci_cb[sci_id].state = SCI_STATE_TX;
    sci_cb[sci_id].error = SCI_ERROR_OK;

    /* reset pointers to start of buffer */
    sci_cb[sci_id].p_read = sci_cb[sci_id].buffer;
    sci_cb[sci_id].p_write = sci_cb[sci_id].buffer;

    sci_atom_tx_start(sci_id, num_bytes);
}

/*****************************************************************************
** Function:    sci_osi_rx_start
**
** Purpose:     Enter recieve(rx) state.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_osi_rx_start(ULONG sci_id)
{
    sci_cb[sci_id].state = SCI_STATE_RX;
    /* reset pointers to start of buffer */
    sci_cb[sci_id].p_read  = sci_cb[sci_id].buffer;
    sci_cb[sci_id].p_write = sci_cb[sci_id].buffer;

    sci_cb[sci_id].first_rx       = 0;
    sci_cb[sci_id].rx_complete    = 0;
    sci_cb[sci_id].bytes_expected = -1;

    sci_atom_rx_start(sci_id);
}

/*****************************************************************************
** Function:    sci_osi_set_para
**
** Purpose:     Set the current Smart Card parameters.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if((p_sci_parameters != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
    {
        rc = sci_osi_set_para_T    (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_f    (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_ETU  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_WWT  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_CWT  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_BWT  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_EGT  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_CLK_p(sci_id, p_sci_parameters);
        rc = sci_osi_set_para_check(sci_id, p_sci_parameters);
        rc = sci_osi_set_para_class(sci_id, p_sci_parameters);

        /* programming not supported, just set the values */
        sci_cb[sci_id].sci_parameters.P = p_sci_parameters->P;
        sci_cb[sci_id].sci_parameters.I = p_sci_parameters->I;
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_T
**
** Purpose:     Set the current Smart Card parameters of T.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
**************************************************************************/
SCI_ERROR sci_osi_set_para_T(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    /* set the protocol T=0 or 1 of sci */
    if((p_sci_parameters->T == 0) || (p_sci_parameters->T == 1))
    {
        if(sci_cb[sci_id].sci_parameters.T != p_sci_parameters->T)
        {
            k_state = os_enter_critical_section();
            sci_atom_set_para_T(sci_id, p_sci_parameters);
            os_leave_critical_section(k_state);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_f
**
** Purpose:     Set the current Smart Card parameters of f.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_f(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    /* set the f of sci */
    if((p_sci_parameters->f >= SCI_MIN_F) &&
       (p_sci_parameters->f <= SCI_MAX_F) &&
       (p_sci_parameters->f <= (__STB_SYS_CLK / 2)))
    {
        if(sci_cb[sci_id].sci_parameters.f != p_sci_parameters->f)
        {
            k_state = os_enter_critical_section();
            sci_atom_set_para_f(sci_id, p_sci_parameters);
            os_leave_critical_section(k_state);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_ETU
**
** Purpose:     Set the current Smart Card parameters of ETU.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_ETU(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    if((p_sci_parameters->ETU >= SCI_MIN_ETU) &&
       (p_sci_parameters->ETU <= SCI_MAX_ETU))
    {
        if(sci_cb[sci_id].sci_parameters.ETU != p_sci_parameters->ETU)
        {
            k_state = os_enter_critical_section();
            sci_atom_set_para_ETU(sci_id, p_sci_parameters);
            os_leave_critical_section(k_state);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_WWT
**
** Purpose:     Set the current Smart Card parameters of WWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_WWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    if((p_sci_parameters->WWT >= SCI_MIN_WWT) &&
       (p_sci_parameters->WWT <= SCI_MAX_WWT))
    {
        if(sci_cb[sci_id].sci_parameters.WWT != p_sci_parameters->WWT)
        {
            k_state = os_enter_critical_section();
            sci_atom_set_para_WWT(sci_id, p_sci_parameters);
            os_leave_critical_section(k_state);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_CWT
**
** Purpose:     Set the current Smart Card parameters of CWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_CWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    if((p_sci_parameters->CWT >= SCI_MIN_CWT) &&
       (p_sci_parameters->CWT <= SCI_MAX_CWT))
    {
        if(sci_cb[sci_id].sci_parameters.CWT != p_sci_parameters->CWT)
        {
            k_state = os_enter_critical_section();
            sci_atom_set_para_CWT(sci_id, p_sci_parameters);
            os_leave_critical_section(k_state);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_BWT
**
** Purpose:     Set the current Smart Card parameters of BWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_BWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    if((p_sci_parameters->BWT >= SCI_MIN_BWT) &&
       (p_sci_parameters->BWT <= SCI_MAX_BWT))
    {
        if(sci_cb[sci_id].sci_parameters.BWT != p_sci_parameters->BWT)
        {
            k_state = os_enter_critical_section();
            sci_atom_set_para_BWT(sci_id, p_sci_parameters);
            os_leave_critical_section(k_state);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_EGT
**
** Purpose:     Set the current Smart Card parameters of EGT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_EGT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    if((p_sci_parameters->EGT >= SCI_MIN_EGT) &&
       (p_sci_parameters->EGT <= SCI_MAX_EGT))
    {
        if(sci_cb[sci_id].sci_parameters.EGT != p_sci_parameters->EGT)
        {
            k_state = os_enter_critical_section();
            sci_atom_set_para_EGT(sci_id, p_sci_parameters);
            os_leave_critical_section(k_state);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_CLK_p
**
** Purpose:     Set the current Smart Card parameters of clock stop polarity.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_CLK_p(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if((p_sci_parameters->clock_stop_polarity == 0) ||
       (p_sci_parameters->clock_stop_polarity == 1))
    {
        if(sci_cb[sci_id].sci_parameters.clock_stop_polarity !=
            p_sci_parameters->clock_stop_polarity)
        {
            sci_cb[sci_id].sci_parameters.clock_stop_polarity =
                p_sci_parameters->clock_stop_polarity;
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_check
**
** Purpose:     Set the current Smart Card parameters of check.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_check(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    if((p_sci_parameters->check == 1) || (p_sci_parameters->check == 2))
    {
        if(sci_cb[sci_id].sci_parameters.check != p_sci_parameters->check)
        {
            k_state=os_enter_critical_section();
            sci_atom_set_para_check(sci_id, p_sci_parameters);
            os_leave_critical_section(k_state);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_class
**
** Purpose:     Set the current Smart Card parameters of class.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_class(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if(p_sci_parameters->U != SCI_CLASS_B)
    {
        /* the SCI is class A- class A and class AB cards OK */
        sci_cb[sci_id].sci_parameters.U = p_sci_parameters->U;
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}


