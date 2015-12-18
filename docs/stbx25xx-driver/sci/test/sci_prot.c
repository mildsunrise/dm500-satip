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
| File:      sci_prot.c
| Purpose:   Smart Card protocol layer functions for the Smart Card.
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/03/2001  MAL, Zongwei Liu  Fixed error handling bug in sc_apdu() by adding 
|                    check_incoming_data(). This fixed incorrect receive length
|                    occurring when card responds with a two byte error-code
|                    instead of the expected number of bytes. Also fixed error
|                    handling problem in sc_t1_command(). Improved data handling
|                    efficiency in sc_t0_command().
+----------------------------------------------------------------------------*/

#include <stdio.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <linux/ioctl.h>

#include "sci/sci_inf.h"
#include "sc.h"
#include "sci_prot.h"

static SC_CONTROL_BLOCK sc_cb[SCI_NUMBER_OF_CONTROLLERS];

/* tables from ISO/IEC 7816 */
static unsigned long Fi_TABLE[] = {
    372, 372, 558, 744, 1116, 1488, 1860, RFU,
    RFU, 512, 768, 1024, 1536, 2048, RFU, RFU
};

static unsigned long f_TABLE[] = {
    4000000, 5000000, 6000000, 8000000, 12000000, 16000000, 20000000, RFU,
    RFU, 5000000, 7500000, 10000000, 15000000, 20000000, RFU, RFU
};

static unsigned long Di_TABLE[] = {
    RFU, 1, 2, 4, 8, 16, 32, RFU,
    12, 20, RFU, RFU, RFU, RFU, RFU, RFU
};

extern int fd[2];

static void check_incoming_data(unsigned char *buffer,
                                unsigned long *length,
                                unsigned char SW1,
                                unsigned char SW2
);

