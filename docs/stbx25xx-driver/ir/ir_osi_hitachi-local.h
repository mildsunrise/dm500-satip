//pallas/drv/ircombo/osi_hitachi-local.h
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
//  Local data of OS Independent Hitachi TV/VCR IR remote controller code decoder
//Revision Log:   
//  Sept/11/2001                         Created by YYD

#ifndef _DRV_IRCOMBO_OSI_HITACHI_LOCAL_H_INC_
#define _DRV_IRCOMBO_OSI_HITACHI_LOCAL_H_INC_

#include "os/os-types.h"
#include "ir/ir_osi_device.h"

#ifndef  _IR_RAWMODE_    // receive all codes and output unfiltered

// The Hitachi Group code
#define HITACHI_VCR_GROUP	0x06F9
#define HITACHI_TV_GROUP	0x0AF5
#define HITACHI_OTHER_GROUP	0x8679

// The hitachi key code
#define HITACHI_KEY_POWER	    0xE8
#define HITACHI_KEY_MUTE 	    0xD0
#define HITACHI_KEY_VOLUP	    0x48
#define HITACHI_KEY_VOLDOWN	    0xA8
#define HITACHI_KEY_PREVCH	    0x54
#define HITACHI_KEY_CHUP	    0x98
#define HITACHI_KEY_CHDOWN	    0x18
#define HITACHI_KEY_ONE  	    0xB0
#define HITACHI_KEY_TWO		    0x70
#define HITACHI_KEY_THREE	    0xF0
#define HITACHI_KEY_FOUR	    0x38
#define HITACHI_KEY_FIVE	    0xB8
#define HITACHI_KEY_SIX  	    0x78
#define HITACHI_KEY_SEVEN 	    0xF8
#define HITACHI_KEY_EIGHT	    0x20
#define HITACHI_KEY_NINE	    0xA0
#define HITACHI_KEY_ZERO	    0x30
#define HITACHI_KEY_ENTER	    0x68
#define HITACHI_KEY_ENTERTV	    0x60
#define HITACHI_KEY_REW  	    0x50
#define HITACHI_KEY_PLAY	    0x28
#define HITACHI_KEY_FF   	    0x90
#define HITACHI_KEY_REC		    0x28
#define HITACHI_KEY_STOP	    0xD0
#define HITACHI_KEY_PAUSE	    0x58
#define HITACHI_KEY_PREVCHTV	0x50

/* GMCK adding the TV VCR menu keys for Cinema 7 */
#define HITACHI_KEY_TV_MENU_LEFT    0xCE
#define HITACHI_KEY_TV_MENU_RIGHT   0x4E
#define HITACHI_KEY_TV_MENU_UP      0x0E
#define HITACHI_KEY_TV_MENU_DOWN    0x8E
#define HITACHI_KEY_TV_MENU_SEL     0xCC
#define HITACHI_KEY_VCR_MENU_LEFT   0x21
#define HITACHI_KEY_VCR_MENU_RIGHT  0xC1
#define HITACHI_KEY_VCR_MENU_UP     0x00
#define HITACHI_KEY_VCR_MENU_DOWN   0x80
#define HITACHI_KEY_VCR_MENU_SEL    0xA6
/* GMCK end */

/* CRL add for Cinema 7, YYD */
#define HITACHI_KEY_TV_SLEEP        0x88
#define HITACHI_KEY_TV_MENU         0xCA
#define HITACHI_KEY_TV_PROG         0xDA
#define HITACHI_KEY_TV_EXIT         0x90
#define HITACHI_KEY_TV_SURROUND     0xD8
#define HITACHI_KEY_TV_DISPLAY      0x08
#define HITACHI_KEY_TV_ENTER        0x60

#define HITACHI_KEY_VCR_MENU        0x02
#define HITACHI_KEY_VCR_PROG        0xDC
#define HITACHI_KEY_VCR_EXIT        0x82
#define HITACHI_KEY_VCR_SURROUND    0x7C
#define HITACHI_KEY_VCR_DISPLAY     0x42
#define HITACHI_KEY_VCR_ENTER       0x22
/* CRL add */

typedef struct raw_key_def
{
    USHORT group;
    BYTE   key;
    USHORT key_code;
    char key_name[13];
} raw_key_def_t;


