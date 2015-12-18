//pallas/drv/include/gpt/gpt_ports.h
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
//  Defination of GPT port relative and bases 
//Revision Log:   
//  Sept/07/2001			Created by YYD

#ifndef  _DRV_INCLUDE_GPT_GPT_PORTS_H_INC_
#define  _DRV_INCLUDE_GPT_GPT_PORTS_H_INC_

#define __GPT_TIME_BASE     13500000            // 13.5 MHz

// for Pallas
#define __GPT_PORT_BASE     0x40050000          // the base port address
#define __GPT_PORT_RANGE    0x00000128          // the port address range

#define __GPT_TBC           (0x0000)         // 0x40050000  
#define __GPT_GPTCE         (0x0004)         // 0x40050004
#define __GPT_GPTEC         (0x0008)         // 0x40050008
#define __GPT_GPTSC         (0x000C)         // 0x4005000C
#define __GPT_GPTOE         (0x0010)         // 0x40050010
#define __GPT_GPTOL         (0x0014)         // 0x40050014
#define __GPT_GPTIM         (0x0018)         // 0x40050018
#define __GPT_GPTISS        (0x001C)         // 0x4005001C
#define __GPT_GPTISC        (0x0020)         // 0x40050020
#define __GPT_GPTIE         (0x0024)         // 0x40050024
#define __GPT_CAPT0         (0x0040)         // 0x40050040
#define __GPT_CAPT1         (0x0044)         // 0x40050044
#define __GPT_COMP0         (0x0080)         // 0x40050080
#define __GPT_COMP1         (0x0084)         // 0x40050084
#define __GPT_COMP2         (0x0088)         // 0x40050088
#define __GPT_COMP3         (0x008C)         // 0x4005008C
#define __GPT_COMP4         (0x0090)         // 0x40050090
#define __GPT_COMP5         (0x0094)         // 0x40050094
#define __GPT_COMP6         (0x0098)         // 0x40050098
#define __GPT_MASK0         (0x00C0)         // 0x400500C0
#define __GPT_MASK1         (0x00C4)         // 0x400500C4
#define __GPT_MASK2         (0x00C8)         // 0x400500C8
#define __GPT_MASK3         (0x00CC)         // 0x400500CC
#define __GPT_MASK4         (0x00D0)         // 0x400500D0
#define __GPT_MASK5         (0x00D4)         // 0x400500D4
#define __GPT_MASK6         (0x00D8)         // 0x400500D8
#define __GPT_PWMW0         (0x0100)         // 0x40050100
#define __GPT_PWMW1         (0x0104)         // 0x40050104
#define __GPT_PWM_VFG_CTRL  (0x010C)         // 0x4005010C
#define __GPT_DCT0          (0x0110)         // 0x40050110
#define __GPT_DCT1          (0x0114)         // 0x40050114
#define __GPT_DCT2          (0x0118)         // 0x40050118
#define __GPT_DCTIS         (0x011C)         // 0x4005011C
#define __GPT_VFGPCNT       (0x0120)         // 0x40050120
#define __GPT_VFGPWCNT      (0x0124)         // 0x40050124


#endif  //  _DRV_INCLUDE_GPT_GPT_PORTS_H_INC_



