/*----------------------------------------------------------------------------+
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
/*----------------------------------------------------------------------------+
|   Author    :  Ian Govett
|   Component :  xp
|   File      :  pvr_osd.h
|   Purpose   :  header file for PVR API 
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  --------------------------------------------------------
|	10-OCt-01  LGH	Created
+----------------------------------------------------------------------------*/
//#include <linux/fs.h>

#define	PVR_IOC_MAGIC 'y'
#define PVR_SET_MODE		_IO(PVR_IOC_MAGIC,1)
#define PVR_STOP		_IO(PVR_IOC_MAGIC,2)
#define PVR_GET_STATUS		_IO(PVR_IOC_MAGIC,3)
#define PVR_BUFF_FLUSH          _IO(PVR_IOC_MAGIC,4)
#define PVR_SET_VID_PID		_IO(PVR_IOC_MAGIC,5)
#define PVR_ALLOCATE_BUFFER	_IO(PVR_IOC_MAGIC,6)
#define PVR_FREE_BUFFER 	_IO(PVR_IOC_MAGIC,7)
#define PVR_WRITE_BUFFER	_IO(PVR_IOC_MAGIC,8)

typedef enum pvr_usr_playback_mode_t
{
	PLAY_WORD = 0,
	PLAY_LINE
} PVR_USR_PLAYBACK_MODE;

typedef struct pvr_usr_status_t
{
	int Empty;
	int Full;
} PVR_USR_STATUS;

struct pvr_write_buffer_t
{
        void *vaddr;
	unsigned long len;
};	

typedef struct pvr_write_buffer_t PVR_WRITE_BUFFER_PARAM;	

struct pvr_allocate_buffer_t
{
        void *vaddr;
	unsigned long len;
}; 	

typedef struct pvr_allocate_buffer_t PVR_ALLOCATE_BUFFER_PARAM;	

				 
