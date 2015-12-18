//pallas/drv/include/osi_acer_kbms.h
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
//  OS Independent Acer ir keyboard code decoder 
//     Acer IR keyboard Model No: WIL-14M, Revision No: 1.5
//Revision Log:   
//  Sept/10/2001                         Created by YYD

#ifndef _DRV_INCLUDE_OSI_ACER_KBMS_H_INC_
#define _DRV_INCLUDE_OSI_ACER_KBMS_H_INC_

#include "os/os-types.h"


// for the mouse event handler of ir_osi_process_acer_keycode
#define __IR_MOUSE_LBUTTON_DOWN         (0x0001)
#define __IR_MOUSE_LBUTTON_UP           (0x0100)
#define __IR_MOUSE_MBUTTON_DOWN         (0x0002)
#define __IR_MOUSE_MBUTTON_up           (0x0200)
#define __IR_MOUSE_RBUTTON_DOWN         (0x0004)
#define __IR_MOUSE_RBUTTON_UP           (0x0400)


// detect and return hitachi TV/VCR ir rc code
// input the GPTC timediff and pCode
int ir_osi_detect_acer_keycode(UINT32 timediff, UINT32 *pCode);
// return 1, keyboard code detected and returned by *pCode
// return 0, timing code get correctly
// return -1, timimg code error, detector resetted

void ir_osi_reset_acer_detector(void);
// reset the detector plus stat

void ir_osi_get_acer_detector_stat(UINT *pKey, UINT *pMouse, UINT*pError);
// get machine statistics


int ir_osi_process_acer_keycode(UINT32 uCode, void (*keycode_handler)(USHORT wCode, USHORT wDown), void (*mouse_handler)(INT nHorizontal, INT nVertical, USHORT uButtonEvent));


#endif  // _DRV_INCLUDE_OSI_ACER_KBMS_H_INC_
