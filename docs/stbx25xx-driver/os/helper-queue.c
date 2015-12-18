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
//  Sept/05/2001            Created by YYD
//  Sept/21/2001            Added multiple element I/O by YYD


#include "os/helper-queue.h"
#include "helper-queue-local.h"

#ifndef __HELPER_QUEUE_TEST__
    #include "os/os-generic.h"      // for MALLOC, FREE, _OS_MEMCPY
#else
    #include <malloc.h>
    #include <string.h>
    #define MALLOC(a) malloc(a)
    #define FREE(a) free(a)
    #define _OS_MEMCPY(a,b,c) memcpy(a,b,c)
#endif

#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif


INT os_create_queue (QUEUE_T *pQueue, void *pBuffer, UINT uElements, UINT uElementSize)
{
    if (NULL == pQueue) return -1;
    if (0 == uElements*uElementSize) return -1;

    // init the buffer
    if (NULL != pBuffer) // user provides an buffer
    {
        pQueue->pQueue = pBuffer;
        pQueue->uAttr = 0;
    }
    else
    {
        pQueue->pQueue = MALLOC(uElements*uElementSize);
        if (NULL == pQueue->pQueue) return -1;
        pQueue->uAttr = __HELPER_QUEUE_MEMALLOC;
    }
    
    _OS_MEMSET(pQueue->pQueue, 0, uElements*uElementSize);
    
    pQueue->uElementSize = uElementSize;
    pQueue->uQueueSize = uElements;
    pQueue->uReadHead = 0;
    pQueue->uWriteHead = 0;
    
    return 0;
}

INT os_delete_queue (QUEUE_T *pQueue)
{
    if (NULL == pQueue || NULL == pQueue->pQueue) return 0;
    pQueue->uReadHead = 0;
    pQueue->uWriteHead = 0;
    if (pQueue->uAttr & __HELPER_QUEUE_MEMALLOC)  FREE(pQueue->pQueue);
    pQueue->pQueue = NULL;
    pQueue->uAttr = 0;
    pQueue->uElementSize = 0;
    pQueue->uQueueSize = 0;
    return 0;
}


INT os_enqueue (QUEUE_T *pQueue, void *pBuffer)
{
    if (NULL == pQueue || NULL == pQueue->pQueue || 
        NULL == pBuffer || (pQueue->uWriteHead + 1)%pQueue->uQueueSize == pQueue->uReadHead ) return -1;
    _OS_MEMCPY((BYTE *)pQueue->pQueue + pQueue->uElementSize*pQueue->uWriteHead, pBuffer, pQueue->uElementSize);
    pQueue->uWriteHead ++;
    if (pQueue->uWriteHead >= pQueue->uQueueSize) pQueue->uWriteHead = 0;  // wrap around
    return 0;
}

INT os_dequeue (QUEUE_T *pQueue, void *pBuffer)
{
    if (NULL == pQueue || NULL == pQueue->pQueue || 
        NULL == pBuffer || pQueue->uWriteHead == pQueue->uReadHead) return -1;
    _OS_MEMCPY(pBuffer, (BYTE *)pQueue->pQueue + pQueue->uElementSize*pQueue->uReadHead, pQueue->uElementSize);
    pQueue->uReadHead ++;
    if (pQueue->uReadHead >= pQueue->uQueueSize) pQueue->uReadHead = 0;  // wrap around
    return 0;
}

void *os_enqueue_fast_start (QUEUE_T *pQueue)
{
    if (NULL == pQueue || NULL == pQueue->pQueue || (pQueue->uWriteHead + 1)%pQueue->uQueueSize == pQueue->uReadHead) return NULL;
    return (BYTE *)pQueue->pQueue + pQueue->uElementSize*pQueue->uWriteHead;
}

INT os_enqueue_fast_finish (QUEUE_T *pQueue)
{
    if (NULL == pQueue || NULL == pQueue->pQueue || (pQueue->uWriteHead + 1)%pQueue->uQueueSize == pQueue->uReadHead) return -1;
    pQueue->uWriteHead ++;
    if (pQueue->uWriteHead >= pQueue->uQueueSize) pQueue->uWriteHead = 0;  // wrap around
    return 0;
}

void *os_dequeue_fast_start (QUEUE_T *pQueue)
{
    if (NULL == pQueue || NULL == pQueue->pQueue || pQueue->uWriteHead == pQueue->uReadHead) return NULL;
    return (BYTE *)pQueue->pQueue + pQueue->uElementSize*pQueue->uReadHead;
}

INT os_dequeue_fast_finish (QUEUE_T *pQueue)
{
    if (NULL == pQueue || NULL == pQueue->pQueue || pQueue->uWriteHead == pQueue->uReadHead) return -1;
    pQueue->uReadHead ++;
    if (pQueue->uReadHead >= pQueue->uQueueSize) pQueue->uReadHead = 0;  // wrap around
    return 0;
}

INT os_flush_queue (QUEUE_T *pQueue)
{
    if (NULL == pQueue || NULL == pQueue->pQueue) return -1;
    pQueue->uReadHead = 0;
    pQueue->uWriteHead = 0;
    return 0;
}

INT os_get_queue_status (QUEUE_T *pQueue)
{
    if (NULL == pQueue || NULL == pQueue->pQueue) return -1;
    return (INT)((pQueue->uWriteHead + pQueue->uQueueSize - pQueue->uReadHead)%pQueue->uQueueSize);
}


void *os_enqueue_multiple_start (QUEUE_T *pQueue, UINT uWhichElement)
{
    if (NULL == pQueue || NULL == pQueue->pQueue || (uWhichElement+1) >= pQueue->uQueueSize
        || (pQueue->uWriteHead + uWhichElement + 1)%pQueue->uQueueSize >= pQueue->uReadHead ) return NULL;
    return (BYTE *)pQueue->pQueue + 
        pQueue->uElementSize*((pQueue->uWriteHead+uWhichElement)%pQueue->uQueueSize);
}

INT os_enqueue_multiple_finish (QUEUE_T *pQueue, UINT uWhichElement)
{
    if (NULL == pQueue || NULL == pQueue->pQueue || (uWhichElement+1) >= pQueue->uQueueSize
        || (pQueue->uWriteHead + uWhichElement + 1)%pQueue->uQueueSize >= pQueue->uReadHead ) return -1;
    pQueue->uWriteHead += uWhichElement;
    if (pQueue->uWriteHead >= pQueue->uQueueSize) pQueue->uWriteHead = pQueue->uWriteHead%pQueue->uQueueSize;  // wrap around
    return 0;
}

void *os_dequeue_multiple_start (QUEUE_T *pQueue, UINT uWhichElement)
{
    if (NULL == pQueue || NULL == pQueue->pQueue 
        || (pQueue->uWriteHead + pQueue->uQueueSize - pQueue->uReadHead)%pQueue->uQueueSize <= uWhichElement) return NULL;
    return (BYTE *)pQueue->pQueue + pQueue->uElementSize*((pQueue->uReadHead+uWhichElement)%pQueue->uQueueSize);
}

INT os_dequeue_multiple_finish (QUEUE_T *pQueue, UINT uWhichElement)
{
    if (NULL == pQueue || NULL == pQueue->pQueue 
        || (pQueue->uWriteHead + pQueue->uQueueSize - pQueue->uReadHead)%pQueue->uQueueSize <= uWhichElement) return -1;
    pQueue->uReadHead += uWhichElement;
    if (pQueue->uReadHead >= pQueue->uQueueSize) pQueue->uReadHead = pQueue->uReadHead%pQueue->uQueueSize;  // wrap around
    return 0;
}


