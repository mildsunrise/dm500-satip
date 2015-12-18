//pallas/drv/ircombo/osi_hitachi_ir.c
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

#include "os/os-types.h"
#include "ir/ir_osi_hitachi.h"

#include "ir_osi_hitachi-local.h"  // local include

#ifdef __IR_OSI_HITACHI_DEBUG
#define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#define RC_CLOCK_CYCLE			560	//microsecond.
#define RC_CLOCK_BASE			(RC_CLOCK_CYCLE*_IR_SAMPLE_CLOCK/1000000) // 56, for 0.56ms clock base
#define RC_CLOCK_TOLARANCE		(RC_CLOCK_BASE/2)     // half clock
#define RC_TICK_TOLARANCE		1       // 1 clock tolarance for ir rc data
#define RC_START_PULSE_WIDTH		16	
	//	( ( 9000*_IR_SAMPLE_CLOCK/1000000) + RC_CLOCK+TOLARANCE ) / RC_CLOCK_BASE		
#define	RC_START_DATA_WIDTH		24
	//	( (13500*_IR_SAMPLE_CLOCK/1000000) + RC_CLOCK+TOLARANCE ) / RC_CLOCK_BASE	
#define	RC_START_REPEAT_WIDTH		20
	//	( (11250*_IR_SAMPLE_CLOCK/1000000) + RC_CLOCK+TOLARANCE ) / RC_CLOCK_BASE	

#define RC_REPEAT_EATEN         5       // 5 repeat code eaten before repeat code is processed


#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif

typedef enum 
{ 
    _rcst_start, 
    _rcst_repeat, 
    _rcst_bit_det 
}  _ir_rc_status;                  


static _ir_rc_status rc_status;     // the status
static UINT   rc_bit_num;           // the received bits
static UINT32 rc_code;              // the received code
static UINT _rawir_sync, _rawir_key, _rawir_err; // for stat
static USHORT last_sent_code, repeat_count;

int ir_osi_detect_hitachi_code(UINT32 timediff, UINT32 *pCode)
{
    UINT sigWidth, pulseWidth;
    
	if (0 == timediff) return 0;
    sigWidth  = ( (timediff>>16)+ RC_CLOCK_TOLARANCE)/RC_CLOCK_BASE;
    pulseWidth= ( (timediff&0xffff) + RC_CLOCK_TOLARANCE)/RC_CLOCK_BASE;

    switch (rc_status)
    {
    	case _rcst_start  :  //wait for start code or repeat start code
    		if (pulseWidth ==RC_START_PULSE_WIDTH)
    		{ //start pulse
    			if (sigWidth == RC_START_DATA_WIDTH)
    			{
    				rc_status = _rcst_bit_det;
    				rc_bit_num = 0;
				rc_code = 0x00000000;

    			}
    		}
    		return 0;
    	case _rcst_repeat :
    		if (pulseWidth ==RC_START_PULSE_WIDTH)
    		{ //start pulse
    			if (sigWidth == RC_START_DATA_WIDTH)
    			{
    				rc_status = _rcst_bit_det;
    				rc_bit_num = 0;
				rc_code = 0x00000000;
				return 0;
    			}
    			if (sigWidth == RC_START_REPEAT_WIDTH)
    			{
	    	                *pCode = __IR_HITACHI_REPEAT_CODE;
		                return 1;
    			} 
    		}
    		return 0;
    	case _rcst_bit_det:
    		if (pulseWidth == 1)
    		{
    			if ( sigWidth == 4)  //bit 1
    			{
    				rc_code |= (0x80000000) >> rc_bit_num;
                    		rc_bit_num++;
                    	}
                    	else if ( sigWidth == 2) //bit 0
                    		rc_bit_num ++;
                    	else break;
			if (rc_bit_num == 32)
                	{
                    		rc_status = _rcst_repeat; // can repeat
                    		_rawir_key ++;
                    		*pCode = rc_code;
                    		return 1;
                	}
                	return 0;
    		}
    	default:
    		break;
    }	 
    rc_status = _rcst_start;
    return 0;
}

void ir_osi_reset_hitachi_detector(void)
{
    rc_status = _rcst_start;  // wait for another start code
    _rawir_sync = _rawir_key = _rawir_err = 0;
    repeat_count = 0;
    last_sent_code = INVALID_IR_CODE;
}

void ir_osi_get_hitachi_detector_stat(UINT *pCode, UINT *pSync, UINT*pError)
{
    if(pCode) *pCode = _rawir_key;
    if(pSync) *pSync = _rawir_sync;
    if(pError) *pError = _rawir_err;
}



int ir_osi_process_hitachi_code(UINT32 uCode,  void (*handler_call_back)(USHORT uCode))
{
    USHORT group;
    BYTE curr_key;
    BYTE xorkey;

    if(__IR_HITACHI_REPEAT_CODE == uCode) // repeat code
    {
        if(INVALID_IR_CODE != last_sent_code)
        {
            repeat_count++;
            if(repeat_count > RC_REPEAT_EATEN && NULL != handler_call_back) 
            {
#ifdef _IR_RAWMODE_     // no filtering
                handler_call_back(last_sent_code);
                return 1;
#else       // the old filtered way
                handler_call_back(last_sent_code|__IR_KEY_DOWN);
                handler_call_back(last_sent_code);
                return 2;
#endif
            }
        }
        return 0;
    }
    repeat_count = 0;

    group = (uCode >> 16) & 0xFFFF;

#ifdef _IR_RAWMODE_     // no receiving filter
    if(((group&0xff) ^ 0xff) != (group >> 8))  // error vender code
    {
       last_sent_code = INVALID_IR_CODE;
       return -1;
    }
    curr_key = (BYTE)((uCode >> 8) & 0xFF);
    xorkey = (BYTE)uCode;

    if ((curr_key ^ 0xFF) != xorkey)
    {
        last_sent_code = INVALID_IR_CODE;
        return -1;
    }
    last_sent_code = (group & 0xff00) | (curr_key);
    handler_call_back(last_sent_code);
    return 1;

#else   // receive filtering is on

    if ((group == HITACHI_VCR_GROUP) || (group == HITACHI_TV_GROUP) ||
        (group == HITACHI_OTHER_GROUP))
    {
        int i;
        curr_key = (BYTE)((uCode >> 8) & 0xFF);
        xorkey = (BYTE)uCode;

        if ((curr_key ^ 0xFF) != xorkey)
        {
            last_sent_code = INVALID_IR_CODE;
            return -1;
        }

        for (i = 0; keyinfo[i].group != 0; i++)
        {
            if ((keyinfo[i].key == curr_key) && (keyinfo[i].group == group))
            {
                last_sent_code = keyinfo[i].key_code;
                handler_call_back(last_sent_code | __IR_KEY_DOWN);
                handler_call_back(last_sent_code);
                return 2;
            }
        }
        return 0;  // for unknown codes, just don't sent
    }

#endif // _IR_RAWMODE_

    return -1;
}