static raw_key_def_t keyinfo[] = {
    {HITACHI_VCR_GROUP, HITACHI_KEY_POWER,      OPTV_VCR_POWER, {"Power VCR"}},
    {HITACHI_TV_GROUP,  HITACHI_KEY_POWER,      OPTV_TV_POWER, {"Power TV "}},
    {HITACHI_TV_GROUP,  HITACHI_KEY_MUTE,       OPTV_MUTE, {"Mute"}},
    {HITACHI_TV_GROUP,  HITACHI_KEY_VOLUP,      OPTV_VOLUP, {"Volume UP"}},
    {HITACHI_TV_GROUP,  HITACHI_KEY_VOLDOWN,    OPTV_VOLDOWN, {"Volume Down"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_PREVCH,     OPTV_PREVCHVCR, {"Prev. Ch"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_CHUP,       0x0B, {"Channel Up"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_CHDOWN,     0x0A, {"Channel Down"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_ONE,        0x31, {"One"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_TWO,        0x32, {"Two"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_THREE,      0x33, {"Three"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_FOUR,       0x34, {"Four"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_FIVE,       0x35, {"Five"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_SIX,        0x36, {"Six"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_SEVEN,      0x37, {"Seven"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_EIGHT,      0x38, {"Eight"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_NINE,       0x39, {"Nine"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_ZERO,       0x30, {"Zero"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_ENTER,      0x0D, {"Enter"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_REW,        0x08, {"Rewind"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_PLAY,       OPTV_PLAY, {"Play"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_FF,         0x09, {"Fast Forward"}},
    {HITACHI_OTHER_GROUP, HITACHI_KEY_REC,      OPTV_REC, {"Record"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_STOP,       OPTV_STOP, {"Stop"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_PREVCHTV,    OPTV_PREVCHTV, {"Prev. Ch"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_CHUP,        0x0B, {"Chennel Up"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_CHDOWN,      0x0A, {"Channel Down"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_ONE,         0x31, {"One"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TWO,         0x32, {"Two"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_THREE,       0x33, {"Three"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_FOUR,        0x34, {"Four"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_FIVE,        0x35, {"Five"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_SIX,         0x36, {"Six"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_SEVEN,       0x37, {"Seven"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_EIGHT,       0x38, {"Eight"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_NINE,        0x39, {"Nine"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_ZERO,        0x30, {"Zero"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_ENTERTV,     0x0D, {"Enter TV"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_PAUSE,      OPTV_PAUSE, {"Pause"}},
    /* GMCK adding menu keys for Cinema 7 */
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_MENU_LEFT,    0xCE, {"Menu Left TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_MENU_RIGHT,   0x4E, {"Menu Rght TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_MENU_UP,      0x0E, {"Menu Up TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_MENU_DOWN,    0x8E, {"Menu Down TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_MENU_SEL,     0xCC, {"Menu Sel TV"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_MENU_LEFT,  0x21, {"Menu Left VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_MENU_RIGHT, 0xC1, {"Menu Rght VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_MENU_UP,    0x00, {"Menu Up VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_MENU_DOWN,  0x80, {"Menu Down VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_MENU_SEL,   0xA6, {"Menu Sel VCR"}},
    /* CRL add for Cinema 7, YYD */
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_SLEEP,        0x0C01, {"Sleep TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_MENU,         0x0C02, {"Menu TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_PROG,         0x0C03, {"Prog/Guide TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_EXIT,         0x0C04, {"Exit TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_SURROUND,     0x0C05, {"Surround TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_DISPLAY,      0x0C06, {"Display TV"}},
    {HITACHI_TV_GROUP, HITACHI_KEY_TV_ENTER,        0x0C07, {"Enter TV"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_MENU,       0x0D01, {"Menu VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_PROG,       0x0D02, {"ProgGuide VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_EXIT,       0x0D03, {"Exit VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_SURROUND,   0x0D04, {"Surround VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_DISPLAY,    0x0D05, {"Display VCR"}},
    {HITACHI_VCR_GROUP, HITACHI_KEY_VCR_ENTER,      0x0D06, {"Enter VCR"}},
    {0, 0, 0, {0}}          // YYD, I added a stop flag. Thus we don't have to count the number 
                            // of keys. 
    /* CRL add */
};

#else   // #ifndef  _IR_RAWMODE_    // receive all codes

#endif

#endif  // _DRV_IRCOMBO_OSI_HITACHI_LOCAL_H_INC_
