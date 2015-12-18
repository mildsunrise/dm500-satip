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
|       COPYRIGHT   I B M   CORPORATION 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Mike Lepore
| Component: sci
| File:      mu_sci.c
| Purpose:   Menu application functions for the Smart Card Interface driver.
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
+----------------------------------------------------------------------------*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <linux/ioctl.h>

#include "sci/sci_inf.h"
#include "sc.h"
#include "sci_prot.h"
#include "mu_sci.h"

static unsigned long loop_test = 0;
static unsigned long ss_test = 0;
int fd[2];

/****************************************************************************
** Function:    main
**
** Purpose:     get & perform a smart card command
**
** Parameters:  
**              
****************************************************************************/
int main(int argc, char *argv[])
{
    char cmd[256];
    char *dev_name[] = {
        "/dev/sci0\0",
        "/dev/sci1\0"
    };
    unsigned long sci_id = 2;
    int i;
    SCI_ERROR rc;

    printf("sc_test program\n\n");

    while (1)
    {
        printf("\nPls. select the card:\n\n");
        printf("  sc0 = card 0\n");
        printf("  sc1 = card 1\n");
        printf("  exit = quit\n\n");

        scanf("%s", cmd);
        printf("cmd = %s\n", (char *) cmd);

        if (strncmp(cmd, "sc0", 3) == 0)
        {
            printf("You selected card 0\n");

            if ((fd[0] = open(dev_name[0], O_RDWR)) > 0)
            {
                sci_id = 0;
                printf("open /dev/sci0 success\n");
                rc = 0;
            }
            else
            {
                printf("can't open /dev/sci0\n");
            }
        }
        else if (strncmp(cmd, "sc1", 3) == 0)
        {
            printf("You selected card 1\n");

            if ((fd[1] = open(dev_name[1], O_RDWR)) > 0)
            {
                sci_id = 1;
                printf("open /dev/sci1 success\n");
                rc = 0;
            }
            else
            {
                printf("can't open /dev/sci1\n");
            }
        }
        else if (strncmp(cmd, "exit", 4) == 0)
        {
            return (0);
        }
        else
        {
            printf("Error card num\n");
        }

        while (rc == 0)
        {
            printf("input command (h=help):\n");
            scanf("%s", cmd);
            printf("cmd = %s\n", (char *) cmd);
            rc = sci_test(sci_id, cmd);
        }
    }
}

