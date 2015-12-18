/*xp/ts.h, redwood_archive, redwood_1.0 5/10/99 13:26:40*/
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
| OpenTV Operating System
| Author: Paul Gramann
| Component: xp
| File: xp_osd_user.h
| Purpose: Transport defines.
| Changes:
| Date:		Comment:
| -----         --------
| 05-Apr-99     Created							    PAG
| 04-May-99     Placed in library					    PAG
| 10-May-99     Removed "pack" pragma					    MPT
| 25-Jun-01		Add some structures application needed      LGH				
| 30-Sep-01		ported to Linux, update for pallas			LGH
| 10-Oct-01		Add select source ioctl						LGH
| 31-Oct-01     Add struct UnloaderConfig in demux_pes_para LGH
| 10-Apil-02	Add positive enable in section filter para, add get filter num ioctl, for PLR LGH
| 24-Apil-02	Add bucket queue support
| 02-Jun-02		Add STC event process
+----------------------------------------------------------------------------*/



#ifndef TS_H
#define TS_H
#pragma pack(1)

//Demux API ioctl command definition
#define	DEMUX_IOC_MAGIC                   'x'
#define DEMUX_START                       _IO(DEMUX_IOC_MAGIC,1)
#define DEMUX_STOP                        _IO(DEMUX_IOC_MAGIC,2)
#define DEMUX_FILTER_SET                  _IO(DEMUX_IOC_MAGIC,3)
#define DEMUX_FILTER_PES_SET              _IO(DEMUX_IOC_MAGIC,4)
#define DEMUX_SET_BUFFER_SIZE             _IO(DEMUX_IOC_MAGIC,5)
#define DEMUX_FILTER_TS_SET               _IO(DEMUX_IOC_MAGIC,6)
#define DEMUX_SELECT_SOURCE               _IO(DEMUX_IOC_MAGIC,7)
#define DEMUX_GET_FILTER_NUM              _IO(DEMUX_IOC_MAGIC,8)
#define DEMUX_SET_DEFAULT_FILTER_LENGTH   _IO(DEMUX_IOC_MAGIC,9)
#define DEMUX_FILTER_BUCKET_SET           _IO(DEMUX_IOC_MAGIC,10)
#define DEMUX_REGISTER_STC                _IO(DEMUX_IOC_MAGIC,11)
#define DEMUX_GET_STC_EVENT               _IO(DEMUX_IOC_MAGIC,12)
#define DEMUX_RELEASE_STC_EVENT           _IO(DEMUX_IOC_MAGIC,13)
#define DEMUX_GET_CURRENT_STC             _IO(DEMUX_IOC_MAGIC,14)
#define DEMUX_SET_ACPM                    _IO(DEMUX_IOC_MAGIC,15)
#define DEMUX_SET_VCPM                    _IO(DEMUX_IOC_MAGIC,16)
#define DEMUX_FILTER_SET_FLAGS            _IO(DEMUX_IOC_MAGIC,17)
#define DEMUX_FILTER_GET_QUEUE            _IO(DEMUX_IOC_MAGIC,18)
#define DEMUX_FILTER_SET_READPTR          _IO(DEMUX_IOC_MAGIC,19)

#define SIZEOF_PMT_MAP_TYPE     5

#define LENGTH_TABLE_HEADER 3
#define LENGTH_TABLE_CRC	4

#define FILTER_LENGTH	16

typedef enum _OutDevice OutDevice;
typedef enum P_Type PesType;

typedef struct demux_filter_para filter_para;
typedef struct demux_pes_para Pes_para;
typedef struct UnloaderConfig_t UnloaderConfig;     //lingh added in Oct.31
typedef struct demux_bucket_para bucket_para;
typedef struct demux_queue_para queue_para;

typedef enum unloader_type_t
{
    UNLOADER_TYPE_TRANSPORT = 0x0,              //all 188 bytes of the transport packet.
    UNLOADER_TYPE_ADAPTATION = 0x1,             //the 4-byte transport header and the adaptation field.
    UNLOADER_TYPE_ADAPTATION_PRIVATE = 0x2,     //the private data field within the adaptation field.
    UNLOADER_TYPE_PAYLOAD = 0x3,                //the transport packet payload.
    UNLOADER_TYPE_PAYLOAD_AND_BUCKET = 0x4,     //same as Payload, and with the transport header and the adaptation header delivered to the bucket queue.
    UNLOADER_TYPE_BUCKET = 0x5,                 //Transport header and adaptation header delivered to the bucket queue.
    UNLOADER_TYPE_PSI = 0x8,                    //deliver table sections.
    UNLOADER_TYPE_FILTER_PSI = 0x9,             //deliver table sections that match at least one of the table section filters defined for the queue.
    UNLOADER_TYPE_PSI_CRC = 0xa,                //deliver table sections and check them for CRC32 errors.
    UNLOADER_TYPE_FILTER_PSI_CRC = 0xb,         //the combination of the two previous types.
    UNLOADER_TYPE_PSI_BUCKET = 0xc,             //the same as Table Section, with the transport header and adaptation field delivered to the bucket queue.
    UNLOADER_TYPE_FILTER_PSI_BUCKET = 0xd,      //the same as Table Section with Filtering, with the addition of delivering the transport header and adaptation field to the bucket queue.
    UNLOADER_TYPE_PSI_CRC_BUCKET = 0xe,         //same as Table Section with CRC32 Checking, with the addition of delivering the transport header and the adaptation field to the bucket queue.
    UNLOADER_TYPE_FILTER_PSI_CRC_BUCKET = 0xf,  //same as Table Section with Filter and CRC32 Checking, with the addition of delivering the transport header and adaptation field to the bucket queue.
    UNLOADER_TYPE_UNDEFINED = 0x10
} UNLOADER_TYPE;

