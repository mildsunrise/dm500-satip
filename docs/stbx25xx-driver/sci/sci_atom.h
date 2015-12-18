/*---------------------------------------------------------------------------+
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
| Author:    Mike Lepore
| Component: sci
| File:      sci_atom.c
| Purpose:   Atom function prototype for smart card interface control
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/13/2001  MAL, Zongwei Liu  Added EMV2000 support
+----------------------------------------------------------------------------*/

#ifndef _sci_atom_h_
#define _sci_atom_h_

#include "sci_local.h"
#include "os/os-types.h"

/****************************************************************************
** Function:    sci_atom_assign_reg_address
**
** Purpose:     assign the register addresses
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_assign_reg_address(ULONG sci_id);

/*****************************************************************************
** Function:    sci_atom_reg_init
**
** Purpose:     Initialize the registers of Smart Card Interface
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_reg_init(ULONG sci_id);

/****************************************************************************
** Function:    sci_atom_set_chip_connection
**
** Purpose:     Configure the cic0_cr register.
****************************************************************************/
void sci_atom_set_chip_connection(void);

/****************************************************************************
** Name:        sci_atom_clear_UICSR
**
** Purpose:     clear the UIC status
****************************************************************************/
void sci_atom_clear_UICSR(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_get_interrupt_status
**
** Purpose:     get the status of interrrupt
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns      the value of interrupt status register
****************************************************************************/
USHORT sci_atom_get_interrupt_status(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_clear_interrupt_status
**
** Purpose:     clear the specified bits of interrrupt
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              bits:   bit mask
****************************************************************************/
void sci_atom_clear_interrupt_status(ULONG sci_id, USHORT bits);

/****************************************************************************
** Name:        sci_atom_check_rx_data_pending
**
** Purpose:     check any pending recieve data 
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns      0: if no data available
**              1: if any recieve data is pending
****************************************************************************/
INT sci_atom_check_rx_data_pending(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_check_tx_fifo_half_full
**
** Purpose:     check the status of TX FIFO if under half full
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns      0: if FIFO <  1/2 full
**              1: if FIFO >= 1/2 full
****************************************************************************/
INT sci_atom_check_tx_fifo_half_full(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_enable_tx_interrupts
**
** Purpose:     enable Tx related interrupts
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_enable_tx_interrupts(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_enable_tx_threshold_interrupts
**
** Purpose:     enable Tx threshold interrupts
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_enable_tx_threshold_interrupts(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_disable_tx_threshold_interrupts
**
** Purpose:     disable Tx threshold interrupts
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_disable_tx_threshold_interrupts(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_set_rx_threshold_interrupts
**
** Purpose:     Set Rx threshold interrupts according to num_bytes
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              num_bytes: number of bytes to be read
****************************************************************************/
void sci_atom_set_rx_threshold_interrupts(ULONG sci_id, ULONG num_bytes);

/****************************************************************************
** Function:    sci_atom_HW_reset
**
** Purpose:     Initiate a reset (enter atr state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_HW_reset(ULONG sci_id, SCI_RESET_TYPE reset_type);

/*****************************************************************************
** Function:    sci_atom_clock_stop
**
** Purpose:     Stop the SCI/Smart Card clock at a given polarity.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_clock_stop(ULONG sci_id);

/*****************************************************************************
** Function:    sci_atom_clock_start
**
** Purpose:     Start the SCI/Smart Card clock.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_clock_start(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_is_card_present
**
** Purpose:     This checks the state of the detect line and the active
**              polarity of the card detect line, to determine if a card
**              is present in the reader.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     0: card is not present
**              1: card is present
****************************************************************************/
INT sci_atom_is_card_present(ULONG sci_id);

/****************************************************************************
** Name:        sci_atom_is_HW_activated
**
** Purpose:     This determines if the SCI hardware is activated.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     0: hardware is not actived
**              1: hardware is actived
*****************************************************************************/
INT sci_atom_is_HW_activated(ULONG sci_id);

/*****************************************************************************
** Function:    sci_atom_deactivate
**
** Purpose:     Deactivate the interface (enter deac state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_deactivate(ULONG sci_id);

/*****************************************************************************
** Function:    sci_atom_tx_start
**
** Purpose:     set transmit (rx) registers.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              num_bytes: number of bytes to write from out buffer
*****************************************************************************/
void sci_atom_tx_start(ULONG sci_id, ULONG num_bytes);

/*****************************************************************************
** Function:    sci_atom_rx_start
**
** Purpose:     set recieve (rx) registers.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_rx_start(ULONG sci_id);

/*****************************************************************************
** Function:    sci_atom_write_fifo
**
** Purpose:     Transmit certain numbers of bytes to the FIFO
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              num_bytes: number of bytes to be witten
**              p_buffer_addr: intput pointer of the write buffer's address
*****************************************************************************/
void sci_atom_write_fifo(ULONG sci_id, ULONG num_bytes, UCHAR **p_buffer_addr);

/****************************************************************************
** Name:        sci_atom_read_fifo
**
** Purpose:     Read any pending data from FIFO
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_buffer_addr: intput pointer of the read buffer's address
*****************************************************************************/
void sci_atom_read_fifo(ULONG sci_id, UCHAR **p_buffer_addr);

/*****************************************************************************
** Function:    sci_atom_set_para_T
**
** Purpose:     Set the current Smart Card parameters of T.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_T(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

/*****************************************************************************
** Function:    sci_atom_set_para_f
**
** Purpose:     Set the current Smart Card parameters of frequency.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_f(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

/*****************************************************************************
** Function:    sci_atom_set_para_ETU
**
** Purpose:     Set the current Smart Card parameters of ETU.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_ETU(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

/*****************************************************************************
** Function:    sci_atom_set_para_WWT
**
** Purpose:     Set the current Smart Card parameters of WWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_WWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

/*****************************************************************************
** Function:    sci_atom_set_para_CWT
**
** Purpose:     Set the current Smart Card parameters of CWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_CWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

/*****************************************************************************
** Function:    sci_atom_set_para_BWT
**
** Purpose:     Set the current Smart Card parameters of BWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_BWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

/*****************************************************************************
** Function:    sci_atom_set_para_EGT
**
** Purpose:     Set the current Smart Card parameters of EGT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_EGT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

/*****************************************************************************
** Function:    sci_atom_set_para_check
**
** Purpose:     Set the current Smart Card parameters of check.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_check(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);

/****************************************************************************
** Name:        sci_atom_emv_check
**
** Purpose:     check if this chip support EMV mode
**
** Returns      0: if NO EMV mode supported
**              1: if EMV mode support
****************************************************************************/
INT sci_atom_emv_check(void);

/*****************************************************************************
** Function:    sci_atom_set_mode_emv2000
**
** Purpose:     Set the current Smart Card mode of emv2000.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_modes: input pointer to Smart Card modes
*****************************************************************************/
void sci_atom_set_mode_emv2000(ULONG sci_id, SCI_MODES *p_sci_modes);

#endif /* _sci_atom_h_ */