/****************************************************************************
** Function:    sci_test
**
** Purpose:     execute a smart card command
**
** Parameters:  sci_id: 0-based number to identify smart card controller
**              cmd:   pointer to the command string
****************************************************************************/
SCI_ERROR sci_test(unsigned long sci_id, char *cmd)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if (strncmp(cmd, "h", 1) == 0)
    {
        printf("\n\r");
        printf("-Smart Card Menu-------------------");
        printf("-----------------------------------\n\r");
        printf("rt - reset                       ");
        printf(" | ");
        printf("ga - get ATR parameters          ");
        printf("\n\r");
        printf("gp - get communication parameters");
        printf(" | ");
        printf("                                 ");
        printf("\n\r");
        printf("gi - get IFSD                    ");
        printf(" | ");
        printf("si - set IFSD                    ");
        printf("\n\r");
        printf("sf - SELECT FILE command         ");
        printf(" | ");
        printf("ur - UPDATE RECORD command       ");
        printf("\n\r");
        printf("rr - READ RECORD command         ");
        printf(" | ");
        printf("lp - loop sf/ur/rr commands      ");
        printf("\n\r");
        printf("pp - protocol/parameter selection");
        printf(" | ");
        printf("dr - display SCI register values ");
        printf("\n\r");
        printf("gm - get SCI driver modes        ");
        printf(" | ");
        printf("sm - set SCI driver modes        ");
        printf("\n\r");
        printf("dc - deactivate                  ");
        printf(" | ");
        printf("end - stop this driver            ");
        printf("\n\r");
        printf("-----------------------------------");
        printf("-----------------------------------\n\r");
        printf("\n\r");
    }
    else if (strncmp(cmd, "end", 3) == 0)
    {
        close(fd[sci_id]);
        return (-1);
    }
    else
    {
        if (strncmp(cmd, "dc", 2) == 0)
        {
            ioctl(fd[sci_id], IOCTL_SET_DEACTIVATE);
            printf("Deactivation complete.\n\r");
        }
        else if (strncmp(cmd, "lp", 2) == 0)
        {
            loop(sci_id);
        }
        else if (strncmp(cmd, "rt", 2) == 0)
        {
            if ((rc = sc_reset(sci_id)) == SCI_ERROR_OK)
            {
                printf("Reset successful.\n\r");
            }
            else
            {
                print_error("sc_reset", rc);
            }
        }
        else if (strncmp(cmd, "sf", 2) == 0)
        {
            /* intended for the 8K Schlumberger Smart Card */
            /* supplied with the evaluation kit */
            loop_test = 0;
            ss_test = 0;

            if ((rc = select_file(sci_id)) != SCI_ERROR_OK)
            {
                print_error("select_file", rc);
            }
        }
        else if (strncmp(cmd, "rr", 2) == 0)
        {
            /* intended for the 8K Schlumberger Smart Card */
            /* supplied with the evaluation kit */
            loop_test = 0;

            if ((rc = read_record(sci_id)) != SCI_ERROR_OK)
            {
                print_error("read_record", rc);
            }
        }
        else if (strncmp(cmd, "ur", 2) == 0)
        {
            /* intended for the 8K Schlumberger Smart Card */
            /* supplied with the evaluation kit */
            loop_test = 0;

            if ((rc = update_record(sci_id)) != SCI_ERROR_OK)
            {
                print_error("update_record", rc);
            }
        }
        else if (strncmp(cmd, "pp", 2) == 0)
        {
            pps(sci_id);
        }
        else if (strncmp(cmd, "ga", 2) == 0)
        {
            get_ATR_parms(sci_id);
        }
        else if (strncmp(cmd, "gp", 2) == 0)
        {
            get_parms(sci_id);
        }
#ifdef SET_PARAMETERS
        else if (strncmp(cmd, "sp", 2) == 0)
        {
            set_parms(sci_id);
        }
#endif
        else if (strncmp(cmd, "gi", 2) == 0)
        {
            get_ifsd(sci_id);
        }
        else if (strncmp(cmd, "si", 2) == 0)
        {
            set_ifsd(sci_id);
        }
        else if (strncmp(cmd, "gm", 2) == 0)
        {
            get_modes(sci_id);
        }
        else if (strncmp(cmd, "sm", 2) == 0)
        {
            set_modes(sci_id);
        }
        else if (strncmp(cmd, "dr", 2) == 0)
        {
            ioctl(fd[sci_id], IOCTL_DUMP_REGS);
        }
        else if (strncmp(cmd, "ss", 2) == 0)
        {
            /* used for Schlumberger case 0101_1.BIN (T=1) */
            loop_test = 0;
            ss_test = 1;

            if ((rc = select_file(sci_id)) != SCI_ERROR_OK)
            {
                print_error("select_file", rc);
            }
        }
        else
        {
            printf("Error command!\n");
        }
    }

    return (0);
}

/****************************************************************************
** Name         loop
**
** Purpose:     perform select file, update record,
**              and read record commands in a loop
**
** Parameters:  sci_id: 0-based number to identify smart card controller
**
** Returns:     Error status
****************************************************************************/
SCI_ERROR loop(unsigned long sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned long i, rep, fail, msg;
    char input;
    unsigned char prompt[255];
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        printf("Enter number of iterations:\r\n");
        scanf("%s", prompt);
        rep = atol(prompt);
        input = '0';

        while ((input != 'Y') && (input != 'y') && (input != 'N')
               && (input != 'n'))
        {
            printf("Verbose message output (Y or N)?\r\n");
            scanf("%s", prompt);
            input = (char) (prompt[0]);
        }

        msg = 0;

        if ((input == 'Y') || (input == 'y'))
        {
            msg = 1;
        }

        fail = 0;
        loop_test = 1;
        printf("Loop test running...\r\n");

        for (i = 1; i <= rep; i++)
        {
            if (msg == 1)
            {
                printf("%d\r\n", i);
                printf("SELECT FILE\r\n");
            }
            else if ((i % 100) == 0)
            {
                printf("%d\r\n", i);
            }

            rc = select_file(sci_id);

            if (rc == SCI_ERROR_OK)
            {
                if (msg == 1)
                {
                    printf("UPDATE RECORD\r\n");
                }

                if ((rc = update_record(sci_id)) == SCI_ERROR_OK)
                {
                    if (msg == 1)
                    {
                        printf("READ RECORD\r\n");
                    }
                    if ((rc = read_record(sci_id)) != SCI_ERROR_OK)
                    {
                        print_error("read_record", rc);
                        fail++;
                    }
                }
                else
                {
                    print_error("update_record", rc);
                    fail++;
                }
            }
            else
            {
                print_error("select_file", rc);
                fail++;
            }

            if (rc != SCI_ERROR_OK)
            {
                printf("reset\r\n");

                if ((rc = sc_reset(sci_id)) != SCI_ERROR_OK)
                {
                    print_error("sc_reset", rc);
                    printf("Cancelling loop.\r\n");
                    rep = i;
                    i = rep + 1;
                }
            }
        }

        if (rep > 0)
        {
            if (fail)
            {
                printf("%d out of %d iterations failed.\n", fail, rep);
            }
            else
            {
                printf("No failures occurred.\n");
            }
        }
    }
    else
    {
        rc = SCI_ERROR_CARD_NOT_ACTIVATED;
        print_error("t0_loop", rc);
    }

    return (rc);
}