struct  UnloaderConfig_t
{
    UNLOADER_TYPE   unloader_type;          //specifies which data from the packet is to be delivered to the queue.
    unsigned long   threshold;              //This value indicates the number of 256 byte boundaries before generating an interrupt for this queue.
};

struct demux_bucket_para
{
    struct UnloaderConfig_t unloader;
};

typedef enum stream_source_t
{
    INPUT_FROM_CHANNEL0,
    INPUT_FROM_CHANNEL1,
    INPUT_FROM_1394,
    INPUT_FROM_PVR
} STREAM_SOURCE;

enum _OutDevice
{
    OUT_DECODER,                        /*output to A/V decoder directly*/
    OUT_MEMORY,                         /*output to memory*/
    OUT_NOTHING                         /*no output*/
};



enum P_Type
{
    DMX_PES_AUDIO,                      /*Audio PES*/
    DMX_PES_VIDEO,                      /*Video*/
    DMX_PES_TELETEXT,	                /*Teletext*/
    DMX_PES_SUBTITLE,                   /*subtitle*/
    DMX_PES_PCR,                        /*PCR*/
    DMX_PES_OTHER
};

//filter flags definition
#define FILTER_FLAG_NONBUFFERED 1

struct demux_filter_para
{
    unsigned char filter[FILTER_LENGTH];	/*table section filter*/
    unsigned char mask[FILTER_LENGTH];		/*table section filter mask*/
    unsigned char positive[FILTER_LENGTH];	/* positive filterring enabler*/
    int filter_length;				/*number of bytes of the filter*/
    unsigned short pid;				/*table section program PID*/
    unsigned int timeout;
};

struct demux_pes_para
{
    unsigned short pid;				/* PES data program PID*/
    OutDevice output;				/* Output device after recieving the PES*/
    PesType pesType;				/* PES data type*/
    UnloaderConfig  unloader;           	/* PES unloader configuration*/
                                        	/* Only available when OUT_MEMORY */
                                        	/* lingh added in Oct.31*/
};

struct demux_queue_para
{
  unsigned readptr;
  unsigned writeptr;
};

typedef struct pat_map_type
{
	unsigned programNumber_hi:8;        /* program id for the map pid */
    unsigned programNumber_lo:8;        /* program id for the map pid */
    unsigned reserved:3;
    unsigned pid:13;            /* network or program Pid number */
}PAT_MAP_TYPE, *PAT_MAP_PTR;

typedef struct pmt_map_type
{
    unsigned streamType:8;      /* type of elementary stream */
    unsigned reserved_1:3;
    unsigned pid:13;            /* elementary stream pid number */
    unsigned reserved_2:4;
    unsigned info_len1:4;       /* length of descriptor */
    unsigned info_len2:8;       /* length of descriptor */
}PMT_MAP_TYPE, *PMT_MAP_PTR;



typedef struct pat_type
{
    unsigned table_id:8;        /* table type */
    unsigned syntax_ind:1;      /* section syntax indicator */
    unsigned reserved_1:3;      /* */
    unsigned sectionLength:12;  /* length of the remaining data */
    unsigned streamId_hi:8;     /* transport stream id */
    unsigned streamId_lo:8;     /* transport stream id */
    unsigned reserved_2:2;
    unsigned version:5;         /* version of the PAT */
    unsigned current_next:1;    /* 1=use current, 0=use next */
    unsigned sectionNumber:8;   /* current section number */
    unsigned lastSectionNumber:8;       /* last section number for the PAT */
	PAT_MAP_TYPE map[1];        /* 1 or more program/pid mapping */
}PAT_TYPE, *PAT_PTR;

typedef struct pmt_type
{
	unsigned table_id:8;        /* table type */
    unsigned syntax_ind:1;      /* section syntax indicator */
    unsigned reserved_1:3;      /* */
    unsigned sectionLength:12;  /* length of the remaining data */
    unsigned programId_hi:8;    /* program number */
    unsigned programId_lo:8;    /* program number */
    unsigned reserved_2:2;
    unsigned version:5;         /* version of the PAT */
    unsigned current_next:1;    /* 1=use current, 0=use next */
    unsigned sectionNumber:8;   /* current section number */
    unsigned lastSectionNumber:8;       /* last section number for the PAT */
    unsigned reserved_3:3;
    unsigned pcr_pid:13;        /* pid containing pcrs */
    unsigned reserved_4:4;
    unsigned program_info_length:12;    /* number of bytes in descriptors */
}PMT_TYPE, *PMT_PTR;

#pragma pack()
#endif

