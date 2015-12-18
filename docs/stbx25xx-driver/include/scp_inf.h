//vulcan/drv/include/gfx/gfx_inf.h
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
//  Linux device driver interface of Serial Control Port (SCP) 
//Revision Log:   

#ifndef  _DRV_INCLUDE_SCP_INF_H_INC_
#define  _DRV_INCLUDE_SCP_INF_H_INC_

#include <linux/ioctl.h>

typedef struct
{
    char *write_ptr;
    char *read_ptr;
    int count;
} SCP_RW_STRUCT;

#define SCP_SUCCESS             0
#define SCP_NOT_INITIALIZED     1
#define SCP_ABORT               2

#define SCP_READY               0
#define SCP_BUSY                1

#define SCP_BUFFER_SIZE 2048

#define SCP_DEV_MAJOR   (0xF8)  // device major 248

#define SCP_DEV_NAME    "/dev/scp"

#define SCP_IOC_MAGIC   (0xF78)  //  magic number to identify me

#define IOCTL_SCP_DISPLAY_REGS  		_IO  (SCP_IOC_MAGIC, 2)
#define IOCTL_SCP_GET_CDM        		_IOR  (SCP_IOC_MAGIC, 3, unsigned char)
#define IOCTL_SCP_SET_CDM        		_IOW  (SCP_IOC_MAGIC, 4, unsigned char)
#define IOCTL_SCP_GET_REV_DATA        		_IOR  (SCP_IOC_MAGIC, 5, unsigned char)
#define IOCTL_SCP_SET_REV_DATA     		_IO  (SCP_IOC_MAGIC, 6)
#define IOCTL_SCP_GET_CLK_INV        		_IOR  (SCP_IOC_MAGIC, 7, unsigned char)
#define IOCTL_SCP_SET_CLK_INV   		_IO  (SCP_IOC_MAGIC, 8)
#define IOCTL_SCP_GET_LOOPBACK        		_IOR  (SCP_IOC_MAGIC, 9, unsigned char)
#define IOCTL_SCP_SET_LOOPBACK      		_IO  (SCP_IOC_MAGIC, 10)
//#define IOCTL_SFLASH_GET_STATUS                 _IOR (SCP_IOC_MAGIC, 11 , unsigned char)
//#define IOCTL_SFLASH_TEST	                _IO  (SCP_IOC_MAGIC, 12)
#define IOCTL_SCP_RW                            _IOWR (SCP_IOC_MAGIC, 12, unsigned char)
#define IOCTL_SFLASH_INIT        		_IO  (SCP_IOC_MAGIC, 13)

     



#endif //  _DRV_INCLUDE_SCP_INF_H_INC_
