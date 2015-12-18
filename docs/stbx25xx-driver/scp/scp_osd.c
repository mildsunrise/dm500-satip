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
| Author:    Mark Detrick
| Component: scp
| File:      scp_osd.c
| Purpose:   OS dependant functions for serial control port
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 09/19/2003  MSD               Created.
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
#include "scp_osd.h"
#include "scp_atom.h"
#include "scp/scp_inf.h"
#include "xilinx.h"

char flash_cmd[296];

scp_struct scp_unit;
SEMAPHORE_T sSCPInUseSem;
int scp_driver_init = 0;

static void scp_osd_isr();

int scp_osd_init()
{

   if(scp_driver_init == 0)
   {
      scp_atom_regs_init();

      xilinx_enable_scp_bus();

      sSCPInUseSem = os_create_semaphore(1);
    
     // register the ISR for kernel 

      if(os_install_irq(IRQ_SCP, IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG, scp_osd_isr, NULL) != 0)
      {
         (void)printk("int_install() failed\n\r");
         return(-1);
      }
      
      // disable the interrupt until a counter is defined 
      os_disable_irq(IRQ_SCP);
      
      // Initialize SCP Data Structure 
      scp_unit.read_ptr  = NULL;
      scp_unit.write_ptr = NULL;
      scp_unit.count     = 0;
      scp_unit.done_sem  = NULL;

      scp_atom_set_cdm(8);
    
      scp_atom_init();
      
      scp_driver_init = 1;
     
   }
    
   return(0); 
}



int scp_osd_uninit()
{
   if (!scp_driver_init)
      return SCP_NOT_INITIALIZED;
 
   /* free the IRQ */
   os_disable_irq(IRQ_SCP);
   os_uninstall_irq(IRQ_SCP);
    
   scp_atom_uninit();

   return(0);
}


/***********************************************************************/
/*    scp_isr is the ISR for the SCP macro. It will take an            */
/*    interrupt after the first byte has been writen and received.     */
/*    It will continue writing and recieving until count equals 0      */
/***********************************************************************/

static void scp_osd_isr()
{
   unsigned char btTemp;
   
   
   // make sure we are ready to accept the interrupt
   if(scp_unit.done_sem == NULL)
     return;
  
   os_disable_irq(IRQ_SCP);
   if(scp_unit.count == 0)
   {
       scp_unit.write_ptr = NULL;
       btTemp = scp_atom_get_scp_reg(SCP_RXDATA);
       scp_unit.read_ptr  = NULL;
       if(scp_unit.done_sem != NULL)
          os_post_semaphore(scp_unit.done_sem);
   }
   else
   {
      scp_unit.count--;
      // we need a small delay here to ensure that the data is ready to be read
      os_delay(5);            
      *scp_unit.read_ptr =  scp_atom_get_scp_reg(SCP_RXDATA);
      scp_unit.read_ptr++;
      scp_unit.write_ptr++;
      if (scp_atom_check_port() == SCP_BUSY)
      {
          printk("SCP Port is busy\n");
          os_post_semaphore(scp_unit.done_sem);
      }
      
      if (scp_unit.count > 0)
      {
         scp_atom_set_scp_reg(SCP_TXDATA,*scp_unit.write_ptr);
         scp_atom_set_scp_reg(SCP_SPCOM, SCP_SPCOM_START);
        
         os_enable_irq(IRQ_SCP);
      }
      else
      {  // need to re-enable the interrupt for the last transfer
    
         os_enable_irq(IRQ_SCP);
      }
   }
   
}


int scp_osd_rw(char *pInput, char *pOutput, int dwCount, int dwDev)
{
    unsigned char btScpMode = 0;

    if (!scp_driver_init)
        return SCP_NOT_INITIALIZED;

    /* Use SCP Port In Use Semaphore */
    os_wait_semaphore(sSCPInUseSem, -1);
        
    scp_unit.write_ptr = pInput;
    scp_unit.read_ptr  = pOutput;
    scp_unit.count     = dwCount;
    scp_unit.done_sem  = os_create_semaphore(0x0);
    /* set GPIO  low to enable device */
    scp_unit.device    = dwDev;

    /* CS to Flash and Enable SCP GPIO signals */
    xilinx_sflash_chip_select();

    os_enable_irq(IRQ_SCP);  
    
    btScpMode = scp_atom_get_scp_reg(SCP_SPMODE);
    /*Enable Device and Invert Clock for SCP Flash */
    btScpMode |= (SCP_SPMODE_ENABLE | SCP_SPMODE_CI);
    scp_atom_set_scp_reg(SCP_SPMODE, btScpMode); 
    
    scp_atom_set_scp_reg(SCP_TXDATA,*scp_unit.write_ptr);       
    
    scp_atom_set_scp_reg(SCP_SPCOM_OFFSET,SCP_SPCOM_START);  
    
    os_wait_semaphore(scp_unit.done_sem, -1);
    
    os_delete_semaphore(scp_unit.done_sem);
    
    scp_unit.done_sem = 0x0;
    
    scp_atom_set_scp_reg(SCP_SPMODE_OFFSET, scp_atom_get_scp_reg(SCP_SPMODE_OFFSET) & ~SCP_SPMODE_ENABLE);    
    
    os_post_semaphore(sSCPInUseSem);

    /* Deselect Flash and Disable SCP GPIO signals */
    xilinx_sflash_chip_deselect();
    
    if (scp_unit.count != 0)
       return SCP_ABORT;
    else
       return (SCP_SUCCESS);
} 


