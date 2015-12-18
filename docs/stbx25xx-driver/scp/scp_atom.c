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
| File:      scp_atom.c
| Purpose:   Atom functions for serial control port
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
#include "os/os-sync.h"
#include "os/os-interrupt.h"

#include "scp_atom.h"
#include "scp/scp_inf.h"
#include "xilinx.h"

unsigned short * volatile fpga;
unsigned char * volatile scp_regs;
unsigned long *gpio_regs;


void scp_atom_regs_init()
{
   ULONG flag;
   
   flag = os_enter_critical_section();
   
   fpga = ioremap(STB_FPGA_BASE_ADDRESS, 32);
   
   scp_regs = ioremap(SCP_REG_BASE, 7);
        
   gpio_regs = ioremap(STB_GPIO0_BASE_ADDRESS, 72);

   os_leave_critical_section(flag);
   
}


void scp_atom_init()
{
    unsigned long gpio_val;
    ULONG flag;
   
   
   flag = os_enter_critical_section();
   
#ifdef __DRV_FOR_VULCAN__
//     Set GPIO0[10][12] to alternate output 2 to enable SCP CLK and TX 
//     Set GPIO0[11] to alternate input 2 to enable SCP RX              

   gpio_val = gpio_regs[GPIO0_OS0_OFFSET] & ~0x00000FC0;
   gpio_regs[GPIO0_OS0_OFFSET] = gpio_val |  0x00000880;

   gpio_val = gpio_regs[GPIO0_TS0_OFFSET] & ~0x00000FC0;
   gpio_regs[GPIO0_TS0_OFFSET] = gpio_val |  0x00000880;

   gpio_val = gpio_regs[GPIO0_IS20_OFFSET] & ~0x00000300;
   gpio_regs[GPIO0_IS20_OFFSET] = gpio_val |  0x00000100;
   
#endif
#ifdef __DRV_FOR_PALLAS__
//     Set GPIO0[6][8] to alternate output 1 to enable SCP CLK(8) and TX(6) 
//     Set GPIO0[7] to alternate input 1 to enable SCP RX              

   gpio_val = gpio_regs[GPIO0_OS0_OFFSET] & ~0x000FC000;
   gpio_regs[GPIO0_OS0_OFFSET] = gpio_val |  0x00044000;

   gpio_val = gpio_regs[GPIO0_TS0_OFFSET] & ~0x000FC000;
   gpio_regs[GPIO0_TS0_OFFSET] = gpio_val |  0x00044000;

   gpio_val = gpio_regs[GPIO0_IS10_OFFSET] & ~0x00030000;
   gpio_regs[GPIO0_IS20_OFFSET] = gpio_val |  0x00010000;
#endif
  
 os_leave_critical_section(flag);
   
}


void scp_atom_uninit()
{

  /* unmap IO */
   iounmap(fpga);
   iounmap(scp_regs);
   iounmap(gpio_regs);
}

void scp_atom_display_regs()
{   
   printk("scp_osi_display_regs: SPMODE register = 0x%4.4x\n", scp_regs[SCP_SPMODE_OFFSET]);
   printk("scp_osi_display_regs: RXDATA register = 0x%4.4x\n", scp_regs[SCP_RXDATA_OFFSET]);
   printk("scp_osi_display_regs: TXDATA register = 0x%4.4x\n", scp_regs[SCP_TXDATA_OFFSET]);
   printk("scp_osi_display_regs: SPCOM  register = 0x%4.4x\n", scp_regs[SCP_SPCOM_OFFSET]);
   printk("scp_osi_display_regs: STATUS register = 0x%4.4x\n", scp_regs[SCP_STATUS_OFFSET]);
   printk("scp_osi_display_regs: CDM    register = 0x%4.4x\n", scp_regs[SCP_CDM_OFFSET]);

}

int scp_atom_get_cdm(unsigned long *p_value)
{
   *p_value = scp_regs[SCP_CDM_OFFSET];
   return(0);
}
   
    
int scp_atom_set_cdm(unsigned long value)
{
   ULONG flag;
   
   flag = os_enter_critical_section();
   
   scp_regs[SCP_CDM_OFFSET] = value;
   
   os_leave_critical_section(flag);

   return(0);
}

    
int scp_atom_get_reverse_data(unsigned long *p_value)
{
  *p_value = scp_regs[SCP_SPMODE_OFFSET] & SCP_SPMODE_REVDAT;
  return(0);
}     
    
