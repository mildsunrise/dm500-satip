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
| File:      sci_osd.h
| Purpose:   The OS-dependency function prototype of Smart Card Interface
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/13/2001  MAL, Zongwei Liu  Added sci_osd_init() parameters to set detect 
|                               and Vcc enable active polarities 
|                               (which are board dependent).
| 12/13/2001  MAL, Zongwei Liu  Added EMV2000 support and made several changes 
|                               to improve PIO efficiency.
| 01/11/2002  MAL, zongwei Liu  Add timeout to read/write function
+----------------------------------------------------------------------------*/

#ifndef _sci_osd_h_
#define _sci_osd_h_

#include "sci_local.h"

/****************************************************************************
** Function:    sci_osd_init
**
** Purpose:     Initialize the Smart Card interface controller(s) and driver.
**
** Parameters:  detect_polarity: active polarity of the card detect signal
**               0: active low  - detect line is low  when card is inserted
**               1: active high - detect line is high when card is inserted
**              vcc_polarity: active polarity of the Vcc enable signal
**               0: active low  - Vcc enable is  low to apply power to the card
**               1: active high - Vcc enable is high to apply power to the card
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_KERNEL_FAIL: if interrupt handler install fails or

**                  mutex creation fails
****************************************************************************/
SCI_ERROR sci_osd_init(ULONG detect_polarity, ULONG vcc_polarity);

/****************************************************************************
** Function:    sci_osd_uninit
**
** Purpose:     Uninitialize the Smart Card interface controllers and driver.
****************************************************************************/
void sci_osd_uninit(void);

/****************************************************************************
** Function:    sci_osd_reset
**
** Purpose:     Initiate a reset (enter atr state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE:  if sci_id is invalid
**              SCI_ERROR_CARD_NOT_PRESENT: if no Smart Card is
**                  present in the reader
**              SCI_ERROR_ATR_PENDING: if a reset has already been initiated
**                  and an ATR is still pending
****************************************************************************/
SCI_ERROR sci_osd_reset(ULONG sci_id);

/*****************************************************************************
** Function:    sci_osd_write
**
** Purpose:     Write data to the Smart Card (enter tx state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_buffer: input pointer to write buffer
**              num_bytes: number of bytes to write from p_buffer
**              mode_flags: flags to indicate behavior of write
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_buffer is zero or
**                  num_bytes is zero
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
**              SCI_ERROR_TX_PENDING: if a transmission is already pending
*****************************************************************************/
SCI_ERROR sci_osd_write(ULONG sci_id,
                        UCHAR *p_buffer,
                        ULONG num_bytes, 
                        ULONG mode_flags
);

/*****************************************************************************
** Function:    sci_osd_read
**
** Purpose:     Read data from the Smart Card.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_buffer: input pointer to read buffer
**              num_bytes: number of bytes to read into p_buffer
**              p_bytes_read: number of bytes actually read into p_buffer
**              mode_flags: flags to indicate behavior of read
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_buffer is zero or
**                  num_bytes is zero or
**                  p_bytes_read is zero
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osd_read(ULONG sci_id,
                       UCHAR *p_buffer,
                       ULONG num_bytes,
                       ULONG *p_bytes_read, 
                       ULONG mode_flags
);

/*****************************************************************************
** Function:    sci_osd_deactivate
**
** Purpose:     Initiate a deactivation (enter deac state). 
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osd_deactivate(ULONG sci_id);

#endif /* _sci_osd_h_ */
