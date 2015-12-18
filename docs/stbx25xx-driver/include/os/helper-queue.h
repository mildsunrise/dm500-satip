//pallas/drv/include/os/helper-queue.h
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
//  Common Queue helper routines  
//Revision Log:   
//  Sept/03/2001            Created by YYD
//  Sept/21/2001            Added multiple element I/O by YYD

#ifndef  _DRV_INCLUDE_OS_HELPER_QUEUE_H_INC_
#define  _DRV_INCLUDE_OS_HELPER_QUEUE_H_INC_

#include "os-types.h"

struct __QUEUE_T_STRUCT 
{
    void *pQueue;
    UINT uQueueSize;
    UINT uElementSize;
    UINT uReadHead;
    UINT uWriteHead;
    UINT uAttr;    // internel attribute
};

typedef struct __QUEUE_T_STRUCT QUEUE_T;

INT os_create_queue (QUEUE_T *pQueue, void *pBuffer, UINT uElements, UINT uElementSize);

INT os_delete_queue (QUEUE_T *pQueue);

INT os_enqueue (QUEUE_T *pQueue, void *pBuffer);

INT os_dequeue (QUEUE_T *pQueue, void *pBuffer);

void *os_enqueue_fast_start (QUEUE_T *pQueue);

INT os_enqueue_fast_finish (QUEUE_T *pQueue);

void *os_dequeue_fast_start (QUEUE_T *pQueue);

INT os_dequeue_fast_finish (QUEUE_T *pQueue);

INT os_flush_queue (QUEUE_T *pQueue);

INT os_get_queue_status (QUEUE_T *pQueue);

// added 
void *os_enqueue_multiple_start (QUEUE_T *pQueue, UINT uWhichElement);

INT os_enqueue_multiple_finish (QUEUE_T *pQueue, UINT uWhichElement);

void *os_dequeue_multiple_start (QUEUE_T *pQueue, UINT uWhichElement);

INT os_dequeue_multiple_finish (QUEUE_T *pQueue, UINT uWhichElement);

#endif  //  _DRV_INCLUDE_OS_HELPER_QUEUE_H_INC_

