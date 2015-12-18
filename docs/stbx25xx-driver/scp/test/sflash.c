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
| File:      sflash.c
| Purpose:   Data Flash driver.
| Changes:
| Date:     Author     Comment:
| -----     ------     --------
| 05-14-02  Sathyan    API's for FLASH R/W
| 09-26-03  MSD        Ported to Linux
+----------------------------------------------------------------------------*/
/* The necessary header files */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <asm/system.h>

#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "os/drv_debug.h"
#include "os/os-types.h"
#include "os/os-sync.h"
#include "os/os-interrupt.h"

/* local includes */

#include "scp/scp_inf.h"
#include "sflash.h"
#include "sem.h"

char flash_cmd[296];
int sSFlashInUseSem;
extern int fd;

/*-----------------------------------------------------------------------------
|   Function        :   sflash_init
|
|   Parameters      :   None
|
|   Return Value    :   None
|
|   Notes           :   
+----------------------------------------------------------------------------*/

void sflash_init()
{

   if(ioctl(fd, IOCTL_SFLASH_INIT) != 0) 
   {
      printf("scp_flash_stat failed.\n");
      return;
   }

   sSFlashInUseSem = CreateSemaphore(1);

}


/*-----------------------------------------------------------------------------
|   Function        :   sflash_deinit
|
|   Parameters      :   None
|
|   Return Value    :   None
|
|   Notes           :   
+----------------------------------------------------------------------------*/
void sflash_deinit()
{
   DeleteSemaphore(sSFlashInUseSem);
}   

/*-----------------------------------------------------------------------------
|   Function        :   sflash_ready
|
|   Parameters      :   None
|
|   Return Value    :   1 - Device Ready,
|                       0 - Device is busy
|
|   Notes           :   Waits or max of 25ms
+----------------------------------------------------------------------------*/

int sflash_ready()
{
   unsigned char btStatus;
    unsigned long dwNumOfTries = 5;

    while (dwNumOfTries > 0)
    {
        sflash_get_status(&btStatus);
        // see if the busy bit is not on.
        if (btStatus & 0x80)
        {
            return(1);
        }
        usleep(5000);    
        dwNumOfTries--;
    }
    return(0);
}

/*-----------------------------------------------------------------------------
|   Function        :   sflash_send_cmd
|
|   Parameters      :   IN : unsigned char * : Pointer to Input Buffer
|                      OUT : unsigned char * : Pointer to Output Buffer
|                       IN : unsigned long  : Number of bytes
|
|   Return Value    :   Return value of scp_rw
|
|   Notes           :
+------------------------------x----------------------------------------------*/

int sflash_send_cmd(unsigned char * pInput, unsigned char * pOutput, unsigned long dwLen)
{
   SCP_RW_STRUCT scp_rw;
   int i;

   PDEBUG("sflash_send_cmd: pInput = 0x%8.8x, pOutput = 0x%8.8x, dwLen = %d\n", pInput, pOutput, dwLen);
  
    if(dwLen > SCP_BUFFER_SIZE)
   {
      printf("ERROR: sflash_send_cmd: dwLen 0x%x is larger than SCP_BUFFER_SIZE 0x%x\n",dwLen,SCP_BUFFER_SIZE);
      return -1;
   }   

    // setup the SCP_RW structure 
   scp_rw.write_ptr = pInput;
   scp_rw.read_ptr = pOutput;
   scp_rw.count = dwLen;

    /* Use SFLASH In Use Semaphore */
   SemWait(sSFlashInUseSem);

   if(ioctl(fd, IOCTL_SCP_RW, &scp_rw) != 0) 
   {
       printf("sflash_send_cmd failed.\n");
       return -1;
   }

   SemPost(sSFlashInUseSem);

   return 0;
}


/*-----------------------------------------------------------------------------
|   Function        :   sflash_get_status
|
|   Parameters      :   OUT : unsigned char * : Status of Flash Device
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
+----------------------------------------------------------------------------*/
int sflash_get_status( unsigned char *pStat)
{
    int iRetVal = SFLASH_SUCCESS;
    char temp[5];
    SCP_RW_STRUCT scp_rw;
//    int i;

    /* build the command     */
    memset(flash_cmd, 0, 296);
    
    flash_cmd[0] = S_FLASH_STATUS_CMD;
    flash_cmd[1] = S_FLASH_STATUS_CMD;
    flash_cmd[2] = 0x00;
    flash_cmd[3] = 0x00;
    flash_cmd[4] = 0x00;

    // setup the SCP_RW structure
    scp_rw.write_ptr = flash_cmd;
    scp_rw.read_ptr = temp;
    scp_rw.count = 4;
    
    /* Use SFLASH In Use Semaphore */
    SemWait(sSFlashInUseSem);

    if(ioctl(fd, IOCTL_SCP_RW, &scp_rw) != 0) 
    {
        printf("sflash_get_status failed with RC = %x \n", iRetVal);
        return -1;
    }
    *pStat = scp_rw.read_ptr[1];
    
    SemPost(sSFlashInUseSem);

    return 0;
}


