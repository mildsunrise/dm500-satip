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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Mike Lepore
| Component: sci
| File:      sci_atom.c
| Purpose:   Atom functions for smart card interface control
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/13/2001  MAL, Zongwei Liu  Added EMV2000 support
/ 04/21/2003  Detrick, Mark     Fixed bug in sci_atom_write_fifo.
/                               If the num_bytes was not on a word boundry ,
/                               num_bytes would decrement below 0 and a machine
/                               check would occor also added int num_bytes_left
/                               to make sure we didn't receive a negative number
+----------------------------------------------------------------------------*/

#include "sci_atom.h"
#include "sci_local.h"
#include "os/os-types.h"
#include "os/os-io.h"
#include "hw/hardware.h"

extern SCI_CONTROL_BLOCK sci_cb[SCI_NUMBER_OF_CONTROLLERS];
extern SCI_DRV_MODES sci_drv_modes;

/* prototype of local functions */
static void _set_bits   (ULONG sci_id, ULONG reg, UCHAR bits);
static void _clear_bits (ULONG sci_id, ULONG reg, UCHAR bits);
static INT  _check_bits (ULONG sci_id, ULONG reg, UCHAR bits);

/****************************************************************************
** Function:    sci_atom_assign_reg_address
**
** Purpose:     assign the register addresses
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_assign_reg_address(ULONG sci_id)
{
    sci_cb[sci_id].scbuffs   =
        (ULONG *) (sci_cb[sci_id].sci_base_address + SCBUFFS);
    sci_cb[sci_id].scctl0    =
        (UCHAR *) (sci_cb[sci_id].sci_base_address + SCCTL0);
    sci_cb[sci_id].scctl1    =
        (UCHAR *) (sci_cb[sci_id].sci_base_address + SCCTL1);
    sci_cb[sci_id].scinten   =
        (USHORT *)(sci_cb[sci_id].sci_base_address + SCINTEN0);
    sci_cb[sci_id].scint     =
        (USHORT *)(sci_cb[sci_id].sci_base_address + SCINT0);
    sci_cb[sci_id].scstat    =
        (UCHAR *) (sci_cb[sci_id].sci_base_address + SCSTAT);
    sci_cb[sci_id].scbmr     =
        (UCHAR *) (sci_cb[sci_id].sci_base_address + SCBMR);
    sci_cb[sci_id].scclk_cnt0=
        (UCHAR *) (sci_cb[sci_id].sci_base_address + SCCLK_CNT0);
    sci_cb[sci_id].scctl3    =
        (UCHAR *) (sci_cb[sci_id].sci_base_address + SCCTL3);
    sci_cb[sci_id].scetu     =
        (USHORT *)(sci_cb[sci_id].sci_base_address + SCETU0);
    sci_cb[sci_id].scrxlen   =
        (USHORT *)(sci_cb[sci_id].sci_base_address + SCRXLEN);
    sci_cb[sci_id].sctxlen   =
        (USHORT *)(sci_cb[sci_id].sci_base_address + SCTXLEN);
    sci_cb[sci_id].sccwt     =
        (USHORT *)(sci_cb[sci_id].sci_base_address + SCCWT0);
    sci_cb[sci_id].scegt     =
        (UCHAR *) (sci_cb[sci_id].sci_base_address + SCEGT);
    sci_cb[sci_id].scbwt     =
        (ULONG *) (sci_cb[sci_id].sci_base_address + SCBWT0);
}

/*****************************************************************************
** Function:    sci_atom_reg_init
**
** Purpose:     Initialize the registers of Smart Card Interface
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_reg_init(ULONG sci_id)
{
    /* active-low detect;active-low VCC;clock stop low;auto mode */
    _OS_OUTB(sci_cb[sci_id].scctl3, AUTO);
    if(sci_drv_modes.vcc_polarity == ACTIVE_HIGH)
    {
        /* active-high VCC */
        _set_bits(sci_id, SCCTL3, POLVCC);
    }
    else
    {
        /* active-low VCC */
        _clear_bits(sci_id, SCCTL3, POLVCC);
    }

    if(sci_drv_modes.detect_polarity == ACTIVE_HIGH)
    {
        /* active-high detect */
        _set_bits(sci_id, SCCTL3, POLDET);
    }
    else
    {
        /* active-low detect */
        _clear_bits(sci_id, SCCTL3, POLDET);
    }

    if(sci_drv_modes.vcc_polarity == ACTIVE_HIGH)
    {
        /* VCC off(low);all lines disabled;direct */
        _OS_OUTB(sci_cb[sci_id].scctl0, DEACT);
    }
    else
    {
        /* VCC off(high);all lines disabled;direct */
        _OS_OUTB(sci_cb[sci_id].scctl0, (DEACT|VCC));
    }

    /* LRC;T=0;FIFOs disabled */
    _OS_OUTB(sci_cb[sci_id].scctl1, 0);

    /* set the interrupt mask to zero */
    _OS_OUTW(sci_cb[sci_id].scinten, 0);

    /* clear all the interrupts */
    _OS_OUTW(sci_cb[sci_id].scint,
        BOI|REI|TUI|CHI|TSI|RFI|RXLEN|CDI|RFTHI|TFHI|RUI|PARI|LTXI|FRXI);

    /* clear the UIC status */
    sci_atom_clear_UICSR(sci_id);

    /* set the interrupt mask to zero, except card detect */
    _OS_OUTW(sci_cb[sci_id].scinten, ENCDI);

    /* DMA disabled;Rx FIFO not empty:Tx FIFO < half full */
    _OS_OUTB(sci_cb[sci_id].scbmr, TFFT);

    /* set rxlen to max */
    _OS_OUTW(sci_cb[sci_id].scrxlen, 0x1FF);
    _OS_OUTW(sci_cb[sci_id].sctxlen, 0);
}

