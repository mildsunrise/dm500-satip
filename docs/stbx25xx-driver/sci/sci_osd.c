/*-----------------------------------------------------------------------------+
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
+-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------+
| Author:    Mike Lepore, Zongwei Liu
| Component: sci
| File:      sci_osd.c
| Purpose:   The OS-dependency functions of Smart Card Interface
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 03/22/2001  MAL               Initial check-in.
| 03/26/2001  Zongwei Liu       Port to Linux
| 09/26/2001  Zongwei Liu       Port to pallas
| 10/10/2001  Zongwei Liu       Port to OS-Adaption layer
| 12/13/2001  MAL, Zongwei Liu  Added sci_osd_init() parameters to set detect 
|                               and Vcc enable active polarities 
|                               (which are board dependent).
| 12/13/2001  MAL, Zongwei Liu  Added EMV2000 support and made several changes 
|                               to improve PIO efficiency.
| 01/11/2002  MAL, zongwei Liu  Add timeout to read/write function
+----------------------------------------------------------------------------*/

#include "os/os-sync.h"
#include "os/os-interrupt.h"
#include "os/os-types.h"
#include "os/drv_debug.h"

#include <asm/io.h>
#include <linux/sched.h>

#include "sci_osd.h"
#include "sci_atom.h"
#include "sci_osi.h"

extern SCI_CONTROL_BLOCK sci_cb[SCI_NUMBER_OF_CONTROLLERS];

ULONG sci_driver_init;
SCI_DRV_MODES sci_drv_modes;

static DECLARE_WAIT_QUEUE_HEAD(_SCI_WAIT_Q_0);
static DECLARE_WAIT_QUEUE_HEAD(_SCI_WAIT_Q_1);
static wait_queue_head_t *sci_wait_q[2];

static void sci_osd_irq_handler(UINT uIrq, void *v);

/****************************************************************************
** Function:    sci_osd_init
**
** Purpose:     Initialize the Smart Card interface controller(s) and driver
**                  and enter deac state.
**
** Parameters:  detect_polarity: active polarity of the card detect signal
**               0: active low  - detect line is low  when card is inserted
**               1: active high - detect line is high when card is inserted
**              vcc_polarity: active polarity of the Vcc enable signal
**               0: active low  - Vcc enable is  low to apply power to card
**               1: active high - Vcc enable is high to apply power to card
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_KERNEL_FAIL: if interrupt handler install fails or
**                  reader demon task creation fails
****************************************************************************/
SCI_ERROR sci_osd_init(ULONG detect_polarity, ULONG vcc_polarity)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG sci_id;

    ULONG sci_interrupt_masks[] = 
    {
        SCI_INTERRUPT_MASK_0,
        SCI_INTERRUPT_MASK_1
    };
    ULONG sci_interrupt_levels[] = 
    {
        SCI_INTERRUPT_LEVEL_0,
        SCI_INTERRUPT_LEVEL_1
    };
    ULONG sci_base_addresses[] = 
    {
        SCI_BASE_ADDRESS_0,
        SCI_BASE_ADDRESS_1
    };

    PDEBUG("enter\n");

    if(((detect_polarity != ACTIVE_LOW) && (detect_polarity != ACTIVE_HIGH)) ||
       ((vcc_polarity != ACTIVE_LOW) && (vcc_polarity != ACTIVE_HIGH)))
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

    if((sci_driver_init == 0) && (rc == SCI_ERROR_OK))
    {
        /* configure the chip internal connection register */
        sci_atom_set_chip_connection();

        /* initialize globals driver mode */
        sci_drv_modes.detect_polarity = detect_polarity;
        sci_drv_modes.vcc_polarity    = vcc_polarity;
        sci_drv_modes.emv_supported   = sci_atom_emv_check();

        /* initialize the wait queue */
        sci_wait_q[0] = &(_SCI_WAIT_Q_0);
        sci_wait_q[1] = &(_SCI_WAIT_Q_1);

        /* initialize global device structure for each smart card controller */
        for(sci_id=0; sci_id<SCI_NUMBER_OF_CONTROLLERS; sci_id++)
        {
            /* initialize control block values */
            sci_cb[sci_id].sci_interrupt_mask  = sci_interrupt_masks [sci_id];
            sci_cb[sci_id].sci_interrupt_level = sci_interrupt_levels[sci_id];
            sci_cb[sci_id].sci_base_address    = 
                     (ULONG) ioremap(sci_base_addresses[sci_id], SCI_IO_SIZE);

            /* assign the register addresses */
            sci_atom_assign_reg_address(sci_id);

            sci_cb[sci_id].state             = SCI_STATE_INIT;
            sci_cb[sci_id].driver_inuse      = 0;
            sci_cb[sci_id].atr_status        = SCI_WITHOUT_ATR;
            sci_cb[sci_id].waiting           = 0;
            sci_cb[sci_id].sci_modes.emv2000 = 0;
            sci_cb[sci_id].sci_modes.dma     = 0;
            sci_cb[sci_id].sci_modes.man_act = 0;
            //sci_cb[sci_id].sci_modes.rw_mode = SCI_SYNC | SCI_DATA_ANY;
            sci_cb[sci_id].sci_modes.rw_mode = SCI_SYNC;
            sci_cb[sci_id].bytes_expected    = -1;

            /* initialize registers */
            sci_atom_reg_init(sci_id);

            if(os_install_irq(sci_cb[sci_id].sci_interrupt_level,
                              IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG,
                              sci_osd_irq_handler,
                              NULL) == 0)
            {
                sci_driver_init = 1;
                rc = sci_osd_deactivate(sci_id);
            }
            else
            {
                PDEBUG("unable to request interrupt %d\n",
                       (UINT) sci_cb[sci_id].sci_interrupt_level);
                rc = SCI_ERROR_KERNEL_FAIL;
            }
        }
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("error\n");
    }

    PDEBUG("exit\n");
    return(rc);
}

