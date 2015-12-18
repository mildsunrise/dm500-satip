//pallas/drv/ircombo/osi_acer-local.h
/*----------------------------------------------------------------------------+
|
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
//
//Comment: 
//  Local data of OS Independent Acer ir keyboard code decoder 
//     Acer IR keyboard Model No: WIL-14M, Revision No: 1.5
//Revision Log:   
//  Sept/11/2001                         Created by YYD

#ifndef _DRV_IRCOMBO_OSI_ACER_LOCAL_H_INC_
#define _DRV_IRCOMBO_OSI_ACER_LOCAL_H_INC_

#include "os/os-types.h"
#include "ir/ir_osi_device.h"

static USHORT acer_irkb_scan_code[128] = 
{
    0x44,   0x57,   0x09,   -1,     0x42,   -1,     0x40, -1,   /* 0x00-0x07 */
    0x0b,   0x58,   0x41,   -1,     0x17,   -1,     0x3f, -1,   /* 0x08-0x0f */
    0x1b,   -1,     0x1f,   -1,     0x32,   0x34,   0x03, -1,   /* 0x10-0x17 */
    0x27,   -1,     0x11,   -1,     0x21,   -1,     0x04, -1,   /* 0x18-0x1f */
    0x19,   -1,     0x15,   -1,     0x25,   -1,     0x14, -1,   /* 0x20-0x27 */
    0x18,   -1,     0x13,   -1,     0x23,   0x36,   0x06, -1,   /* 0x28-0x2f */
    0xe048, -1,     -1,     -1,     0x30,   0xe04d, 0x29, -1,   /* 0x30-0x37 */
    0x35,   0xe051, 0x1e,   0x38,   0x2e,   -1,     0x3b, -1,   /* 0x38-0x3f */
    0x0c,   -1,     0x16,   -1,     0x0a,   -1,     0x07, -1,   /* 0x40-0x47 */
    0x43,   -1,     0x08,   -1,     0x24,   -1,     0x3e, -1,   /* 0x48-0x4f */
    0x2b,   0x1c,   0x2c,   -1,     0x31,   -1,     0x02, -1,   /* 0x50-0x57 */
    0x28,   -1,     0x10,   0xe04f, 0x2d,   0x2a,   0x3c, -1,   /* 0x58-0x5f */
    0x1a,   0x0d,   0x20,   -1,     0x33,   -1,     0x05, -1,   /* 0x60-0x67 */
    0x26,   0x0e,   0x12,   -1,     0x22,   -1,     0x3d, -1,   /* 0x68-0x6f */
    0xe050, 0xe052, -1,     0x1d,   0x39,   0xe053, 0x0f, -1,   /* 0x70-0x77 */
    0xe04b, 0xe049, 0x3a,   0xe047, 0x2f,   -1,     0x01, -1,   /* 0x78-0x7f */
};


#endif // _DRV_IRCOMBO_OSI_ACER_LOCAL_H_INC_