/****************************************************************************
** Function:    sci_atom_set_chip_connection
**
** Purpose:     Configure the register control the chip internal connection
****************************************************************************/
void sci_atom_set_chip_connection(void)
{
#if defined (__DRV_FOR_VESTA__)
    MT_DCR(CICSEL3, MF_DCR(CICSEL3) | 0x60000000);
#elif defined (__DRV_FOR_PALLAS__)
    MT_DCR(CICCR, MF_DCR(CICCR) | 0x80);
#elif defined (__DRV_FOR_VULCAN__)
#endif
}

/****************************************************************************
** Name:        sci_atom_clear_UICSR
**
** Purpose:     clear the UIC status
****************************************************************************/
void sci_atom_clear_UICSR(ULONG sci_id)
{
    MT_DCR(UICSR, MF_DCR(UICSR) & (~sci_cb[sci_id].sci_interrupt_mask));
}

/****************************************************************************
** Name:        sci_atom_get_interrupt_status
**
** Purpose:     get the status of interrrupt
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns      the value of interrupt status register
****************************************************************************/
USHORT sci_atom_get_interrupt_status(ULONG sci_id)
{
    return(_OS_INW(sci_cb[sci_id].scint));
}

/****************************************************************************
** Name:        sci_atom_clear_interrupt_status
**
** Purpose:     clear the specified bits of interrrupt
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              bits:   bit mask
****************************************************************************/
void sci_atom_clear_interrupt_status(ULONG sci_id, USHORT bits)
{
    _OS_OUTW(sci_cb[sci_id].scint, bits);
}

/****************************************************************************
** Name:        sci_atom_check_rx_data_pending
**
** Purpose:     check any pending recieve data 
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns      0: if no data available
**              1: if any recieve data is pending
****************************************************************************/
INT sci_atom_check_rx_data_pending(ULONG sci_id)
{
    return(_check_bits(sci_id, SCSTAT, RPEND));
}

