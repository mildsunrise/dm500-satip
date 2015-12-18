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
|       COPYRIGHT   I B M   CORPORATION 1997
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Neil Leeder
| Component: EEPROM driver
| File:      eeprom.c
| Purpose:   Implement EEPROM device driver functions.
| Changes:
| Date:      Author  Comment:
| ---------  ------  --------
| 29-Aug-97  NL      Created
| 18-Nov-97  MPT     Added semaphores, and debug options
| 17-Feb-99  MPT     Port to Romeo
| 24-Jun-99  MPT     Major rewrite to handle all cases correctly
| 28-Jun-00  TJC     Port to STB0210x.
| 04-Jun-01	 SL		 Port to Linux on Redwood4 and add write protection operation
| 12-Jul-02	 YYD	 Add support of RW6 (x24256)
+----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
| Note: This is not a eeprom driver, instead it is a eeprom utility library which
| operates eeprom by I2C device driver.
+------------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/wait.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "eeprom.h"

#ifdef  DEBUG
#define PRINTF(fmt,args...)			printf(fmt,##args)
#else
#define PRINTF(fmt,args...)
#endif

static int eeprom_i2c_address = EEPROM_I2C_ADDRESS_WRITE;

#if defined(__DRV_FOR_PALLAS__) || defined(__DRV_FOR_VESTA__)

/*----------------------------------------------------------------------------+
| Function prototypes.
+----------------------------------------------------------------------------*/

static int do_write_i2c(int fd, int hi_adr, int lo_adr,
                        char *data, int length);

static int i2c_page_write(int fd, int hi_adr, int lo_adr,
                          char *data, int length);

/*----------------------------------------------------------------------------+
| Eeprom_init.
+----------------------------------------------------------------------------*/
int eeprom_write_begin(int fd)
{
    unsigned char wrbuf;

    wrbuf = EEPROM_WPR_WEL;

    /* Redwood5 boards were initially configured with the eeprom i2c address */
    /* at 0xC0. Later versions of the board were configured with the eeprom  */
    /* i2c address at 0x40. The following code determines which i2c address  */
    /* by first writing to the eeprom at address 0xC0. If that fails it tries*/
    /* 0x40.                                                                 */
    eeprom_i2c_address = EEPROM_I2C_ADDRESS_WRITE;
    if (do_write_i2c(fd, EEPROM_PROT_HI, 0xff, &wrbuf, 1) < 0)
    {
        eeprom_i2c_address = (0x40)>>1;
        if (do_write_i2c(fd, EEPROM_PROT_HI, 0xff, &wrbuf, 1) < 0)
	{
	  printf("do_write_i2c failure\n");
          return -1;
	}
    }
    printf("eeprom address = 0x%X\n",eeprom_i2c_address<<1);
    wrbuf = EEPROM_WPR_WEL | EEPROM_WPR_RWEL;

    if (do_write_i2c(fd, EEPROM_PROT_HI, 0xff, &wrbuf, 1) < 0)
        return -1;

    wrbuf = EEPROM_WPR_WEL;
    
    if (do_write_i2c(fd, EEPROM_PROT_HI, 0xff, &wrbuf, 1) < 0)
        return -1;
    
    usleep(10);
    
    return 0;
}

int eeprom_write_end(int fd)
{
    char wrbuf;
    
    wrbuf = EEPROM_WPR_WEL;
    
    if (do_write_i2c(fd, EEPROM_PROT_HI, 0xff, &wrbuf, 1) < 0)
        return -1;
    
    wrbuf = EEPROM_WPR_WEL | EEPROM_WPR_RWEL;
    
    if (do_write_i2c(fd, EEPROM_PROT_HI, 0xff, &wrbuf, 1) < 0)
        return -1;
    
    wrbuf = EEPROM_WPR_WEL | EEPROM_WPR_WPEN;
    
    if (do_write_i2c(fd, EEPROM_PROT_HI, 0xff, &wrbuf, 1) < 0)
        return -1;
    
    usleep(10);
    
    return 0;
}