/****************************************************************************
** Function:    sc_reset
**
** Purpose:     Perform a reset, receive and process the ATR, and extract all
**              ATR parameters.
**
** Parameters:  sc_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
*****************************************************************************/
SCI_ERROR sc_reset(unsigned long sc_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    SCI_PARAMETERS sci_parameters;
    I_BYTES p_ibyte[SC_MAX_ATR_SIZE];
    I_BYTES *p_ibyter = 0;
    I_BYTES *p_ibytew = 0;
    unsigned long i = 0;
    unsigned long x = 0;
    unsigned long index = 0;
    ssize_t bytes_rec = 0;
    unsigned char current = 0;
    unsigned char lrc = 0;
    unsigned long Fi = 0;
    unsigned long Di = 0;
    unsigned long f = 0;

    unsigned long card_present;

    if (sc_id < SCI_NUMBER_OF_CONTROLLERS)
    {
        ioctl(fd[sc_id], IOCTL_GET_IS_CARD_PRESENT, &card_present);

        if (card_present == 1)
        {
            /* initialize protocol state */
            sc_cb[sc_id].atr_size = 0;
            sc_cb[sc_id].p_historical = 0;
            sc_cb[sc_id].historical_size = 0;
            sc_cb[sc_id].TCK_present = 0;
            sc_cb[sc_id].firstT = SC_ATR_T;
            sc_cb[sc_id].currentT = SC_ATR_T;
            sc_cb[sc_id].NS = 0;
            sc_cb[sc_id].proposed_IFSD = SC_ATR_IFSD;
            sc_cb[sc_id].IFSD = SC_ATR_IFSD;

            /* set atr parameter defaults */
            sc_cb[sc_id].sc_parameters.T = SC_ATR_T;
            sc_cb[sc_id].sc_parameters.maskT = SC_ATR_T;
            sc_cb[sc_id].sc_parameters.F = SC_ATR_FI;
            sc_cb[sc_id].sc_parameters.D = SC_ATR_DI;
            sc_cb[sc_id].sc_parameters.FI = SC_ATR_FI;
            sc_cb[sc_id].sc_parameters.DI = SC_ATR_DI;
            sc_cb[sc_id].sc_parameters.II = SC_ATR_II;
            sc_cb[sc_id].sc_parameters.PI1 = SC_ATR_PI1;
            sc_cb[sc_id].sc_parameters.PI2 = SC_ATR_PI2;
            sc_cb[sc_id].sc_parameters.WI = SC_ATR_WI;
            sc_cb[sc_id].sc_parameters.XI = SC_ATR_XI;
            sc_cb[sc_id].sc_parameters.UI = SC_ATR_UI;
            sc_cb[sc_id].sc_parameters.N = SC_ATR_N;
            sc_cb[sc_id].sc_parameters.CWI = SC_ATR_CWI;
            sc_cb[sc_id].sc_parameters.BWI = SC_ATR_BWI;
            sc_cb[sc_id].sc_parameters.IFSC = SC_ATR_IFSC;
            sc_cb[sc_id].sc_parameters.check = SC_ATR_CHECK;

            p_ibytew = p_ibyte;
            p_ibyter = p_ibyte;

            *p_ibytew = TS;
            p_ibytew++;

            if (ioctl(fd[sc_id], IOCTL_SET_RESET) == 0)
            {
                if (ioctl(fd[sc_id], IOCTL_GET_PARAMETERS, &sci_parameters)
                    == 0)
                {
                    while ((p_ibyter != p_ibytew) && (rc == SCI_ERROR_OK))
                    {
                        /* read 1 ATR byte */
                        if ((rc = read(fd[sc_id], &current, 1)) > 0)
                        {
                            rc = SCI_ERROR_OK;

                            sc_cb[sc_id].ATR[index] = current;
                            index++;

                            switch (*p_ibyter)
                            {
                                case TS:
                                    *p_ibytew = T0;
                                    p_ibytew++;
                                    break;

                                case T0:
                                    /* save number of historical bytes */
                                    sc_cb[sc_id].historical_size =
                                        (current & 0x0F);

                                    /* check for presence of TA(1) */
                                    if ((current & 0x10) == 0x10)
                                    {
                                        *p_ibytew = TA;
                                        p_ibytew++;
                                    }

                                    /* check for presence of TB(1) */
                                    if ((current & 0x20) == 0x20)
                                    {
                                        *p_ibytew = TB;
                                        p_ibytew++;
                                    }

                                    /* check for presence of TC(1) */
                                    if ((current & 0x40) == 0x40)
                                    {
                                        *p_ibytew = TC;
                                        p_ibytew++;
                                    }

                                    /* check for presence of TD(1) */
                                    if ((current & 0x80) == 0x80)
                                    {
                                        *p_ibytew = TD;
                                        p_ibytew++;
                                    }
                                    else
                                    {
                                        /* TD(1) is absent- protocol T='0' is the only offer */
                                        sci_parameters.T = 0;
                                        sc_cb[sc_id].sc_parameters.T = 0;
                                        sc_cb[sc_id].sc_parameters.maskT = 1;
                                        /* push any TK's and TCK into queue */
                                        for (x = 0;
                                             x < sc_cb[sc_id].historical_size;
                                             x++)
                                        {
                                            *p_ibytew = TK;
                                            p_ibytew++;
                                        }

                                        if (sc_cb[sc_id].sc_parameters.
                                            maskT != 1)
                                        {
                                            /* If T=0 isn't the only offer, a check byte is present 
                                             */
                                            *p_ibytew = TCK;
                                            p_ibytew++;
                                        }
                                    }
                                    i++;
                                    break;

                                case TA:
                                    if (i == 1)
                                    {
                                        /* extract Fl and Dl from TA(1) to determine Fi, Di, and f */
                                        sc_cb[sc_id].sc_parameters.FI =
                                            ((current & 0xF0) >> 4);
                                        sc_cb[sc_id].sc_parameters.DI =
                                            (current & 0x0F);
                                        Fi = Fi_TABLE[sc_cb[sc_id].
                                                      sc_parameters.FI];
                                        Di = Di_TABLE[sc_cb[sc_id].
                                                      sc_parameters.DI];
                                        f = f_TABLE[sc_cb[sc_id].
                                                    sc_parameters.FI];
                                    }
                                    else if (i == 2)
                                    {
                                        /* check bit 5 of TA(2) */
                                        if ((current & 0x10) == 0)
                                        {
                                            /* use Fi and Di from TA(1), if it is present */
                                            if (f != 0)
                                            {
                                                sci_parameters.f = f;
                                            }
                                            if ((Fi != 0) && (Di != 0))
                                            {
                                                sci_parameters.ETU =
                                                    (Fi / Di);
                                            }

                                            /* card starts in specific mode using this protocol */
                                            /* this overrides first offered protocol */
                                            sci_parameters.T =
                                                (current & 0x0F);
                                            sc_cb[sc_id].sc_parameters.T =
                                                sci_parameters.T;
                                            sc_cb[sc_id].sc_parameters.F =
                                                sc_cb[sc_id].sc_parameters.FI;
                                            sc_cb[sc_id].sc_parameters.D =
                                                sc_cb[sc_id].sc_parameters.DI;
                                        }
                                    }
                                    else if (i > 2)
                                    {
                                        /* TD(i-1) must indicate T=1 for IFSC */
                                        if (sc_cb[sc_id].currentT == 1)
                                        {
                                            /* 0x00 and 0xFF are RFU */
                                            if ((current >= 0x01) &&
                                                (current <= 0xFE))
                                            {
                                                sc_cb[sc_id].sc_parameters.
                                                    IFSC = current;
                                            }
                                        }
                                        /* check for global indication from TD(i-1) */
                                        if (sc_cb[sc_id].currentT == 15)
                                        {
                                            /* check for class */
                                            sc_cb[sc_id].sc_parameters.UI =
                                                (current & 0x3F);

                                            switch (sc_cb[sc_id].
                                                    sc_parameters.UI)
                                            {
                                                case 1:
                                                    sci_parameters.U =
                                                        SCI_CLASS_A;
                                                    break;

                                                case 2:
                                                    /* SCI is class A-only class A and AB cards
                                                       allowed */
                                                    sci_parameters.U =
                                                        SCI_CLASS_B;
                                                    rc = SCI_ERROR_FAIL;
                                                    // sci_deactivate(sc_id);
                                                    ioctl(fd[sc_id],
                                                          IOCTL_SET_DEACTIVATE);
                                                    break;

                                                case 3:
                                                    sci_parameters.U =
                                                        SCI_CLASS_AB;
                                                    break;
                                            }

                                            /* check for clock stop capabilities */
                                            sc_cb[sc_id].sc_parameters.XI =
                                                ((current & 0xC0) >> 6);

                                            switch (sc_cb[sc_id].
                                                    sc_parameters.XI)
                                            {
                                                case 0:
                                                    sci_parameters.
                                                        clock_stop_polarity =
                                                        SCI_CLOCK_STOP_DISABLED;
                                                    break;

                                                case 1:
                                                    sci_parameters.
                                                        clock_stop_polarity =
                                                        SCI_CLOCK_STOP_LOW;
                                                    break;

                                                case 2:

                                                case 3:
                                                    sci_parameters.
                                                        clock_stop_polarity =
                                                        SCI_CLOCK_STOP_HIGH;
                                                    break;
                                            }
                                        }
                                    }
                                    break;

                                case TB:
                                    if (i == 1)
                                    {
                                        sc_cb[sc_id].sc_parameters.II =
                                            ((current & 0x60) >> 5);
                                        switch (sc_cb[sc_id].sc_parameters.II)
                                        {
                                            case 0:
                                                sci_parameters.I = 25;
                                                break;
                                            case 1:
                                                sci_parameters.I = 50;
                                                break;
                                        }
                                        sc_cb[sc_id].sc_parameters.PI1 =
                                            (current & 0x1F);
                                        sci_parameters.P =
                                            sc_cb[sc_id].sc_parameters.PI1;
                                    }
                                    else if (i == 2)
                                    {
                                        sc_cb[sc_id].sc_parameters.PI2 =
                                            current;
                                        /* if present PI2, overrides PI1 for the value of P */
                                        /* PI2 is in decivolts, but P is in volts */
                                        sci_parameters.P =
                                            sc_cb[sc_id].sc_parameters.PI2 / 10;
                                    }
                                    else if (i > 2)
                                    {
                                        /* TD(i-1) must indicate T=1 */
                                        if (sc_cb[sc_id].currentT == 1)
                                        {
                                            /* extract CWI and BWI from TB(3) */
                                            sc_cb[sc_id].sc_parameters.CWI =
                                                (current & 0x0F);
                                            sc_cb[sc_id].sc_parameters.BWI =
                                                ((current & 0xF0) >> 4);
                                            /* calculate CWT and BWT */
                                            sci_parameters.CWT =
                                                11+(1<<sc_cb[sc_id].sc_parameters.CWI);
                                            sci_parameters.BWT =
                                                11+(1<<sc_cb[sc_id].sc_parameters.BWI)*960;
                                        }
                                    }
                                    break;

                                case TC:
                                    if (i == 1)
                                    {
                                        /* extract N to determine EGT */
                                        sc_cb[sc_id].sc_parameters.N =
                                            current;
                                    }
                                    else if (i == 2)
                                    {
                                        sc_cb[sc_id].sc_parameters.WI =
                                            current;
                                        sci_parameters.WWT =
                                            sc_cb[sc_id].sc_parameters.WI*960;
                                    }
                                    else if (i > 2)
                                    {
                                        /* TD(i-1) must indicate T=1 */
                                        if (sc_cb[sc_id].currentT == 1)
                                        {
                                            /* determine error detection code */
                                            if ((current & 0x01) == 0x00)
                                            {
                                                /* LRC checking is 1 byte (default) */
                                                sci_parameters.check = 1;
                                                sc_cb[sc_id].sc_parameters.
                                                    check = 1;
                                            }
                                            else
                                            {
                                                /* CRC checking is 2 bytes */
                                                sci_parameters.check = 2;
                                                sc_cb[sc_id].sc_parameters.
                                                    check = 2;
                                            }
                                        }
                                    }
                                    break;

                                case TD:
                                    if (i == 1)
                                    {
                                        /* first offered protocol */
                                        sc_cb[sc_id].firstT = current & 0x0F;
                                        /* assume card starts in negotiable mode using this
                                           protocol */
                                        sci_parameters.T = sc_cb[sc_id].firstT;
                                        sc_cb[sc_id].sc_parameters.T =
                                            sci_parameters.T;
                                    }

                                    /* get current protocol T, add to mask */
                                    sc_cb[sc_id].currentT = (current & 0x0F);
                                    sc_cb[sc_id].sc_parameters.maskT |=
                                        (1 << (sc_cb[sc_id].currentT));

                                    /* check for presence of TA(i+1) */
                                    if ((current & 0x10) == 0x10)
                                    {
                                        *p_ibytew = TA;
                                        p_ibytew++;
                                    }

                                    /* check for presence of TB(i+1) */
                                    if ((current & 0x20) == 0x20)
                                    {
                                        *p_ibytew = TB;
                                        p_ibytew++;
                                    }

                                    /* check for presence of TC(i+1) */
                                    if ((current & 0x40) == 0x40)
                                    {
                                        *p_ibytew = TC;
                                        p_ibytew++;
                                    }

                                    /* check for presence of TD(i+1) */
                                    if ((current & 0x80) == 0x80)
                                    {
                                        *p_ibytew = TD;
                                        p_ibytew++;
                                    }
                                    else
                                    {
                                        /* this is the last TD, push any TK's and TCK into queue */
                                        for (x = 0;
                                             x < sc_cb[sc_id].historical_size;
                                             x++)
                                        {
                                            *p_ibytew = TK;
                                            p_ibytew++;
                                        }

                                        if (sc_cb[sc_id].sc_parameters.
                                            maskT != 1)
                                        {
                                            /* If T=0 isn't the only offer, a check byte is present 
                                             */
                                            *p_ibytew = TCK;
                                            p_ibytew++;
                                        }
                                    }
                                    i++;
                                    break;

                                case TK:
                                    if (sc_cb[sc_id].p_historical == 0)
                                    {
                                        sc_cb[sc_id].p_historical =
                                            (sc_cb[sc_id].ATR + index - 1);
                                    }
                                    break;

                                case TCK:
                                    /* perform LRC check */
                                    sc_cb[sc_id].TCK_present = 1;
                                    lrc = 0;
                                    for (x = 1; x < index; x++)
                                    {
                                        lrc = ((sc_cb[sc_id].ATR[x]) ^ lrc);
                                    }
                                    if (lrc != 0)
                                    {
                                        rc = SCI_ERROR_LRC_FAIL;
                                    }
                                    break;
                            }
                            p_ibyter++;
                        }
                    }

                    if (rc == SCI_ERROR_OK)
                    {
                        /* set EGT based on N and protocol */
                        if ((sc_cb[sc_id].sc_parameters.N == 255)
                            && (sc_cb[sc_id].sc_parameters.T == 0))
                        {
                            /* for T=0, N=255 means minimum guard time in the SCI hardware */
                            sci_parameters.EGT = 0;
                        }
                        else
                        {
                            sci_parameters.EGT = sc_cb[sc_id].sc_parameters.N;
                        }

                        if (ioctl
                            (fd[sc_id], IOCTL_SET_PARAMETERS,
                             &sci_parameters) == 0)
                        {
                            rc = SCI_ERROR_OK;
                            sc_cb[sc_id].atr_size = (unsigned char) index;
                        }

                        ioctl(fd[sc_id], IOCTL_SET_ATR_READY);
                    }
                    else
                    {
                        /* problem receiving ATR, so deactivate */
                        ioctl(fd[sc_id], IOCTL_SET_DEACTIVATE);
                    }
                }
                else
                {
                    rc = -1;
                    ioctl(fd[sc_id], IOCTL_SET_DEACTIVATE);
                }
            }
            else
            {
                rc = -1;
                ioctl(fd[sc_id], IOCTL_SET_DEACTIVATE);
            }
        }
        else
        {
            rc = SCI_ERROR_CARD_NOT_PRESENT;
        }
    }
    else
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

    return (rc);
}

