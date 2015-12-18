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
| File:      sflash_test.c
| Purpose:   Data Flash driver.
| Changes:
| Date:     Author     Comment:
| -----     ------     --------
| 05-16-02  Sathyan    API's for Testing FLASH R/W routines
| 09-18-03  MSD        Ported to Linux
+----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
        Test Routines 
-----------------------------------------------------------------------------*/

/* The necessary header files */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <asm/system.h>
#include "os/os-types.h"
#include "os/drv_debug.h"
#include "scp/scp_inf.h"
#include "sflash.h"

#define FALSE 0
#define TRUE 1

int BufferWrite(unsigned char btBufNum)
{
    unsigned char * pData;
    unsigned long dwCount;
    int iRetVal = SFLASH_SUCCESS;

    pData = (unsigned char *)malloc(300);

    if (pData == NULL)
    {
        printf("BufferWrite: Mem Alloc failed\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    for (dwCount = 0; dwCount < 300; dwCount++)
    {
        pData[dwCount] = dwCount+1;
    }

    iRetVal = sflash_write_buffer(btBufNum, 0, 264, pData);
    if (iRetVal != SFLASH_SUCCESS)
    {
        printf("sflash_write_buffer() failed : %d\n", iRetVal);
    }

    free(pData);

    return iRetVal;
}

int BufferRead(unsigned char btBufNum)
{
    unsigned char * pData;
    unsigned long dwCount;
    int iRetVal;

    pData = (unsigned char *)malloc(264);

    if (pData == NULL)
    {
        printf("BufferRead: Mem Alloc failed\n");
        return SFLASH_ERR_MEM_ALLOC;
    }

    memset(pData, 0, 264);

    iRetVal = sflash_read_buffer(btBufNum, 0, 264, pData);

    if (iRetVal == SFLASH_SUCCESS)
    {
        for (dwCount = 0; dwCount < 264; )
        {
            printf("%0.2X ", pData[dwCount]);

            dwCount++;
        
            if (!(dwCount % 16))
                printf("\n");
        }
    }
    else
    {
        printf("sflash_read_buffer() failed: %d\n", iRetVal);
    }

    free(pData);
    return iRetVal;
}


void test_sflash_read_write(unsigned long dwStartAddr, unsigned long dwLen)
{
    unsigned char btFail = 0;
    unsigned long dwCount;
    unsigned long iRetVal;
    unsigned char * pData;

    pData = (unsigned char *)malloc(1000);

    for (dwCount = 0; dwCount < dwLen; dwCount++)
        pData[dwCount] = dwCount + 1;

    printf("Writing Data @ %d, Num Of Bytes -> %d\n", dwStartAddr, dwLen);

    iRetVal = sflash_write(dwStartAddr, dwLen, pData);

    if (iRetVal != SFLASH_SUCCESS)
    {
        printf("sflash_write() failed: %d\n", iRetVal);
        return;
    }

    if (sflash_ready() == FALSE)
    {
        printf("Flash Device is busy: Test Aborted\n");
        return;
    }
   
    printf("Reading back..\n");

    memset(pData, 0, 1000);

    if(sflash_read(dwStartAddr, dwLen, pData)!=0)
    {
       printf("test_sflash_read_write: sflash_read failed.\n");
    }
    else
    {

    for (dwCount=0; dwCount<dwLen; dwCount++)
    {
        if (pData[dwCount] != ( (dwCount+1) % 256) )
        {
            printf("Data mismatch @ %d, Exp-> 0x%x, Act -> 0x%x\n", dwCount, (dwCount+1) % 256, pData[dwCount]);
            btFail = 1;
        }
    }

    if (btFail)
        printf("----------- FAIL --------\n");
    else
        printf("----------- PASS --------\n");
    } 
    free(pData);
}
void test_byte_pattern(unsigned long dwStartAddr, unsigned long dwLen, unsigned char btByte)
{
    unsigned char btFail = 0;
    unsigned long dwCount;
    unsigned long iRetVal;
    unsigned char * pData;
    int i;    
    unsigned long dwTempStartAddr, dwTempLen, offset; 
    
    PDEBUG("test_byte_pattern: dwStartAddr = 0x%8.8x, dwLen = 0x%8.8x, btByte = 0x%8.8x\n", dwStartAddr,dwLen,btByte);
    
    pData = (unsigned char *) malloc(dwLen);
    if(pData == NULL)
    {
       printf("test_byte_pattern: unable to allocate %d bytes of memory for this test\n", dwLen);
       return;
    }  

    memset(pData, btByte, dwLen);

    printf("Writing Data from %d, Num Of Bytes -> %d\n", dwStartAddr, dwLen);

    iRetVal = sflash_write(dwStartAddr, dwLen, pData);

    if (iRetVal != SFLASH_SUCCESS)
    {
        printf("sflash_write() failed: %d\n", iRetVal);
        return;
    }

    if (sflash_ready() == FALSE)
    {
        printf("Flash Device is busy: Test Aborted\n");
        return;
    }

    printf("Reading back..\n");

    memset(pData, 0, dwLen);
  

    if(dwLen > SCP_BUFFER_SIZE - 32)
    {
       dwTempLen = SCP_BUFFER_SIZE - 32;
       dwTempStartAddr = dwStartAddr;   
       offset = 0;
       while(dwTempStartAddr < (dwStartAddr + dwLen))
       {
          sflash_read(dwTempStartAddr, dwTempLen, pData+offset);
          dwTempStartAddr += dwTempLen;
          offset += dwTempLen;
       }   
    }
    else
    {    
       sflash_read(dwStartAddr, dwLen, pData);
    }
    
    for (dwCount=0; dwCount<dwLen; dwCount++)
    {
        if (pData[dwCount] != btByte)
        {
            printf("Data mismatch @ %d, Exp-> %d, Act -> %d\n", dwCount, btByte, pData[dwCount]);
            btFail = 1;
        }
    }

    if (btFail)
        printf("----------- FAIL --------\n");
    else
        printf("----------- PASS --------\n");

    free(pData);
}

void ErasePage(USHORT wPageNum)
{
    unsigned char pData[264];
    unsigned long dwCount;

    printf("Test Description : Erasing Page %d\n",wPageNum);
    
    sflash_erase_page(wPageNum);

    printf("Verfying Page Erase..\n");

    sflash_read_page(wPageNum, 0, 264, pData);

    for (dwCount = 0; dwCount < 264; dwCount++)
    {
        if (pData[dwCount] != 0xFF)
        {
            break;
        }
    }

    if (dwCount != 264)
        printf("-------------- FAIL ------------\n");
    else
        printf("-------------- PASS  ------------\n");
}

void EraseBlock(USHORT wBlockNum)
{
    unsigned char pData[264];
    unsigned long dwCount;
    unsigned long dwPageCount;
    USHORT wPageNum;

    printf("Test Description : Erasing Block %d\n",wBlockNum);
    
    sflash_erase_block(wBlockNum);

    printf("Verfying Block Erase..\n");


    wPageNum = wBlockNum * 8;

    for (dwPageCount = 0; dwPageCount < 8; dwPageCount ++)
    {
        sflash_read_page(wPageNum + dwPageCount , 0, 264, pData);

        for (dwCount = 0; dwCount < 264; dwCount++)
        {
            if (pData[dwCount] != 0xFF)
            {
                printf("Data mismatch @ page %d, count %d, Exp-> 0x%x, Act -> 0x%x\n", dwPageCount, dwCount, 0xFF, pData[dwCount]);
                break;
            }
        }

        if (dwCount != 264)
            break;
    }

    if (dwPageCount != 8)
    {
        printf("-------------- FAIL ------------\n");
	}
    else
        printf("-------------- PASS  ------------\n");
}


void Test1()
{
    printf("Test 1: Test Description: Data Read Write: Loc=0, Len > PAGE_SIZE \n");
   test_sflash_read_write(0, 1000);
}

void Test2()
{
    printf("Test 2: Test Description : Data Read Write: Loc > 0, Len > PAGE_SIZE\n");
    test_sflash_read_write(10, 1000);
}

void Test3()
{
    printf("Test 3: Test Description : Data Read Write: Loc > 0, Len < PAGE_SIZE\n");
    test_sflash_read_write(10, 255);
}

void Test4()
{
    printf("Test 4: Test Description : Data Read Write: Crossing one page boundary: Two partial page writes\n");
    test_sflash_read_write(300, 600);
}

void Test5()
{
    printf("Test 5: Test Description : Last Page write \n");
    test_sflash_read_write(1023 * 264, 264);
}

void Test6()
{
    printf("Test 6: Test Description : Wrapping over the last page\n");
    test_sflash_read_write(1023 * 264, 512);
}


void Test7()
{
    printf("Test 7: \n");
    ErasePage(0);
}

void Test8()
{
    printf("TEST 8: n");
    ErasePage(512);
}

void Test9()
{
    printf("TEST 9: n");
    ErasePage(1023);
}

void Test10()
{
    printf("Test 10: n");
    EraseBlock(1);
}

void Test11()
{
int i;
    printf("Test 11: Test Description : Writing 0x55 from 0 to 264K\n");
    printf("This might take a while\n");
    
    test_byte_pattern(0, 264*1024, 0x55);

}

void Test12()
{
    printf("Test 12: Test Description : Writing 0xAA from 0 to 264K\n");
    printf("This might take a while\n");

    test_byte_pattern(0, 264*1024, 0xAA);
}

#if 0
void Test11()
{
    printf("------ Test 11----\n");
    printf("Test Description : Write Protect\n");

    printf("Write Protecting the flash\n");

/*    xilinx_sflash_wp_on();*/

    printf("Attempting to write and then read\n");

    Test1();
}
#endif

void sflash_test()
{
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
    Test8();
    Test9();
    Test10();
    Test11();
    Test12();
    
}