/****************************************************************************
** Name:        sci_atom_check_tx_fifo_half_full
**
** Purpose:     check the status of TX FIFO if under half full
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns      0: if FIFO <  1/2 full
**              1: if FIFO >= 1/2 full
****************************************************************************/
INT sci_atom_check_tx_fifo_half_full(ULONG sci_id)
{
    return(_check_bits(sci_id, SCSTAT, 0x20));
}

/****************************************************************************
** Name:        sci_atom_enable_tx_interrupts
**
** Purpose:     enable Tx related interrupts
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_enable_tx_interrupts(ULONG sci_id)
{
    /* enable Tx interrupts */
    _OS_OUTW(sci_cb[sci_id].scinten, (ENBOI|ENTUI|ENCDI|EPARI|ELTXI));
}

/****************************************************************************
** Name:        sci_atom_enable_tx_threshold_interrupts
**
** Purpose:     enable Tx threshold interrupts
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_enable_tx_threshold_interrupts(ULONG sci_id)
{
    _set_bits(sci_id, SCINTEN1, ETFHI);
}

/****************************************************************************
** Name:        sci_atom_disable_tx_threshold_interrupts
**
** Purpose:     disable Tx threshold interrupts
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_disable_tx_threshold_interrupts(ULONG sci_id)
{
    _clear_bits(sci_id, SCINTEN1, ETFHI);
}

/****************************************************************************
** Name:        sci_atom_set_rx_threshold_interrupts
**
** Purpose:     Set Rx threshold interrupts according to num_bytes
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              num_bytes: number of bytes to be read
****************************************************************************/
void sci_atom_set_rx_threshold_interrupts(ULONG sci_id, ULONG num_bytes)
{
    if(num_bytes < 16)
    {
        /* set RX threshold for not empty */
        _clear_bits(sci_id, SCBMR, RFFT);
        //outbyte(sci_cb[sci_id].scbmr,inbyte(sci_cb[sci_id].scbmr)&~RFFT);
    }
    else
    {
        /* set RX threshold for >=1/2 full */
        _set_bits(sci_id, SCBMR, RFFT);
        //outbyte(sci_cb[sci_id].scbmr,inbyte(sci_cb[sci_id].scbmr)|RFFT);
    }
}

/****************************************************************************
** Function:    sci_atom_HW_reset
**
** Purpose:     Initiate a hard ware reset
**
** Parameters:  sci_id: zero-based number to identify smart card controller
****************************************************************************/
void sci_atom_HW_reset(ULONG sci_id, SCI_RESET_TYPE reset_type)
{
    ULONG time0 = 0;
    ULONG time1 = 0;
    ULONG t1    = 0;

    switch(reset_type)
    {
    /* cold reset */
    case COLD:
        if(sci_cb[sci_id].sci_modes.emv2000 == 1)
        {
            _set_bits(sci_id, SCCTL3, SCIEMV);
            t1 = SCI_EMV_T1_MIN;
        }
        else
        {
            _clear_bits(sci_id, SCCTL3, SCIEMV);
            t1 = SCI_ISO_T1_MIN;
        }
        if(sci_cb[sci_id].sci_modes.man_act == 1)
        {
            /* clear AUTO bit (manual) */
            _clear_bits(sci_id, SCCTL3, AUTO);
            /* set SDFLT bit */
            _set_bits(sci_id, SCCTL0, SDFLT);
            /* need 2 OPB cycles of delay */
            time0 = MF_SPR(SPR_TBL);
            /* Enable VCC */
            if(sci_drv_modes.vcc_polarity == ACTIVE_HIGH)
            {
                _set_bits(sci_id, SCCTL0, VCC);
            }
            else
            {
                _clear_bits(sci_id, SCCTL0, VCC);
            }
            /* Enable I/O (IO_EN) */
            _set_bits(sci_id, SCCTL0, IO_EN);
            /* Enable clock (CLKEN) */
            _set_bits(sci_id, SCCTL0, CLKEN);
            /* Clock must be high for > 400 SCCLK before asserting reset */
            time0 = MF_SPR(SPR_TBL);
            do{
                time1 = MF_SPR(SPR_TBL);
                if(time0 < time1)
                {
                    time1 = time1 - time0;
                }
                else{
                    time1 = 0xFFFFFFFF - (time0 - time1);
                }
            }
            while(time1 < (t1 * 2 * _OS_INB(sci_cb[sci_id].scclk_cnt0)));
            /* Assert reset line (SCRST) */
            _set_bits(sci_id, SCCTL0, SCRST);
            /* set AUTO/MAN bit (auto) */
            _set_bits(sci_id, SCCTL3, AUTO);
        }
        else
        {
            /* activate the reader and reset the Smart Card */
            _set_bits(sci_id, SCCTL3, CRESET);
            /* wait for h/w reset cycle to complete */
            while(_check_bits(sci_id, SCCTL3, CRESET) == 1);
        }
        break;

    /* warm reset */
    case WARM:
        _set_bits(sci_id, SCCTL0, RST);
        /* wait for h/w reset cycle to complete */
        while(_check_bits(sci_id, SCCTL0, RST) == 1);
        break;
    }
}

