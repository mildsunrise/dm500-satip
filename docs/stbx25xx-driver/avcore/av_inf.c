//pallas/drv/avcore/av_inf.c
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
//Comment: 
//  Dummy Linux Moduler interface for AV core
//Revision Log:   
//  Nov/22/2001                         Created by YYD

// The necessary header files
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>

#include "os/drv_debug.h"
#include "hw/hardware.h"

#include "os/pversion.h"

#ifdef MODULE
MODULE_AUTHOR("IBM CRL");
#endif

#ifdef __DRV_FOR_VULCAN__

#define AV_DRIVER_NAME   "STBx25xx AV"
#ifdef MODULE
MODULE_DESCRIPTION("Audio / Video / Demux driver for IBM Vulcan STB Chip");
#endif

#elif defined(__DRV_FOR_PALLAS__)

#define AV_DRIVER_NAME   "STB04xxx AV"
#ifdef MODULE
MODULE_DESCRIPTION("Audio / Video / Demux driver for IBM Pallas STB Chip");
#endif

#elif defined(__DRV_FOR_VESTA__)

#define AV_DRIVER_NAME   "STB03xxx AV"
#ifdef MODULE
MODULE_DESCRIPTION("Audio / Video / Demux driver for IBM Vesta STB Chip");
#endif

#else   // why 

#error "Unsupported architecture, please specify it in 'include/hw/hardware.h'"

#endif


// vid
extern int  vid_init_module(void);
extern void vid_cleanup_module(void);

//aud
extern int  aud_init_module(void);
extern void aud_cleanup_module(void);

//demux
extern int  demux_init_module(void);
extern void demux_cleanup_module(void);


//scrman
//extern int  scrman_init_module(void);
//extern void scrman_cleanup_module(void);


static int dummy_inf_init(void)
{
    // print the driver verision info for futher reference
    PVERSION(AV_DRIVER_NAME);


    if(demux_init_module()) goto err1;
	if(vid_init_module()) goto err3;
	if(aud_init_module()) goto err4;

	return 0;
err4:
	PFATAL("Audio device init failed!\n");
	vid_cleanup_module();
err3:
	PFATAL("Video device init failed!\n");

    demux_cleanup_module();
err1:	
	PFATAL("Demux device init failed!\n");
	return -1;
}

static void dummy_inf_deinit(void)
{
	aud_cleanup_module();
	vid_cleanup_module();
	demux_cleanup_module();
}

module_init(dummy_inf_init);
module_exit(dummy_inf_deinit);

