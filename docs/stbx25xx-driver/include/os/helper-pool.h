//pallas/drv/include/os/helper-pool.h
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
//  Common Pool helper routines  
//Revision Log:   
//  Sept/26/2001            Created by YYD

#ifndef  _DRV_INCLUDE_OS_HELPER_POOL_H_INC_
#define  _DRV_INCLUDE_OS_HELPER_POOL_H_INC_

#include "os-types.h"

typedef struct __OS_POOL_STRUCTURE_
{
    void *pBuffer;
    void *pPoolHead;
    UINT uFreeElements;
    UINT uElements;
    UINT uElementSize_plus_pointer;
    UINT uAttr;    // internel attribute
} POOL_T;

#define os_tell_pool_buffer_size(uElements, uElementSize) \
    ((UINT)(uElements)*((UINT)(uElementSize)+sizeof(void *)))

INT os_create_pool (POOL_T *pPool, void *pBuffer, UINT uElements, UINT uElementSize);

void * os_get_from_pool (POOL_T *pPool);

INT os_put_back_to_pool(POOL_T *pPool, void *pElement);

INT os_delete_pool (POOL_T *pPool);

INT os_get_pool_status(POOL_T *pPool);

INT os_validate_pool_element(POOL_T *pPool, void *pElement);

void * os_walk_up_pool(POOL_T *pPool, UINT uWhichElement);

#endif  //  _DRV_INCLUDE_OS_HELPER_QUEUE_H_INC_