/*****************************************************************************
** Function:    sci_atom_clock_stop
**
** Purpose:     Stop the SCI/Smart Card clock at a given polarity.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_clock_stop(ULONG sci_id)
{
    /* determine and set polarity */
    if(sci_cb[sci_id].sci_parameters.clock_stop_polarity == 
       SCI_CLOCK_STOP_LOW)
    {
        _clear_bits(sci_id, SCCTL3, CLKSTP);
    }
    else if(sci_cb[sci_id].sci_parameters.clock_stop_polarity ==
            SCI_CLOCK_STOP_HIGH)
    {
        _set_bits(sci_id, SCCTL3, CLKSTP);
    }
    /* stop the clock */
    _clear_bits(sci_id, SCCTL0, CLKEN);
}

/*****************************************************************************
** Function:    sci_atom_clock_start
**
** Purpose:     Start the SCI/Smart Card clock.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_clock_start(ULONG sci_id)
{
    _set_bits(sci_id, SCCTL0, CLKEN);
}

/****************************************************************************
** Name:        sci_atom_is_card_present
**
** Purpose:     This checks the state of the detect line and the active
**              polarity of the card detect line, to determine if a card
**              is present in the reader.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     0: card is not present
**              1: card is present
****************************************************************************/
INT sci_atom_is_card_present(ULONG sci_id)
{
    INT rc;

    if(_check_bits(sci_id, SCCTL3, POLDET) == 
       _check_bits(sci_id, SCSTAT, DET))
    {
        /* card is present */
        rc = 1;
    }
    else
    {
        /* card not present */
        rc = 0;
    }

    return(rc);
}

/****************************************************************************
** Name:        sci_atom_is_HW_activated
**
** Purpose:     This determines if the SCI hardware is activated.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     0: hardware is not actived
**              1: hardware is actived
*****************************************************************************/
INT sci_atom_is_HW_activated(ULONG sci_id)
{
    return(_check_bits(sci_id, SCCTL0, IO_EN));
}

