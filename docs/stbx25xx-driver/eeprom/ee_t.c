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
/*-----------------------------------------------------------------------------
|		Test file for eeprom xicor X24165
|		4-Jun-01 Create by shaol
|
+-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "eeprom.h"

#define MAX_NUM		0x30
unsigned char buf[MAX_NUM];

unsigned char r_buf[MAX_NUM];

int main()
{
    int fd, i;
    int ret, err = 0;

    if ((fd = open(EEPROM_I2C_BUS, O_RDWR)) < 0)
    {
        printf("open dev file '%s' error\n", EEPROM_I2C_BUS);
        return 0;
    }

    for (i = 0; i < MAX_NUM; i++)
    {
        buf[i] = i + 1;
    }

    if (eeprom_write_begin(fd) < 0)
    {
        printf("write begin error\n");
        goto EE_END;
    }

    /* ret = eeprom_read(fd, 0x7ff, r_buf, 1); printf("pro reg = %x\n", r_buf[0]); */

    ret = eeprom_write(fd, 0x1023, buf, MAX_NUM - 1);

    printf("%x number of bytes written\n", ret);

    usleep(10);

    ret = eeprom_read(fd, 0x1023 , r_buf, MAX_NUM - 1);

    printf("%x number of bytes read\n", ret);

    for (i = 0; i < MAX_NUM - 1; i++)
    {
        if (buf[i] != r_buf[i])
        {
            printf(" [%d] = %d, %d\n", i, buf[i], r_buf[i]);
            err++;
        }
    }

    printf("error number = %d\n", err);

  EE_END:
    close(fd);
    return 0;
}
