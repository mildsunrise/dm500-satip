//pallas/drv/include/osi_hitachi_ir.h
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
//  OS Independent Hitachi TV/VCR IR remote controller code decoder
//Revision Log:   
//  Sept/10/2001                         Created by YYD

#ifndef _DRV_INCLUDE_OSI_HITACHI_IR_H_INC_
#define _DRV_INCLUDE_OSI_HITACHI_IR_H_INC_

#include "os/os-types.h"

// the return code of ir_osi_detect_hitachi_code
#define __IR_HITACHI_REPEAT_CODE  0x66666666    // this should not be a vlaid hitachi code

// the return code of ir_osi_translate_hitachi_code
#define INVALID_IR_CODE     (0xffff)
#define UNKNOWN_IR_CODE     (0x0000)

#define __IR_KEY_DOWN       (0x1000)        // ORed with the return code

#define OPTV_VCR_POWER		266
#define OPTV_TV_POWER		261
#define OPTV_MUTE		    267
#define OPTV_VOLUP		    268
#define OPTV_VOLDOWN		269
#define OPTV_PREVCHVCR 		265
#define OPTV_PLAY		    264
#define OPTV_REC		    272
#define OPTV_STOP		    260
#define OPTV_PREVCHTV		265
#define OPTV_PAUSE		    273
// not all return codes are defined, please refer back to drv/ircombo/osi_hitachi-local.h


// detect and return hitachi TV/VCR ir rc code
// input the GPTC timediff and pCode
int ir_osi_detect_hitachi_code(UINT32 timediff, UINT32 *pCode);
// return 1, code detected and returned by *pCode
// return 0, timing code get correctly
// return -1, timimg code error, detector resetted


void ir_osi_reset_hitachi_detector(void);
// reset the detector plus stat

void ir_osi_get_hitachi_detector_stat(UINT *pCode, UINT *pSync, UINT*pError);
// get machine statistics


int ir_osi_process_hitachi_code(UINT32 uCode,  void (*handler_call_back)(USHORT uCode));
// return the number of codes sent on success
// return -1 on code error

#endif  // _DRV_INCLUDE_OSI_HITACHI_IR_H_INC_
