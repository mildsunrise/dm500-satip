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
/*----------------------------------------------------------------------------+
|
|  Author    :  Lin Guo Hui
|  Component :
|  File      :
|  Purpose   :  user space API headerfile
|  Changes   :
|  Date         Comments
|  ----------------------------------------------------------------------
|  25-JUN-2001    Created
|  30-Sep-2001    Update for Pallas
|  25-Mar-2002    Add match_id to struct demux_filter_t,
|                 add struct DEMUX_CHANNEL to struct demux_device_s
|  24-Api-2002    Add bucket support
+----------------------------------------------------------------------------*/
#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
#include <linux/wait.h>
#include <xp/xp_osd_user.h>
#include <os/os-generic.h>
#include <os/os-types.h>
#include <os/pm-alloc.h>
#include <os/os-interrupt.h>
#include "hw/hardware.h"

/*----------------------------------------------------------------------------+
| Defines
+----------------------------------------------------------------------------*/
#define WAIT_QUEUE  wait_queue_head_t

//Filter type definition
#define FILTER_TYPE_SEC        0
#define FILTER_TYPE_PES        1
#define FILTER_TYPE_TS         2
#define FILTER_TYPE_BUCKET     3

//filter status definition
#define FILTER_STAT_FREE       0
#define FILTER_STAT_ALLOC      1
#define FILTER_STAT_SET        2
#define FILTER_STAT_READY      3
#define FILTER_STAT_START      4
#define FILTER_STAT_START_CC   5

//filter flag definition
#define FILTER_FLAG_UNBUFFERED 1
#define FILTER_FLAG_MMAPPED    2

//interrupt characteristic
#define IRQ_DEFAULT_TRIG (IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG)

#define XP_TYPE_PVR 3               //pvr device minor number

#define MAX_DEMUX_FILTER_NUM    128
#define MAX_DEMUX_CHANNEL_NUM   32
#define DEFAULT_BUF_SIZE        8192

#ifdef __DRV_FOR_PALLAS__
#define MAX_XP_NUM  3
#else
#define MAX_XP_NUM  1
#endif

#define MAX_BUF_ITEM    128

//reserved 2M buf for every demux
#define DEFAULT_RESERVED_BUF_SIZE   0x200000


#define FILTER_LENGTH           16

#define ILLEGAL_CHANNEL ((unsigned int)(-1))
#define ILLEGAL_FILTER  ((unsigned int)(-1))

#define DEMUX_DISABLE   0
#define DEMUX_ENABLE    1
#define DEMUX_RESET     2

#define TABLE_ID_CAPABILITY             0
#define TABLE_ID_EXTENSION_CAPABILITY   1
#define VERSION_NUMBER_CAPABILITY       2
#define FIRST_WORD_PAYLOAD_CAPABILITY   3
#define SECOND_WORD_PAYLOAD_CAPABILITY  4

#define CHAN_CHAN_HARDWARE  0
#define CHAN_CHAN_SOFTWARE  1

/*----------------------------------------------------------------------------+
| Declarations
+----------------------------------------------------------------------------*/
typedef struct demux_filter_t      DEMUX_FILTER;
typedef struct demux_device_s      DEMUX_DEVICE;
typedef struct mpeg_section_header *SECTION_HEADER_PTR;
typedef struct demux_filter_buf_s  DEMUX_FILTER_BUF;
typedef struct o_channel_t         DEMUX_CHANNEL;

struct o_channel_t
{
    UINT              chid;
    INT               inuse;
    XP_CHANNEL_STATUS state;
};

struct demux_filter_buf_s
{
    UCHAR  *plData;
    ULONG ulSize;               //buffer size specified for section or packet recieving in internal buffer circle queue
    INT   count;                //number of buffer items in the buffer pool
    ULONG ulRead;               //pointer to buffer pool reading
    ULONG ulWrite;              //pointer to buffer pool writing
    UCHAR *plBQueue;            //pointer to the start addr of internal buffer queue
    UCHAR *plEQueue;            //pointer to the end addr of internal buffer queue
    WAIT_QUEUE queue;           //wait queue for asyncronize event processing
    int error;
};

//Filter data structure
struct demux_filter_t
{
    UINT chid;                  //channel ID filter attached
    UINT fid;                   //Filter ID

    int type;                   //FIlter type (Sec,PES,TS etc.)
    int states;                 //status of the filter
    filter_para para;           //Section filter parameters
    Pes_para pes_para;          //PES filter parameters
    bucket_para bucket_para;    //Bucket filter parameters

    ULONG ulNotifySize;         //size of data recieved
    ULONG ulMatchWord;          //bit mask of filters "hit"

    DEMUX_FILTER_BUF buffer;    //internel buffer filter used

    DEMUX_DEVICE *pDemuxDev;    //point to demux device the filter belongs
    USHORT pid;                 //PID
    int flags;
    struct fasync_struct *async_queue;  // for asyncronized access
};

//demux device
struct demux_device_s
{
    int uDeviceIndex;          //device index, maybe 0,1,2, for Pallas
    int uFilterNum;            //filter number
    DEMUX_CHANNEL chid[MAX_DEMUX_CHANNEL_NUM];  //channel data structure including chid and inuse number
    DEMUX_FILTER filter[MAX_DEMUX_FILTER_NUM];  //filter data structure
    int uAlreadyInit;          //Is this device has been initialized?
    int users;                 //users in this device
};

//MPEG2 Section header
#pragma pack(1)
struct mpeg_section_header
{
    unsigned table_id:8;        //Section table ID
    unsigned syntax_ind:1;
    unsigned reserved:3;
    unsigned sectionLength:12;  //section length
};
#pragma pack()

/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/
int demux_filter_release(
   UINT         uDeviceIndex,
   struct inode *inode,
   struct file  *file);

int demux_filter_open(
   DEMUX_DEVICE *pDemuxDev,
   UINT         uDeviceIndex,
   struct inode *inode,
   struct file  *filp);

ssize_t demux_filter_read(
   UINT         uDeviceIndex,
   struct file  *file,
   char         *buf,
   size_t       count,
   loff_t       *ppos);

int demux_filter_ioctl(
   UINT         uDeviceIndex,
   struct inode *inode,
   struct file  *filp,
   unsigned int cmd,
   ULONG        arg);

unsigned int demux_filter_poll(
   UINT                     uDeviceIndex,
   struct file              *file,
   struct poll_table_struct *wait);

int demux_filter_mmap(
   UINT         uDeviceIndex,
   struct file  *file,
   struct vm_area_struct *vma);
   
