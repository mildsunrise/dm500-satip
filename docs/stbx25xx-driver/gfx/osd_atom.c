//vulcan/drv/gfx/osd_atom.c
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
//  Atom function of OSD controls 
//Revision Log:   
//  Jun/04/2002                          Created by YYD
//  Oct/21/2002                          Added code to handle OSD_CNTL_CHFSR for
//                                       setting custom horizontal scaling ratio.  BJC

#include <os/os-sync.h>

#include "osd_atom.h"
#include "osd_local.h"
#include "osd_dcr.h"
#include "../vid/vid_atom_hw.h"

//#define __OSD_ATOM_DEBUG

#ifdef __OSD_ATOM_DEBUG
	#define __DRV_DEBUG
#endif

#include "os/drv_debug.h"


int osd_atom_set_display_control(STB_OSD_DISPLAY_CONTROL_T cntl, UINT uAttr)
{
   UINT32 flags;

    if(OSD_CNTL_BACKCOLOR == cntl)
    {
        UINT32 uDispM = MF_DCR(DCR_VID0_DISP_MODE);
        MT_DCR(DCR_VID0_DISP_MODE, ((uDispM&0xffff) | uAttr << 16));
    }
    else
    {
        UINT32    uCntlReg = MF_DCR(DCR_VID0_OSD_MODE);
    
        PDEBUG("Control = 0x%8.8x, attr = 0x%8.8x,  OSD_MODE = 0x%8.8x -> ", (UINT)cntl, uAttr, uCntlReg);
        switch(cntl)
        {
        case OSD_CNTL_AFVP:  	// Anti-flicker video plane, 0 disable, 1 enable                  	
            uCntlReg = (uCntlReg & 0xffff7fff) | (uAttr ? 0x00008000 : 0);
            break;
        case OSD_CNTL_EDAF:  	// Enable display anti-flicker, 0 disable, 1 enable               	
            uCntlReg = (uCntlReg & 0xffdfffff) | (uAttr ? 0x00200000 : 0);
            break;
        case OSD_CNTL_AFDT:  	// Anti-flicker detection threshold, 2 bits attr                  	
            uCntlReg = (uCntlReg & 0xfff3ffff) | ((uAttr&0x03) << 18);
            break;
        case OSD_CNTL_VPAFC: 	// Video plane anti-flicker correction, 2 bits attr               	
            uCntlReg = (uCntlReg & 0xfffcffff) | ((uAttr&0x03) << 16);
            break;
        case OSD_CNTL_E32BCO:	// Enable 32bit color option, 0 no, 1 yes                         	
            uCntlReg = (uCntlReg & 0xffefffff) | (uAttr ? 0x00100000 : 0);
            break;
        case OSD_CNTL_ANIM:  	// Animation mode, 0 no, 1 yes       
            if(uAttr != 0) 
            {
               // enable OSD animation interrupts from video decoder
               flags = os_enter_critical_section();
               MT_DCR(VID_MASK, MF_DCR(VID_MASK) | DECOD_HOST_INT_OSD_DATA);
               os_leave_critical_section(flags);
            }
            else
            {
              // disable OSD animation interrupts from video decoder
              flags = os_enter_critical_section();
              MT_DCR(VID_MASK, MF_DCR(VID_MASK) & ~DECOD_HOST_INT_OSD_DATA);
              os_leave_critical_section(flags);
            }
            uCntlReg = (uCntlReg & 0xfffffff7) | (uAttr ? 0x00000008 : 0);
            break;
        case OSD_CNTL_ANIMR:  	// Animation rate, 3 bits attr                                    	
            uCntlReg = (uCntlReg & 0xfffffff8) | (uAttr & 0x07);
            break;
        case OSD_CNTL_CHFSR:  	// Custom horizontal FIR scaling ratio, 9 bits attr -- BJC 102102
            uCntlReg = (uCntlReg & 0x007fffff) | ((uAttr & 0x1ff) << 23);
            break;
        case OSD_CNTL_BACKCOLOR:    // to get rid of compiler warning
            break;
        }
    
        PDEBUG(" 0x%8.8x\n", uCntlReg);
    
        MT_DCR(DCR_VID0_OSD_MODE, uCntlReg);
    }
    return 0;
}               