/****************************************************************************
** Name         select_file
**
** Purpose:     issue SELECT FILE command to the Smart Card.
**


** Parameters:  sci_id: 0-based number to identify smart card controller
**
** Returns:     Error status
****************************************************************************/
SCI_ERROR select_file(unsigned long sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned long i = 0;
    unsigned char capdu[20];
    unsigned long length = 0;
    unsigned char rapdu[258];
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {

        if (ss_test == 0)
        {
            length = 8;
            capdu[0] = 0xc0;    /* CLA byte */
            capdu[1] = 0xa4;    /* INS byte- SELECT FILE */
            capdu[2] = 0x00;    /* P1 */
            capdu[3] = 0x00;    /* P2 */
            capdu[4] = 0x02;    /* Lc */
            capdu[5] = 0x10;    /* Data field */
            capdu[6] = 0x05;
            capdu[7] = 0x00;    /* Le set to max.- causes GET RESPONSE */
        }
        else
        {
            length = 20;
            capdu[0] = 0xc0;    /* CLA byte */
            capdu[1] = 0xa4;    /* INS byte- SELECT FILE */
            capdu[2] = 0x04;    /* P1 means select a DF */
            capdu[3] = 0x00;    /* P2 */
            capdu[4] = 0x0E;    /* Lc */
            capdu[5] = 0x31;    /* Data field */
            capdu[6] = 0x50;
            capdu[7] = 0x41;
            capdu[8] = 0x59;
            capdu[9] = 0x2E;
            capdu[10] = 0x53;
            capdu[11] = 0x59;
            capdu[12] = 0x53;
            capdu[13] = 0x2E;
            capdu[14] = 0x44;
            capdu[15] = 0x44;
            capdu[16] = 0x46;
            capdu[17] = 0x30;
            capdu[18] = 0x31;
            capdu[19] = 0x00;
        }

        /* response APDU will be only the end sequence */
        if ((rc = sc_apdu(sci_id, capdu, rapdu, &length)) == SCI_ERROR_OK)
        {
            if (loop_test == 0)
            {
                printf("SELECT FILE command successful.\r\n");
                printf("Response APDU in hexadecimal:\r\n");

                for (i = 0; i < length; i++)
                {
                    if (rapdu[i] < 0x10)
                    {
                        printf("0");
                    }
                    printf("%x ", rapdu[i]);
                }
                printf("\r\n");
            }
        }
        else
        {
            printf("\r\n");
            print_error("sc_apdu", rc);
        }
    }
    else
    {
        rc = SCI_ERROR_CARD_NOT_ACTIVATED;
    }

    return (rc);
}