static int i2c_page_write(int fd, int hi_adr, int lo_adr,
                          char *data, int length)
{
    int left, len, off, rem;
    
    off = 0;
    left = length;
    PRINTF("EEPROM: Page write hi = %d, lo = %d, len= %d\n",
        hi_adr, lo_adr, length);
    
    if (left > EEPROM_PAGE_LEN)
        len = EEPROM_PAGE_LEN;
    else
        len = left;
    
        /*if ((rem = (lo_adr % EEPROM_PAGE_LEN)) != 0)
        {
        len = len - rem;
}*/
    
    //fit write data in one page
    if(( rem = (EEPROM_PAGE_LEN - (lo_adr % EEPROM_PAGE_LEN))) != 0 )
    {
        if( len > rem )
            len = rem;
    }
    
    if (do_write_i2c(fd, hi_adr, lo_adr + off, data + off, len) < 0)
    {
        PRINTF("EEPROM: page write error\n");
        return -1;
    }
    
    usleep(10);
    left -= len;
    off += len;
    
    while (left)
    {
        if (left > EEPROM_PAGE_LEN)
            len = EEPROM_PAGE_LEN;
        else
            len = left;
        
        if (do_write_i2c(fd, hi_adr, lo_adr + off, data + off, len) < 0)
        {
            PRINTF("EEPROM: page write error\n");
            return -1;
        }
        
        usleep(10);
        left -= len;
        off += len;
    }
    
    return 0;
}

static int do_write_i2c(int fd, int hi_adr, int lo_adr,
                        char *data, int length)
{
    int i, ret;
    char buf[EEPROM_PAGE_LEN + 1];
    
    struct i2c_rdwr_ioctl_data i2c_data;
    
    struct i2c_msg msgs;
    
    if (length > EEPROM_PAGE_LEN)
        return -1;
    
    PRINTF("EEPROM: do write %d, %d, %d\n", hi_adr, lo_adr, length);
    
    if (ioctl(fd, I2C_SLAVE_FORCE, eeprom_i2c_address | hi_adr) < 0)
    {
        PRINTF("EEPROM: set slave addr error %x, %x\n", hi_adr, lo_adr);
        return -1;
    }
    
    /* if(write(fd, &lo_adr, 1) != 1) { PRINTF("EEPROM: write data error %x, %x\n", hi_adr,
    lo_adr); return -1; } */
    msgs.addr = eeprom_i2c_address | hi_adr;
    
    msgs.flags = 0x00;
    
    msgs.len = length + 1;
    
    msgs.buf = buf;
    
    i2c_data.nmsgs = 1;
    
    i2c_data.msgs = &msgs;
    
    buf[0] = lo_adr;
    
    for (i = 1; i <= length; i++)
    {
        buf[i] = data[i - 1];
    }

    if (write(fd, buf, length + 1) != length + 1)
    {
        PRINTF("EEPROM: write data error %x, %x\n", hi_adr, lo_adr);
        return -1;
    }

    return length;
    /* ret = ioctl(fd, I2C_RDWR, (unsigned long*)(&i2c_data)); if(ret < 0) { PRINTF("EEPROM: write
    data error %x, %x\n", hi_adr, lo_adr); return -1; } */
}

int eeprom_write(int fd, int sub_adr, unsigned char *data, int length)
{
    int hi_adr = 0;
    int lo_adr = 0;
    int cur_adr;
    int offset, left, len = 0;
    
    if (sub_adr > EEPROM_MAX_ADDRESS)
        return -1;
    
    left = length;
    
    offset = 0;
    
    while (left)
    {
        cur_adr = sub_adr + offset;
        hi_adr = cur_adr >> 8;
        lo_adr = cur_adr & 0xff;
        len = 0xff - lo_adr + 1;
        PRINTF("EEPROM: eeprom write %d, %d, %d\n", hi_adr, lo_adr, len);
        
        if (hi_adr > EEPROM_MAX_ADDRESS >> 8)
            break;
        
        if (len > left)
            len = left;
        
        if (i2c_page_write(fd, hi_adr, lo_adr, data + offset, len) < 0)
            break;
        
        left -= len;
        
        offset += len;
    }
    
    return length - left;
}

int eeprom_read(int fd, int sub_adr, unsigned char *data, int length)
{
    // write read address to eeprom
    char hi_adr, lo_adr;
    int ret, count = 0;
    int i;
    char buf;
    
    struct i2c_rdwr_ioctl_data i2c_data;

    struct i2c_msg msgs;
    
    hi_adr = sub_adr >> 8;
    lo_adr = sub_adr & 0xff;
    
    PRINTF("ee_read: sub_adr = %x, hi_adr = %x, lo_adr = %x\n",
        sub_adr, hi_adr, lo_adr);

    if (ioctl(fd, I2C_SLAVE_FORCE, eeprom_i2c_address | hi_adr) < 0)
    {
        PRINTF("EEPROM: set slave addr error %x, %x\n", hi_adr, lo_adr);
        return -1;
    }
    
    msgs.addr = eeprom_i2c_address | hi_adr;
    msgs.flags = 0x00;
    msgs.len = 1;
    msgs.buf = &lo_adr;
    
    i2c_data.nmsgs = 1;
    i2c_data.msgs = &msgs;
    
    /* ret = ioctl(fd, I2C_RDWR, (unsigned long *)(&i2c_data)); if(ret < 0) { PRINTF("EEPROM: write 
    data error %x, %x\n", hi_adr, lo_adr); return -1; } */
    
    if (write(fd, &lo_adr, 1) != 1)
    {
        PRINTF("EEPROM: write data error %x, %x\n", hi_adr, lo_adr);
        return -1;
    }
    
    /* for(i=0; i < length; i++) { ret = read(fd, data+i, 1); if(ret < 1) return count; count +=
    ret; } */
    return read(fd, data, length);
}


