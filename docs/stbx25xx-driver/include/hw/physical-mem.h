//vulcan/drv/include/hw/physical-mem.h
/*----------------------------------------------------------------------------+
|
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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
//
//Comment: 
//  Physical memory allocation definations for Linux  
//Revision Log:   
//  Sept/03/2001  Created by YYD
//  May/03/2002	  Modified for Vulcan by YYD
//  June/2003     Modified to allow for more flexible memory configurations MD

#ifndef  _DRV_INCLUDE_HW_PHYSICAL_MEM_H_INC_
#define  _DRV_INCLUDE_HW_PHYSICAL_MEM_H_INC_

 
// NOTE:
// The following are the default valus for a 64+64MB Redwood 5 Platform .
// If these values are changed the appropriate changes 
// must also be made in the stbbios platform.h and initbrd.S files.

#define __STB_SDRAM0_MEM_BASE_ADDR   (0x00000000) // physical base starting address of sdram 0 memory.
                                                 
#define __STB_SDRAM0_MEM_SIZE        (0x04000000) // physical memory size of sdram0 (64MB) 
                                                  // Valid physical addresses are 0x00000000 - 0x03ffffff. 

#define __STB_SDRAM1_MEM_BASE_ADDR   (0xA0000000)  // physical base starting address of sdram 1 memory

#define __STB_SDRAM1_MEM_SIZE        (0x04000000)  // physical memory size of sdram1 (64MB) 
                                                   // Valid physical addresses are 0xA0000000 - 0xA3ffffff. 


// NOTE:
// The following defines assume the default kernel configuration, where the kernel uses all the memory of SDRAM0
// and the drivers use the memory of SDRAM1. It is possible to reconfigure the kernel to use less memory and
// allow the drivers to use a portion of SDRAM0 as well. One way to reduce the Linux kernel memory size is to add
// a mem=xxm parameter to the initial kernel command string. Using the kernel configuration tool select 
// "General setup" followed by "Default bootloader kernel arguments" followed by "Initial kernel command string". 
// Please reference the IBM STB0xxx Device Driver Reference (Linux Edition) documentation for instructions on changing
// the Linux Kernel configuration.
// For example: if you wish to limit the Linux Kernel to the first 48 Mega Bytes of memory(NOTE: The kernel always 
// starts at address 0) add the mem=48m parameter to the initial kernel command string,save the configuration and 
// rebuild the kernel. The next time the kernel is loaded you will see the the mem=48m parameter added to the Kernel
// command line (this is displayed when the kernel is loading).
// With this configuration the first 48 mega bytes of SDRAM0 (addresses 0x00000000 - 0x02ffffff) will be used by 
// the kernel and the remaining 12 mega bytes of SDRAM0 (addresses 0x00300000 - 0x003fffff) are available to the drivers.


#define __STB_V_FB_MEM_BASE_ADDR     (0xA0000000)  // video frame buffer base address. Must start on 1 MB boundary

#define __STB_V_FB_MEM_SIZE          (0x00200000)  // Size is fixed at. 2MB do not change this

#define __STB_V_MEM_BASE_ADDR        (0xA0200000)  // Video physical base address. Must start on 128 byte boundary 

#define __STB_V_MEM_SIZE             (0x00200000)  // 2 MB is the recommended size
                                                   // __STB_V_MEM_SIZE includes 512 bytes of user data, 
                                                   // VBI data, and video rate buffer. 
                                                   // The rate buffer size is determined by:
                                                   // __STB_V_MEM_SIZE - 512 bytes of user data - VBI data size 
                                                   // Video user data, VBI and rate buffer memory areas are 
                                                   // defined in vid_atom.h 


#define __STB_A_MEM_BASE_ADDR        (0xa0400000)  // Audio physical base address.  Must start on 128 byte boundary   
                                                   
#define __STB_A_MEM_SIZE             (0x00069000)  // Audio memory size
                                                   // __STB_A_MEM_SIZE includes 0x00038800 byes of memory for the
                                                   // audio rate buffer and audio work buffers. This is followed by
                                                   // 96k bytes for the audio clip buffer and another 96k bytes for
                                                   // the audio mixer buffer. NOTE: the clip buffer and mixer buffers
                                                   // must start on a 4k boundry. The buffers will be forced to the
                                                   // next 4k boundry for you however this must be accounted for in the
                                                   // __STB_A_MEM_SIZE.


#define __STB_GRAPHICS_MEM_BASE_ADDR (0xA0469000)  // Physical base address for graphics memory pool
                                                   // Must start on 128 byte boundary

#define __STB_GRAPHICS_MEM_SIZE      (0x00800000)  // NOTE: if __STB_GRAPHICS_MEM_SIZE is set to 0 the graphics
                                                   // memory will be allocated from the generic memory pool
                                                   // NOTE: the maximun value  for this parameter is 0x03700000
                                                   // (55 megabytes). 

#define __STB_ALLOC_MEM_BASE_ADDR    (0xA0C69000)  // Physical base address for the generic memory pool
                                                   // Must start on 128 byte boundary
                                                   // This memory is used for transport queues and for graphics
                                                   // buffers when __STB_GRAPHICS_MEM_SIZE is 0
                                                   // NOTE: if __STB_GRAPHICS_MEM_SIZE is set to 0 the maximum value
                                                   // for this paranmeter is 0x03700000 (55 megabytes).

#define __STB_ALLOC_MEM_SIZE (__STB_SDRAM1_MEM_SIZE-(__STB_ALLOC_MEM_BASE_ADDR-__STB_SDRAM1_MEM_BASE_ADDR)) //use the ramainder of sdram1 memory

#endif  // _DRV_INCLUDE_HW_PHYSICAL_MEM_H_INC_
