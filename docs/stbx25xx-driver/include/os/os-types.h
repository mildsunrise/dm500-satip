//vulcan/drv/include/os/os-types.h
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
//  A common architecture independent type defs 
//Revision Log:   
//  Aug/31/2001			Created by YYD

#ifndef  _DRV_INCLUDE_OS_OS_TYPES_H_INC_
#define  _DRV_INCLUDE_OS_OS_TYPES_H_INC_


// Architecture specific
typedef int INT;
typedef unsigned int UINT;

typedef short int SHORT;
typedef unsigned short int USHORT;

typedef unsigned char BYTE;
typedef unsigned char UCHAR;
typedef signed char SBYTE;

typedef long LONG;
typedef unsigned long ULONG;


//try to be architecture independent
// for PPC 4xx, the following is true
typedef signed char INT8;
typedef unsigned char UINT8;

typedef short int INT16;
typedef unsigned short int UINT16;

typedef int INT32;
typedef unsigned int UINT32;

// only gcc support these
typedef long long int INT64;
typedef unsigned long long int UINT64;

#define INLINE	inline

#endif  //  _DRV_INCLUDE_OS_OS_TYPES_H_INC_