UINT osd_atom_get_display_control(STB_OSD_DISPLAY_CONTROL_T cntl)
{
    UINT32    uCntlReg;

    if(OSD_CNTL_BACKCOLOR == cntl)
    {
        uCntlReg = MF_DCR(DCR_VID0_DISP_MODE);
        uCntlReg >>= 16;
    }
    else
    {
        uCntlReg = MF_DCR(DCR_VID0_OSD_MODE);
    
        PDEBUG("DISP_CNTL = 0x%8.8x \n", uCntlReg);
        switch(cntl)
        {
        case OSD_CNTL_AFVP:  	// Anti-flicker video plane, 0 disable, 1 enable                  	
            uCntlReg = (uCntlReg & 0x00008000) ? 1 : 0;
            break;
        case OSD_CNTL_EDAF:  	// Enable display anti-flicker, 0 disable, 1 enable               	
            uCntlReg = (uCntlReg & 0x00200000) ? 1 : 0;
            break;
        case OSD_CNTL_AFDT:  	// Anti-flicker detection threshold, 2 bits attr                  	
            uCntlReg = (uCntlReg & 0x000c0000) >> 18;
            break;
        case OSD_CNTL_VPAFC: 	// Video plane anti-flicker correction, 2 bits attr               	
            uCntlReg = (uCntlReg & 0x00030000) >> 16;
            break;
        case OSD_CNTL_E32BCO:	// Enable 32bit color option, 0 no, 1 yes                          	
            uCntlReg = (uCntlReg & 0x00100000) ? 1 : 0;
            break;
        case OSD_CNTL_ANIM:  	// Animation mode, 0 no, 1 yes                                    	
            uCntlReg = (uCntlReg & 0x00000008) ? 1 : 0;
            break;
        case OSD_CNTL_ANIMR:  	// Animation rate, 3 bits attr                                    	
            uCntlReg = (uCntlReg & 0x07);
            break;
        case OSD_CNTL_CHFSR:  	// Custom horizontal FIR scaling ratio, 9 bits attr -- BJC 102102
            uCntlReg = (uCntlReg & 0xff800000) >> 23;
            break;
        case OSD_CNTL_BACKCOLOR:    // to get rid of compiler warning
            break;
        }
    }
    return uCntlReg;
}

                
int osd_atom_set_plane_control(STB_OSD_PLANE_CONTROL_T cntl, UINT uAttr)
{
    UINT32 uReg;

    PDEBUG("Control = 0x%8.8x, attr = 0x%8.8x\n", (UINT)cntl, uAttr);

    uReg = MF_DCR(DCR_VID0_OSD_MODE);

    PDEBUG("Old OSD_MODE = 0x%8.8x\n", uReg);

    switch(cntl)
    {
    case OSD_PLANE_A_ENABLE:                 // enable plane, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE | OSD_PLANE_BACKGROUND
        if(uAttr&OSD_PLANE_CURSOR)
        {
            uReg |= 0x00000800;
        }
        if(uAttr&OSD_PLANE_GRAPHICS)
        {
            uReg |= 0x00000040;
        }
        if(uAttr&OSD_PLANE_IMAGE)
        {
            uReg |= 0x00004000;
        }
        break;

    case OSD_PLANE_A_DISABLE:                // disable plane, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE | OSD_PLANE_BACKGROUND                            
        if(uAttr&OSD_PLANE_CURSOR)
        {
            uReg &= 0xfffff7ff;
        }
        if(uAttr&OSD_PLANE_GRAPHICS)
        {
            uReg &= 0xffffffbf;
        }
        if(uAttr&OSD_PLANE_IMAGE)
        {
            uReg &= 0xffffbfff;
        }
        break;
    case OSD_PLANE_A_BMLA:  // enable/disable BMLA mode, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE
        if(uAttr&1)       // enable
        {
            if(uAttr&OSD_PLANE_CURSOR)
            {
                uReg |= 0x00000400;
            }
            if(uAttr&OSD_PLANE_GRAPHICS)
            {
                uReg |= 0x00000020;
            }
            if(uAttr&OSD_PLANE_IMAGE)
            {
                uReg |= 0x00002000;
            }
        }
        else // disable
        {
            if(uAttr&OSD_PLANE_CURSOR)
            {
                uReg &= 0xfffffbff;
            }
            if(uAttr&OSD_PLANE_GRAPHICS)
            {
                uReg &= 0xffffffdf;
            }
            if(uAttr&OSD_PLANE_IMAGE)
            {
                uReg &= 0xffffdfff;
            }
        }
        break;
        
    case OSD_PLANE_A_422_AVERAGING:         // update control block, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE | OSD_PLANE_BACKGROUND                     
        uReg = (uReg & 0xffffefff) | ((uAttr&1) ? 0x00001000 : 0);
        break;

    case OSD_PLANE_G_PER_COLOR_BLEND_MODE:
        if(uAttr&OSD_PLANE_GRAPHICS)       // enable
        {
            uReg = (uReg & 0xffffffef) | ((uAttr&1) ? 0x00000010  : 0);
        }
        break;

    default:
        return -1;
    }

    PDEBUG("New OSD_MODE = 0x%8.8x\n", uReg);

    // update the changed register
    MT_DCR(DCR_VID0_OSD_MODE, uReg);
    return 0;
}