/****************************************************************************
** Name         update_record
**
** Purpose:     issue an UPDATE RECORD command to the Smart Card
**
** Parameters:  sci_id: zero-based number to identify Smart Card controller
**
** Returns:     Error status
****************************************************************************/
SCI_ERROR update_record(unsigned long sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned long i = 0;
    unsigned char capdu[260];
    unsigned long length = 260;
    unsigned char rapdu[258];
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        capdu[0] = 0xc0;        /* CLA byte */
        capdu[1] = 0xdc;        /* INS byte- UPDATE RECORD */
        capdu[2] = 0x00;        /* P1 */
        capdu[3] = 0x00;        /* P2 */
        capdu[4] = 0xff;        /* Lc */
        //capdu[4] = 25;        /* Lc */

        for (i = 0; i < 255; i++)
        {
            capdu[i + 5] = (unsigned char) i;
        }
		length = i + 5;

        if (loop_test == 0)
        {
            printf("Enter the text to be written(up to 255 characters):\r\n");
            scanf("%s", (char *) (capdu + 5));
        }

        /* response APDU will be only the end sequence */
        if ((rc = sc_apdu(sci_id, capdu, rapdu, &length)) == SCI_ERROR_OK)
        {
            if (loop_test == 0)
            {
                printf("Response APDU in hexadecimal:\r\n");

                for (i = 0; i < length; i++)
                {
                    if (rapdu[i] < 0x10)
                    {
                        printf("0");
                    }
                    printf("%x ", rapdu[i]);
                }
                printf("\r\n");

                /* check SW1 and SW2 for success */
                if ((rapdu[length - 2] == 0x90)
                    && (rapdu[length - 1] == 0x00))
                {
                    printf("UPDATE RECORD command successful.\r\n");
                }
                else
                {
                    printf
                        ("Must use SELECT FILE prior to UPDATE RECORD.\r\n");
                }
            }
        }
        else
        {
            printf("\r\n");
            print_error("sc_apdu", rc);
        }
    }
    else
    {
        rc = SCI_ERROR_CARD_NOT_ACTIVATED;
    }

    return (rc);
}

/****************************************************************************
** Name         read_record
**
** Purpose:     issue a READ RECORD command to the Smart Card
**
** Parameters:  sci_id:     zero-based number to identify smart card controller
**
** Returns:     Error status
****************************************************************************/
SCI_ERROR read_record(unsigned long sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned long i = 0;
    unsigned char capdu[5];
    unsigned long length = 5;
    unsigned char rapdu[258];
    unsigned char string[255];
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        capdu[0] = 0xc0;        /* CLA byte */
        capdu[1] = 0xb2;        /* INS byte- READ RECORD */
        capdu[2] = 0x00;        /* P1 */
        capdu[3] = 0x00;        /* P2 */
        capdu[4] = 0xFF;        /* Lc */

        /* response APDU will be data and end sequence */

        if ((rc = sc_apdu(sci_id, capdu, rapdu, &length)) == SCI_ERROR_OK)
        {
            if (loop_test == 0)
            {
                printf("Response APDU in hexadecimal:\r\n");

                for (i = 0; i < length; i++)
                {
                    if (rapdu[i] < 0x10)
                    {
                        printf("0");
                    }
                    printf("%x ", rapdu[i]);
                }
                printf("\r\n");

                /* check SW1 and SW2 for success */
                if ((rapdu[length - 2] == 0x90)
                    && (rapdu[length - 1] == 0x00))
                {
                    printf("READ RECORD command successful.\r\n");

                    if ((length > 2) && (rapdu[0] == 0))
                    {
                        printf
                            ("Use UPDATE RECORD to write text to the Smart Card.\r\n");
                    }
                    else
                    {
                        memcpy((void *) string, (const void *) rapdu, 255);
                        printf("Text read from the Smart Card:\r\n%s\r\n",
                               string);
                    }
                }
                else
                {
                    printf("Must use SELECT FILE prior to READ RECORD.\r\n");
                }
            }
        }
        else
        {
            printf("\r\n");
            print_error("sc_apdu", rc);
        }
    }
    else
    {
        rc = SCI_ERROR_CARD_NOT_ACTIVATED;
    }

    return (rc);
}