int scp_atom_set_reverse_data(unsigned long value)
{
   ULONG flag;
   if((value < 0) ||value > 1) 
   {
      printk("scp_atom_set_clock_invert: invalid value specified\n");
      return (-1);
   }   
   
   flag = os_enter_critical_section();
   if(value == 1)
      scp_regs[SCP_SPMODE_OFFSET] = scp_regs[SCP_SPMODE_OFFSET]| SCP_SPMODE_REVDAT;
   if(value == 0)
      scp_regs[SCP_SPMODE_OFFSET] = scp_regs[SCP_SPMODE_OFFSET]& ~SCP_SPMODE_REVDAT;
   
   os_leave_critical_section(flag);
      
   return (0);   

}       

int scp_atom_get_clock_invert(unsigned long *p_value)
{
  *p_value = scp_regs[SCP_SPMODE_OFFSET] & SCP_SPMODE_CI;
   return(0);
}             

int scp_atom_set_clock_invert(unsigned long value)
{
   ULONG flag;
   
   if((value < 0) ||value > 1) 
   {
      printk("scp_atom_set_clock_invert: invalid value specified\n");
      return (-1);
   }   

   flag = os_enter_critical_section();
   
   if(value == 1)
      scp_regs[SCP_SPMODE_OFFSET] = scp_regs[SCP_SPMODE_OFFSET]| SCP_SPMODE_CI;
   if(value == 0)
      scp_regs[SCP_SPMODE_OFFSET] = scp_regs[SCP_SPMODE_OFFSET]& ~SCP_SPMODE_CI;
   
   os_leave_critical_section(flag);
      
   return (0);   
}           

int scp_atom_get_loopback(unsigned long *p_value)
{
   *p_value = scp_regs[SCP_SPMODE_OFFSET] & SCP_SPMODE_LOOP;
   return(0); 
}   

int scp_atom_set_loopback(unsigned long value)
{
   ULONG flag;
   
   if((value < 0) ||value > 1) 
   {
      printk("scp_atom_set_clock_invert: invalid value specified\n");
      return (-1);
   }   
   
   flag = os_enter_critical_section();
   
   if(value == 1)
      scp_regs[SCP_SPMODE_OFFSET] = scp_regs[SCP_SPMODE_OFFSET]| SCP_SPMODE_LOOP;
   if(value == 0)
      scp_regs[SCP_SPMODE_OFFSET] = scp_regs[SCP_SPMODE_OFFSET]& ~SCP_SPMODE_LOOP;
   
   os_leave_critical_section(flag);

   return (0);   

}        

  
int scp_atom_check_port()
{
    unsigned long dwCount;
    unsigned char btSCPStatus;
        
    for (dwCount = 0; dwCount < 5; dwCount++)
    {
        btSCPStatus = scp_regs[SCP_STATUS_OFFSET];
        if (btSCPStatus & 0x20)
        {
            os_delay(10000);
        }
        else 
        {
            return SCP_READY;
        }             
    }
    return SCP_BUSY;
}

unsigned char scp_atom_get_scp_reg (int regid)
{
   unsigned char value; 
   
   switch(regid)
   {
      case SCP_SPMODE_OFFSET:
         value = scp_regs[SCP_SPMODE_OFFSET];  
         break;
      case SCP_RXDATA_OFFSET:
         value = scp_regs[SCP_RXDATA_OFFSET];  
         break;
      case SCP_TXDATA_OFFSET:
         value = scp_regs[SCP_TXDATA_OFFSET];  
         break;
      case SCP_SPCOM_OFFSET:
         value = scp_regs[SCP_SPCOM_OFFSET];  
         break;
      case SCP_STATUS_OFFSET:
         value = scp_regs[SCP_STATUS_OFFSET];  
         break;
      case SCP_CDM_OFFSET:
         value = scp_regs[SCP_CDM_OFFSET];  
         break;
      default:
         printk("get_scp_reg: Invalid register id\n");
         value = 0;
   }
   
   return value;
}                              

int scp_atom_set_scp_reg (int regid, unsigned char value)
{
   int rc = 0; 
   ULONG flag;   

   flag = os_enter_critical_section();
   
   switch(regid)
   {
      case SCP_SPMODE_OFFSET:
         scp_regs[SCP_SPMODE_OFFSET] = value;  
         break;
      case SCP_RXDATA_OFFSET:
         scp_regs[SCP_RXDATA_OFFSET] = value;  
         break;
      case SCP_TXDATA_OFFSET:
         scp_regs[SCP_TXDATA_OFFSET] = value;  
         break;
      case SCP_SPCOM_OFFSET:
         scp_regs[SCP_SPCOM_OFFSET] = value;  
         break;
      case SCP_STATUS_OFFSET:
         scp_regs[SCP_STATUS_OFFSET] = value;  
         break;
      case SCP_CDM_OFFSET:
         scp_regs[SCP_CDM_OFFSET] = value;  
         break;
      default:
         printk("set_scp_reg: Invalid register id\n");
         rc = -1;
   }
   
   os_leave_critical_section(flag);
   
   return rc;
}                              