/*****************************************************************************
** Function:    sci_atom_deactivate
**
** Purpose:     Deactivate the interface (enter deac state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_deactivate(ULONG sci_id)
{
    /* set the interrupt mask to zero */
    _OS_OUTW(sci_cb[sci_id].scinten, ENCDI);

    /* clear all the interrupts */
    _OS_OUTW(sci_cb[sci_id].scint,
        BOI|REI|TUI|CHI|TSI|RFI|RXLEN|RFTHI|TFHI|RUI|PARI|LTXI|FRXI);

    /* clear the UIC status */
    sci_atom_clear_UICSR(sci_id);

    /* Don't attempt deactivation if a panic deactivation has occoured */
    if((sci_cb[sci_id].state != SCI_STATE_INIT) &&
       (sci_cb[sci_id].atr_status == SCI_ATR_READY))
    {
        /* clock must be running, else NDEACT bit with stay high */
        if(_check_bits(sci_id, SCCTL0, CLKEN) != 1)
        {
            /* we must be in clock stop, so start the clock */
            _set_bits(sci_id, SCCTL0, CLKEN);
        }
        /* initiate normal deactivation */
        _set_bits(sci_id, SCCTL3, NDEACT);
        /* NDEACT is auto cleared by HW, it indicates deactivation finish */
        while(_check_bits(sci_id, SCCTL3, NDEACT) == 1);
    }
}

/*****************************************************************************
** Function:    sci_atom_tx_start
**
** Purpose:     set transmit (tx) registers.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              num_bytes: number of bytes to write from out buffer
*****************************************************************************/
void sci_atom_tx_start(ULONG sci_id, ULONG num_bytes)
{
    /* set FIFO to 32-byte Tx */
    _set_bits  (sci_id, SCCTL1, FIFO1);
    _clear_bits(sci_id, SCCTL1, FIFO2);

    /* reset Tx FIFO */
    _set_bits(sci_id, SCCTL1, TFIFOR);
    /* reset Rx FIFO */
    _set_bits(sci_id, SCCTL1, RFIFOR);

    /* set hardware registers */
    /* set the interrupt mask to zero */
    _OS_OUTW(sci_cb[sci_id].scinten, ENCDI);

    /* clear all the interrupts */
    sci_atom_clear_interrupt_status(sci_id,
        BOI|REI|TUI|CHI|TSI|RFI|RXLEN|RFTHI|TFHI|RUI|PARI|LTXI|FRXI);

    /* clear the UIC status */
    sci_atom_clear_UICSR(sci_id);

    /* set rxlen to max */
    _OS_OUTW(sci_cb[sci_id].scrxlen, 0x1FF);
    _OS_OUTW(sci_cb[sci_id].sctxlen, num_bytes);

    /* enable Tx interrupts */
    //_OS_OUTW(sci_cb[sci_id].scinten, (ENBOI|ENTUI|ENCDI|EPARI|ELTXI));
}

/*****************************************************************************
** Function:    sci_atom_rx_start
**
** Purpose:     set recieve (rx) registers.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
*****************************************************************************/
void sci_atom_rx_start(ULONG sci_id)
{
    /* set FIFO to 32-byte Rx */
    _set_bits  (sci_id, SCCTL1, FIFO2);
    _clear_bits(sci_id, SCCTL1, FIFO1);

    /* set rx threshold for >=1/2 full */
    _OS_OUTB(sci_cb[sci_id].scbmr, _OS_INB(sci_cb[sci_id].scbmr) | RFFT);

    /* reset Tx FIFO */
    _set_bits(sci_id, SCCTL1, TFIFOR);

    /* reset Rx FIFO */
    _set_bits(sci_id, SCCTL1, RFIFOR);

    /* set rxlen to max */
    _OS_OUTW(sci_cb[sci_id].scrxlen, 0x1FF);

    /* set the interrupt mask to zero */
    _OS_OUTW(sci_cb[sci_id].scinten, ENCDI);

    /* clear all the interrupts */
    sci_atom_clear_interrupt_status(sci_id,
         BOI|REI|TUI|CHI|TSI|RFI|RXLEN|RFTHI|TFHI|RUI|PARI|LTXI|FRXI);

    /* enable Rx interrupts */
    _OS_OUTW(sci_cb[sci_id].scinten,
         ENBOI|ENREI|ENTSI|ENRFI|ENCDI|ERFTHI|ERUI|EPARI|EFRXI);
}

