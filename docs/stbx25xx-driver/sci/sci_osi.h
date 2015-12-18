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
| Author:    Mike Lepore
| Component: sci
| File:      sci_osi.h
| Purpose:   The OS-independency function prototype of Smart Card Interface
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/13/2001  MAL, Zongwei Liu  Move init, uninit, reset, read, write and irq
|                               handler to the OS-dependent layer
|                               Merge sci_osi.c and sci_osi_local.c.
+----------------------------------------------------------------------------*/

#ifndef _sci_osi_h_
#define _sci_osi_h_

#include "sci_local.h"

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
SCI_ERROR sci_osi_clock_stop(ULONG sci_id);

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
SCI_ERROR sci_osi_clock_start(ULONG sci_id);

/****************************************************************************
** Name:        sci_osi_is_card_present
**
** Purpose:     This checks the state of the detect line and the active
**              polarity of the card detect line, to determine if a card
**              is present in the reader.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     0: card is not present
**              1: card is present
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
****************************************************************************/
SCI_ERROR sci_osi_is_card_present(ULONG sci_id);

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
SCI_ERROR sci_osi_is_card_activated(ULONG sci_id);

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
SCI_ERROR sci_osi_set_modes(ULONG sci_id, SCI_MODES * p_sci_modes);

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
SCI_ERROR sci_osi_get_modes(ULONG sci_id, SCI_MODES *p_sci_modes);

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
                                 SCI_PARAMETERS *p_sci_parameters
);

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
                                 SCI_PARAMETERS *p_sci_parameters
);

/*****************************************************************************
** Function:    sci_osi_tx_start
**
** Purpose:     Enter transmit (tx) state.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              num_bytes: number of bytes to write from out buffer
*****************************************************************************/
void sci_osi_tx_start (ULONG sci_id, ULONG num_bytes);

/*****************************************************************************
** Function:    sci_osi_rx_start
**
** Purpose:     Enter recieve(rx) state.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_osi_rx_start (ULONG sci_id);

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
SCI_ERROR sci_osi_set_para (ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

#endif /* _sci_osi_h_ */
