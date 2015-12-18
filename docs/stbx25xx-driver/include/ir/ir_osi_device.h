//pallas/drv/include/osi_irdevice.h
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
//  OS Independent IR device driver 
//Revision Log:   
//  Sept/10/2001                         Created by YYD

#ifndef _DRV_INCLUDE_OSI_IRDEVICE_H_INC_
#define _DRV_INCLUDE_OSI_IRDEVICE_H_INC_

#include "os/os-types.h"

#define _IR_SAMPLE_CLOCK	500000

typedef struct __IR_DEVICE_CHAIN_STRUCT_
{
    INT     (*init)(void);          // calls when ir device is being initlized
    void    (*deinit)(void);        // calls when ir device is being deinitilized
    void    (*event)(UINT32 time_diff);  // calls when there is an ir event

    // private
    struct __IR_DEVICE_CHAIN_STRUCT_ *pNext;
}  __IR_DEVICE;


INT ir_osi_register_device(__IR_DEVICE *pDev);
INT ir_osi_unregister_device(__IR_DEVICE *pDev);

INT ir_osi_init(void);  // requires GPT atom device been initialized

INT ir_osi_deinit(void);

INT ir_osi_start(void);

INT ir_osi_stop(void);

#endif // _DRV_INCLUDE_OSI_IRDEVICE_H_INC_