/*****************************************************************************
** Function:    sci_atom_write_fifo
**
** Purpose:     Transmit certain numbers of bytes to the FIFO
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              num_bytes: number of bytes to be witten
**              p_buffer_addr: intput pointer of the write buffer's address
*****************************************************************************/
void sci_atom_write_fifo(ULONG sci_id, ULONG num_bytes, UCHAR **p_buffer_addr)
{
    UCHAR *p_buffer;
    int num_bytes_left;

    num_bytes_left = (int)num_bytes;

    p_buffer = *p_buffer_addr;

    if((_OS_INB(sci_cb[sci_id].scctl1) & FIFO0) == FIFO0)
    {
        /* word FIFO access */
        while((num_bytes_left> 0) &&
              (_check_bits(sci_id, SCCTL3, POLDET) ==
               _check_bits(sci_id, SCSTAT, DET)))
        {
            _OS_OUTL(sci_cb[sci_id].scbuffs, *(ULONG *)p_buffer);

	    p_buffer += 4;
            if(num_bytes_left >= 4)
            {
                num_bytes_left -= 4;
            }
            else
            {
                num_bytes_left = 0;
            }
        }
    }
    else
    {
        /* byte FIFO access */
        while((num_bytes_left > 0) &&
              (_check_bits(sci_id, SCCTL3, POLDET) ==
               _check_bits(sci_id, SCSTAT, DET)))
        {
            _OS_OUTB(sci_cb[sci_id].scbuffs, *p_buffer);
            p_buffer ++;
            num_bytes_left --;
        }
    }

    *p_buffer_addr = p_buffer;
}

/****************************************************************************
** Name:        sci_atom_read_fifo
**
** Purpose:     Read any pending data from FIFO
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_buffer_addr: intput pointer of the read buffer's address
*****************************************************************************/
void sci_atom_read_fifo(ULONG sci_id, UCHAR **p_buffer_addr)
{
    UCHAR validb;
    UCHAR *p_buffer;

    p_buffer = *p_buffer_addr;

    if((_OS_INB(sci_cb[sci_id].scctl1) & FIFO0) == FIFO0)
    {
        /* word FIFO access */
        /* only attempt read if data is available */
        while(((_OS_INB(sci_cb[sci_id].scstat) & RSTAT) != 0) &&
              (_check_bits(sci_id, SCCTL3, POLDET) ==
               _check_bits(sci_id, SCSTAT, DET)))
        {
            /* check for valid data on the OPB wall */
            if((_OS_INB(sci_cb[sci_id].scstat) & RPEND) == 0)
            {
                /* data must be flushed to OPB wall before reading */
                _OS_OUTB(sci_cb[sci_id].scctl1,
                    (_OS_INB(sci_cb[sci_id].scctl1) | FLUSH));
                /* wait for flush to complete - 1 OPB clock cycle */
                while(((_OS_INB(sci_cb[sci_id].scctl1) & 0x02) == 0x02) &&
                      (_check_bits(sci_id, SCCTL3, POLDET) ==
                       _check_bits(sci_id, SCSTAT, DET)));
            }
            /* read 1 word from the Rx FIFO */
            *(ULONG *)p_buffer = _OS_INL(sci_cb[sci_id].scbuffs);
            /* determine the number of valid bytes in the word */
            validb = ((_OS_INB(sci_cb[sci_id].scstat) & VALIDB) >> 6);
            if(validb == 0)
            {
                validb = 4;
            }
            p_buffer += validb;
            if(sci_cb[sci_id].bytes_expected != -1)
            {
                if((sci_cb[sci_id].bytes_expected - validb) >= 0)
                {
                    sci_cb[sci_id].bytes_expected -= validb;
                }
                else
                {
                    sci_cb[sci_id].bytes_expected = 0;
                }
                if(sci_cb[sci_id].bytes_expected < 16)
                {
                    /* set RX threshold for not empty */
                    _clear_bits(sci_id, SCBMR, RFFT);
                    //outbyte(sci_cb[sci_id].scbmr,inbyte(sci_cb[sci_id].scbmr)&~RFFT);
                }
            }
        }
    }
    else
    {
        /* byte FIFO access */
        while((_check_bits(sci_id, SCSTAT, RPEND) == 1) &&
              (_check_bits(sci_id, SCCTL3, POLDET) ==
               _check_bits(sci_id, SCSTAT, DET)))
        {
            *p_buffer = _OS_INB(sci_cb[sci_id].scbuffs);
            p_buffer ++;
            if(sci_cb[sci_id].bytes_expected != -1)
            {
                if(sci_cb[sci_id].bytes_expected > 0)
                {
                    sci_cb[sci_id].bytes_expected --;
                }
                if(sci_cb[sci_id].bytes_expected < 16)
                {
                    /* set RX threshold for not empty */
                    _clear_bits(sci_id, SCBMR, RFFT);
                    //outbyte(sci_cb[sci_id].scbmr,inbyte(sci_cb[sci_id].scbmr)&~RFFT);
                }
            }
        }
    }

    *p_buffer_addr = p_buffer;
}

