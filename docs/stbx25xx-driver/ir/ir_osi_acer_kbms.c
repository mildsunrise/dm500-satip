//pallas/drv/ircombo/osi_acer_kbms.c
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
//Comment: 
//  OS Independent Acer ir keyboard code decoder 
//     Acer IR keyboard Model No: WIL-14M, Revision No: 1.5
//Revision Log:   
//  Sept/10/2001  Created                                                 YYD
//  Feb/27/2002   Fixed mistake of IR mouse button status recover code    YYD
//                (wrong status bits were used previously)

#include "os/os-types.h"
//#include "gpt/gpt_ports.h"   // for __GPT_TIME_BASE
#include "ir/ir_osi_acer_kbms.h"
#include "ir_osi_acer-local.h"
//#define __IR_OSI_ACER_KBMS_DEBUG

#ifdef __IR_OSI_ACER_KBMS_DEBUG
#define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#define KB_CLOCK_CYCLE			833	//microsecond
#define KB_CLOCK_BASE			(KB_CLOCK_CYCLE*_IR_SAMPLE_CLOCK/1000000) // 11250, calcualted for 1200 bps
#define KB_CLOCK_TOLARANCE		(KB_CLOCK_BASE/2)       // half clock

#define KB_ERROR_FLAG			1
#define KB_TIMEOUT_FLAG			2
#define KB_END_FLAG			4

// internally used
#define __IR_ACER_MOUSE_MOVEMENT       (0x80000000)


//YYD, I defined some enum status codes for better readability
typedef enum
{ 
    _st_sync, 
    _st_data1,
    _st_data2,
 }  _ir_key_status; 

static _ir_key_status current_state;    // YYD
//static USHORT head_line, status_line, key_line, keyo_line, mouse_x_line, mouse_y_line;
static UINT32 uCode;
static USHORT uByte_line, bit_line;
static int	flag_kb;
static int 	flag_reverse;
//static UINT last_is_mouse;
static UINT status_count, key_count, mouse_count;

static USHORT old_shift_status;     // last known shift status
static USHORT shift_down;   // <msl><lsft><lalt><lctl>

static int odd_check(unsigned short code)
{
    int j;
    unsigned short start = 0;   /* odd check */

    for (j = 0; j < 9; j++)
    {
        start ^= code;
        code >>= 1;
    }

    return (start & 0x1);
}

#define clear_state()  { current_state=_st_sync; flag_reverse=1;flag_kb = 0; }

int ir_osi_detect_acer_keycode(UINT32 timediff, UINT32 *pCode)
{
    UINT sigWidth, pulseWidth;
    USHORT	data=0;
    int 	ret = 0;;
    
    if (0 == timediff ) return 0;
    //in case it reach fifo bottom.

    sigWidth  = ( ((timediff>>16)&0xffff)+ KB_CLOCK_TOLARANCE)/KB_CLOCK_BASE;
    pulseWidth= ( (timediff&0xffff) + KB_CLOCK_TOLARANCE)/KB_CLOCK_BASE;
    //PDEBUG("sigWidth %x  pulseWidth %x \n",sigWidth,pulseWidth);
    //if (sigWidth <= 10)
    {
    	if (sigWidth == 0) { sigWidth = 10; flag_kb = KB_TIMEOUT_FLAG;} //timeout ;
    	if ((sigWidth + bit_line ) > 10) sigWidth = 10 - bit_line;
    	bit_line +=sigWidth;
    	if (flag_reverse)
    	{	//use reverse bit order
    	// 10 bits: start bit, 8 data bit, parity bit, stop bit and idle bit is ignored.
    		data = 1<<(sigWidth -1);
    		uByte_line = (uByte_line <<sigWidth )|data;
    	}
    	else
    	{	// for mouse movement count
    	// 10 bits: stop bit , parity bit, 8 data bit, start bit and idle bit is ignored.
    		uByte_line |= (1<<(bit_line-1));
    	}
    	
    }
/*
    else
    {
    	flag_kb = KB_ERROR_FLAG;
    	bit_line= uByte_line = 0;
    }
*/

    if (bit_line == 10)	
    {
        PDEBUG("%x \n", uByte_line);
        if (odd_check(uByte_line&0x1ff ))	//get a byte.
        {       
		if (flag_reverse) uByte_line >>=1;
		uByte_line &=0xff;
		switch (current_state)
		{
			case	_st_sync:	//check for sync byte
				//PDEBUG("_st_sync %x  \n",uByte_line);
				if (( (uByte_line&0xd0) == 0xC0 ) || (uByte_line == 0x50))
				{// the first byte is in reverse order, 
					current_state = _st_data1;
					uCode = uByte_line;
					if (uByte_line == 0x50)
						flag_reverse = 0;
				}
				else 	clear_state();
				break;
			case	_st_data1:	//get first data byte;
				//PDEBUG("_st_data1   %x \n",uByte_line);
				current_state = _st_data2;
				uCode = (uCode << 8) + (uByte_line&0xff);
				break;
			case 	_st_data2:	//get second data byte;
				PDEBUG("_st_data2  %x \n",uByte_line);
				if ( (uCode &0xff00) == 0x5000 )  //mouse movement
				{
                            		*pCode =  __IR_ACER_MOUSE_MOVEMENT 
                            		| ((uCode&0xff) << 8) | uByte_line;
                            		ret = 1;
					flag_kb |= KB_END_FLAG;
                            	}
				else // must be key code.
				{
					if ( ((uCode^uByte_line)&0xff) == 0xff)
					{
						*pCode = (uCode & 0x3fff);
						PDEBUG("code %x \n",*pCode);
						ret = 1;
						flag_kb |= KB_END_FLAG;
					}
					else
					{
						flag_kb |= KB_ERROR_FLAG;
						ret = 0;
					}
					
				}
				break;
			default:
				flag_kb |= KB_ERROR_FLAG;
				ret = 0;
				break;
		}
		
	}
        else
        {
            	PDEBUG("clear state key: line: %x\n",   uByte_line);
            	flag_kb = KB_ERROR_FLAG;
        }
        bit_line = uByte_line = 0;
    }
    if (flag_kb) 	clear_state();
    return ret;
}