/****************************************************************************
** Function:    print_error
**
** Purpose:     Prints the text of an error
**
** Parameters:  fn_name: string of the function name that generated the error
**              error_code: the error code
****************************************************************************/
void print_error(char *fn_name, int error_code)
{
    if (error_code != 0)
    {
        printf("Function \"%s\" failed.\r\n", fn_name);
    }

    switch (error_code)
    {
        case SCI_ERROR_DRIVER_NOT_INITIALIZED:
            printf("The SCI driver has not been initialized");
            break;

        case SCI_ERROR_FAIL:
            printf("Unspecified error");
            break;

        case SCI_ERROR_KERNEL_FAIL:
            printf("Kernel error");
            break;

        case SCI_ERROR_NO_ATR:
            printf("No ATR (answer-to-reset) received");
            break;

        case SCI_ERROR_TS_CHARACTER_INVALID:
            printf("Invalid TS character in ATR (answer-to-reset)");
            break;

        case SCI_ERROR_LRC_FAIL:
            printf("LRC check failure");
            break;

        case SCI_ERROR_CRC_FAIL:
            printf("CRC check failure");
            break;

        case SCI_ERROR_LENGTH_FAIL:
            printf("More data received than expected");
            break;

        case SCI_ERROR_PARITY_FAIL:
            printf("Parity error");
            break;

        case SCI_ERROR_RX_OVERFLOW_FAIL:
            printf("Receive buffer overflow");
            break;

        case SCI_ERROR_TX_UNDERRUN_FAIL:
            printf("Transmit buffer underrun");
            break;

        case SCI_ERROR_CARD_NOT_PRESENT:
            printf("Smart Card not inserted");
            break;

        case SCI_ERROR_CARD_NOT_ACTIVATED:
            printf("Smart Card not activated- a cold reset must be done");
            break;

        case SCI_ERROR_AWT_TIMEOUT:
            printf("Maximum total time to receive ATR was exceeded.");
            break;

        case SCI_ERROR_WWT_TIMEOUT:
            printf("Work Waiting Time (WWT) time-out");
            break;

        case SCI_ERROR_CWT_TIMEOUT:
            printf("Character Waiting Time (CWT) time-out");
            break;

        case SCI_ERROR_BWT_TIMEOUT:
            printf("Block Waiting Time (BWT) time-out");
            break;

        case SCI_ERROR_PARAMETER_OUT_OF_RANGE:
            printf("One or more parameters are out of range");
            break;

        case SCI_ERROR_TRANSACTION_ABORTED:
            printf("Current transaction has been aborted");
            break;

        case SCI_ERROR_CLOCK_STOP_DISABLED:
            printf("Clock stop is disabled and/or unsupported by this card");
            break;

        case SCI_ERROR_TX_PENDING:
            printf
                ("A transmission was attempted while the driver was already transmitting");
            break;

        case SCI_ERROR_ATR_PENDING:
            printf
                ("A reset was attempted while waiting on an ATR from a previous reset");
            break;

        default:
            printf("Unknown error");
            break;
    }

    printf(":error code %d\r\n", error_code);
}

/****************************************************************************
** Function:    pps
**
** Purpose:     Initiate an PPS (Protocol Parameter Selection) exchange.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void pps(unsigned long sci_id)
{
    int rc = SCI_ERROR_OK;
    SC_PARAMETERS sc_parameters;
    char read_data[80];
    unsigned char T = 0;
    unsigned char F = 0;
    unsigned char D = 0;
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        if ((rc = sc_get_parameters(sci_id, &sc_parameters)) == SCI_ERROR_OK)
        {
            /* Assume use of FI and DI, unless otherwise specified */
            T = sc_parameters.T;
            F = sc_parameters.FI;
            D = sc_parameters.DI;
            printf
                ("Default values to be used for Protocol Parameter Selection:\r\n");
            printf("T: %u\r\n", T);
            printf("F: %u\r\n", F);
            printf("D: %u\r\n", D);
            printf("Enter proposed T:\r\n");
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                T = (unsigned char) atol(read_data);
            }

            printf("Enter proposed F:\r\n");
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                F = (unsigned char) atol(read_data);
            }

            printf("Enter proposed D:\r\n");
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                D = (unsigned char) atol(read_data);
            }

            if ((rc = sc_pps(sci_id, T, F, D)) == SCI_ERROR_OK)
            {
                printf("PPS exchange successful.\r\n");
            }
            else
            {
                printf("PPS exchange unsuccessful.\r\n");
                printf("Card does not support this request.\r\n");
                printf
                    ("Note: PPS must be initiated immediately following a reset.\r\n");
                printf("Initiating reset...\r\n");

                if ((rc = sc_reset(sci_id)) == SCI_ERROR_OK)
                {
                    printf("Reset successful.\r\n");
                }
                else
                {
                    print_error("sc_reset", rc);
                }
            }
        }
    }
    else
    {
        printf("Smart Card not activated- a cold reset must be done\r\n");
    }
}