/*****************************************************************************
** Function:    sci_atom_set_para_T
**
** Purpose:     Set the current Smart Card parameters of T.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_T(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    sci_cb[sci_id].sci_parameters.T = p_sci_parameters->T;

    if(p_sci_parameters->T == 0)
    {
        _clear_bits(sci_id, SCCTL1, PRTCL);
        /* byte wide FIFO */
        _clear_bits(sci_id, SCCTL1, FIFO0);
        /* Rx FIFO threshold not empty */
        _clear_bits(sci_id, SCBMR, RFFT);
    }
    else
    {
        _set_bits(sci_id, SCCTL1, PRTCL);
        /* word wide FIFO */
        _set_bits(sci_id, SCCTL1, FIFO0);
        /* Rx FIFO threshold >= half full */
        _set_bits(sci_id, SCBMR, RFFT);
    }
}

/*****************************************************************************
** Function:    sci_atom_set_para_f
**
** Purpose:     Set the current Smart Card parameters of frequency.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_f(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    ULONG divisor, frequency;

    /* set scclk to nearest possible frequency */
    divisor = 0;
    do
    {
        divisor++;
        frequency = __STB_SYS_CLK / (2 * divisor);
    }
    while(frequency > p_sci_parameters->f);

    /* ensure this bit is not set before updating */
    while(_check_bits(sci_id, SCETU0, CNT0BUSY) == 1);

    sci_cb[sci_id].sci_parameters.f = frequency;
    _OS_OUTB(sci_cb[sci_id].scclk_cnt0, divisor);
}