/****************************************************************************
** Name         sc_pps
**
** Purpose:     initiate a protocol and parameter selection request
**
** Parameters:  sc_id: zero-based number to identify Smart Card controller
**              T: protocol to propose to the Smart Card
**              F: clock rate conversion factor to propose to the Smart Card
**              D: baud rate adjustment factor to propose to the Smart Card
**
** Returns:     SCI_ERROR_OK: If the PPS exchange was successful
**              other error code if a failure occurs
****************************************************************************/
SCI_ERROR sc_pps(unsigned long sc_id,
                 unsigned char T, unsigned char F, unsigned char D)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    SCI_PARAMETERS sci_parameters;
    unsigned char request[6], response[6];
    unsigned long readit, index, left, i;
    unsigned long Fi = 0;
    unsigned long Di = 0;
    unsigned long f = 0;
    unsigned char current_T = 0;
    unsigned long current_f = 0;
    unsigned long current_ETU = 0;

    unsigned long card_actived;

    if (sc_id < SCI_NUMBER_OF_CONTROLLERS)
    {
        ioctl(fd[sc_id], IOCTL_GET_IS_CARD_ACTIVATED, &card_actived);

        if (card_actived == 1)
        {
            /* PPSS: initial byte-always 0xFF */
            request[0] = 0xFF;
            /* PPS0: format byte-encodes T and PPS1 to follow */
            request[1] = (0x10 | T);
            /* PPS1: parameter byte-encodes F and D */
            request[2] = (D | (F << 4));
            /* PPS2 and PPS3 are RFU, so ignore */
            /* PCK : check byte-bitwise XOR of PPSS,PPS0,and PPS1 */
            request[3] = (request[0] ^ request[1] ^ request[2]);

            response[0] = 0;
            response[1] = 0;
            response[2] = 0;
            response[3] = 0;
            response[4] = 0;
            response[5] = 0;

            index = 0;
            readit = 0;
            left = 3;

            if ((rc = write(fd[sc_id], request, 4)) > 0)
            {
                rc = SCI_ERROR_OK;

                /* make sure card responds and the response is good */
                while ((rc == SCI_ERROR_OK) && (left > 0))
                {
                    if ((readit = read(fd[sc_id], (response + index), 1)) > 0)
                    {
                        rc = SCI_ERROR_OK;
                    }
                    else
                    {
                        rc = readit;
                    }

                    if (readit == 1)
                    {
                        left--;
                        if (index == 1)
                        {
                            for (i = 0; i < 3; i++)
                            {
                                if ((response[1] & (0x10 << i)) ==
                                    (0x10 << i))
                                {
                                    left++;
                                }
                            }
                        }
                        index++;
                    }
                }

                if (rc == SCI_ERROR_OK)
                {
                    for (i = 0; i < index; i++)
                    {
                        if (request[i] != response[i])
                        {
                            rc = SCI_ERROR_FAIL;
                        }
                    }

                    if (rc == SCI_ERROR_OK)
                    {
                        if (ioctl
                            (fd[sc_id], IOCTL_GET_PARAMETERS,
                             &sci_parameters) == 0)
                        {
                            current_T = sci_parameters.T;
                            current_f = sci_parameters.f;
                            current_ETU = sci_parameters.ETU;
                            Fi = Fi_TABLE[F];
                            Di = Di_TABLE[D];
                            f = f_TABLE[F];

                            if (f != 0)
                            {
                                sci_parameters.f = f;
                            }

                            if ((Fi != 0) && (Di != 0))
                            {
                                sci_parameters.ETU = (Fi / Di);
                            }

                            sci_parameters.T = T;
                            if (ioctl
                                (fd[sc_id], IOCTL_SET_PARAMETERS,
                                 &sci_parameters) == 0)
                            {
                                rc = SCI_ERROR_OK;
                                sc_cb[sc_id].sc_parameters.T = T;
                                sc_cb[sc_id].sc_parameters.F = F;
                                sc_cb[sc_id].sc_parameters.D = D;
                            }
                            else
                            {
                                sci_parameters.T = current_T;
                                sci_parameters.f = current_f;
                                sci_parameters.ETU = current_ETU;
                                if (ioctl
                                    (fd[sc_id], IOCTL_SET_PARAMETERS,
                                     &sci_parameters) == 0)
                                {
                                    rc = SCI_ERROR_OK;
                                }
                                else
                                {
                                    rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
                                }
                            }

                        }
                        else
                        {
                            rc = SCI_ERROR_FAIL;
                        }
                    }
                }
            }
        }
        else
        {
            rc = SCI_ERROR_CARD_NOT_ACTIVATED;
        }
    }
    else
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

    return (rc);
}