/****************************************************************************
** Function:    get_ATR_parms
**
** Purpose:     Display all ATR parameters
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void get_ATR_parms(unsigned long sci_id)
{
    int rc = SCI_ERROR_OK;
    unsigned char i = 0;
    unsigned char atr_size = 0;
    unsigned char historical_size = 0;
    unsigned char historical_offset = 0;
    unsigned char ATR[33];
    SC_PARAMETERS sc_parameters;
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        /* retrieve the Answer-to-Reset */

        if ((rc = sc_get_atr(sci_id, ATR, &atr_size,
                             &historical_offset,
                             &historical_size)) == SCI_ERROR_OK)
        {
            /* get the Smart Card's default parameters */

            if ((rc =
                 sc_get_parameters(sci_id, &sc_parameters)) == SCI_ERROR_OK)
            {
                printf("Smart Card %d ATR Parameters:\r\n", sci_id);
                printf("------------------------------------");
                printf("------------------------------------\n\r");
                printf("ATR (Answer-to-Reset) in hexadecimal:\r\n");
                for (i = 0; i < atr_size; i++)
                {
                    if (ATR[i] < 0x10)
                    {
                        printf("0", ATR[i]);
                    }
                    printf("%x ", ATR[i]);
                }
                printf("\r\n");

                printf("Historical bytes in hexadecimal:\r\n");
                for (i = 0; i < historical_size; i++)
                {
                    if (ATR[i + historical_offset] < 0x10)
                    {
                        printf("0", ATR[i + historical_offset]);
                    }
                    printf("%x ", ATR[i + historical_offset]);
                }
                printf("\r\n");

                printf
                    ("T      (Current Protocol):                     %d\r\n",
                     sc_parameters.T);

                printf("All T  (Mask of All Offered Protocols 15:0):   ");
                for (i = 0; i < 16; i++)
                {
                    if (((sc_parameters.maskT) & (0x8000 >> i)) ==
                        (0x8000 >> i))
                    {
                        printf("1");
                    }
                    else
                    {
                        printf("0");
                    }
                }
                printf("\r\n");

                printf
                    ("F      (Current Clock Rate Conversion Factor): %d\r\n",
                     sc_parameters.F);
                printf
                    ("D      (Current Baud Rate Adjustment Factor):  %d\r\n",
                     sc_parameters.D);
                printf
                    ("FI     (Card Clock Rate Conversion Factor):    %d\r\n",
                     sc_parameters.FI);
                printf
                    ("DI     (Card Baud Rate Adjustment Factor):     %d\r\n",
                     sc_parameters.DI);
                printf
                    ("II     (Maximum Programming Current Factor):   %d\r\n",
                     sc_parameters.II);
                printf
                    ("PI1    (Programming Voltage-volts):            %d\r\n",
                     sc_parameters.PI1);
                printf
                    ("PI2    (Programming Voltage-decivolts):        %d\r\n",
                     sc_parameters.PI2);
                printf
                    ("WI     (Work Waiting Time Factor):             %d\r\n",
                     sc_parameters.WI);
                printf
                    ("XI     (Clock Stop Indicator):                 %d\r\n",
                     sc_parameters.XI);
                printf
                    ("UI     (Class Indicator):                      %d\r\n",
                     sc_parameters.UI);
                printf
                    ("N      (Extra Guardtime Factor):               %d\r\n",
                     sc_parameters.N);
                printf
                    ("CWI    (Character Waiting Time Factor):        %d\r\n",
                     sc_parameters.CWI);
                printf
                    ("BWI    (Block Waiting Time Factor):            %d\r\n",
                     sc_parameters.BWI);
                printf
                    ("IFSC   (Information Field Size of the Card):   %d\r\n",
                     sc_parameters.IFSC);
                printf
                    ("checks (1=LRC 2=CRC):                          %d\r\n",
                     sc_parameters.check);
            }
            else
            {
                print_error("sc_get_parameters", rc);

            }
        }
        else
        {
            print_error("sc_get_atr", rc);
        }
    }
    else
    {
        printf("Smart Card not activated- a cold reset must be done\r\n");
    }
}