/*-----------------------------------------------------------------------------
|   Function        :   sflash_read_buffer
|
|   Parameters      :   IN :unsigned char : BufferNumber
|                       IN : unsigned short: Starting Address in the buffer
|                       IN : unsigned short: Number of Bytes to Read
|                      OUT : unsigned char *: Pointer to data buffer.
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :
+----------------------------------------------------------------------------*/

int sflash_read_buffer(unsigned char btBufNum, unsigned short wStartAddr, unsigned short wLen, unsigned char * pData)
{
    int iRetVal = SFLASH_SUCCESS;
    unsigned char *pOutParam;
    unsigned long dwNumOfBytes;

    /* Setup the Input parameters  */
    memset(flash_cmd, 0, 296);

    if (btBufNum == SFLASH_BUFFER_1)
        flash_cmd[0] = SFLASH_READ_BUF1_CMD;
    else if (btBufNum == SFLASH_BUFFER_2)
        flash_cmd[0] = SFLASH_READ_BUF2_CMD;
    else
        return SFLASH_ERR_INVALID_PARAM;

    /* place address */
    flash_cmd[1] = 0x0;
    flash_cmd[2] = (unsigned char)(wStartAddr & 0xFF00) >> 8;
    flash_cmd[3] = (unsigned char)(wStartAddr & 0x00FF);

    /* Setup the output parameters */

    /* Total bytes = dwNumOfBytes + Flash Command to Read */
    dwNumOfBytes = wLen + 5 ;

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pOutParam == NULL)
    {
        printf("SFLASH: sflash_read_buf: malloc failed()\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    iRetVal = sflash_send_cmd(flash_cmd, pOutParam, dwNumOfBytes);

    if (iRetVal == SCP_SUCCESS)
    {
        memcpy(pData, pOutParam + 5, wLen);
    }
    else
    {
        printf("scp_flash_stat failed with RC = %x \n", iRetVal);
    }

    free(pOutParam);

    return (iRetVal);
}

/*-----------------------------------------------------------------------------
|   Function        :   sflash_write_buffer
|
|   Parameters      :   IN :unsigned char : BufferNumber
|                       IN : unsigned short: Starting Address in the buffer
|                       IN : unsigned short: Number of Bytes to Write
|                       IN : unsigned char *: Pointer to Data.
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :
+----------------------------------------------------------------------------*/

int sflash_write_buffer(unsigned char btBufNum, unsigned short wStartAddr, unsigned short wLen, unsigned char * pData)
{
    unsigned int iRetVal = SFLASH_SUCCESS;
    unsigned char * pOutParam;
    unsigned long dwNumOfBytes;

    /* Setup the Input parameters */
    memset(flash_cmd, 0, 296);

    if (btBufNum == SFLASH_BUFFER_1)
        flash_cmd[0] = SFLASH_WRITE_BUFFER1_CMD;
    else if (btBufNum == SFLASH_BUFFER_2)
        flash_cmd[0] = SFLASH_WRITE_BUFFER2_CMD;
    else
        return (SFLASH_ERR_INVALID_PARAM);

    /* place address */
    flash_cmd[1] = 0x0;
    flash_cmd[2] = (wStartAddr & 0xFF00) >> 8;
    flash_cmd[3] = (wStartAddr & 0x00FF);

    memcpy(&flash_cmd[4], pData, wLen);

    /* Setup the output parameters */

    /* Total bytes = dwNumOfBytes + Flash Command to Write */
    dwNumOfBytes = wLen + 4 ;

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pOutParam == NULL)
    {
        printf("SFLASH: sflash_write_buf: malloc failed()\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    iRetVal = sflash_send_cmd(flash_cmd, pOutParam, dwNumOfBytes);

    if(iRetVal != SCP_SUCCESS)
    {
        printf("scp_rw failed : RC = %x \n", iRetVal);
    }

    free(pOutParam);

    return (iRetVal);
}


/*-----------------------------------------------------------------------------
|   Function        :   sflash_write_page
|
|   Parameters      :   IN : unsigned short: Page Number < 0 to 1023>
|                       IN :unsigned char : Buffer Number to use
|                       IN : unsigned short: Number of Bytes to Write
|                       IN : unsigned char *: Pointer to Data.
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :
+----------------------------------------------------------------------------*/

int sflash_write_page(unsigned short wPageNum,unsigned char btBufNum, unsigned short wLen, unsigned char * pData)
{
    int iRetVal = SFLASH_SUCCESS;
    unsigned long dwTemp;
    unsigned long dwNumOfBytes;
    unsigned char * pOutParam;

    /* Setup the Input parameters */
    memset(flash_cmd, 0, 296);

    /* Use Buffer 1 for page writes */
    if (btBufNum == SFLASH_BUFFER_1)
        flash_cmd[0] = SFLASH_WRITE_PAGE_1_CMD;
    else if (btBufNum == SFLASH_BUFFER_2)
        flash_cmd[1] = SFLASH_WRITE_PAGE_2_CMD;
    else
        return SFLASH_ERR_INVALID_PARAM;

    /* place address */
    dwTemp = 0;                 /* Starting Address in Buffer */
    dwTemp |= (wPageNum << 9);   /* Page Number                */
    dwTemp <<= 8;               /* Remove the first bytes     */

    memcpy(&flash_cmd[1], &dwTemp, 3);

    /* Copy the Data to write */
    memcpy(&flash_cmd[4], pData, wLen);

    /* Setup the output parameters */
    /* Total bytes = dwNumOfBytes + Flash Command to Write */
    dwNumOfBytes = wLen + 4;

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pOutParam == NULL)
    {
        printf("SFLASH: sflash_page_write: malloc failed()\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    iRetVal = sflash_send_cmd(flash_cmd, pOutParam, dwNumOfBytes);

    if(iRetVal != SCP_SUCCESS)
    {
        printf("scp_flash_stat failed with RC = %x \n", iRetVal);
    }

    free(pOutParam);

    return (iRetVal);
}


/*-----------------------------------------------------------------------------
|   Function        :   sflash_read_page
|
|   Parameters      :   IN : unsigned short: Page Number < 0 to 1023>
|                       IN : unsigned short: Start Address in Page
|                       IN : unsigned short: Number of Bytes to Write
|                      OUT : unsigned char *: Pointer to Data.
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :   Direct Read: Bypasses Buffer1 and Buffer2.
|                       IMPORTANT: sflash_read_page always returns data from
|                           Start Address + 1. Use your judgment when using this
|                           API. Sent an E-mail to Atmel. Expecting reply.
+----------------------------------------------------------------------------*/

int sflash_read_page(unsigned short wPageNum, unsigned short wStartAddr, unsigned short wLen, unsigned char * pData)
{
    unsigned long dwTemp;
    unsigned long dwNumOfBytes;
    unsigned char * pOutParam;
    int iRetVal = SFLASH_SUCCESS;

    /* Setup the Input parameters */
    memset(flash_cmd, 0, 296);

    /* Use Buffer 1 for page writes */
    flash_cmd[0] = SFLASH_READ_PAGE_CMD;

    /* place address */
    dwTemp = wStartAddr;                 /* Starting Address in Buffer */
    dwTemp |= (wPageNum << 9);            /* Page Number                */
    dwTemp <<= 8;                        /* Remove the first bytes     */

    memcpy(&flash_cmd[1], &dwTemp, 3);

    /* Setup the output parameters */
    /* Total bytes = dwNumOfBytes + Flash Command to Write */
    dwNumOfBytes = wLen + 4 + 4 ;

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pOutParam == NULL)
    {
        printf("SFLASH: sflash_page_write: malloc failed()\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    iRetVal = sflash_send_cmd(flash_cmd, pOutParam, dwNumOfBytes);

    if(iRetVal == SFLASH_SUCCESS)
    {
        memcpy(pData, pOutParam + 8, wLen);
    }
    else
    {
        printf("scp_flash_stat failed with RC = %x \n", iRetVal);
    }

    free(pOutParam);

    return (iRetVal);
}

/*-----------------------------------------------------------------------------
|   Function        :   sflash_buffer_to_page
|
|   Parameters      :   IN : unsigned short: Page Number < 0 to 1023>
|                       IN :unsigned char : Buffer Number to use < 1 or 2>
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :   Assumes that data is already written to buffer.
+----------------------------------------------------------------------------*/

int sflash_buffer_to_page(unsigned short wPageNum,unsigned char btBufNum)
{
    int iRetVal = SFLASH_SUCCESS;
    unsigned long dwTemp;
    unsigned long dwNumOfBytes;
    unsigned char * pOutParam;

    /* Setup the Input parameters */
    memset(flash_cmd, 0, 296);

    /* Use Buffer 1 for page writes */
    if (btBufNum == SFLASH_BUFFER_1)
        flash_cmd[0] = SFLASH_BUFF1_TO_PAGE_CMD;
    else if (btBufNum == SFLASH_BUFFER_2)
        flash_cmd[0] = SFLASH_BUFF2_TO_PAGE_CMD;
    else
        return SFLASH_ERR_INVALID_PARAM;


    /* place address */
    dwTemp = 0;                 /* Starting Address in Buffer */
    dwTemp |= (wPageNum << 9);   /* Page Number                */
    dwTemp <<= 8;               /* Remove the first bytes     */

    memcpy(&flash_cmd[1], &dwTemp, 3);

    /* Setup the output parameters */
    /* Total bytes = dwNumOfBytes + Flash Command to Write */
    dwNumOfBytes = 4;

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pOutParam == NULL)
    {
        printf("SFLASH: sflash_page_write: malloc failed()\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    iRetVal = sflash_send_cmd(flash_cmd, pOutParam, dwNumOfBytes);

    if(iRetVal != SCP_SUCCESS)
    {
        printf("scp_flash_stat failed with RC = %x \n", iRetVal);
    }

    free(pOutParam);

    return (iRetVal);
}

/*-----------------------------------------------------------------------------
|   Function        :   sflash_page_to_buffer
|
|   Parameters      :   IN : unsigned short: Page Number < 0 to 1023>
|                       IN :unsigned char : Buffer Number to use
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :
+----------------------------------------------------------------------------*/

int sflash_page_to_buffer(unsigned short wPageNum,unsigned char btBufNum)
{
    int iRetVal = SFLASH_SUCCESS;
    unsigned long dwTemp;
    unsigned long dwNumOfBytes;
    unsigned char * pOutParam;
    int i;

    /* Setup the Input parameters */
    memset(flash_cmd, 0, 296);

    /* Use Buffer 1 for page writes */
    if (btBufNum == SFLASH_BUFFER_1)
        flash_cmd[0] = SFLASH_PAGE_TO_BUFF1_CMD;
    else if (btBufNum == SFLASH_BUFFER_2)
        flash_cmd[0] = SFLASH_PAGE_TO_BUFF2_CMD;
    else
        return SFLASH_ERR_INVALID_PARAM;


    /* place address */
    dwTemp = 0;                 /* Starting Address in Buffer */
    dwTemp |= (wPageNum << 9);   /* Page Number                */
    dwTemp <<= 8;               /* Remove the first bytes     */
    memcpy(&flash_cmd[1], &dwTemp, 3);

    /* Setup the output parameters */
    /* Total bytes = dwNumOfBytes + Flash Command to Write */
    dwNumOfBytes = 4;

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pOutParam == NULL)
    {
        printf("SFLASH: sflash_page_write: malloc failed()\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    iRetVal = sflash_send_cmd(flash_cmd, pOutParam, dwNumOfBytes);

    if(iRetVal != SCP_SUCCESS)
    {
        printf("scp_flash_stat failed with RC = %x \n", iRetVal);
    }

    free(pOutParam);

    return (iRetVal);
}


/*-----------------------------------------------------------------------------
|   Function        :   sflash_partial_page_write
|
|   Parameters      :   IN : unsigned short : Page Number
|                       IN : unsigned short : Start Address
|                       IN : unsigned char * : Pointer to Data
|                       IN : unsigned long  : Length
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :
+----------------------------------------------------------------------------*/

int sflash_partial_page_write(unsigned short wPageNum, unsigned short wStartAddr, unsigned char * pData, unsigned long dwLen)
{

    /* Read the Page into Buffer 1*/
    if (sflash_ready() == 0)
        return SFLASH_OP_ABORTED;

    sflash_page_to_buffer(wPageNum, 1);

    if (sflash_ready() == 0)
        return SFLASH_OP_ABORTED;
        
    /* Copy the user data into buffer */
    sflash_write_buffer(SFLASH_BUFFER_1, wStartAddr, dwLen, pData);

    if (sflash_ready() == 0)
        return SFLASH_OP_ABORTED;

    /* Commit the page */
    sflash_buffer_to_page(wPageNum, SFLASH_BUFFER_1);

    return SFLASH_SUCCESS;

}


/*-----------------------------------------------------------------------------
|   Function        :   sflash_read
|
|   Parameters      :   IN : unsigned long : Start Address <0 - 264K>
|                       IN : unsigned long : Number of Bytes to READ <0 - 264>
|                      OUT : unsigned char *: Pointer to Data.
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :   Bypasses Buffer 1 & 2.
+----------------------------------------------------------------------------*/

int sflash_read(unsigned long dwStartAddr, unsigned long dwLen, unsigned char * pData)
{
    int iRetVal = 0;
    unsigned long dwNumOfBytes;
    unsigned char * pInParam;
    unsigned char * pOutParam;
    unsigned long dwTemp;
    unsigned short wStartAddrInPage;
    unsigned short wPageNum;
    

    PDEBUG("sflash_read: dwStartAddr = 0x%x  dwLen = 0x%x\n", dwStartAddr, dwLen);
    dwNumOfBytes = dwLen + 4 + 4 ;
    
    if(dwNumOfBytes > SCP_BUFFER_SIZE)
    {
       printf("sflash_read: number of btyes to read 0x%x is greater than SCP_BUFFER_SIZE\n");
       return -1;
    }

    pInParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pInParam == NULL)
    {
        return SFLASH_ERR_MEM_ALLOC;
    }

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);

    if (pOutParam == NULL)
    {
        free(pInParam);
        return SFLASH_ERR_MEM_ALLOC;
    }

    /* place address */
    wPageNum = dwStartAddr / SFLASH_PAGE_SIZE;
    wStartAddrInPage = dwStartAddr % SFLASH_PAGE_SIZE;

    dwTemp = wStartAddrInPage;                 /* Starting Address in Buffer */
    dwTemp |= (wPageNum << 9);            /* Page Number                */
    dwTemp <<= 8;                        /* Remove the first bytes     */

    pInParam[0] = SFLASH_CONT_READ_CMD;
    memcpy(&pInParam[1], &dwTemp, 3);
    
    iRetVal = sflash_send_cmd(pInParam, pOutParam, dwNumOfBytes);

    if(iRetVal == SCP_SUCCESS)
    {
        memcpy(pData, pOutParam + 8, dwLen);
    }
    else
    {
        printf("scp_flash_stat failed with RC = %x \n", iRetVal);
    }

    free(pInParam);
    free(pOutParam);

    return (iRetVal);
}

/*-----------------------------------------------------------------------------
|   Function        :   sflash_write
|
|   Parameters      :   IN : unsigned long : Start Address < 0 to 264K>
|                       IN : unsigned long : Number of Bytes to WRITE
|                      OUT : unsigned char *: Pointer to Data.
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :
+----------------------------------------------------------------------------*/

int sflash_write(unsigned long dwStartAddr, unsigned long dwLen, unsigned char * pData)
{
    int iRetVal = SFLASH_SUCCESS;
    unsigned long dwBytesLeft = dwLen;
    unsigned long dwBytesToWrite = 0;
    unsigned short wStartAddrInPage;
    unsigned short wPageNum;
    unsigned char btBufNum = 0;
    unsigned char * pCurPos;

    PDEBUG("sflash_write: dwStartAddr = 0x%x  dwLen = 0x%x\n",dwStartAddr, dwLen);
    
    /* place address */
    wPageNum = dwStartAddr / SFLASH_PAGE_SIZE;
    wStartAddrInPage = dwStartAddr % SFLASH_PAGE_SIZE;
    pCurPos = pData;

    btBufNum = 2;
    if (wStartAddrInPage)
    {
        /* Partial Page Write */

        dwBytesToWrite = SFLASH_BUFFER_SIZE - wStartAddrInPage;

        iRetVal = sflash_partial_page_write(wPageNum, wStartAddrInPage, pCurPos, dwBytesToWrite);
        if (iRetVal != SFLASH_SUCCESS)
            return iRetVal;

        dwBytesLeft -= dwBytesToWrite;
        pCurPos += dwBytesToWrite;
        wPageNum++;

        btBufNum = 1;
    }


    while (dwBytesLeft >= SFLASH_BUFFER_SIZE)
    {

        /* Write the data into Buffer - Use 1 and 2 alternatively */
        btBufNum = btBufNum == 1 ? 2: 1;

        if (sflash_ready() == 0)
        {
            return SFLASH_OP_ABORTED;
        }

        sflash_write_buffer(btBufNum, 0, SFLASH_BUFFER_SIZE, pCurPos);

        /* Check to see if Device is not busy */
        if (sflash_ready() == 0)
        {
            return SFLASH_OP_ABORTED;
        }

        /* Execute command to write the buffer to flash page */
        sflash_buffer_to_page(wPageNum, btBufNum);

        pCurPos += SFLASH_BUFFER_SIZE;
        wPageNum++;
        dwBytesLeft -= SFLASH_BUFFER_SIZE;
    }

    /* Partial Page write */
    if (dwBytesLeft)
    {
 
        dwBytesToWrite = dwBytesLeft;

        sflash_partial_page_write(wPageNum, 0, pCurPos, dwBytesToWrite);
    }

    return (iRetVal);
}

/*-----------------------------------------------------------------------------
|   Function        :   sflash_erase_page
|
|   Parameters      :   IN : unsigned short: Page Number to erase
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :
+----------------------------------------------------------------------------*/

int sflash_erase_page(unsigned short wPageNum)
{
    int iRetVal = SFLASH_SUCCESS;
    unsigned char *pOutParam;
    unsigned long dwNumOfBytes;
    unsigned long dwTemp;

    /* Setup the Input parameters to scp_io() */
    memset(flash_cmd, 0, 296);

    if (wPageNum > MAX_PAGES)
        return SFLASH_ERR_INVALID_PARAM;

    if (sflash_ready() == 0)
        return SFLASH_ERR_DEVICE_BUSY;

    /* place address */
    flash_cmd[0] = SFLASH_ERASE_PAGE_CMD;

    dwTemp = 0;
    dwTemp = wPageNum << 9;
    dwTemp = dwTemp << 8;

    memcpy(&flash_cmd[1], &dwTemp, 3);

    /* Setup the output parameters to scp_io() */

    /* Total bytes = dwNumOfBytes + Flash Command to Read */
    dwNumOfBytes = 4 ;

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pOutParam == NULL)
    {
        printf("SFLASH: sflash_read_buf: malloc failed()\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    iRetVal = sflash_send_cmd(flash_cmd, pOutParam, dwNumOfBytes);

    if (iRetVal != SFLASH_SUCCESS)
    {
        printf("scp_flash_stat failed with RC = %x \n", iRetVal);
    }

    free(pOutParam);

    return (iRetVal);
}

/*-----------------------------------------------------------------------------
|   Function        :   sflash_erase_block
|
|   Parameters      :   IN : unsigned short: Page Number to erase
|
|   Return Value    :   0 - on Success, Non-zero on failure
|
|   Notes           :
+----------------------------------------------------------------------------*/

int sflash_erase_block(unsigned short wBlockNum)
{
    int iRetVal = SFLASH_SUCCESS;
    unsigned char *pOutParam;
    unsigned long dwNumOfBytes;
    unsigned long dwTemp;

    /* Setup the Input parameters to scp_io() */
    memset(flash_cmd, 0, 296);

    if (wBlockNum > MAX_BLOCKS)
        return SFLASH_ERR_INVALID_PARAM;

    if (sflash_ready() == 0)
        return SFLASH_ERR_DEVICE_BUSY;

    /* place address */
    flash_cmd[0] = SFLASH_ERASE_BLOCK_CMD;

    dwTemp = 0;
    dwTemp = wBlockNum << 12;
    dwTemp = dwTemp << 8;

    memcpy(&flash_cmd[1], &dwTemp, 3);

    /* Setup the output parameters to scp_io() */

    /* Total bytes = dwNumOfBytes + Flash Command to Read */
    dwNumOfBytes = 4 ;

    pOutParam = (unsigned char *)malloc(dwNumOfBytes);
    if (pOutParam == NULL)
    {
        printf("SFLASH: sflash_read_buf: malloc failed()\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    iRetVal = sflash_send_cmd(flash_cmd, pOutParam, dwNumOfBytes);
    
    // allow some time for the erase to complete
    usleep(100);
    
    if (iRetVal != SFLASH_SUCCESS)
    {
        printf("scp_flash_stat failed with RC = %x \n", iRetVal);
    }

    free(pOutParam);

    return (iRetVal);
}


