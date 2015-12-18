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
| File:      scp_atom.h
| Purpose:   Atom functions for serial control port
| Changes:
|
| Date:       Author            Comment:
| ----------  ----------------  -----------------------------------------------
| 09/19/2003  MSD               Created.
+----------------------------------------------------------------------------*/

#ifndef _scp_atom_h_
#define _scp_atom_h_

#define STB_SCC1_BASE_ADDRESS   0xC0000000
#define STB_SCC2_BASE_ADDRESS   0xC0010000
#define STB_SCI0_BASE_ADDRESS   0xC0020000
#define STB_IIC0_BASE_ADDRESS   0xC0030000
#define STB_SCC0_BASE_ADDRESS   0xC0040000
#define STB_GPT0_BASE_ADDRESS   0xC0050000
#define STB_GPIO0_BASE_ADDRESS  0xC0060000
#define STB_SCI1_BASE_ADDRESS   0xC0070000
#define STB_SCP0_BASE_ADDRESS   0xC00C0000
#define STB_SSP0_BASE_ADDRESS   0xC00D0000

/*----------------------------------------------------------------------------+
| GPIO register definitions. These are offsets from STB_GPIO0_BASE_ADDRESS
+----------------------------------------------------------------------------*/
#define GPIO0_OUTPUT_OFFSET                  0
#define GPIO0_TC_OFFSET                      1
#define GPIO0_OS0_OFFSET                     2
#define GPIO0_OS1_OFFSET                     3
#define GPIO0_TS0_OFFSET                     4
#define GPIO0_TS1_OFFSET                     5
#define GPIO0_OD_OFFSET                      6
#define GPIO0_INPUT_OFFSET                   7
#define GPIO0_R1_OFFSET                      8
#define GPIO0_R2_OFFSET                      9
#define GPIO0_R3_OFFSET                      10
#define GPIO0_IS10_OFFSET                    12
#define GPIO0_IS11_OFFSET                    13
#define GPIO0_IS20_OFFSET                    14
#define GPIO0_IS21_OFFSET                    15
#define GPIO0_IS30_OFFSET                    16
#define GPIO0_IS31_OFFSET                    17

#define STB_XILINX_REG0_OFFSET          0x00000
#define STB_XILINX_REG1_OFFSET          0x00001
#define STB_XILINX_REG2_OFFSET          0x00002
#define STB_XILINX_REG3_OFFSET          0x00003
#define STB_XILINX_REG4_OFFSET          0x00004
#define STB_XILINX_REG5_OFFSET          0x00005
#define STB_XILINX_REG6_OFFSET          0x00006
#define STB_XILINX_ID_OFFSET            0x00007
#define STB_XILINX_FLUSH_OFFSET         0x00007
#define STB_FPGA_BASE_ADDRESS           0xF2040000


/*-----------------------------------------------------------------------
**                  SCP/SPI Register ADDRESSES
**----------------------------------------------------------------------*/
/*#define SCP_REG_BASE STB_SCP0_BASE_ADDRESS*/
#define SCP_REG_BASE            0x400C0000
#define SCP_SPMODE_OFFSET       0
#define SCP_RXDATA_OFFSET       1
#define SCP_TXDATA_OFFSET       2
#define SCP_SPCOM_OFFSET        3
#define SCP_STATUS_OFFSET       4
#define SCP_CDM_OFFSET          6


/******************************************************************/
/* SCP SPMODE register                                            */
/******************************************************************/
#define SCP_SPMODE_LOOP         0x01
#define SCP_SPMODE_CI           0x02
#define SCP_SPMODE_REVDAT       0x04
#define SCP_SPMODE_ENABLE       0x08

/******************************************************************/
/* SCP SPCOM  register                                            */
/******************************************************************/
#define SCP_SPCOM_START         0x01


void scp_atom_regs_init();
void scp_atom_init();
void scp_atom_uninit();
void scp_atom_display_regs();
int scp_atom_get_cdm(unsigned long *p_value);
int scp_atom_set_cdm(unsigned long value);
int scp_atom_get_reverse_data(unsigned long *p_value);
int scp_atom_set_reverse_data(unsigned long value);
int scp_atom_get_clock_invert(unsigned long *p_value);
int scp_atom_set_clock_invert(unsigned long value);
int scp_atom_get_loopback(unsigned long *p_value);
int scp_atom_set_loopback(unsigned long value);
int scp_atom_check_port();
int scp_atom_set_scp_reg (int regid, unsigned char value);
unsigned char scp_atom_get_scp_reg (int regid);



#endif