/****************************************************************************
** Function:    scd_osd_uninit
**
** Purpose:     Uninitialize the Smart Card interface controllers and driver
**                  and enter deac state.
****************************************************************************/
void sci_osd_uninit (void)
{
    ULONG sci_id;

    PDEBUG("enter\n");

    for (sci_id = 0; sci_id < SCI_NUMBER_OF_CONTROLLERS; sci_id++)
    {
        sci_osd_deactivate(sci_id);
        /* free the IRQ */
        os_disable_irq(sci_cb[sci_id].sci_interrupt_level);
        os_uninstall_irq(sci_cb[sci_id].sci_interrupt_level);
        /* unmap IO */
        iounmap((void *) sci_cb[sci_id].sci_base_address);
    }

    PDEBUG("exit\n");
}

/****************************************************************************
** Function:    sci_osd_irq_handler
**
** Purpose:     Smart Card Interface interrupt handler
****************************************************************************/
static void sci_osd_irq_handler (UINT uIrq, void *v)
{
    ULONG state  = 0;
    ULONG sci_id;
    USHORT isr;
    INT bytes_left;

    if(uIrq==sci_cb[0].sci_interrupt_level)
    {
        sci_id = 0;
    }
    else if(uIrq==sci_cb[1].sci_interrupt_level)
    {
        sci_id = 1;
    }
    else
    {
        sci_id = 2;
    }

    if(sci_id < 2)
    {
        state = os_enter_critical_section();
        isr = sci_atom_get_interrupt_status(sci_id);

        if(isr != 0)
        {
            /* check for card being inserted or removed */
            if((isr & CDI) == CDI)
            {
                if(sci_cb[sci_id].waiting == 1)
                {
                    sci_cb[sci_id].waiting = 0;
                    wake_up_interruptible(sci_wait_q[sci_id]);
                    //os_release_mutex(sci_cb[sci_id].mutex);
                    sci_cb[sci_id].rx_complete = 1;
                }
                sci_cb[sci_id].atr_status = SCI_WITHOUT_ATR;
                sci_cb[sci_id].state = SCI_STATE_DEAC;

                sci_atom_clear_interrupt_status(sci_id, isr);
            }
            else
            {
                switch(sci_cb[sci_id].state)
                {
                case SCI_STATE_RX:
                    if((isr & TSI) == TSI)
                    {
                        sci_cb[sci_id].error = SCI_ERROR_TS_CHARACTER_INVALID;
                        sci_cb[sci_id].rx_complete = 1;
                        sci_atom_clear_interrupt_status(sci_id, TSI);
                    }
                    if((isr & RFI) == RFI)
                    {
                        if(sci_cb[sci_id].first_rx == 1)
                        {
                            /* we have received some data */
                            sci_cb[sci_id].error = SCI_ERROR_AWT_TIMEOUT;
                        }
                        else
                        {
                            /* no data received */
                            sci_cb[sci_id].error = SCI_ERROR_NO_ATR;
                        }
                        sci_cb[sci_id].rx_complete = 1;
                        sci_atom_clear_interrupt_status(sci_id, RFI);
                    }
                    if((isr & FRXI) == FRXI)
                    {
                        sci_cb[sci_id].first_rx = 1;
                        sci_atom_clear_interrupt_status(sci_id, FRXI);
                    }
                    if((isr & BOI) == BOI)
                    {
                        sci_cb[sci_id].error = SCI_ERROR_RX_OVERFLOW_FAIL;
                        sci_cb[sci_id].rx_complete = 1;
                        sci_atom_clear_interrupt_status(sci_id, BOI);
                    }
                    if((isr & PARI) == PARI)
                    {
                        sci_cb[sci_id].error = SCI_ERROR_PARITY_FAIL;
                        sci_cb[sci_id].rx_complete = 1;
                        sci_atom_clear_interrupt_status(sci_id, PARI);
                    }
                    if(((isr & RFTHI) == RFTHI) || ((isr & REI) == REI))
                    {
                        /* in either case, read any pending data            */
                        /* reset pointers to start of buffer if equal       */
                        /* this will prevent buffer overruns for extensive  */
                        /* waiting time extensions in T=0                   */
                        if(sci_cb[sci_id].p_read == sci_cb[sci_id].p_write)
                        {
                            sci_cb[sci_id].p_read  = sci_cb[sci_id].buffer;
                            sci_cb[sci_id].p_write = sci_cb[sci_id].buffer;
                        }
                        /* read any pending data into private buffer */
                        sci_atom_read_fifo(sci_id, &sci_cb[sci_id].p_write);
                    }
                    if((isr & RFTHI) == RFTHI)
                    {
                        if((isr & REI) != REI)
                        {
                            /* REI will wakeup read, if it is present */
                            if(sci_cb[sci_id].waiting == 1)
                            {
                                if((sci_cb[sci_id].read_flags & SCI_DATA_ANY) == SCI_DATA_ANY)
                                {
                                    sci_cb[sci_id].waiting = 0;
                                    wake_up_interruptible(sci_wait_q[sci_id]);
                                }
                                else if(sci_cb[sci_id].bytes_expected == 0)
                                {
                                    sci_cb[sci_id].bytes_expected = -1;
                                    sci_cb[sci_id].waiting = 0;
                                    wake_up_interruptible(sci_wait_q[sci_id]);
                                }
                            }
                        }
                        sci_atom_clear_interrupt_status(sci_id, RFTHI);
                    }
                    if((isr & REI) == REI)
                    {
                        if(sci_cb[sci_id].rx_complete == 0)
                        {
                            if(sci_cb[sci_id].sci_parameters.T == 0)
                            {
                                sci_cb[sci_id].error = SCI_ERROR_WWT_TIMEOUT;
                            }
                            else
                            {
                                /* T=1 */
                                if((isr & RXLEN) == RXLEN)
                                {
                                    if(sci_cb[sci_id].first_rx == 1)
                                    {
                                        /* we have received some data */
                                        sci_cb[sci_id].error = SCI_ERROR_CWT_TIMEOUT;
                                    }
                                    else
                                    {
                                        /* no data received since Tx */
                                        sci_cb[sci_id].error = SCI_ERROR_BWT_TIMEOUT;
                                    }
                                }
                                else if((isr & CHI) == CHI)
                                {
                                    /* checks error */
                                    if(sci_cb[sci_id].sci_parameters.check == 1)
                                    {
                                        sci_cb[sci_id].error = SCI_ERROR_LRC_FAIL;
                                    }
                                    else
                                    {
                                        sci_cb[sci_id].error = SCI_ERROR_CRC_FAIL;
                                    }
                                }
                            }
                            sci_cb[sci_id].rx_complete = 1;
                        }
                        sci_atom_clear_interrupt_status(sci_id, REI|RXLEN);
                    }
                    if(sci_cb[sci_id].rx_complete == 1)
                    {
                        if(sci_cb[sci_id].waiting == 1)
                        {
                            sci_cb[sci_id].waiting = 0;
                            wake_up_interruptible(sci_wait_q[sci_id]);
                        }
                    }
                    break;

                case SCI_STATE_TX:
                    if((isr & TFHI) == TFHI)
                    {
                        /* ensure fifo stat indicates fifo has dropped below half full */
                        if(sci_atom_check_tx_fifo_half_full(sci_id) == 0)
                        {
                            bytes_left = (sci_cb[sci_id].p_write - sci_cb[sci_id].p_read);
                            /* the transmitter is less than 1/2 full- it can accept 16 bytes */
                            if(bytes_left > 16)
                            {
                                /* more than 16 bytes left to transmit */
                                sci_atom_write_fifo(sci_id, 16, &sci_cb[sci_id].p_read);
                            }
                            else
                            {
                                /* transmit rest of the bytes */
                                sci_atom_write_fifo(sci_id, bytes_left, &sci_cb[sci_id].p_read);
                                sci_atom_disable_tx_threshold_interrupts(sci_id);
                            }
                        }
                        sci_atom_clear_interrupt_status(sci_id, TFHI);
                    }
                    if((isr & PARI) == PARI)
                    {
                        sci_cb[sci_id].error = SCI_ERROR_PARITY_FAIL;
                        sci_atom_clear_interrupt_status(sci_id, PARI);
                    }
                    if((isr & BOI) == BOI)
                    {
                        sci_cb[sci_id].error = SCI_ERROR_TX_OVERFLOW_FAIL;
                        sci_atom_clear_interrupt_status(sci_id, BOI);
                    }
                    if((isr & TUI) == TUI)
                    {
#ifdef ERROR_ON_TUI
                        sci_cb[sci_id].error = SCI_ERROR_TX_UNDERRUN_FAIL;
#endif
                        sci_atom_clear_interrupt_status(sci_id, TUI);
                    }
                    if((sci_cb[sci_id].error != SCI_ERROR_OK) ||
                      ((isr & LTXI) == LTXI))
                    {
                        if(sci_cb[sci_id].waiting == 1)
                        {
                            sci_cb[sci_id].waiting = 0;
                            wake_up_interruptible(sci_wait_q[sci_id]);
                        }
                        /* start rx state */
                        sci_osi_rx_start(sci_id);
                    }
                    break;
                default:
                    break;
                }
            }
        }
        sci_atom_clear_UICSR(sci_id);
        os_leave_critical_section(state);
    }
}