#elif defined(__DRV_FOR_VULCAN__)

// xicor x24256

int eeprom_write_begin(int fd)
{
    return 0;
}

int eeprom_write_end(int fd)
{
    return 0;
}

int eeprom_write(int fd, int sub_adr, unsigned char *data, int length)
{
    int hi_adr = 0;
    int lo_adr = 0;
    int cur_adr;
    int offset, left, len = 0, i;
    char buf[EEPROM_PAGE_LEN + 2];
    
    if (sub_adr < 0 || sub_adr > EEPROM_MAX_ADDRESS || length < 0)
        return -1;
    
    if(length + sub_adr > EEPROM_MAX_ADDRESS+1)
        left = EEPROM_MAX_ADDRESS+1 - sub_adr;
    else
        left = length;
    
    offset = 0;
    
    while (left)
    {
        cur_adr = sub_adr + offset;
        hi_adr = cur_adr >> 8;
        lo_adr = cur_adr & 0xff;
        len = EEPROM_PAGE_LEN - (lo_adr&(EEPROM_PAGE_LEN-1));
        PRINTF("EEPROM: eeprom write hi_adr %d, lo_adr %d, len %d, left %d\n", hi_adr, lo_adr, len, left);
        
        if (len > left)
            len = left;
        
        if (ioctl(fd, I2C_SLAVE_FORCE, eeprom_i2c_address) < 0)
        {
            PRINTF("EEPROM: set slave addr error %x, %x\n", hi_adr, lo_adr);
            return -1;
        }
        
        buf[0] = hi_adr;
        buf[1] = lo_adr;
        
        for (i = 0; i < len; i++)
        {
            buf[i+2] = data[offset+i];
        }
        
        if (write(fd, buf, len + 2) != (len + 2) )
        {
            PRINTF("EEPROM: write data error 0x%x, 0x%x\n", hi_adr, lo_adr);
            break;
        }

        left -= len;
        
        offset += len;
        
        //Dummy delay. The second write in loop does not succeed without this delay.
        //Is I2C device not ready without the delay ?
        for(i=0; i<50000; i++);
    }
    
    return offset;
}

int eeprom_read(int fd, int sub_adr, unsigned char *data, int length)
{
    int hi_adr = 0;
    int lo_adr = 0;
    int cur_adr;
    int offset, left, len = 0;
    char buf[3];
    
    struct i2c_rdwr_ioctl_data i2c_data;
    
    struct i2c_msg msgs;
    
    if (sub_adr < 0 || sub_adr > EEPROM_MAX_ADDRESS || length < 0)
        return -1;
    
    if(length + sub_adr > EEPROM_MAX_ADDRESS+1)
        left = EEPROM_MAX_ADDRESS+1 - sub_adr;
    else
        left = length;
    
    offset = 0;
    
    while (left)
    {
        cur_adr = sub_adr + offset;
        hi_adr = cur_adr >> 8;
        lo_adr = cur_adr & 0xff;
        PRINTF("EEPROM: eeprom read %d, %d, %d\n", hi_adr, lo_adr, left);
        
        if (ioctl(fd, I2C_SLAVE_FORCE, eeprom_i2c_address) < 0)
        {
            PRINTF("EEPROM: set slave addr error 0x%x, 0x%x\n", hi_adr, lo_adr);
            return -1;
        }
        
        buf[0] = hi_adr;
        buf[1] = lo_adr;
        
        if (write(fd, buf, 2) != 2)
        {
            PRINTF("EEPROM: set sub address error 0x%x, 0x%x\n", hi_adr, lo_adr);
            break;
        }
        
        if (ioctl(fd, I2C_SLAVE_FORCE, eeprom_i2c_address) < 0)
        {
            PRINTF("EEPROM: set slave addr error 0x%x, 0x%x\n", hi_adr, lo_adr);
            break;
        }
        
        len = read(fd, data+offset, left);
        if(len <= 0)
        {
            PRINTF("EEPROM: failed to read addr %x, %x\n", hi_adr, lo_adr);
            break;
        }        
        left -= len;
        
        offset += len;
    }
    
    return offset;
}



#else

    #error "unknown architecture"

#endif
