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
| Author:    Mark Wisner
| Component: Menus
| File:      mu_scp.c
| Purpose:   SCP test menu.
| Changes:
| Date:      Author  Comment:
| ---------  ------  --------
| 04-Jan-00  tjc     Moved from SCP Driver.
| 07-Jun-02  sathyan Ported to Europa & Vulcan
| 25-Sept-03 msd     Ported to Linux
+----------------------------------------------------------------------------*/
#include <fcntl.h>
#include "scp/scp_inf.h"
#include "sflash.h"

int fd;

/****************************************************************************
** Function:    main
**
** Purpose:     get & perform a SCP command
**
** Parameters:  
**              
****************************************************************************/
int main(int argc, char *argv[])
{
    char cmd[256];
    char *dev_name = "/dev/scp\0";
    int i;
    int rc;

    if ((fd = open(dev_name, O_RDWR)) > 0)
    {
       printf("open /dev/scp success\n");
       rc = 0;
       sflash_init();
    }
    else
    {
       printf("can't open /dev/scp\n");
       return -1;
    }

    while (1)
    {     
        while (1)
        {
            printf("input command (h=help):\n");
            scanf("%s", cmd);
//            printf("cmd = %s\n", (char *) cmd);
            if (strncmp(cmd, "end", 3) == 0)
            {
               sflash_deinit();
               return (0);
             }
   
            else
               rc = scp_test(cmd);
        }
      
    }
}

/*----------------------------------------------------------------------------+
| Scp_test.  The main SCP routine
+----------------------------------------------------------------------------*/
int scp_test(char *cmd)
{
    unsigned long        offset;
    unsigned long        value;
    unsigned long        num_bytes;
    unsigned char        stat;
    char                 *temp;
    int                  i;
    char                 answer;
    char                 read_data[100];
    
    if (strncmp(cmd, "help", 1)==0) 
    {
        printf("-SCP Menu------------------------------");
        printf("---------------------------------------\n\r");
        printf("help          - display help menu      ");
        printf("                                      |\n\r");
        printf("status        - display  Flash status  ");
        printf("                                      |\n\r");
        printf("dreg          - display all SCP regs   ");
        printf("                                      |\n\r");
        printf("clkd          - set the SCP clock div  ");
        printf("                                      |\n\r");
        printf("rdata         - set reverse data bit   ");
        printf("                                      |\n\r");
        printf("rclk          - set reverse clock bit  ");
        printf("                                      |\n\r");
        printf("loop          - set loop mode bit      ");
        printf("                                      |\n\r");
        printf("test          - Test the slave device(Serial Flash)");
        printf("                          |\n\r");
        printf("end           - Terminate the scp tests");
        printf("                                      |\n\r");
        printf("---------------------------------------");
        printf("---------------------------------------\n\r");


    } 
    else if (strncmp(cmd, "status", 6) == 0)
    {
       sflash_get_status(&stat);
       printf("Serial Flash Status = 0x%x\n", stat);
    }
    else if (strncmp(cmd, "dreg", 4)==0) 
    {
       ioctl(fd, IOCTL_SCP_DISPLAY_REGS);
    } 
    else if(strncmp(cmd, "clkd", 4)==0) 
    {
       printf("Enter value:\r\n");
       scanf("%s", read_data);

       if (isdigit((int) read_data[0]))
       {
          value = (unsigned char) atol(read_data);
          ioctl(fd, IOCTL_SCP_SET_CDM, &value);
       }
       else
          printf("incorrect value\n\r");
    }
    else if(strncmp(cmd, "rdata", 5)==0) 
    {
       printf("Enter value:\r\n");
       scanf("%s", read_data);

       if (isdigit((int) read_data[0]))
       {
          value = (unsigned char) atol(read_data);
          ioctl(fd, IOCTL_SCP_SET_REV_DATA, &value);
       }
       else
          printf("incorrect value\n\r");
    }
    else if(strncmp(cmd, "rclk", 4)==0) 
    {
       printf("Enter value:\r\n");
       scanf("%s", read_data);

       if (isdigit((int) read_data[0]))
       {
          value = (unsigned char) atol(read_data);
          ioctl(fd, IOCTL_SCP_SET_CLK_INV, &value);
       }
       else
          printf("incorrect value\n\r");
    } 
    else if(strncmp(cmd, "loop", 4)==0) 
    {
       printf("Enter value:\r\n");
       scanf("%s", read_data);

       if (isdigit((int) read_data[0]))
       {
          value = (unsigned char) atol(read_data);
          ioctl(fd, IOCTL_SCP_SET_LOOPBACK, &value);
       }
       else
          printf("incorrect value\n\r");
    } 
    else if (strncmp(cmd, "test", 4) == 0)
    {
      sflash_test();
    }
    else if (strncmp(cmd, "end", 3) == 0)
    {
       return(0);
    }
    else
    { 
        printf("Unknown SCP command\n\r");
        return(0);
    }

    return(0);
}
