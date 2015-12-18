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
|       COPYRIGHT   I B M   CORPORATION 1997
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Neil Leeder
| Component: Include file.
| File:      eeprom.h
| Purpose:   EEPROM definitions.  For Xicor X24165
| Changes:
| Date:         Comment:
| -----         --------
| 29-Aug-97     Created                                                     NL
| 23-Feb-98     Port to Redwood board                                       MPT
| 04-June-01	Port to Linux on Redwood4									SL
+----------------------------------------------------------------------------*/
#ifndef _eeprom_h_
#define _eeprom_h_

#include "hw/hardware.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*----------------------------------------------------------------------------+
    | Write Protect Register bits.  This register is located at address 0x7FF.
    +----------------------------------------------------------------------------*/
#define EEPROM_WPR_WEL                  0x02
#define EEPROM_WPR_RWEL                 0x04
#define EEPROM_WPR_BP0					0x08
#define EEPROM_WPR_BP1					0x10
#define EEPROM_WPR_WPEN					0x80

    /*----------------------------------------------------------------------------+
    | Debug defines.
    +----------------------------------------------------------------------------*/
#define EEPROM_DEBUG_READ               0x00000001
#define EEPROM_DEBUG_WRITE              0x00000002
#define EEPROM_DEBUG_READ_OP            0x00000004
#define EEPROM_DEBUG_WRITE_OP           0x00000008

#ifdef __DRV_FOR_VESTA__
/*----------------------------------------------------------------------------+
| Device select constants:
| bit: 0    = 1 (START)
| bit: 1-3  = 111 (Select EEPROM)
| bit: 4-6  = nnn (High order EEPROM addresses, A10, A9, A8)
| bit: 7    = 0 (write), 1 (read)
+----------------------------------------------------------------------------*/
#define EEPROM_I2C_ADDRESS_WRITE        0x90 >> 1
#define EEPROM_I2C_ADDRESS_READ         0x91 >> 1

/*----------------------------------------------------------------------------+
| Maximum EEPROM address.
+----------------------------------------------------------------------------*/
#define EEPROM_MAX_ADDRESS              2047
#define EEPROM_PROT_HI                  0x07
#define EEPROM_I2C_BUS              "/dev/i2c-1"

#define EEPROM_PAGE_LEN				    32

#elif defined( __DRV_FOR_PALLAS__)

/*----------------------------------------------------------------------------+
| Xicor x24645  8k*8
| Device select constants:
| bit: -1    = 1 (START)
| bit: 0-1  = 11 (Select EEPROM)
| bit: 2-6  = nnnn (High order EEPROM addresses, A12, A11, A10, A9, A8)
| bit: 7    = 0 (write), 1 (read)
+----------------------------------------------------------------------------*/
#define EEPROM_I2C_ADDRESS_WRITE        0xC0 >> 1
#define EEPROM_I2C_ADDRESS_READ         0xC1 >> 1

/*----------------------------------------------------------------------------+
| Maximum EEPROM address.
+----------------------------------------------------------------------------*/

#define EEPROM_MAX_ADDRESS              8192
#define EEPROM_PROT_HI                  0x1f
#define EEPROM_I2C_BUS              "/dev/i2c-1"
#define EEPROM_PAGE_LEN				    32

#elif defined( __DRV_FOR_VULCAN__ )

/*----------------------------------------------------------------------------+
| It is Atmel 24C64N on the board. Schematics says 24256... something is wrong
+----------------------------------------------------------------------------*/
#define EEPROM_I2C_ADDRESS_WRITE        0xA6 >> 1
#define EEPROM_I2C_ADDRESS_READ         0xA6 >> 1

/*----------------------------------------------------------------------------+
| Maximum EEPROM address.
+----------------------------------------------------------------------------*/

#define EEPROM_MAX_ADDRESS              8191                 //32767
#define EEPROM_I2C_BUS                  "/dev/i2c-0"
#define EEPROM_PAGE_LEN		        32                   //64

#else

    #error "unknown architecture"

#endif



int eeprom_write_begin(int fd);
int eeprom_write_end(int fd);
int eeprom_write(int fd, int sub_adr, unsigned char *data, int length);
int eeprom_read(int fd, int sub_adr, unsigned char *data, int length);

#ifdef __cplusplus
}

#endif

#endif                          /* _eeprom_h_ */