/****************************************************************************
** Name         sc_get_ifsd
**
** Purpose:     get the current IFSD setting
**
** Parameters:  sc_id: zero-based number to identify Smart Card controller
**              ifsd:  output pointer to buffer which will hold IFSD value
**
** Returns:     SCI_ERROR_OK: If IFSD was successfully retrieved
**              other error code if a failure occurs
****************************************************************************/
SCI_ERROR sc_get_ifsd(unsigned long sc_id, unsigned char *ifsd)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned long card_actived;

    if ((sc_id < SCI_NUMBER_OF_CONTROLLERS) && (ifsd != 0))
    {
        ioctl(fd[sc_id], IOCTL_GET_IS_CARD_ACTIVATED, &card_actived);

        if (card_actived == 1)
        {
            (*ifsd) = (sc_cb[sc_id].proposed_IFSD);
        }
        else
        {
            rc = SCI_ERROR_CARD_NOT_ACTIVATED;
        }
    }
    else
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

    return (rc);
}

/****************************************************************************
** Name         sc_set_ifsd
**
** Purpose:     set the current IFSD setting
**
** Parameters:  sc_id: zero-based number to identify Smart Card controller
**              ifsd:  new IFSD value
**
** Returns:     SCI_ERROR_OK: If IFSD was set successfully
**              other error code if a failure occurs
****************************************************************************/
SCI_ERROR sc_set_ifsd(unsigned long sc_id, unsigned char ifsd)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned long card_actived;

    if ((sc_id < SCI_NUMBER_OF_CONTROLLERS) && (ifsd >= SC_MIN_IFSD)
        && (ifsd <= SC_MAX_IFSD))
    {
        ioctl(fd[sc_id], IOCTL_GET_IS_CARD_ACTIVATED, &card_actived);

        if (card_actived == 1)
        {
            sc_cb[sc_id].proposed_IFSD = ifsd;
        }
        else
        {
            rc = SCI_ERROR_CARD_NOT_ACTIVATED;
        }
    }
    else
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

    return (rc);
}

