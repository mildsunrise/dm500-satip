//pallas/drv/include/os/os-sync.h
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
//  Protytypes of OS independent syncronization helper functions 
//Revision Log:   
//  Sept/06/2001                          Created by YYD

#ifndef  _DRV_INCLUDE_OS_OS_SYNC_H_INC_
#define  _DRV_INCLUDE_OS_OS_SYNC_H_INC_

#include "os-types.h"

typedef void* SEMAPHORE_T; 

SEMAPHORE_T os_create_semaphore(UINT uInitialValue);
INT  os_wait_semaphore(SEMAPHORE_T semaphore, INT nTimeout);
INT  os_try_wait_semaphore(SEMAPHORE_T semaphore, INT nTimeout);
void os_post_semaphore(SEMAPHORE_T semaphore);
void os_delete_semaphore(SEMAPHORE_T semaphore);


typedef void* MUTEX_T;     

MUTEX_T os_create_mutex (void);
INT		os_get_mutex(MUTEX_T mutex);
INT     os_try_get_mutex(MUTEX_T mutex);
void	os_release_mutex(MUTEX_T mutex);
void	os_delete_mutex(MUTEX_T mutex);

UINT32 os_enter_critical_section();
void   os_leave_critical_section(UINT32 flags);

void   os_sleep(ULONG ticks);   //1tick = 10ms
void   os_delay(ULONG usecs);   //1usec = 1us
void   os_cpu_clock_delay(ULONG clocks); //wait cpu clocks

#endif  //  _DRV_INCLUDE_OS_OS_SYNC_H_INC_