void ir_osi_reset_acer_detector(void)
{
    clear_state();
    bit_line = uByte_line = 0;
    old_shift_status = shift_down = 0;
}
// reset the detector plus stat

void ir_osi_get_acer_detector_stat(UINT *pKey, UINT *pMouse, UINT*pError)
{
    if (pKey) *pKey = 0;
    if (pMouse) *pMouse = 0;
    if (pError) *pError = 0;
}



// <msr><rsft><ralt><rctl>
#define ir_lsft_down	0x0040
#define ir_lalt_down	0x0020
#define ir_lctl_down	0x0010
#define ir_rsft_down	0x0004
#define ir_ralt_down	0x0002    // this is not used because there has no such key on Acer IR KB
#define ir_rctl_down	0x0001    // this is not used

#define ir_lbtn_down    0x8000
#define ir_rbtn_down    0x0800

// define the Acer IR KB status code field
    /* uCode = 00 <r><m><l> shift(1) alt(1) ctrl(1) */
#define _ir_sft_rbtn   0x20
#define _ir_sft_lbtn   0x08
#define _ir_sft_sft    0x04
#define _ir_sft_alt    0x02
#define _ir_sft_ctl    0x01



int ir_osi_process_acer_keycode(UINT32 uCode, 
                                void (*keycode_handler)(USHORT wCode, USHORT wDown), 
                                void (*mouse_handler)(INT nHorizontal, INT nVertical, USHORT uButtonEvent))
{
    
    BYTE key, down;
    USHORT shift_status, shift_changes; // current shift status
    USHORT scan_code;
#ifdef _IR_KEYBOARD_AUTO_KEY_UP
    UBYTE _is_shift = 1;    
#endif
    int cnt=0;
    
    if (__IR_ACER_MOUSE_MOVEMENT & uCode)  // mouse movement
    {
        if (mouse_handler)
        {
            INT nHmove = (INT)((uCode>>8)&0xff);
            INT nVmove = (INT)((uCode)&0xff);
            if (nHmove & 0x80) nHmove |= (-1)^0xff; 
            if (nVmove & 0x80) nVmove |= (-1)^0xff;
            mouse_handler(nHmove, nVmove, 0);
            return 1;
        }
        return 0;
    }
    
    // else here is a key code
    key = (uCode & 0xFF);
    
    down = key & 0x1;
    
    // ### Yudong Yang
    // first I have to keep my own shift key status
    switch (key >> 1)
    {
        
    case 0x5d:     // left shift
        if (down)
            shift_down |= ir_lsft_down;
        else
            shift_down &= ~ir_lsft_down;
        
        PDEBUG("L%d", down);
        break;
        
    case 0x2d:     // right shift
        if (down)
            shift_down |= ir_rsft_down;
        else
            shift_down &= ~ir_rsft_down;
        
        PDEBUG("R%d", down);
        break;
        
    case 0x3b:     // alt
        if (down)
            shift_down |= ir_lalt_down;
        else
            shift_down &= ~ir_lalt_down;
        
        PDEBUG("A%d", down);
        break;
        
    case 0x73:     // ctl
        if (down)
            shift_down |= ir_lctl_down;
        else
            shift_down &= ~ir_lctl_down;
        
        PDEBUG("C%d", down);
        break;
        
    case 0x5F:     // mouse left button
        if (down)
            shift_down |= ir_lbtn_down;
        else
            shift_down &= ~ir_lbtn_down;
        
        PDEBUG("BL%d", down);
        break;
        
    case 0x1F:     // mouse right button
        if (down)
            shift_down |= ir_rbtn_down;
        else
            shift_down &= ~ir_rbtn_down;
        
        PDEBUG("BR%d", down);
        break;
#ifdef _IR_KEYBOARD_AUTO_KEY_UP
    default:   // other keys
        _is_shift = 0;
#endif
    }
    
    // check to see if shift status is changed
    /* uCode = 00 <r><m><l> shift(1) alt(1) ctrl(1) , key(7) up/down(1) */
    shift_status = (USHORT) (uCode >> 8);
    // get current shift status with this key
    shift_changes = shift_status ^ old_shift_status;  // and the changes
    if (shift_changes)
    {
        // shift changed, check if we should patch it
        PDEBUG("%d %d \n", old_shift_status, shift_status);
        if ((shift_changes) & _ir_sft_sft)   // shift
        {
            if (shift_status & _ir_sft_sft)    // check if we need to send shift down
            {
                // check if we have some record of shift down
                if (!(shift_down & (ir_lsft_down | ir_rsft_down)))
                {
                    // we need to regenerate it
                    PDEBUG("L1");
                    // assume only left shift down is missing
                    if (keycode_handler) keycode_handler(0x2A, 1);
                    // or we can also send right shift up if it 
                    // will not break keyboard routines in kernel
                    // if (keycode_handler) keycode_handler(0x36, 1);
                    shift_down |= ir_lsft_down;
                    cnt ++;
                }
            }
            else        // check if we should send shift up
            {
                if (shift_down & ir_lsft_down)  // we need to send it
                {
                    PDEBUG("L0");
                    // left shift up missing
                    if (keycode_handler) keycode_handler(0x2A | 0x80, 0);
                    shift_down &= ~ir_lsft_down;
                    cnt ++;
                }
                
                if (shift_down & ir_rsft_down)  // we need to send it
                {
                    PDEBUG("R0");
                    // right shift up missing
                    if (keycode_handler) keycode_handler(0x36 | 0x80, 0);
                    shift_down &= ~ir_rsft_down;
                    cnt ++;
                }
            }
        }
        
        if ((shift_changes) & _ir_sft_alt)   // check for alt status 
        {
            if (shift_status & _ir_sft_alt)    // check if we need to send alt down
            {
                if (!(shift_down & (ir_lalt_down)))     // we need to send it
                {       // assume is left alt down missing
                    if (keycode_handler) keycode_handler(0x38, 1);
                    shift_down |= ir_lalt_down;
                    cnt ++;
                }
            }
            else        // check if we should send alt up
            {
                if (shift_down & ir_lalt_down)  // we need to send it
                {       // left alt up missing
                    if (keycode_handler) keycode_handler(0x38 | 0x80, 0);
                    shift_down &= ~ir_lalt_down;
                    cnt ++;
                }
            }
        }
        
        if ((shift_changes) & _ir_sft_ctl)   // check for ctl status
        {
            if (shift_status & _ir_sft_ctl)    // check if we need to send ctl down
            {
                if (!(shift_down & (ir_lctl_down)))     // we need to send it
                {       // assume is left ctl down missing
                    if (keycode_handler) keycode_handler(0x1D, 1);
                    shift_down |= ir_lctl_down;
                    cnt ++;
                }
            }
            else        // check if we should send ctl up
            {
                if (shift_down & ir_lctl_down)  // we need to send it
                {       // left ctl up missing
                    if (keycode_handler) keycode_handler(0x1D | 0x80, 0);
                    shift_down &= ~ir_lctl_down;
                    cnt ++;
                }
            }
        }
        
        if ((shift_changes) & _ir_sft_rbtn)   // check for right button status
        {
            if (shift_status & _ir_sft_rbtn)    // check if we need to send rbutton down
            {
                if (!(shift_down & (ir_rbtn_down)))     // we need to send it
                {   
                    // assume is rbutton down missing
                    if (mouse_handler) mouse_handler(0,0, __IR_MOUSE_RBUTTON_DOWN);
                    shift_down |= ir_rbtn_down;
                    cnt ++;
                }
            }
            else        // check if we should send rbutton up
            {
                if (shift_down & ir_rbtn_down)  // we need to send it
                {
                    // right button up missing
                    if (mouse_handler) mouse_handler(0,0, __IR_MOUSE_RBUTTON_UP);
                    shift_down &= ~ir_rbtn_down;
                    cnt ++;
                }
            }
        }
        
        if ((shift_changes) & _ir_sft_lbtn)   // check for left button status
        {
            if (shift_status & _ir_sft_lbtn)    // check if we need to send lbutton down
            {
                if (!(shift_down & (ir_lbtn_down)))     // we need to send it
                {
                    // assume is lbutton down missing
                    if (mouse_handler) mouse_handler(0,0, __IR_MOUSE_LBUTTON_DOWN);
                    shift_down |= ir_lbtn_down;
                    cnt ++;
                }
            }
            else        // check if we should send lbutton up
            {
                if (shift_down & ir_lbtn_down)  // we need to send it
                {       
                    // left button up missing
                    if (mouse_handler) mouse_handler(0,0, __IR_MOUSE_LBUTTON_UP);
                    shift_down &= ~ir_lbtn_down;
                    cnt ++;
                }
            }
        }
    }
    
    old_shift_status = shift_status;    // ok, remenber current status
    // ###
    
    switch (key >> 1)
    {
    case 0x5F:     // mouse left button
        PDEBUG("%x %x \n", key, shift_status);
        if (mouse_handler) mouse_handler(0,0, down ? __IR_MOUSE_LBUTTON_DOWN : __IR_MOUSE_LBUTTON_UP);
        cnt ++;
        break;
        
    case 0x1F:     // mouse right button
        PDEBUG("%x %x ", key, shift_status);
        if (mouse_handler) mouse_handler(0,0, down ? __IR_MOUSE_RBUTTON_DOWN : __IR_MOUSE_RBUTTON_UP);
        cnt ++;
        break;
        
    case 0x55:     /* pause */
        if (keycode_handler) keycode_handler(0xe1, 0);
        if (keycode_handler) keycode_handler(down ? 0x1d : 0x9d, down);
        if (keycode_handler) keycode_handler(down ? 0x45 : 0xc5, down);
        cnt +=3;
        break;
        
    case 0x65:     /* print screen */
        if (keycode_handler) keycode_handler(0xe0, 0);
        cnt++;
        if (down)
        {
            if (keycode_handler) keycode_handler(0x2a, 1);
            cnt++;
        }
        else
        {
            if (keycode_handler) keycode_handler(0x37, 0);
            if (keycode_handler) keycode_handler(0x46, 0);
            cnt+=2;
        }
        break;
        
    default:
        PDEBUG("%x %x \n", key, shift_status);
        scan_code = acer_irkb_scan_code[key >> 1];
        
        if (scan_code != 0xFFFF)
        {
            if (down) 
            {
                if (scan_code & 0xFF00) /* 0xe0 */
                {
                    if (keycode_handler) keycode_handler((scan_code & 0xFF00) >> 8, 1);
                    cnt++;
                }
                if (keycode_handler) keycode_handler(scan_code & 0xFF, 1);       /* down */
                cnt++;
#ifdef _IR_KEYBOARD_AUTO_KEY_UP
                if (!_is_shift) 
                {
                    if (scan_code & 0xFF00) /* 0xe0 */
                    {
                        if (keycode_handler) keycode_handler((scan_code & 0xFF00) >> 8, 1);
                        cnt++;
                    }
                    if (keycode_handler) keycode_handler((scan_code & 0xFF) | 0x80, 0); // up
                    cnt++;
                }  
#endif
            }    
            else 
            {
#ifdef _IR_KEYBOARD_AUTO_KEY_UP
                if (_is_shift) // shift up
                {
                    if (scan_code & 0xFF00) /* 0xe0 */
                    {
                        if (keycode_handler) keycode_handler((scan_code & 0xFF00) >> 8, 1);
                        cnt++;
                    }
                    if (keycode_handler) keycode_handler((scan_code & 0xFF) | 0x80, 0); // or the up code should be already sent   
                    cnt++;
                }
#else
                if (scan_code & 0xFF00) /* 0xe0 */
                {
                    if (keycode_handler) keycode_handler((scan_code & 0xFF00) >> 8, 1);
                    cnt++;
                }
                if (keycode_handler) keycode_handler((scan_code & 0xFF) | 0x80, 0);      /* up */
                cnt++;
#endif
            }
        }    /* if (scan_code != 0xffff) */
    }        /* switch */
    return cnt; 
}
