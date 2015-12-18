/*---------------------------------------------------------------------------+
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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Sathyan Doraiswamy
| Component: Serial Flash
| File:      scp_drv.h
| Purpose:   SCP driver.
| Changes:
| Date:         Author          Comment:
| -----         -----           --------
| 05/13/02      Sathyan         Created this file
+----------------------------------------------------------------------------*/
#ifndef _sflash_h_
#define _sflash_h


#define		SFLASH_BUFFER_1		1
#define		SFLASH_BUFFER_2		2

#define     SFLASH_PAGE_SIZE            264
#define     SFLASH_BUFFER_SIZE          264
#define     MAX_BLOCKS                  128
#define     MAX_PAGES                   1024


#define	    SFLASH_SUCCESS		0
#define	    SFLASH_ERR_MEM_ALLOC	1
#define     SFLASH_OP_ABORTED           2
#define     SFLASH_ERR_INVALID_PARAM    3
#define     SFLASH_ERR_DEVICE_BUSY      4


#define     S_FLASH_STATUS_CMD          0x57

#define     SFLASH_READ_BUF1_CMD        0xD4
#define     SFLASH_READ_BUF2_CMD        0xD6

#define     SFLASH_WRITE_BUFFER1_CMD    0x84
#define     SFLASH_WRITE_BUFFER2_CMD    0x87

#define     SFLASH_WRITE_PAGE_1_CMD     0x82 
#define     SFLASH_WRITE_PAGE_2_CMD     0x85

#define     SFLASH_READ_PAGE_CMD        0xD2

#define     SFLASH_BUFF1_TO_PAGE_CMD    0x83 
#define     SFLASH_BUFF2_TO_PAGE_CMD    0x86
#define     SFLASH_PAGE_TO_BUFF1_CMD    0x53 
#define     SFLASH_PAGE_TO_BUFF2_CMD    0x55

#define     SFLASH_CONT_READ_CMD        0xE8
#define     SFLASH_ERASE_PAGE_CMD       0x81
#define     SFLASH_ERASE_BLOCK_CMD      0x50

#define     SCP_FLASH 1

void sflash_init();
void sflash_deinit();

int sflash_get_status(unsigned char * stat);

int sflash_read_buffer(unsigned char btBufNum, unsigned short wStartAddr, unsigned short dwLen, unsigned char * pData);
int sflash_write_buffer(unsigned char btBufNum, unsigned short wStartAddr, unsigned short dwLen, unsigned char * pData);

int sflash_read_page(unsigned short wPageNum, unsigned short wStartAddr, unsigned short wLen, unsigned char * pOutData);
int sflash_write_page(unsigned short wPageNum, unsigned char btBufNum, unsigned short wLen, unsigned char * pInData);

int sflash_buffer_to_page(unsigned short wPageNum, unsigned char btBufNum);
int sflash_page_to_buffer(unsigned short wPageNum, unsigned char btBufNum);

int sflash_read(unsigned long dwStartAddr, unsigned long dwLen, unsigned char * pOutData);
int sflash_write(unsigned long dwStartAddr, unsigned long dwLen, unsigned char * pOutData);

int sflash_erase_page(unsigned short wPageNum);
int sflash_erase_block(unsigned short wBlockNum);


int sflash_partial_page_write(unsigned short wPageNum, unsigned short wStartAddr, unsigned char * pData, unsigned long dwLen);

int sflash_ready();

int sflash_send_cmd(unsigned char * pInput, unsigned char  *pOutput, unsigned long dwLen);

void sflash_test();

#endif