/****************************************************************************
** Name         sc_apdu
**
** Purpose:     Send a command APDU (Application Protocol Data Unit) and
**              receive the response APDU (see ISO/IEC 7816-4:1995(E),
**              section 5.3,pages 7-9).
**
** Parameters:  sc_id: zero-based Smart Card identifier
**              p_capdu: (input)  pointer to the command APDU buffer
**              p_rapdu: (output) pointer to the response APDU buffer
**              p_length:(input)  pointer the length of the command APDU
**                       (output) pointer the length of the response APDU
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
****************************************************************************/
SCI_ERROR sc_apdu(unsigned long sc_id,
                  unsigned char *p_capdu,
                  unsigned char *p_rapdu, unsigned long *p_length)
{
    SCI_ERROR rc = SCI_ERROR_OK;        /* return code */
    unsigned char *p_body = 0;  /* pointer to the body */
    unsigned char *p_data = 0;  /* pointer to the data */
    unsigned long L = 0;        /* length of the body */
    unsigned short Lc = 0;      /* length of command data */
    unsigned long Le = 0;       /* length of expected data */
    unsigned char SW1 = 0;      /* first byte of end sequence */
    unsigned char SW2 = 0;      /* second byte of end sequence */
    unsigned char t0_header[5]; /* t0 header */
    int i;

    unsigned long card_present;

    if ((sc_id < SCI_NUMBER_OF_CONTROLLERS) &&
        (p_capdu != 0) && (p_rapdu != 0) && (p_length != 0)
        && (*p_length != 0))
    {
        ioctl(fd[sc_id], IOCTL_GET_IS_CARD_PRESENT, &card_present);

        if (card_present == 1)
        {
            /* determine L */
            L = (*p_length) - 4;
            /* identify body */
            p_body = p_capdu + 4;
            /* parse body into relevant fields */

            if ((rc =
                 process_body(p_body, L, &p_data, &Lc, &Le)) == SCI_ERROR_OK)
            {
                /* transmit/receive APDU data based on transmission protocol */

                if (sc_cb[sc_id].sc_parameters.T == 0)
                {
                    memcpy((void *) t0_header, (const void *) p_capdu, 4);

                    if (Lc == 0)
                    {
                        /* incoming data command */
                        if (Le == 0)
                        {
                            t0_header[4] = 0;
                            *p_length = 2;
                        }
                        else
                        {
                            if (Le < 256)
                            {
                                t0_header[4] = (unsigned char) Le;
                                *p_length = Le + 2;
                            }
                            else
                            {
                                t0_header[4] = 0;
                                *p_length = 258;
                            }
                        }

                        rc = sc_t0_command(sc_id, t0_header, p_rapdu,
                                           (p_rapdu + Le), 1);
                        /* ensure SW1 and SW2 do not indicate an aborted process */
                        check_incoming_data(p_rapdu,
                                            p_length,
                                            *(p_rapdu + Le),
                                            *(p_rapdu + Le + 1));
                    }
                    else if (Lc < 256)
                    {
                        /* outgoing data command, ignore Le for the moment */
                        t0_header[4] = (unsigned char) Lc;

                        /* no incoming data, so p_rapdu will contain only the end sequence */
                        if ((rc = sc_t0_command(sc_id, t0_header, p_data, p_rapdu,0))
                            == SCI_ERROR_OK)
                        {
                            if (Le == 0)
                            {
                                *p_length = 2;
                            }
                            else
                            {
                                /* expect to issue a GET_RESPONSE command */
                                SW1 = p_rapdu[0];
                                SW2 = p_rapdu[1];

                                if ((SW1 == 0x61) ||
                                   ((SW1 == 0x90) && (SW2 == 0x00)))
                                {
                                    /* must issue a GET RESPONSE command */
                                    if ((SW1 == 0x61) && (SW2 < Le))
                                    {
                                        /* Lx=SW2, change Le to min(Le,Lx) */
                                        Le = (unsigned long) SW2;
                                    }
                                    /* change INS to GET RESPONSE command */
                                    t0_header[1] = 0xC0;
                                    /* change P3 to new Le */
                                    t0_header[4] = (unsigned char) Le;
                                    *p_length = Le + 2;
                                    rc = sc_t0_command(sc_id,
                                                       t0_header,
                                                       p_rapdu,
                                                       (p_rapdu + Le),
                                                       1);
                                    /* ensure SW1 and SW2 do not indicate an aborted process */
                                    check_incoming_data(p_rapdu,
                                                        p_length,
                                                        *(p_rapdu+Le),
                                                        *(p_rapdu+Le+1));
                                }
                                else
                                {
                                    /* GET_RESPONSE command not indicated-an error occurred */
                                    *p_length = 2;
                                }
                            }
                        }
                    }
                    else
                    {
                        *p_rapdu = 0x67;
                        *p_length = 2;
                    }
                }
                else if (sc_cb[sc_id].sc_parameters.T == 1)
                {
                    rc = sc_t1_command(sc_id, p_capdu, p_rapdu, p_length);

		}
                else
                {
                    /* unsupported protocol */
                    rc = SCI_ERROR_FAIL;
                    printf("unsupported protocol");
                }
            }
        }
        else
        {
            rc = SCI_ERROR_CARD_NOT_PRESENT;
        }
    }
    else
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

    return (rc);
}

/****************************************************************************
** Function:    sc_get_parameters
**
** Purpose:     Retrieve the current Smart Card ATR parameters.
**
** Parameters:  sc_id: zero-based number to identify smart card controller
**              p_sci_parameters: output pointer to Smart Card ATR parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
*****************************************************************************/
SCI_ERROR sc_get_parameters(unsigned long sc_id,
                            SC_PARAMETERS * p_sc_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    unsigned long card_actived;

    if ((p_sc_parameters != 0) && (sc_id < SCI_NUMBER_OF_CONTROLLERS))
    {
        ioctl(fd[sc_id], IOCTL_GET_IS_CARD_ACTIVATED, &card_actived);

        if (card_actived == 1)
        {
            p_sc_parameters->T = sc_cb[sc_id].sc_parameters.T;
            p_sc_parameters->maskT = sc_cb[sc_id].sc_parameters.maskT;
            p_sc_parameters->F = sc_cb[sc_id].sc_parameters.F;
            p_sc_parameters->D = sc_cb[sc_id].sc_parameters.D;
            p_sc_parameters->FI = sc_cb[sc_id].sc_parameters.FI;
            p_sc_parameters->DI = sc_cb[sc_id].sc_parameters.DI;
            p_sc_parameters->II = sc_cb[sc_id].sc_parameters.II;
            p_sc_parameters->PI1 = sc_cb[sc_id].sc_parameters.PI1;
            p_sc_parameters->PI2 = sc_cb[sc_id].sc_parameters.PI2;
            p_sc_parameters->WI = sc_cb[sc_id].sc_parameters.WI;
            p_sc_parameters->XI = sc_cb[sc_id].sc_parameters.XI;
            p_sc_parameters->UI = sc_cb[sc_id].sc_parameters.UI;
            p_sc_parameters->N = sc_cb[sc_id].sc_parameters.N;
            p_sc_parameters->CWI = sc_cb[sc_id].sc_parameters.CWI;
            p_sc_parameters->BWI = sc_cb[sc_id].sc_parameters.BWI;
            p_sc_parameters->IFSC = sc_cb[sc_id].sc_parameters.IFSC;
            p_sc_parameters->check = sc_cb[sc_id].sc_parameters.check;
        }
        else
        {
            rc = SCI_ERROR_CARD_NOT_ACTIVATED;
        }
    }
    else
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

    return (rc);
}

/****************************************************************************
** Function:    sc_get_atr
**
** Purpose:     Retrieve the ATR. 
**
** Parameters:  sc_id:    zero-based number to identify smart card controller
**              ATR:               output pointer to ATR buffer
**              atr_size:          output pointer to ATR size
**              historical_offset: output pointer to historical offset
**              historical_size:   output pointer to historical size
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
*****************************************************************************/
SCI_ERROR sc_get_atr(unsigned long sc_id,
                     unsigned char *ATR,
                     unsigned char *atr_size,
                     unsigned char *historical_offset,
                     unsigned char *historical_size)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    int i = 0;
    unsigned long card_actived;

    if ((sc_id < SCI_NUMBER_OF_CONTROLLERS) &&
        (ATR != 0) && (atr_size != 0) && (historical_offset != 0)
        && (historical_size != 0))
    {
        ioctl(fd[sc_id], IOCTL_GET_IS_CARD_ACTIVATED, &card_actived);

        if (card_actived == 1)
        {
            *atr_size = sc_cb[sc_id].atr_size;

            if (sc_cb[sc_id].atr_size > 0)
            {
                for (i = 0; i < (sc_cb[sc_id].atr_size); i++)
                {
                    ATR[i] = (sc_cb[sc_id].ATR[i]);
                }

                *historical_offset =
                    (unsigned char) (sc_cb[sc_id].p_historical -
                                     sc_cb[sc_id].ATR);
                *historical_size = sc_cb[sc_id].historical_size;
            }
            else
            {
                rc = SCI_ERROR_NO_ATR;
            }
        }
        else
        {
            rc = SCI_ERROR_CARD_NOT_ACTIVATED;
        }
    }
    else
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

    return (rc);
}

