//pallas/drv/os/helper-pool-local.h
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
//  Private header of Common Pool helper routines  
//Revision Log:   
//  Sept/27/2001            Created by YYD

#ifndef  _DRV_OS_HELPER_POOL_LOCAL_H_INC_
#define  _DRV_OS_HELPER_POOL_LOCAL_H_INC_

#define __HELPER_POOL_MEMALLOC    0x0001

typedef struct __POOL_CHAIN_STRUCTURE__
{
    struct __POOL_CHAIN_STRUCTURE__ *pNext;
} POOL_CHAIN_T;

#endif  //  _DRV_OS_HELPER_QUEUE_LOCAL_H_INC_