/*****************************************************************************
** Function:    sci_atom_set_para_ETU
**
** Purpose:     Set the current Smart Card parameters of ETU.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_ETU(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    sci_cb[sci_id].sci_parameters.ETU = p_sci_parameters->ETU;
    _OS_OUTW(sci_cb[sci_id].scetu, p_sci_parameters->ETU);
}

/*****************************************************************************
** Function:    sci_atom_set_para_WWT
**
** Purpose:     Set the current Smart Card parameters of WWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_WWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    sci_cb[sci_id].sci_parameters.WWT = p_sci_parameters->WWT;
    if(p_sci_parameters->T==0)
    {
        _OS_OUTL(sci_cb[sci_id].scbwt, p_sci_parameters->WWT);
    }
}

/*****************************************************************************
** Function:    sci_atom_set_para_CWT
**
** Purpose:     Set the current Smart Card parameters of CWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_CWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    sci_cb[sci_id].sci_parameters.CWT = p_sci_parameters->CWT;
    _OS_OUTW(sci_cb[sci_id].sccwt, p_sci_parameters->CWT);
}

/*****************************************************************************
** Function:    sci_atom_set_para_BWT
**
** Purpose:     Set the current Smart Card parameters of BWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_BWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    sci_cb[sci_id].sci_parameters.BWT = p_sci_parameters->BWT;
    if(p_sci_parameters->T==1)
    {
        _OS_OUTL(sci_cb[sci_id].scbwt, p_sci_parameters->BWT);
    }
}

/*****************************************************************************
** Function:    sci_atom_set_para_EGT
**
** Purpose:     Set the current Smart Card parameters of EGT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_EGT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    sci_cb[sci_id].sci_parameters.EGT = p_sci_parameters->EGT;
    _OS_OUTB(sci_cb[sci_id].scegt, p_sci_parameters->EGT);
}

/*****************************************************************************
** Function:    sci_atom_set_para_check
**
** Purpose:     Set the current Smart Card parameters of check.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_check(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    sci_cb[sci_id].sci_parameters.check = p_sci_parameters->check;
    if(p_sci_parameters->check == 1)
    {
        _clear_bits(sci_id, SCCTL1, ERRDET);
    }
    else
    {
        _set_bits(sci_id, SCCTL1, ERRDET);
    }
}

/****************************************************************************
** Name:        sci_atom_emv_check
**
** Purpose:     check if this chip support EMV mode
**
** Returns      0: if NO EMV mode supported
**              1: if EMV mode support
****************************************************************************/
INT sci_atom_emv_check(void)
{
    INT rc = 0;

#if defined (__DRV_FOR_PALLAS__)
    /* STB04xxx pass-1 has no EMV support */
    if(MF_SPR(SPR_PVR) != PPC_PALLAS_PBA)
    {
        rc = 1;
    }
#elif defined (__DRV_FOR_VULCAN__)
    rc = 1;
#elif defined (__DRV_FOR_VESTA__)
#endif

    return(rc);
}

/*****************************************************************************
** Function:    sci_atom_set_mode_emv2000
**
** Purpose:     Set the current Smart Card mode of emv2000.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_modes: input pointer to Smart Card modes
*****************************************************************************/
void sci_atom_set_mode_emv2000(ULONG sci_id, SCI_MODES *p_sci_modes)
{
    sci_cb[sci_id].sci_modes.emv2000 = p_sci_modes->emv2000;
    
    if(p_sci_modes->emv2000 == 1)
    {
        _set_bits(sci_id, SCCTL3, SCIEMV);
    }
    else
    {
        _clear_bits(sci_id, SCCTL3, SCIEMV);
    }
}

/****************************************************************************
** Name:        _set_bits
**
** Purpose:     set bits in a register
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              reg:    register offset from base address
**              bits:   bit mask
****************************************************************************/
static void _set_bits (ULONG sci_id, ULONG reg, UCHAR bits)
{
    ULONG *reg_address;

    reg_address = (ULONG *)(sci_cb[sci_id].sci_base_address + reg);
    _OS_OUTB(reg_address, _OS_INB(reg_address) | bits);
}

/****************************************************************************
** Name:        _clear_bits
**
** Purpose:     clear bits in a register
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              reg:    register offset from base address
**              bits:   bit mask
****************************************************************************/
static void _clear_bits (ULONG sci_id, ULONG reg, UCHAR bits)
{
    ULONG *reg_address;

    reg_address = (ULONG *)(sci_cb[sci_id].sci_base_address + reg);
    _OS_OUTB(reg_address, _OS_INB(reg_address) & ~bits);
}

/****************************************************************************
** Name:        _check_bits
**
** Purpose:     check bits in a register
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              reg:    register offset from base address
**              bits:   bit mask
**
** Returns      0: if any bits in the bit mask are cleared
**              1: if all bits in the bit mask are set
****************************************************************************/
static INT _check_bits (ULONG sci_id, ULONG reg, UCHAR bits)
{
    ULONG *reg_address;
    INT rc = 0;

    reg_address = (ULONG *)(sci_cb[sci_id].sci_base_address + reg);
    if((_OS_INB(reg_address) & bits) == bits)
	{
        rc = 1;
	}

    return(rc);
}