/*****************************************************************************
** Function:    sci_osd_write
**
** Purpose:     Write data to the Smart Card (enter tx state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_buffer: input pointer to write buffer
**              num_bytes: number of bytes to write from p_buffer
**              mode_flags: flags to indicate behavior of write
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_buffer is zero or
**                  num_bytes is zero
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
**              SCI_ERROR_TX_PENDING: if a transmission is already pending
*****************************************************************************/
SCI_ERROR sci_osd_write (ULONG sci_id,
                         UCHAR *p_buffer,
                         ULONG num_bytes,
                         ULONG mode_flags
)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if (sci_driver_init != 1)
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }
    else if (sci_osi_is_card_activated(sci_id) != 1)
    {
        rc = SCI_ERROR_CARD_NOT_ACTIVATED;
    }
    else if (sci_cb[sci_id].atr_status != SCI_ATR_READY)
    {
        rc = SCI_ERROR_NO_ATR;
    }
    else if(sci_cb[sci_id].state == SCI_STATE_TX)
    {
        rc = SCI_ERROR_TX_PENDING;
    }
    else if((num_bytes == 0) || 
            (p_buffer == 0)  ||
            (sci_id >= SCI_NUMBER_OF_CONTROLLERS))
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    else
    {
        sci_osi_tx_start(sci_id, num_bytes);

        if(num_bytes > 32)
        {
            /* write up to 32 bytes directly to the FIFO */
            sci_atom_write_fifo(sci_id, 32, &p_buffer);
            num_bytes -= 32;
        }
        else
        {
            sci_atom_write_fifo(sci_id, num_bytes, &p_buffer);
            num_bytes = 0;
        }

        /* enable Tx interrupts */
        sci_atom_enable_tx_interrupts(sci_id);

        k_state = os_enter_critical_section();

        if((mode_flags & SCI_SYNC) == SCI_SYNC)
        {
            if(num_bytes > 0)
            {
                /* copy buffer pointers and transmit from user buffer */
                sci_cb[sci_id].p_read  = p_buffer;
                sci_cb[sci_id].p_write = (p_buffer + num_bytes);
                /* enable Tx threshold interrupts */
                sci_atom_enable_tx_threshold_interrupts(sci_id);
            }
            /* block here- syncronous mode */
            if(sci_cb[sci_id].state == SCI_STATE_TX)
            {
                sci_cb[sci_id].waiting = 1;
                if(interruptible_sleep_on_timeout(sci_wait_q[sci_id],
                                                  SCI_TIMEOUT_MS / 10) == 0)
                {
                    if(sci_cb[sci_id].waiting == 0)
                    {
                        /* suspend timed-out, but resume was called- OS problem */
                        sci_cb[sci_id].error = SCI_ERROR_KERNEL_FAIL;
                    }
                    else
                    {
                        /* suspend timed-out, resume was not called- SCI problem */
                        sci_cb[sci_id].error = SCI_ERROR_TRANSACTION_ABORTED;
                    }
                }
                rc = sci_cb[sci_id].error;
                sci_cb[sci_id].error = SCI_ERROR_OK;
            }
        }
        else if(num_bytes > 0)
        {
            /* copy data into private buffer */
            memcpy((void *)sci_cb[sci_id].buffer,
                   (const void *)p_buffer,
                   num_bytes);
            sci_cb[sci_id].p_read  = sci_cb[sci_id].buffer;
            sci_cb[sci_id].p_write = (sci_cb[sci_id].buffer + num_bytes);
            /* enable Tx threshold interrupts */
            sci_atom_enable_tx_threshold_interrupts(sci_id);
        }
        os_leave_critical_section(k_state);
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/*****************************************************************************
** Function:    sci_osd_read
**
** Purpose:     Read data from the Smart Card.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_buffer: input pointer to read buffer
**              num_bytes: number of bytes to read into p_buffer
**              p_bytes_read: number of bytes actually read into p_buffer
**              mode_flags: flags to indicate behavior of read
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_buffer is zero or
**                  num_bytes is zero or
**                  p_bytes_read is zero
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osd_read (ULONG sci_id,
                        UCHAR *p_buffer,
                        ULONG num_bytes,
                        ULONG *p_bytes_read,
                        ULONG flags
)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;
    ULONG sci_bytes;
    INT block = 0;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if (sci_driver_init != 1)
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }
    else if (sci_osi_is_card_activated(sci_id) != 1)
    {
        rc = SCI_ERROR_CARD_NOT_ACTIVATED;
    }
    else if((sci_id >= SCI_NUMBER_OF_CONTROLLERS) || 
            (p_buffer  == 0) ||
            (num_bytes == 0) ||
            (p_bytes_read == 0))
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    else
    {
        sci_cb[sci_id].read_flags = flags;
        //block = 0;
        (*p_bytes_read) = 0;

        if((flags & SCI_SYNC) == SCI_SYNC)
        {
            sci_atom_set_rx_threshold_interrupts(sci_id, num_bytes);
            /* wait for TX to complete */
            if(sci_cb[sci_id].state == SCI_STATE_TX)
            {
                block = 1;
            }
            else
            {
                /* we want any amount of data */
                if((flags & SCI_DATA_ANY) == SCI_DATA_ANY)
                {
                    if((sci_cb[sci_id].p_read == sci_cb[sci_id].p_write))
                    {
                        /* but no data has been received, so block */
                        block = 1;
                    }
                }
                else
                {
                    if((sci_cb[sci_id].rx_complete != 1) &&
                      ((sci_cb[sci_id].p_write - sci_cb[sci_id].p_read) < num_bytes))
                    {
                        /* we want all the data and RX is not complete, so block */
                        sci_cb[sci_id].bytes_expected = (INT) num_bytes;
                        block = 1;
                    }
                }
            }

            if(block == 1)
            {
                k_state = os_enter_critical_section();
                if((sci_cb[sci_id].rx_complete == 0) &&
                   (sci_osi_is_card_activated(sci_id) == 1))
                {
                    sci_cb[sci_id].waiting = 1;
                    if(interruptible_sleep_on_timeout(sci_wait_q[sci_id],
                                                      SCI_TIMEOUT_MS/10) == 0)
                    {
                        if(sci_cb[sci_id].waiting == 0)
                        {
                            /* suspend timed-out, but resume was called- OS problem */
                            sci_cb[sci_id].error = SCI_ERROR_KERNEL_FAIL;
                        }
                        else
                        {
                            /* suspend timed-out, resume was not called- SCI or transmission problem */
                            sci_cb[sci_id].error = SCI_ERROR_TRANSACTION_ABORTED;
                        }
                    }
                }
                os_leave_critical_section(k_state);
            }
        }

        k_state = os_enter_critical_section();
        /* copy any available data from private buffer */
        if(sci_cb[sci_id].p_read < sci_cb[sci_id].p_write)
        {
            if(num_bytes > 0)
            {
                sci_bytes = (sci_cb[sci_id].p_write - sci_cb[sci_id].p_read);
                if(sci_bytes > num_bytes)
                {
                    sci_bytes = num_bytes;
                }
                memcpy((void *)p_buffer,
                       (const char *)sci_cb[sci_id].p_read,
                       sci_bytes);
                sci_cb[sci_id].p_read += sci_bytes;
                (*p_bytes_read) += sci_bytes;
            }
        }
        rc = sci_cb[sci_id].error;
        sci_cb[sci_id].error = SCI_ERROR_OK;
        os_leave_critical_section(k_state);
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }

    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/****************************************************************************
** Function:    sci_osd_reset
**
** Purpose:     Initiate a reset (enter atr state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE:  if sci_id is invalid
**              SCI_ERROR_CARD_NOT_PRESENT: if no Smart Card is
**                  present in the reader
**              SCI_ERROR_ATR_PENDING: if a reset has already been initiated
**                  and an ATR is still pending
****************************************************************************/
SCI_ERROR sci_osd_reset(ULONG sci_id)
{
    SCI_PARAMETERS sci_parameters;
    SCI_RESET_TYPE reset_type = COLD;
    SCI_ERROR rc = SCI_ERROR_OK;
    ULONG k_state;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if(sci_id < SCI_NUMBER_OF_CONTROLLERS)
        {
            if(sci_atom_is_card_present(sci_id) == 1)
            {
                k_state = os_enter_critical_section();
                /* initialize control block values */
                if(sci_cb[sci_id].state == SCI_STATE_DEAC)
                {
                    reset_type = COLD;
                }
                else
                {
                    reset_type = WARM;
                    if(sci_cb[sci_id].waiting == 1)
                    {
                        sci_cb[sci_id].waiting = 0;
                        wake_up_interruptible(sci_wait_q[sci_id]);
                        sci_cb[sci_id].rx_complete = 1;
                    }
                }
                sci_cb[sci_id].error = SCI_ERROR_OK;
                /* reset pointers to start of buffer */
                sci_cb[sci_id].p_read  = sci_cb[sci_id].buffer;
                sci_cb[sci_id].p_write = sci_cb[sci_id].buffer;

                /* set default parameter values based on mode */
                sci_parameters.T   = SCI_ATR_T;
                sci_parameters.f   = SCI_ATR_F;
                sci_parameters.ETU = SCI_ATR_ETU;
                sci_parameters.WWT = SCI_ATR_WWT;
                if(sci_cb[sci_id].sci_modes.emv2000 == 1)
                {
                   sci_parameters.CWT = SCI_ATR_AWT;
                }
                else
                {
                   sci_parameters.CWT = SCI_ATR_CWT;
                }
                sci_parameters.BWT = SCI_ATR_BWT;
                sci_parameters.EGT = SCI_ATR_EGT;
                sci_parameters.clock_stop_polarity 
                                   = SCI_CLOCK_STOP_DISABLED;
                sci_parameters.check = SCI_ATR_CHECK;
                sci_parameters.P = SCI_ATR_P;
                sci_parameters.I = SCI_ATR_I;
                sci_parameters.U = SCI_ATR_U;
                rc = sci_osi_set_para(sci_id, &sci_parameters);

                /* start atr or rx state */
                sci_osi_rx_start(sci_id);
                sci_atom_clear_UICSR(sci_id);
                os_leave_critical_section(k_state);

                sci_atom_HW_reset(sci_id, reset_type);
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
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/*****************************************************************************
** Function:    sci_osd_deactivate
**
** Purpose:     Initiate a deactivation (enter deac state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osd_deactivate(ULONG sci_id)
{
    ULONG k_state;
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if(sci_id < SCI_NUMBER_OF_CONTROLLERS)
        {
            if(sci_cb[sci_id].state != SCI_STATE_DEAC)
            {
                k_state = os_enter_critical_section();
                /* abort any current transactions                         */
                /* assign abort error code if one is not already assigned */
                if(sci_cb[sci_id].waiting == 1)
                {
                    sci_cb[sci_id].waiting = 0;
                    wake_up_interruptible(sci_wait_q[sci_id]);
                    //os_release_mutex(sci_cb[sci_id].mutex);
                    sci_cb[sci_id].error = SCI_ERROR_TRANSACTION_ABORTED;
                    sci_cb[sci_id].rx_complete = 1;
                }
                sci_atom_deactivate(sci_id);
                sci_cb[sci_id].state = SCI_STATE_DEAC;
                os_leave_critical_section(k_state);
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}