/****************************************************************************
** Name         process_body
**
** Purpose:     Parse ADPU body into fields.
**
** Parameters:  p_body: (input) pointer to the command APDU body
**              L:      (input) length of the command APDU body
**              pp_data:(output) pointer to the data field pointer
**              p_Lc:   (output) pointer to the data field length
**              p_Le:   (output) pointer to the expected data length
**
** Returns:     SCI_ERROR_OK:   if successful
**              SCI_ERROR_FAIL: if one or more fields are invalid
****************************************************************************/
static SCI_ERROR process_body(unsigned char *p_body,
                              unsigned long L,
                              unsigned char **pp_data,
                              unsigned short *p_Lc, unsigned long *p_Le)
{
    SCI_ERROR rc = SCI_ERROR_OK;        /* return code */
    unsigned short B2B3 = 0;    /* length of expected data */

    /* determine Lc and Le */

    if (L == 0)
    {
        /* case 1 */
        *p_Lc = 0;
        *pp_data = 0;
        *p_Le = 0;
    }
    else if (L == 1)
    {
        /* case 2S */
        *p_Lc = 0;
        *pp_data = 0;
        *p_Le = (unsigned long) p_body[0];

        if (*p_Le == 0)
        {
            *p_Le = 256;
        }
    }
    else if ((L == (unsigned long) (1 + p_body[0])) && (p_body[0] != 0))
    {
        /* case 3S */
        *p_Lc = (unsigned short) p_body[0];
        *pp_data = p_body + 1;
        *p_Le = 0;
    }
    else if ((L == (unsigned long) (2 + p_body[0])) && (p_body[0] != 0))
    {
        /* case 4S */
        *p_Lc = (unsigned short) p_body[0];
        *pp_data = p_body + 1;
        *p_Le = (unsigned long) p_body[L - 1];

        if (*p_Le == 0)
        {
            *p_Le = 256;
        }
    }
    else if ((L == 3) && (p_body[0] == 0))
    {
        /* case 2E */
        *p_Lc = 0;
        *pp_data = 0;
        *p_Le = (*(unsigned long *) (p_body + L - 2));

        if (*p_Le == 0)
        {
            *p_Le = 65536;
        }
    }
    else
    {
        B2B3 = *(unsigned short *) (p_body + 1);

        if ((p_body[0] == 0) && (B2B3 != 0))
        {
            if (L == (unsigned long) (3 + B2B3))
            {
                /* case 3E */
                *p_Lc = B2B3;
                *pp_data = p_body + 3;
                *p_Le = 0;
            }
            else if (L == (unsigned long) (5 + B2B3))
            {
                /* case 4E */
                *p_Lc = B2B3;
                *pp_data = p_body + 3;
                *p_Le = (*(unsigned long *) (p_body + L - 2));

                if (*p_Le == 0)
                {
                    *p_Le = 65536;
                }
            }
            else
            {
                rc = SCI_ERROR_FAIL;
            }
        }
        else
        {
            rc = SCI_ERROR_FAIL;
        }
    }

    return (rc);
}

/****************************************************************************
** Name         sc_t0_command
**
** Purpose:     Perform a protocol T=0 command transaction.
**              (see ISO/IEC 7816-3:1997(E),section 8,pages 15-16).
**
** Parameters:  p_header: the 5-byte command header input buffer
**              p_body: the data field, input/output buffer
**              p_end_sequence: the SW1 and SW2 values, output buffer
**              direction: direction of the data transfer-
**                         0: write
**                         1: read
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
****************************************************************************/
static SCI_ERROR sc_t0_command(unsigned long sc_id,
                               unsigned char *p_header,
                               unsigned char *p_body,
                               unsigned char *p_end_sequence,
                               unsigned long direction)
{
    ssize_t bytes_rec = 0;      /* bytes received */
    unsigned long bytes_left = 0;       /* bytes left to transmit */
    unsigned long done = 0;     /* command complete flag */
    unsigned long end = 0;      /* end sequence flag */
    unsigned char ins = 0;      /* instruction byte used for acknowledgment */
    unsigned char current = 0;  /* current procedure byte */
    SCI_ERROR rc = SCI_ERROR_OK;        /* return code */
    int i;

    ins = p_header[1];
    bytes_left = p_header[4];

    if(bytes_left == 0)
    {
        bytes_left = 256;
    }

    /* send the command */
    if ((rc = write(fd[sc_id], p_header, 5)) > 0)
    {
        rc = SCI_ERROR_OK;

        while ((done == 0) && (rc == SCI_ERROR_OK))
        {
            if(direction == 1)
            {
                if(bytes_rec == 2)
                {
                   current = sc_cb[sc_id].temp_buf[1];
                }
                else
                {
                    /* read all expected data-ack(1), body(bytes_left), and end sequence(2) */
                    if((bytes_rec = read(fd[sc_id], &sc_cb[sc_id].temp_buf, bytes_left+3)) > 0)
                    {
                        rc = SCI_ERROR_OK;
                    }
                    else
                    {
                        rc = bytes_rec;
                    }
                    if((rc == SCI_ERROR_OK) || (rc == SCI_ERROR_WWT_TIMEOUT))
                    {
                        if(bytes_rec == (bytes_left + 3))
                        {
                            /* all expect data received */
                            memcpy(p_body, sc_cb[sc_id].temp_buf+1, bytes_left);
                            p_end_sequence[0] = sc_cb[sc_id].temp_buf[bytes_left + 1];
                            current = sc_cb[sc_id].temp_buf[bytes_left + 2];
                            end = 1;
                        }
                        else if((bytes_rec == 1) || (bytes_rec == 2))
                        {
                            /* just a procedure byte or an end sequence was received */
                            current = sc_cb[sc_id].temp_buf[0];
                            if(bytes_rec == 2)
                            {
                                /* an end sequence was received, so clear any SCI_ERROR_WWT_TIMEOUT */
                                rc = SCI_ERROR_OK;
                            }
                        }
                        else
                        {
                            rc=SCI_ERROR_FAIL;
                        }
                    }
                }
            }
            else
            {
                /* read 1 procedure byte */
                if ((bytes_rec = read(fd[sc_id], &current, 1)) > 0)
                {
                    rc = SCI_ERROR_OK;
                }
                else
                {
                    rc = bytes_rec;
                }
            }

            /* WWT timeout OK if we successfully read the procedure byte */
            if ((rc == SCI_ERROR_OK) || (rc == SCI_ERROR_WWT_TIMEOUT))
            {
                //rc = SCI_ERROR_OK;
                if(bytes_rec > 0)
                {
                    if (end == 1)
                    {
                        p_end_sequence[1] = current;
                        rc = SCI_ERROR_OK;
                        done = 1;
                    }
                    else if (((current >= 0x61) && (current <= 0x6F)) ||
                             (current >= 0x90) && (current <= 0x9F))
                    {
                        /* this is the end sequence, so save SW1 and flag the end sequence */
                        p_end_sequence[0] = current;
                        end = 1;
                    }
                    else if ((current == ins) || (current == (ins ^ 0x01)))
                    {
                        /* if the ACK is equal to the instruction, send/receive the command data */
                        if (direction == 0)
                        {
                            //if (write(fd[sc_id], p_body, bytes_left) <= 0)
                            if ((rc = write(fd[sc_id], p_body, bytes_left)) <= 0)
                            {
                                //rc = errno;
                                bytes_left = 0;
                                done = 1;
                            }
                            else
                            {
                                rc = SCI_ERROR_OK;
                            }
                        }
                    }
                    else if ((current == (ins ^ 0xFF))
                             || (current == (ins ^ 0xFE)))
                    {
                        /* send/receive only one byte */
                        if (direction == 0)
                        {
                            if (bytes_left > 0)
                            {
                                //if (write(sc_id, p_body, 1) > 0)
                                if ((rc = write(sc_id, p_body, 1)) > 0)
                                {
                                    rc = SCI_ERROR_OK;
                                }
                                /*else
                                {
                                    rc = errno;
                                }*/

                                p_body++;
                                bytes_left--;
                            }
                            else
                            {
                                rc = SCI_ERROR_FAIL;
                                done = 1;
                            }
                        }
                        else
                        {
                            if ((bytes_rec = read(fd[sc_id], p_body, 1)) > 0)
                            {
                                rc = SCI_ERROR_OK;
                                if (bytes_rec == 1)
                                {
                                    bytes_left--;
                                    p_body++;
                                }
                                else
                                {
                                    rc = SCI_ERROR_FAIL;
                                    done = 1;
                                }
                            }
                            else
                            {
                                //rc = errno;
                                rc = bytes_rec;
                            }
                        }
                    }
                    else
                    {
                        if (current != 0x60)
                        {
                            rc = SCI_ERROR_FAIL;
                            done = 1;
                        }
                    }
                }
                else
                {
                    if(rc == SCI_ERROR_OK)
                    {
                        rc = SCI_ERROR_FAIL;
                    }
                    done = 1;
                }
            }
        }
    }

    return (rc);
}

