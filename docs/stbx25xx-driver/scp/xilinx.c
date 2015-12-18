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
| Author:    Maciej P. Tyrlik
| Component: scp
| File:      xilinx.c
| Purpose:   Xilinx helper functions.
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 09/19/2003  MSD               Ported to Linux.
+----------------------------------------------------------------------------*/
#include "scp_atom.h"
#include "os/os-interrupt.h"
#include "os/os-sync.h"

extern unsigned short * volatile fpga;

/*----------------------------------------------------------------------------+
| XILINX_GET_BASE_ADDRESS()
+----------------------------------------------------------------------------*/
void *xilinx_get_base_address(void)
{
  return((void *)STB_FPGA_BASE_ADDRESS);
}

/*----------------------------------------------------------------------------+
| SCP_CHIP_SELECT().
+----------------------------------------------------------------------------*/
void xilinx_sflash_chip_select()
{

   unsigned short reg;
   unsigned long flag;

   flag = os_enter_critical_section();
   /*-------------------------------------------------------------------------+
   | Chip Select
   +-------------------------------------------------------------------------*/
   reg = fpga[STB_XILINX_REG5_OFFSET];

   fpga[STB_XILINX_REG5_OFFSET] = reg & 0xFF7F;

   fpga[STB_XILINX_FLUSH_OFFSET] =  0xFFFF;

   os_leave_critical_section(flag);

  return;
}

/*----------------------------------------------------------------------------+
| SCP_CHIP_DESELECT().
+----------------------------------------------------------------------------*/
void xilinx_sflash_chip_deselect()
{
   unsigned short       reg;
   unsigned long flag;

   flag = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   | Chip Select
   +-------------------------------------------------------------------------*/
   reg = fpga[STB_XILINX_REG5_OFFSET];

   fpga[STB_XILINX_REG5_OFFSET] = reg | 0x0080;

   fpga[STB_XILINX_FLUSH_OFFSET] = 0xFFFF;

   os_leave_critical_section(flag);

   return;
}

/*----------------------------------------------------------------------------+
| ENABLE_SCP_BUS().
+----------------------------------------------------------------------------*/
void xilinx_enable_scp_bus()
{
   unsigned short       reg;
   unsigned long flag;

   flag = os_enter_critical_section();
   
   /*-------------------------------------------------------------------------+
   | Enable SCP Signals
   +-------------------------------------------------------------------------*/
#ifdef __DRV_FOR_VULCAN__   
   reg = fpga[STB_XILINX_REG0_OFFSET] & 0x8FFF;
   fpga[STB_XILINX_REG0_OFFSET] =  reg | 0x5000;
#endif
#ifdef __DRV_FOR_PALLAS__  
   reg = fpga[STB_XILINX_REG0_OFFSET];
   fpga[STB_XILINX_REG0_OFFSET] =  reg | 0x0200;
#endif
   
   fpga[STB_XILINX_FLUSH_OFFSET] = 0xFFFF;
  
   os_leave_critical_section(flag);
   return;
}

/*----------------------------------------------------------------------------+
| DISABLE_SCP_BUS().
+----------------------------------------------------------------------------*/
void xilinx_disable_scp_bus()
{
   unsigned short       reg; 
   unsigned long flag;

   flag = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   | Disable SCP Signals
   +-------------------------------------------------------------------------*/
   reg = fpga[STB_XILINX_REG0_OFFSET];
   
#ifdef __DRV_FOR_VULCAN__  
   fpga[STB_XILINX_REG0_OFFSET] =  reg & (~0x7FFF);
#endif
#ifdef __DRV_FOR_PALLAS__   
   fpga[STB_XILINX_REG0_OFFSET] =  reg & (~0x0200);
#endif   

   fpga[STB_XILINX_FLUSH_OFFSET] = 0xFFFF;

   os_leave_critical_section(flag);

   return;
}

/*----------------------------------------------------------------------------+
| Write Protect
+----------------------------------------------------------------------------*/
void xilinx_sflash_wp_on()
{
   unsigned short       reg;
   unsigned long flag;

   flag = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   | Reset Serial Flash and Disable WP
   +-------------------------------------------------------------------------*/
   reg = fpga[STB_XILINX_REG5_OFFSET];
   
#ifdef __DRV_FOR_VULCAN__  
   fpga[STB_XILINX_REG5_OFFSET] = reg & ~0x0100;
#endif
#ifdef __DRV_FOR_PALLAS__     
   fpga[STB_XILINX_REG5_OFFSET] = reg & ~0x2000;
#endif

   fpga[STB_XILINX_FLUSH_OFFSET] = 0xFFFF;

   os_leave_critical_section(flag);

   return;
}

/*----------------------------------------------------------------------------+
| Reset Serial Flash().
+----------------------------------------------------------------------------*/
void xilinx_sflash_wp_off()
{
   unsigned short       reg;
   unsigned long flag;

   flag = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   | Reset Serial Flash and Disable WP
   +-------------------------------------------------------------------------*/
   reg=fpga[STB_XILINX_REG5_OFFSET];

   fpga[STB_XILINX_REG5_OFFSET] = reg | (0x2000);
   
   fpga[STB_XILINX_FLUSH_OFFSET] = 0xFFFF;
   
   os_leave_critical_section(flag);
    
   return;
}

/*----------------------------------------------------------------------------+
| Reset Serial Flash().
+----------------------------------------------------------------------------*/
void xilinx_sflash_reset_on()
{
   unsigned short       reg;
   unsigned long flag;

   flag = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   | Reset Serial Flash and Disable WP
   +-------------------------------------------------------------------------*/
   reg = fpga[STB_XILINX_REG5_OFFSET];
   
   fpga[STB_XILINX_REG5_OFFSET] =  reg & ~0x0200;
    
   fpga[STB_XILINX_FLUSH_OFFSET] =  0xFFFF;

   os_leave_critical_section(flag);

   return;
}

/*----------------------------------------------------------------------------+
| Reset Serial Flash().
+----------------------------------------------------------------------------*/
void xilinx_sflash_reset_off()
{
   unsigned short       reg;
   unsigned long flag;

   flag = os_enter_critical_section();

   /*-------------------------------------------------------------------------+
   | Reset Serial Flash and Disable WP
   +-------------------------------------------------------------------------*/
   reg = fpga[STB_XILINX_REG5_OFFSET];

   fpga[STB_XILINX_REG5_OFFSET] =  reg | (0x0200);
      
   fpga[STB_XILINX_FLUSH_OFFSET] =  0xFFFF;
   
   os_leave_critical_section(flag);
   
   return;
}