/****************************************************************************
** Function:    get_parms
**
** Purpose:     Print all current communication parameters. 
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void get_parms(unsigned long sci_id)
{
    int rc = SCI_ERROR_OK;
    SCI_PARAMETERS sci_parameters;
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        /* get the Smart Card's default parameters */
        if (ioctl(fd[sci_id], IOCTL_GET_PARAMETERS, &sci_parameters) == 0)
        {
            printf("Smart Card %d Communication Parameters:\r\n", sci_id);
            printf("------------------------------------");
            printf("------------------------------------\n\r");
            printf("T      (Protocol):                           %d\r\n",
                   sci_parameters.T);
            printf("f      (Clock Frequency in Hz):              %d\r\n",
                   sci_parameters.f);
            printf("ETU    (Clocks per Bit):                     %d\r\n",
                   sci_parameters.ETU);
            printf("WWT    (Work Waiting Time in ETUs):          %d\r\n",
                   sci_parameters.WWT);
            printf("CWT    (Character Waiting Time in ETUs):     %d\r\n",
                   sci_parameters.CWT);
            printf("BWT    (Block Waiting Time in ETUs):         %d\r\n",
                   sci_parameters.BWT);
            printf("EGT    (Extra Guard Time in ETUs):           %d\r\n",
                   sci_parameters.EGT);
            printf("checks (1=LRC 2=CRC):                        %d\r\n",
                   sci_parameters.check);
            printf("Clock Stop Polarity:                         %d\r\n",
                   sci_parameters.clock_stop_polarity);
            printf("------------------------------------");
            printf("------------------------------------\n\r");
        }
        else
        {
            print_error("sc_get_parameters", rc);
        }
    }
    else
    {
        printf("Smart Card not activated- a cold reset must be done\r\n");
    }
}

/****************************************************************************
** Function:    set_parms
**
** Purpose:     Set communication parameters, overriding ATR values
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** NOTE:        The changing of these parameters may result in undesirable
**              effects due to invalid parameter settings or conbinations of
**              settings, therefore we have removed this option from the menu.
**              However, we are leaving the code here as a test vechile for
**              experienced users.
****************************************************************************/
#ifdef SET_PARAMETERS
void set_parms(unsigned long sci_id)
{
    int rc = SCI_ERROR_OK;
    char read_data[80];
    SCI_PARAMETERS sci_parameters;
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        /* get the current Smart Card parameters */
        if (ioctl(fd[sci_id], IOCTL_GET_PARAMETERS, &sci_parameters) == 0)
        {
            /* display current parameters */
            get_parms(sci_id);
            printf("Hit enter to keep current value.\r\n");
            printf("Enter Protocol (0 or 1):\r\n");
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.T = (unsigned char) atol(read_data);
            }

            printf("Enter Clock Frequency in Hz (%u to %u):\r\n", SCI_MIN_F,
                   SCI_MAX_F);
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.f = atol(read_data);
            }

            printf("Enter Clocks per Bit (ETU) (%u to %u):\r\n", SCI_MIN_ETU,
                   SCI_MAX_ETU);
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.ETU = atol(read_data);
            }

            printf("Enter Work Waiting Time (WWT) in ETUs (%u to %u):\r\n",
                   SCI_MIN_WWT, SCI_MAX_WWT);
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.WWT = atol(read_data);
            }

            printf
                ("Enter Character Waiting Time (CWT) in ETUs (%u to %u):\r\n",
                 SCI_MIN_CWT, SCI_MAX_CWT);
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.CWT = atol(read_data);
            }

            printf("Enter Block Waiting Time (BWT) in ETUs (%u to %u):\r\n",
                   SCI_MIN_BWT, SCI_MAX_BWT);
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.BWT = atol(read_data);
            }

            printf("Enter Extra Guard Time (EGT) in ETUs (%u to %u):\r\n",
                   SCI_MIN_EGT, SCI_MAX_EGT);
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.EGT = atol(read_data);
            }

            printf("Enter Error Check Type (1=LRC 2=CRC):\r\n");
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.check = (unsigned char) atol(read_data);
            }

            printf("Enter Clock Stop Polarity (0=NEG 1=POS):\r\n");
            scanf("%s", read_data);

            if (isdigit((int) read_data[0]))
            {
                sci_parameters.clock_stop_polarity = atol(read_data);
            }

            /* set the new Smart Card parameters */
            if (ioctl(fd[sci_id], IOCTL_SET_PARAMETERS, &sci_parameters) == 0)
            {
                printf("Parameters set successfully.\r\n");
            }
            else
            {
                print_error("sci_set_parameters", rc);
            }
        }
        else
        {
            print_error("sci_get_parameters", rc);
        }
    }
    else
    {
        printf("Smart Card not activated- a cold reset must be done\r\n");
    }
}
#endif