/****************************************************************************
** Name         sc_t1_command
**
** Purpose:     Perform a protocol T=1 command transaction.
**
** Parameters:  sc_id: zero-based Smart Card identifier
**              p_capdu: (input)  pointer to the command APDU buffer
**              p_rapdu: (output) pointer to the response APDU buffer
**              p_length:(input)  pointer the length of the command APDU
**                       (output) pointer the length of the response APDU
**
** Returns:     SCI_ERROR_OK: if successful
**              other error code if a failure occurs
****************************************************************************/
static SCI_ERROR sc_t1_command(unsigned long sc_id,
                               unsigned char *p_capdu,
                               unsigned char *p_rapdu,
                               unsigned long *p_length)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    SCI_PARAMETERS sci_parameters;
    unsigned long done = 0;
    unsigned char loop = 0;
    BLOCKS d_block_type = I;
    BLOCKS c_block_type = I;
    unsigned long capdu_length = 0;
    unsigned long rapdu_length = 0;
    unsigned long c_block_length = 0;
    unsigned char c_block[SC_MAX_T1_BLOCK_SIZE];
    unsigned char d_block[SC_MAX_T1_BLOCK_SIZE];
    unsigned char *p_data;
    unsigned char retry = 0;
    unsigned char resynch = 0;
    unsigned char resend = 0;
    //unsigned long i_block_size = 0;
    //unsigned char *i_block;
    unsigned long time = 0;

    p_data = p_capdu;
    capdu_length = *p_length;
    loop = 1;
    done = 0;
    /* this code does not use NAD byte */
    c_block[0] = 0;

    /* first send IFSD request, if IFSD is not maximum */

    if (sc_cb[sc_id].IFSD != sc_cb[sc_id].proposed_IFSD)
    {
        d_block_type = S;
        d_block[1] = S_IFS_REQUEST;
        d_block[2] = 1;
        d_block[3] = sc_cb[sc_id].proposed_IFSD;
    }

    while ((done == 0) && (rc == SCI_ERROR_OK))
    {
        /* copy NAD */
        d_block[0] = c_block[0];
        /* create next block to send */

        if (d_block_type == I)
        {
            if (resend == 0)
            {
                /* I-block */
                /* set current state of N(S) in PCB byte */

                if ((sc_cb[sc_id].NS) == 0)
                {
                    d_block[1] = 0;
                }
                else
                {
                    d_block[1] = I_NS;
                }

                /* toggle N(S) value in PCB byte for next I-block */
                sc_cb[sc_id].NS = (sc_cb[sc_id].NS + 1) % 2;

                if (capdu_length <= sc_cb[sc_id].sc_parameters.IFSC)
                {
                    /* no chaining required, send only 1 block */
                    /* set LEN byte */
                    d_block[2] = (unsigned char) capdu_length;
                    capdu_length = 0;
                }
                else
                {
                    /* set LEN byte */
                    d_block[2] = sc_cb[sc_id].sc_parameters.IFSC;
                    capdu_length -= sc_cb[sc_id].sc_parameters.IFSC;
                    /* set more data bit in PCB byte */
                    d_block[1] |= I_M;
                }

                /* copy INF field if it exists */
                if (d_block[2] > 0)
                {
                    memcpy((void *) (d_block + 3), (const void *) p_data,
                           (size_t) d_block[2]);
                    //i_block = p_data;
                    p_data += d_block[2];
                }
                //i_block_size = (d_block[2] + 3);
            }
            /*else
            {
                memcpy((void *) d_block, (const void *) i_block,
                       (size_t) i_block_size);
                resend = 0;
            }*/
        }

        /* write the block */
        //if (write(fd[sc_id], d_block, (d_block[2] + 3)) > 0)
        if ((rc = write(fd[sc_id], d_block, (d_block[2] + 3))) > 0)
        {
            rc = SCI_ERROR_OK;
            /* read the response block */

            while (loop > 0)
            {
                /* this may have to loop on read if a WTX request is made */
                if ((c_block_length =
                     read(fd[sc_id], c_block, SC_MAX_T1_BLOCK_SIZE)) > 0)
                {
                    rc = SCI_ERROR_OK;
                }
                else
                {
                    rc = c_block_length;
                }
                if (rc != SCI_ERROR_BWT_TIMEOUT)
                {
                    loop = 0;
                }
                else
                {
                    loop --;
                }
            }

            loop = 1;

            if (rc == SCI_ERROR_OK)
            {
                /* the block was successfully read */
                /* determine block type */

                if (((c_block[1]) & 0x80) == 0x80)
                {
                    if (((c_block[1]) & 0x40) == 0x40)
                    {
                        c_block_type = S;
                    }
                    else
                    {
                        c_block_type = R;
                    }
                }
                else
                {
                    c_block_type = I;
                }

                switch (c_block_type)
                {

                    case I:
                        /* copy INF data to RAPDU */
                        memcpy((void *) (p_rapdu + rapdu_length),
                               (const void *) (c_block + 3),
                               (size_t) c_block[2]);
                        rapdu_length += (c_block[2]);
                        /* check more data bit */

                        if (((c_block[1]) & I_M) == I_M)
                        {
                            /* next block to send is an R-block */
                            d_block_type = R;
                            d_block[2] = 0;
                            /* N(R) is next expected N(S) */

                            if (((c_block[1]) & I_NS) == I_NS)
                            {
                                d_block[1] = 0x80;
                            }
                            else
                            {
                                d_block[1] = (0x80 | R_NR);
                            }
                        }
                        else
                        {
                            done = 1;
                        }

                        break;

                    case R:
                        if ((((c_block[1]) & R_NR) >> 4) != sc_cb[sc_id].NS)
                        {
                            if(resend < 2)
                            {
                               resend ++;
                               d_block_type = I;
                            }
                            else
                            {
                               resend  = 0;
                               resynch = 1;
                               rc = SCI_ERROR_FAIL;
                            }
                        }
                        else
                        {
                            /* if not chaining, an error has occurred */

                            if (capdu_length == 0)
                            {
                                rc = SCI_ERROR_FAIL;
                            }
                        }

                        break;

                    case S:
                        if ((c_block[1]) == S_WTX_REQUEST)
                        {
                            /* 1-byte INF field holds BWT multiplier */
                            loop = c_block[3];
                            d_block_type = S;
                            d_block[1] = S_WTX_RESPONSE;
                            d_block[2] = 1;
                            d_block[3] = c_block[3];
                        }
                        else if ((c_block[1]) == S_IFS_RESPONSE)
                        {
                            if (((c_block[2]) == 1)
                                && ((c_block[3]) == d_block[3]))
                            {
                                /* valid response */
                                sc_cb[sc_id].IFSD =
                                    sc_cb[sc_id].proposed_IFSD;
                                d_block_type = I;
                            }

                            /* else re-send request, so no change to d_block */
                        }
                        else if ((c_block[1]) == S_IFS_REQUEST)
                        {
                            d_block_type = S;
                            d_block[1] = S_IFS_RESPONSE;
                            d_block[2] = 1;
                            d_block[3] = c_block[3];
                            sc_cb[sc_id].sc_parameters.IFSC = c_block[3];
                        }
                        else if ((c_block[1]) == S_RESYNCH_RESPONSE)
                        {
                            /* reset protocol parameters */
                            sc_cb[sc_id].NS = 0;
                            sc_cb[sc_id].sc_parameters.IFSC = SC_ATR_IFSC;
                            sc_cb[sc_id].IFSD = SC_ATR_IFSD;
                            resend = 0;
                            /* reset pointers in order to re-transmit original I-block */
                            p_data = p_capdu;
                            capdu_length = *p_length;
                            rapdu_length = 0;
                            d_block_type = I;
                        }
                        else if ((c_block[1]) == S_ABORT_REQUEST)
                        {
                            d_block_type = S;
                            d_block[1] = S_ABORT_RESPONSE;
                            d_block[2] = 0;
                            resend = 0;
                            /* reset pointers in order to re-transmit original I-block */
                            p_data = p_capdu;
                            capdu_length = *p_length;
                        }

                        break;
                }
            }

            if (rc != SCI_ERROR_OK)
            {
                /* must wait at least the greater of CWT or BWT */
                /* before attempting to resend */
                ioctl(fd[sc_id], IOCTL_GET_PARAMETERS, &sci_parameters);

                if (sci_parameters.BWT > sci_parameters.CWT)
                {
                    time =
                        ((sci_parameters.BWT) * (sci_parameters.ETU) * 1000) /
                        sci_parameters.f;
                }
                else
                {
                    time =
                        ((sci_parameters.CWT) * (sci_parameters.ETU) * 1000) /
                        sci_parameters.f;
                }

                if (time == 0)
                {
                    time = 1;
                }

                sleep(time / 1000);

                if (resynch == 0)
                {
                    if (retry < 3)
                    {
                        /* if last block was an S-block, just re-attempt to send it */

                        if (d_block_type != S)
                        {
                            /* for R and I blocks, request re-transmit of last block */
                            d_block_type = R;
                            /* N(R) is next expected N(S) (opposite next N(S) to send) */

                            if (sc_cb[sc_id].NS == 0)
                            {
                                d_block[1] = (0x80 | R_NR);
                            }
                            else
                            {
                                d_block[1] = 0x80;
                            }

                            d_block[2] = 0;
                        }

                        rc = SCI_ERROR_OK;
                        retry++;
                    }
                    else
                    {
                        retry = 0;
                        resynch++;
                    }
                }

                if (resynch == 1)
                {
                    if (retry < 3)
                    {
                        /* issue resynch request */
                        d_block_type = S;
                        d_block[1] = S_RESYNCH_REQUEST;
                        d_block[2] = 0;
                        rc = SCI_ERROR_OK;
                        retry++;
                    }
                    else
                    {
                        retry = 0;
                        resynch++;
                    }
                }
                else
                {
                    /* just return error */
                }
            }
        }
        /*else
        {
            rc = errno;
        }*/
    }

    *p_length = rapdu_length;

    if (rc != SCI_ERROR_OK)
    {
        /* resynch failed, reset */
        sc_reset(sc_id);
    }

    return (rc);
}

static void check_incoming_data(unsigned char *buffer,
                                unsigned long *length,
                                unsigned char SW1,
                                unsigned char SW2
)
{
    /* if process was aborted, only 2 status bytes received, not the expected Le */
    /* (see ISO/IEC 7816-4:1995(E),section 5.3.3 and 5.3.4, pages 9 and 11).     */
    if((SW1 == 0x64) || (SW1 == 0x65) || ((SW1 >= 0x67) && (SW1 <= 0x6F)))
    {
        *length   = 2;
        buffer[0] = SW1;
        buffer[1] = SW2;
    }
}
