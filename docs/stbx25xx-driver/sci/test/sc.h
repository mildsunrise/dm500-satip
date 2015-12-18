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
| File:      sc.h
| Purpose:   Smart Card protocol layer PRIVATE header file.
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  zongwei Liu       Port to pallas
+----------------------------------------------------------------------------*/

#ifndef _sc_h_
#define _sc_h_

/* constants */
#define SC_MAX_ATR_SIZE             33
#define SC_MAX_HISTORICAL_SIZE      15
#define SC_MAX_T0_DATA_SIZE         255
#define SC_MAX_T1_BLOCK_SIZE        259

#define SC_MIN_IFSD             1
#define SC_MAX_IFSD             254

/* ATR parameters */

typedef struct
{
    unsigned char T;
    unsigned short maskT;
    unsigned char F;
    unsigned char D;
    unsigned char FI;
    unsigned char DI;
    unsigned char II;
    unsigned char PI1;
    unsigned char PI2;
    unsigned char WI;
    unsigned char XI;
    unsigned char UI;
    unsigned char N;
    unsigned char CWI;
    unsigned char BWI;
    unsigned char IFSC;
    unsigned char check;
}

SC_PARAMETERS;

/* public API prototypes */

/****************************************************************************
** Function:    sc_reset
**
** Purpose:     Perform a reset, receive and process the ATR, and extract all
**              ATR parameters.
**
** Parameters:  sc_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
*****************************************************************************/
SCI_ERROR sc_reset(unsigned long sc_id);

/****************************************************************************
** Name         sc_pps
**
** Purpose:     initiate a protocol and parameter selection request
**
** Parameters:  sc_id: zero-based number to identify Smart Card controller
**              T: protocol to propose to the Smart Card
**              F: clock rate conversion factor to propose to the Smart Card
**              D: baud rate adjustment factor to propose to the Smart Card
**
** Returns:     SCI_ERROR_OK: If the PPS exchange was successful
**              other error code if a failure occurs
****************************************************************************/
SCI_ERROR sc_pps(unsigned long sc_id,
                 unsigned char T, unsigned char F, unsigned char D);

/****************************************************************************
** Name         sc_get_ifsd
**
** Purpose:     get the current IFSD setting
**
** Parameters:  sc_id: zero-based number to identify Smart Card controller
**              ifsd:  output pointer to buffer which will hold IFSD value
**
** Returns:     SCI_ERROR_OK: If the PPS exchange was successful
**              other error code if a failure occurs
****************************************************************************/
SCI_ERROR sc_get_ifsd(unsigned long sc_id, unsigned char *ifsd);

/****************************************************************************
** Name         sc_set_ifsd
**
** Purpose:     set the current IFSD setting
**
** Parameters:  sc_id: zero-based number to identify Smart Card controller
**              ifsd:  new IFSD value
**
** Returns:     SCI_ERROR_OK: If the PPS exchange was successful
**              other error code if a failure occurs
****************************************************************************/
SCI_ERROR sc_set_ifsd(unsigned long sc_id, unsigned char ifsd);

/****************************************************************************
** Name         sc_apdu
**
** Purpose:     Send a command APDU (Application Protocol Data Unit) and
**              receive the response APDU.
**
** Parameters:  sc_id: zero-based Smart Card identifier
**              p_capdu: (input)  pointer to the command APDU buffer
**              p_rapdu: (output) pointer to the response APDU buffer
**              p_length:(input)  pointer the length of the command APDU
**                       (output) pointer the length of the response APDU
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
****************************************************************************/
SCI_ERROR sc_apdu(unsigned long sc_id,
                  unsigned char *p_capdu,
                  unsigned char *p_rapdu, unsigned long *p_length);

/****************************************************************************
** Function:    sc_get_parameters
**
** Purpose:     Retrieve the current Smart Card ATR parameters.
**
** Parameters:  sc_id: zero-based number to identify smart card controller
**              p_sci_parameters: output pointer to Smart Card ATR parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
*****************************************************************************/
SCI_ERROR sc_get_parameters(unsigned long sc_id,
                            SC_PARAMETERS * p_sc_parameters);

/****************************************************************************
** Function:    sc_get_atr
**
** Purpose:     Retrieve the ATR. 
**
** Parameters:  sc_id:    zero-based number to identify smart card controller
**              ATR:               output pointer to ATR buffer
**              atr_size:          output pointer to ATR size
**              historical_offset: output pointer to historical offset
**              historical_size:   output pointer to historical size
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
*****************************************************************************/
SCI_ERROR sc_get_atr(unsigned long sc_id,
                     unsigned char *ATR,
                     unsigned char *atr_size,
                     unsigned char *historical_offset,
                     unsigned char *historical_size);

#endif /*_sci_h_*/