/****************************************************************************
** Function:    get_parms
**
** Purpose:     Print the current IFSD (Information Field Size for the Device). 
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void get_ifsd(unsigned long sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned char ifsd = 0;
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        if ((rc = sc_get_ifsd(sci_id, &ifsd)) == SCI_ERROR_OK)

        {
            printf("IFSD (Information Field Size of the Device): %u\r\n",
                   ifsd);
        }
        else
        {
            print_error("sc_get_ifsd", rc);
        }
    }
    else
    {
        printf("Smart Card not activated- a cold reset must be done\r\n");
    }
}

/****************************************************************************
** Function:    set_ifsd
**
** Purpose:     Set the current IFSD (Information Field Size for the Device).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void set_ifsd(unsigned long sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned char ifsd = 0;
    char read_data[80];
    unsigned long ioctl_param;

    ioctl(fd[sci_id], IOCTL_GET_IS_CARD_ACTIVATED, &ioctl_param);

    if (ioctl_param == 1)
    {
        get_ifsd(sci_id);
        printf("Hit enter to keep current value.\r\n");
        printf("Enter IFSD (%u to %u):\r\n", SC_MIN_IFSD, SC_MAX_IFSD);
        scanf("%s", read_data);

        if (isdigit((int) read_data[0]))
        {
            if ((atol(read_data) >= SC_MIN_IFSD)
                && (atol(read_data) <= SC_MAX_IFSD))
            {
                ifsd = (unsigned char) atol(read_data);

                if ((rc = sc_set_ifsd(sci_id, ifsd)) == SCI_ERROR_OK)
                {
                    printf("IFSD set successfully.\r\n");
                }
                else
                {
                    print_error("sc_set_ifsd", rc);
                }
            }
            else
            {
                printf("IFSD set failed- value out of valid range.\r\n");
            }
        }
    }
    else
    {
        printf("Smart Card not activated- a cold reset must be done\r\n");
    }
}

/****************************************************************************
** Function:    get_modes

**
** Purpose:     Print all current SCI driver modes
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void get_modes(unsigned long sci_id)
{
    int rc = SCI_ERROR_OK;
    SCI_MODES sci_modes;

    /* get the Smart Card's default parameters */
    if (ioctl(fd[sci_id], IOCTL_GET_MODES, &sci_modes) == 0)
    {
        printf("\r\n");
        printf("Smart Card %d Driver Modes:\r\n", sci_id);
        printf("------------------------------------");
        printf("------------------------------------\n\r");
        printf("Specification Compliance: ");

        if (sci_modes.emv2000 == 1)
        {
            printf("EMV2000-Book1\r\n");
        }
        else
        {
            printf("ISO/IEC 7816\r\n");
        }

        printf("Activation:               ");

        if (sci_modes.man_act == 1)
        {
            printf("Manual\r\n");
        }
        else
        {
            printf("Automatic\r\n");
        }

        printf("------------------------------------");

        printf("------------------------------------\n\r");

        printf("\r\n");
    }
    else
    {
        print_error("sci_get_modes", rc);
    }
}

/****************************************************************************
** Function:    set_modes
**
** Purpose:     Set SCI driver modes
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void set_modes(unsigned long sci_id)
{
    int rc = SCI_ERROR_OK;
    char read_data[80];
    SCI_MODES sci_modes;

    /* get the current Smart Card driver modes */
    if (ioctl(fd[sci_id], IOCTL_GET_MODES, &sci_modes) == 0)
    {
        /* display current modes */
        get_modes(sci_id);
        printf("EMV2000-Book1? (0=disable 1=enable):\r\n");
        scanf("%s", read_data);

        if (isdigit((int) read_data[0]))
        {
            sci_modes.emv2000 = atoi(read_data);
        }

        printf("Manual Activation? (0=disable 1=enable):\r\n");
        scanf("%s", read_data);

        if (isdigit((int) read_data[0]))
        {
            sci_modes.man_act = atoi(read_data);
        }

        /* set the new Smart Card parameters */
        if (ioctl(fd[sci_id], IOCTL_SET_MODES, &sci_modes) == 0)
        {
            printf("Modes set successfully.\r\n");
        }
        else
        {
            print_error("sci_set_modes", rc);
        }
    }
    else
    {
        print_error("sci_get_modes", rc);
    }
}
