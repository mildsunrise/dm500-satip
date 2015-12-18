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
| File:      scp_osi.c
| Purpose:   OS independant functions for serial control port
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

#include "scp_osi.h"
#include "scp_osd.h"
#include "scp_atom.h"
#include "scp/scp_inf.h"

extern int scp_driver_init;


int scp_osi_display_regs()
{
   if (!scp_driver_init)
      return SCP_NOT_INITIALIZED;
      
   scp_atom_display_regs();   
        
   return(0);
}

        
int scp_osi_get_cmd(unsigned long *p_value)
{
   int rc;

   if (!scp_driver_init)
       return SCP_NOT_INITIALIZED;
       
   rc = scp_atom_get_cdm(p_value);   
        
   return(rc);
}
   
    
int scp_osi_set_cmd(unsigned long value)
{
   int rc;

   if (!scp_driver_init)
       return SCP_NOT_INITIALIZED;
   rc = scp_atom_set_cdm(value);
   
   return(rc);
}

    
int scp_osi_get_reverse_data(unsigned long *p_value)
{
   int rc;
   
   if (!scp_driver_init)
       return SCP_NOT_INITIALIZED;
 
   rc = scp_atom_get_reverse_data(p_value);

   return(rc);
}     
    
int scp_osi_set_reverse_data(unsigned long value)
{
   int rc;

   if (!scp_driver_init)
       return SCP_NOT_INITIALIZED;
 
   rc = scp_atom_set_reverse_data(value);
 
   return (rc);   

}       

int scp_osi_get_clock_invert(unsigned long *p_value)
{
   int rc;

   if (!scp_driver_init)
       return SCP_NOT_INITIALIZED;
 
   rc = scp_atom_get_clock_invert(p_value);
   
   return(rc);
}             

int scp_osi_set_clock_invert(unsigned long value)
{
   int rc;
   
   if (!scp_driver_init)
       return SCP_NOT_INITIALIZED;
 
 
   rc = scp_atom_set_clock_invert(value);
 
   return (rc);   
}           

int scp_osi_get_loopback(unsigned long *p_value)
{
   int rc;
   

   if (!scp_driver_init)
       return SCP_NOT_INITIALIZED;
 
   rc = scp_atom_get_loopback(p_value);
  
   return(rc); 
}   

int scp_osi_set_loopback(unsigned long value)
{
   int rc;
   
   if (!scp_driver_init)
       return SCP_NOT_INITIALIZED;
  
   rc = scp_atom_set_loopback(value);

   return(rc);   

}        

  