UINT osd_atom_get_plane_control(STB_OSD_PLANE_CONTROL_T cntl, UINT uAttr)
{
    UINT32 reg, rtn;

    PDEBUG("Control = 0x%8.8x, attr = 0x%8.8x\n", (UINT)cntl, uAttr);

    reg = MF_DCR(DCR_VID0_OSD_MODE);

    rtn = 0;

    switch(cntl)
    {
    case OSD_PLANE_A_ENABLE:                 // enable plane, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE | OSD_PLANE_BACKGROUND
    case OSD_PLANE_A_DISABLE:                 // enable plane, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE | OSD_PLANE_BACKGROUND
        if(uAttr&OSD_PLANE_CURSOR)
        {
            rtn = (reg & 0x00000800) ? 1 : 0;
        }
        else if(uAttr&OSD_PLANE_GRAPHICS)
        {
            rtn = (reg & 0x00000040) ? 1 : 0;
        }
        else if(uAttr&OSD_PLANE_IMAGE)
        {
            rtn = (reg & 0x00004000) ? 1 : 0;
        }
        break;

    case OSD_PLANE_A_BMLA:  // enable/disable BMLA mode, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE
        if(uAttr&OSD_PLANE_CURSOR)
        {
            rtn = (reg & 0x00000400) ? 1 : 0;
        }
        if(uAttr&OSD_PLANE_GRAPHICS)
        {
            rtn = (reg & 0x00000020) ? 1 : 0;
        }
        if(uAttr&OSD_PLANE_IMAGE)
        {
            rtn = (reg & 0x00002000) ? 1 : 0;
        }
        break;

    case OSD_PLANE_A_422_AVERAGING:         // enable/disable 444 to 422 conversion chroma averaging of CG plane, 0 disable/ 1 enable                                  
        rtn = (reg & 0x00001000) ? 1 : 0;
        break;

    case OSD_PLANE_G_PER_COLOR_BLEND_MODE:
        if(uAttr&OSD_PLANE_GRAPHICS)       // enable
        {
            rtn = (reg & 0x00000010) ? 1 : 0;
        }
        break;

    default:
        break;
    }

    return rtn;
}


int osd_atom_set_base_address(STB_OSD_BASE_ADDR_T index, UINT uAddr)
{
    PDEBUG("BASE = 0x%8.8x, Addr = 0x%8.8x\n", (UINT)index, uAddr);

    switch(index)
    {
    case OSD_ADDR_CURSOR_BASE:
        MT_DCR(DCR_VID0_CPBASE, uAddr);
        break;
    case OSD_ADDR_CURSOR_LINK:
        MT_DCR(DCR_VID0_CSLA, uAddr);
        break;
    case OSD_ADDR_GRAPHICS_BASE:
        MT_DCR(DCR_VID0_GPBASE, uAddr);
        break;
    case OSD_ADDR_GRAPHICS_LINK:
        MT_DCR(DCR_VID0_GSLA, uAddr);
        break;
    case OSD_ADDR_IMAGE_BASE:
        MT_DCR(DCR_VID0_IPBASE, uAddr);
        break;
    case OSD_ADDR_IMAGE_LINK:
        MT_DCR(DCR_VID0_ISLA, uAddr);
        break;
    }
    return 0;
}
