//vulcan/drv/gfx/gfx_osi_surface.c
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
//  OSI GFX surface helper functions 
//Revision Log:   
//  Oct/17/2001                                             Created by YYD
//  Jun/04/2002                       Ported to Vulcan architecture by YYD
//  Oct/21/2002     Enabled horizontal scaling in region header when a new
//                  region is created.  Default scaling is set to 1x in 
//                  osd_osi_init() and is changed with the OSD_CNTL_CHFSR
//                  control to osd_atom_set_display_control().         BJC


#include "os/os-types.h"
#include "os/pm-alloc.h"
#include "os/os-sync.h"
#include "os/os-generic.h"
#include "gfx_surface.h"
#include "osd_atom.h"
#include "hw/physical-mem.h"
#include "gfx_osi_local.h"



// #define __GFX_SURFACE_DEBUG

#ifdef __GFX_SURFACE_DEBUG
    #define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif

extern ULONG guGraphicsVideoOffset;


//////////////////////////////////////////////////////////////////////////////
// Graphics surface defination
//////////////////////////////////////////////////////////////////////////////

static UINT __justify_gfx_byte(UINT bpl)
{
    return (bpl%GFX_SURFACE_LINE_JUSTIFY) 
        ?  (bpl + GFX_SURFACE_LINE_JUSTIFY - (bpl%GFX_SURFACE_LINE_JUSTIFY))
        :  bpl;
}

void gfx_osi_init_surface_t(GFX_SURFACE_T * pSurface)
{
    if(!pSurface) return;
    _OS_MEMSET(pSurface, 0, sizeof(GFX_SURFACE_T));
    pSurface->attachedDev = GFX_VDEV_NULL;

    return;
}

// used to empty a new unused surface_t for next steps

INT gfx_osi_reset_surface_palette(GFX_SURFACE_T *pSurface)
{
    GFX_PALETTE_T  *pgPal;
    UINT uNumPal;
    if(!pSurface )
    {
	    PDEBUG("pSurface is NULL!");
	    return -1;
    }
    if(!gfx_osi_pSurface_alloc(pSurface))
    {
	    PDEBUG("pSurface is not initialized !");
	    return -1;
    }
    if(!pSurface->pPalette )
    {
	    return 0;   // we don't have palette
    }

    switch(pSurface->uPlaneConfig)
    {
    case GFX_SURFACE_CLUT4BPPP_ARGB:  
    case GFX_SURFACE_CLUT4BPP_ARGB:   
    case GFX_SURFACE_CLUT4BPP_AYCBCR: 
    case GFX_SURFACE_CLUT4BPPP_AYCBCR:
    case GFX_SURFACE_CURSOR4BPP_YCBCR:
    case GFX_SURFACE_CURSOR4BPPP_YCBCR:
    case GFX_SURFACE_CURSOR4BPP_RGB:
    case GFX_SURFACE_CURSOR4BPPP_RGB:
        pgPal = gGFX_RGB4_Palette;
        uNumPal=16;             // for cursor compatible
        break;
    case GFX_SURFACE_CLUT2BPPP_ARGB:  
    case GFX_SURFACE_CLUT2BPP_ARGB:   
    case GFX_SURFACE_CLUT2BPP_AYCBCR: 
    case GFX_SURFACE_CLUT2BPPP_AYCBCR:
    case GFX_SURFACE_CURSOR2BPP_YCBCR:
    case GFX_SURFACE_CURSOR2BPPP_YCBCR:
    case GFX_SURFACE_CURSOR2BPP_RGB:
    case GFX_SURFACE_CURSOR2BPPP_RGB:
        pgPal = gGFX_RGB2_Palette;
        uNumPal=4;             // for cursor compatible
        break;

    case GFX_SURFACE_CLUT8BPP_ARGB:
    case GFX_SURFACE_CLUT8BPP_AYCBCR: 
    case GFX_SURFACE_CLUT8BPPP_ARGB:
    case GFX_SURFACE_CLUT8BPPP_AYCBCR: 
        pgPal = gGFX_RGB8_Palette;
        uNumPal=256;             // for cursor compatible
        break;
    default:
        PDEBUG("Invalid plane config 0x%8.8x !\n", pSurface->uPlaneConfig);
        return -1;
    }
    return gfx_osi_set_surface_palette(pSurface, pgPal, 0, uNumPal);
}


INT gfx_osi_set_surface_palette(GFX_SURFACE_T *pSurface, GFX_PALETTE_T *pPal, UINT uStart, UINT uNumEntry)
{
    UINT uMaxPal, i;
//printk("gfx_osi_set_surface_palette called starting at number %d num entries = %d\n",uStart,uNumEntry);
    if(!pSurface || !pPal)
    {
	    PDEBUG("pSurface / pPal is NULL!");
	    return -1;
    }
    if(!gfx_osi_pSurface_alloc(pSurface))
    {
	    PDEBUG("pSurface is not initialized !");
	    return -1;
    }
    if(!pSurface->pPalette )
    {
	    PDEBUG("pSurface doesn't have palette !");
	    return 0;   // we don't have palette
    }
    uMaxPal = 1<<pSurface->plane[0].uPixelSize;

    if(uStart >= uMaxPal || uNumEntry == 0)
    {
       PDEBUG("Palette index s(%d) n(%d) out of range!\n", uStart, uNumEntry);
       return 0;
    }
    if(uStart + uNumEntry > uMaxPal)
        uNumEntry = uMaxPal - uStart;

    PDEBUGE("Set palette %d to %d\n",  uStart, uStart + uNumEntry-1);

    if(IS_GFX_SURFACE_YUV(pSurface->uPlaneConfig))
    {
        if(pSurface->uAttr&__GFX_SURFACE_OSDCURSOR)  // cursor
        {
            STB_OSD_C_PALETTE_T *pPhyPal = (STB_OSD_C_PALETTE_T *)pSurface->pPal + uStart;
            GFX_PALETTE_T *pLogPal = pSurface->pPalette + uStart;
            for(i=0; i<uNumEntry; i++)
            {
                pLogPal[i] = pPal[i];
				if(pPal[i].a)
				{
					pPhyPal[i].y  = pLogPal[i].r >> 2;  // 6 bits
					pPhyPal[i].cb = pLogPal[i].g >> 4;  // 4 bits
					pPhyPal[i].cr = pLogPal[i].b >> 4;  // 4 bits
				}
				else	// transparent
				{
					pPhyPal[i].y  = 0; 
					pPhyPal[i].cb = 0; 
					pPhyPal[i].cr = 0; 
				}
                pPhyPal[i].peep   = 0;
                pPhyPal[i].steady = 1;
                PDEBUGE("CUR PALETTE %03d = %04x (argb = %d, %d, %d, %d)\n",
                    i, *((USHORT *)&pPhyPal[i]), pPal[i].a, pPal[i].r, pPal[i].g, pPal[i].b);
            }
        }
        else
        {
            STB_OSD_GI_PALETTE_T *pPhyPal = (STB_OSD_GI_PALETTE_T *)pSurface->pPal + uStart;
            GFX_PALETTE_T *pLogPal = pSurface->pPalette + uStart;
 printk("setting surface palette for yuv\n");

            for(i=0; i<uNumEntry; i++)
            {
                UINT alpha;
                pLogPal[i] = pPal[i];
                
                //  0 	0 - 25			4
                //.25	26 - 90			3
                //.50	91 - 155		2
                //.75	156 - 220		1
                //1.0	221 - 255		0

                alpha = (285 - pPal[i].a) / 65;
                
                if(alpha > 3)   // transparenet
                {
                    pPhyPal[i].y = pPhyPal[i].cb = pPhyPal[i].cr = 0;
                }
                else
                {
                    pPhyPal[i].y  = pLogPal[i].r >> 2;  // 6 bits
                    pPhyPal[i].cb = pLogPal[i].g >> 4;  // 4 bits
                    pPhyPal[i].cr = pLogPal[i].b >> 4;  // 4 bits
                    pPhyPal[i].bf0 = (alpha&1) ? 1 : 0;
                    pPhyPal[i].bf1 = (alpha&2) ? 1 : 0;
                }

                PDEBUGE("PALETTE %03d = %04x (argb = %d, %d, %d, %d)\n",
                    i, *((USHORT *)&pPhyPal[i]), pPal[i].a, pPal[i].r, pPal[i].g, pPal[i].b);
            }
        }
    }
    else    // rgb surface, we need to convert palette to yuv
    {
        if(pSurface->uAttr&__GFX_SURFACE_OSDCURSOR)  // cursor
        {
            STB_OSD_C_PALETTE_T *pPhyPal = (STB_OSD_C_PALETTE_T *)pSurface->pPal + uStart;
            GFX_PALETTE_T *pLogPal = pSurface->pPalette + uStart;
            for(i=0; i<uNumEntry; i++)
            {
                BYTE y,u,v;
                gfx_osi_rgb2ycbcr(
                    pPal[i].r, pPal[i].g, pPal[i].b,  
                    &y,  &u,  &v);
                pLogPal[i] = pPal[i];
				if(pPal[i].a)
				{
					pPhyPal[i].y  = y >> 2;  // 6 bits
					pPhyPal[i].cb = u >> 4;  // 4 bits
					pPhyPal[i].cr = v >> 4;  // 4 bits
				}
				else	// transparent
				{
					pPhyPal[i].y  = 0; 
					pPhyPal[i].cb = 0; 
					pPhyPal[i].cr = 0; 
				}
                pPhyPal[i].peep   = 0;
                pPhyPal[i].steady = 1;
                PDEBUGE("CUR PALETTE %03d = %04x (argb = %d, %d, %d, %d)\n",
                    i, *((USHORT *)&pPhyPal[i]), pPal[i].a, pPal[i].r, pPal[i].g, pPal[i].b);
            }
        }
        else
        {
            STB_OSD_GI_PALETTE_T *pPhyPal = (STB_OSD_GI_PALETTE_T *)pSurface->pPal + uStart;
            GFX_PALETTE_T *pLogPal = pSurface->pPalette + uStart;
//printk("setting surface palette for rgb");

            for(i=0; i<uNumEntry; i++)
            {
                UINT alpha;
                BYTE y,u,v;
                gfx_osi_rgb2ycbcr(
                    pPal[i].r, pPal[i].g, pPal[i].b,  
                    &y,  &u,  &v);
                pLogPal[i] = pPal[i];
                
                //  0 	0 - 25			4
                //.25	26 - 90			3
                //.50	91 - 155		2
                //.75	156 - 220		1
                //1.0	221 - 255		0

                alpha = (285 - pPal[i].a) / 65;
                
//printk("   alpha = %d\n", alpha);		
                if(alpha > 3)   // transparenet
                {
                    pPhyPal[i].y = pPhyPal[i].cb = pPhyPal[i].cr = 0;
                }
                else
                {
                    pPhyPal[i].y  = y >> 2;  // 6 bits
                    pPhyPal[i].cb = u >> 4;  // 4 bits
                    pPhyPal[i].cr = v >> 4;  // 4 bits
                    pPhyPal[i].bf0 = (alpha&1) ? 1 : 0;
                    pPhyPal[i].bf1 = (alpha&2) ? 1 : 0;
                }

//printk("PALETTE %03d = %04x (argb = %d, %d, %d, %d)\n",
//                    i, *((USHORT *)&pPhyPal[i]), pPal[i].a, pPal[i].r, pPal[i].g, pPal[i].b);
                PDEBUGE("PALETTE %03d = %04x (argb = %d, %d, %d, %d)\n",
                    i, *((USHORT *)&pPhyPal[i]), pPal[i].a, pPal[i].r, pPal[i].g, pPal[i].b);
            }
        }
    }
    return 0;
}

INT gfx_osi_get_surface_palette(GFX_SURFACE_T *pSurface, GFX_PALETTE_T *pPal, UINT uStart, UINT uNumEntry)
{
    UINT uMaxPal;
    if(!pSurface || !pPal)
    {
	    PDEBUG("pSurface / pPal is NULL!");
	    return -1;
    }
    if(!gfx_osi_pSurface_alloc(pSurface))
    {
	    PDEBUG("pSurface is not initialized !");
	    return -1;
    }
    if(!pSurface->pPalette )
    {
	    PDEBUG("pSurface doesn't have palette !");
	    return 0;   // we don't have palette
    }
    uMaxPal = 1<<pSurface->plane[0].uPixelSize;

    if(uStart >= uMaxPal || uNumEntry == 0)
    {
       PDEBUG("Palette index s(%d) n(%d) out of range!\n", uStart, uNumEntry);
       return 0;
    }
    if(uStart + uNumEntry > uMaxPal)
        uNumEntry = uMaxPal - uStart;

    PDEBUGE("Get palette %d to %d\n",  uStart, uStart + uNumEntry-1);
    _OS_MEMCPY(pPal, pSurface->pPalette + uStart, uNumEntry*sizeof(GFX_PALETTE_T));
    return 0;
}

static int __gfx_osi_adjust_surface_wh(UINT uPlaneConfig, UINT *puWidth, UINT *puHeight, UINT *puBytePerLine)
{
    switch(uPlaneConfig)
    {
    case GFX_SURFACE_CLUT8BPP_ARGB:
    case GFX_SURFACE_CLUT8BPP_AYCBCR: 
    case GFX_SURFACE_AYCBCR_422_8888: 
    case GFX_SURFACE_YCBCR_422_888:
    case GFX_SURFACE_AYCBCR_420_8888: 
    case GFX_SURFACE_YCBCR_420_888:
        *puWidth = (*puWidth + 3)&0xfffffffc;       // 4 pixel justify
        *puHeight = (*puHeight + 1) & 0xfffffffe;       // must be even lines
        *puBytePerLine = *puWidth;
        break;

    case GFX_SURFACE_CLUT8BPPP_ARGB:
    case GFX_SURFACE_CLUT8BPPP_AYCBCR: 
    case GFX_SURFACE_CLUT4BPP_ARGB:   
    case GFX_SURFACE_CLUT4BPP_AYCBCR: 
    case GFX_SURFACE_CURSOR4BPP_RGB:   
    case GFX_SURFACE_CURSOR4BPP_YCBCR: 
        *puWidth = (*puWidth + 7)&0xfffffff8;       // 8 pixel justify
        *puHeight = (*puHeight + 1) & 0xfffffffe;       // must be even lines
        *puBytePerLine = *puWidth/2;
        break;

    case GFX_SURFACE_CLUT4BPPP_ARGB:  
    case GFX_SURFACE_CLUT4BPPP_AYCBCR:
    case GFX_SURFACE_CLUT2BPP_ARGB:   
    case GFX_SURFACE_CLUT2BPP_AYCBCR: 
    case GFX_SURFACE_CURSOR4BPPP_RGB:   
    case GFX_SURFACE_CURSOR4BPPP_YCBCR: 
    case GFX_SURFACE_CURSOR2BPP_RGB:   
    case GFX_SURFACE_CURSOR2BPP_YCBCR: 
        *puWidth = (*puWidth + 15)&0xfffffff0;       // 16 pixel justify
        *puHeight = (*puHeight + 1) & 0xfffffffe;       // must be even lines
        *puBytePerLine = *puWidth/4;
       break;

    case GFX_SURFACE_CLUT2BPPP_ARGB:   
    case GFX_SURFACE_CLUT2BPPP_AYCBCR: 
    case GFX_SURFACE_CURSOR2BPPP_RGB:   
    case GFX_SURFACE_CURSOR2BPPP_YCBCR: 
        *puWidth = (*puWidth + 31)&0xffffffe0;       // 32 pixel justify
        *puHeight = (*puHeight + 1) & 0xfffffffe;       // must be even lines
        *puBytePerLine = *puWidth/8;
        break;

    case GFX_SURFACE_ARGB_8888:       
        *puBytePerLine = *puWidth*4;
        break;
    case GFX_SURFACE_RAW8BPP :
        *puBytePerLine = *puWidth;
        break;
    case GFX_SURFACE_RAW1BPP :
        *puBytePerLine = (*puWidth + 7)/8;
        break;

    default:
        PDEBUG("Invalid plane config 0x%8.8x !\n", uPlaneConfig);
        return -1;
    }

	if(IS_SURFACE_CURSOR_COMP(uPlaneConfig) && *puBytePerLine > 32)	// cursor must be less than 32bpl in stb03 and stbx2
		return -1;
    return 0;
}


INT gfx_osi_create_surface(GFX_SURFACE_T *pSurface, GFX_VISUAL_DEVICE_ID_T graphDev, UINT uPlaneConfig, UINT uWidth, UINT uHeight)
{
    UINT uNumBuf, i;
    UINT uHeaderSize;
    UINT uAllocSize, uNumPal, uBytePerLine;
    UINT cursor=0, twinheader=0;

    uNumBuf = GET_GFX_SURFACE_SUBPLANES(uPlaneConfig);
    // first, a rough check of the parameters
    if(!pSurface )
    {
	    PDEBUG("pSurface is NULL!");
	    return -1;
    }
    if(gfx_osi_pSurface_alloc(pSurface))
    {
	    PDEBUG("pSurface is already initialized !");
	    return -1;
    }
	    
    if(   uNumBuf < 1 // wrong plane config
        || uWidth < 4 || uHeight < 2
        || uWidth > GFX_SURFACE_MAX_WIDTH || uHeight > GFX_SURFACE_MAX_HEIGHT) 
    {
        PDEBUG("Invalid parameter: plane config = %08x, width=%d, height=%d !\n", uPlaneConfig, uWidth, uHeight);
        return -1;    // we cann't do it
    }

    if(__gfx_osi_adjust_surface_wh(uPlaneConfig, &uWidth, &uHeight, &uBytePerLine) < 0)
    {
        PDEBUG("Invalid parameter: plane config = %08x, width=%d, height=%d !\n", uPlaneConfig, uWidth, uHeight);
        return -1;    // we cann't do it
    }


    // then, we need to check if the planeconfig is compatible with the device
    switch(graphDev)
    {
    case GFX_VDEV_OSDCUR:
        if(!(uPlaneConfig & GFX_COMP_OSDCUR))
        {
            PDEBUG("Desired planeConfig is incompatible with cursor device!\n");
            return -1;
        }
        break;

    case GFX_VDEV_OSDGFX:
        if(!(uPlaneConfig & GFX_COMP_OSDGFX))
        {
            PDEBUG("Desired planeConfig is incompatible with graphics device!\n");
            return -1;
        }
        break;

    case GFX_VDEV_OSDIMG:
        if(!(uPlaneConfig & GFX_COMP_OSDIMG))
        {
            PDEBUG("Desired planeConfig is incompatible with still device!\n");
            return -1;
        }
        break;

    default:        // GFX_VDEV_NULL
        break;
    }

    gfx_osi_init_surface_t(pSurface);  // just make sure it's is empty

    //ok, we now try to alloc the surface
    switch(uPlaneConfig)
    {

    case GFX_SURFACE_CLUT8BPP_ARGB:
    case GFX_SURFACE_CLUT8BPP_AYCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 8;      // 8 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_GI_CONTROL_BLOCK_T) + 256*sizeof(STB_OSD_GI_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        break;

    case GFX_SURFACE_CLUT8BPPP_ARGB:
    case GFX_SURFACE_CLUT8BPPP_AYCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth/2;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 8;      // 8 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_GI_CONTROL_BLOCK_T) + 256*sizeof(STB_OSD_GI_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        break;

    case GFX_SURFACE_CLUT4BPP_ARGB:   
    case GFX_SURFACE_CLUT4BPP_AYCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 4;      // 4 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_GI_CONTROL_BLOCK_T) + 16*sizeof(STB_OSD_GI_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        break;

    case GFX_SURFACE_CLUT4BPPP_ARGB:  
    case GFX_SURFACE_CLUT4BPPP_AYCBCR:
        pSurface->plane[0].uAllocWidth = uWidth/2;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 4;      // 4 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_GI_CONTROL_BLOCK_T) + 16*sizeof(STB_OSD_GI_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        break;

    case GFX_SURFACE_CLUT2BPP_ARGB:   
    case GFX_SURFACE_CLUT2BPP_AYCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 2;      // 4 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_GI_CONTROL_BLOCK_T) + 4*sizeof(STB_OSD_GI_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        break;

    case GFX_SURFACE_CLUT2BPPP_ARGB:   
    case GFX_SURFACE_CLUT2BPPP_AYCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth/2;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 2;      // 4 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_GI_CONTROL_BLOCK_T) + 4*sizeof(STB_OSD_GI_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        break;


    case GFX_SURFACE_CURSOR4BPP_RGB:   
    case GFX_SURFACE_CURSOR4BPP_YCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 4;      // 4 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_CURSOR_CONTROL_BLOCK_T) + 16*sizeof(STB_OSD_C_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        cursor = 1;
        break;

    case GFX_SURFACE_CURSOR4BPPP_RGB:   
    case GFX_SURFACE_CURSOR4BPPP_YCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth/2;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 4;      // 4 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_CURSOR_CONTROL_BLOCK_T) + 16*sizeof(STB_OSD_C_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        cursor = 1;
        break;

    case GFX_SURFACE_CURSOR2BPP_RGB:   
    case GFX_SURFACE_CURSOR2BPP_YCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 4;      // 4 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_CURSOR_CONTROL_BLOCK_T) + 4*sizeof(STB_OSD_C_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        cursor = 1;
        break;

    case GFX_SURFACE_CURSOR2BPPP_RGB:   
    case GFX_SURFACE_CURSOR2BPPP_YCBCR: 
        pSurface->plane[0].uAllocWidth = uWidth/2;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 4;      // 4 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_CURSOR_CONTROL_BLOCK_T) + 4*sizeof(STB_OSD_C_PALETTE_T);
        pSurface->pPalette = (GFX_PALETTE_T *)1;  // we have palette
        cursor = 1;
        break;

    case GFX_SURFACE_AYCBCR_422_8888: 
        // A
        pSurface->plane[2].uAllocWidth = uWidth;
        pSurface->plane[2].uAllocHeight = uHeight;
        pSurface->plane[2].uPixelSize = 8;      // 8 bits
        pSurface->plane[2].uPixelJustify = 2;    // on 2 pixel boundary
        pSurface->plane[2].uBytePerLine = uBytePerLine;
        pSurface->plane[2].a.uNumbits = 8;
        pSurface->plane[2].r.uNumbits = 0;
        pSurface->plane[2].g.uNumbits = 0;
        pSurface->plane[2].b.uNumbits = 0;
        pSurface->plane[2].a.uOffset = 0;
        pSurface->plane[2].r.uOffset = 0;
        pSurface->plane[2].g.uOffset = 0;
        pSurface->plane[2].b.uOffset = 0;
        twinheader = 1;
        // fall through
    case GFX_SURFACE_YCBCR_422_888:
        // Y
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 8;      // 8 bits
        pSurface->plane[0].uPixelJustify = 2;    // on 2 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 0;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 0;
        pSurface->plane[0].b.uNumbits = 0;
        pSurface->plane[0].a.uOffset = 0;
        pSurface->plane[0].r.uOffset = 0;
        pSurface->plane[0].g.uOffset = 0;
        pSurface->plane[0].b.uOffset = 0;
        // UV
        pSurface->plane[1].uAllocWidth = uWidth;
        pSurface->plane[1].uAllocHeight = uHeight;
        pSurface->plane[1].uPixelSize = 8;      // 8 bit for U/V
        pSurface->plane[1].uPixelJustify = 2;    // on 1 pixel boundary
        pSurface->plane[1].uBytePerLine = uBytePerLine;
        pSurface->plane[1].a.uNumbits = 0;
        pSurface->plane[1].r.uNumbits = 0;
        pSurface->plane[1].g.uNumbits = 8;
        pSurface->plane[1].b.uNumbits = 8;
        pSurface->plane[1].a.uOffset = 0;
        pSurface->plane[1].r.uOffset = 0;
        pSurface->plane[1].g.uOffset = 0;
        pSurface->plane[1].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_GI_CONTROL_BLOCK_T);
        break;

    case GFX_SURFACE_AYCBCR_420_8888: 
        // A
        pSurface->plane[2].uAllocWidth = uWidth;
        pSurface->plane[2].uAllocHeight = uHeight;
        pSurface->plane[2].uPixelSize = 8;      // 8 bits
        pSurface->plane[2].uPixelJustify = 2;    // on 2 pixel boundary
        pSurface->plane[2].uBytePerLine = uBytePerLine;
        pSurface->plane[2].a.uNumbits = 0;
        pSurface->plane[2].r.uNumbits = 0;
        pSurface->plane[2].g.uNumbits = 0;
        pSurface->plane[2].b.uNumbits = 0;
        pSurface->plane[2].a.uOffset = 0;
        pSurface->plane[2].r.uOffset = 0;
        pSurface->plane[2].g.uOffset = 0;
        pSurface->plane[2].b.uOffset = 0;
        twinheader = 1;
        // fall through
    case GFX_SURFACE_YCBCR_420_888:
        // Y
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 8;      // 8 bits
        pSurface->plane[0].uPixelJustify = 2;    // on 2 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 0;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 0;
        pSurface->plane[0].b.uNumbits = 0;
        pSurface->plane[0].a.uOffset = 0;
        pSurface->plane[0].r.uOffset = 0;
        pSurface->plane[0].g.uOffset = 0;
        pSurface->plane[0].b.uOffset = 0;
        // UV
        pSurface->plane[1].uAllocWidth = uWidth;
        pSurface->plane[1].uAllocHeight = uHeight/2;
        pSurface->plane[1].uPixelSize = 8;      // 8 bit for U/V bits
        pSurface->plane[1].uPixelJustify = 2;    // on 1 pixel boundary
        pSurface->plane[1].uBytePerLine = uBytePerLine;
        pSurface->plane[1].a.uNumbits = 0;
        pSurface->plane[1].r.uNumbits = 0;
        pSurface->plane[1].g.uNumbits = 8;
        pSurface->plane[1].b.uNumbits = 8;
        pSurface->plane[1].a.uOffset = 0;
        pSurface->plane[1].r.uOffset = 0;
        pSurface->plane[1].g.uOffset = 0;
        pSurface->plane[1].b.uOffset = 0;
        uHeaderSize = sizeof(STB_OSD_GI_CONTROL_BLOCK_T);
        break;

    case GFX_SURFACE_ARGB_8888:       
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 32;      // 32 bits
        pSurface->plane[0].uPixelJustify = 1;    // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = 0;
        break;

    case GFX_SURFACE_RAW8BPP :
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 8;      // 8 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = 0;
        pSurface->pPalette = (GFX_PALETTE_T *)0;  // we don't have palette
        break;

    case GFX_SURFACE_RAW1BPP :
        pSurface->plane[0].uAllocWidth = uWidth;
        pSurface->plane[0].uAllocHeight = uHeight;
        pSurface->plane[0].uPixelSize = 1;      // 1 bits
        pSurface->plane[0].uPixelJustify = 1;   // on 1 pixel boundary
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        pSurface->plane[0].a.uNumbits = 8;
        pSurface->plane[0].r.uNumbits = 8;
        pSurface->plane[0].g.uNumbits = 8;
        pSurface->plane[0].b.uNumbits = 8;
        pSurface->plane[0].a.uOffset = 24;  // for palette entry
        pSurface->plane[0].r.uOffset = 16;
        pSurface->plane[0].g.uOffset = 8;
        pSurface->plane[0].b.uOffset = 0;
        uHeaderSize = 0;
        pSurface->pPalette = (GFX_PALETTE_T *)0;  // we don't have palette
        break;
    default:
        PDEBUG("Invalid plane config 0x%8.8x !\n", uPlaneConfig);
        return -1;
    }

    PDEBUG("uPlaneConfig = %08x \n", uPlaneConfig);
    PDEBUG("Parameter: numbuf = %d, width=%d, height=%d\n", uNumBuf, uWidth, uHeight);
    // __os_alloc_physical_heap_walk();

    uHeaderSize = ((uHeaderSize + OSD_LINK_ALIGNMENT-1) / OSD_LINK_ALIGNMENT) * OSD_LINK_ALIGNMENT;
    if(twinheader)
    {
        uAllocSize = ((uHeaderSize + OSD_BUFFER_ALIGNMENT-1) / OSD_BUFFER_ALIGNMENT)*2;
    }
    else
        uAllocSize = (uHeaderSize + OSD_BUFFER_ALIGNMENT-1) / OSD_BUFFER_ALIGNMENT;

    PDEBUG("uHeaderSize = %08x, uAllocSize = %08x\n", uHeaderSize, uAllocSize);

    for(i=0; i<uNumBuf; i++)
    {
        UINT uBufferSize = (pSurface->plane[i].uAllocHeight*pSurface->plane[i].uBytePerLine + OSD_BUFFER_ALIGNMENT-1) / OSD_BUFFER_ALIGNMENT;
        pSurface->plane[i].uBMLA = uAllocSize * OSD_BUFFER_ALIGNMENT;
        pSurface->plane[i].uAllocSize = uBufferSize*OSD_BUFFER_ALIGNMENT;
        PDEBUG("Plane %d BMLA = %08x\n", i, pSurface->plane[i].uBMLA);
        uAllocSize += uBufferSize;
    }

    uAllocSize *= OSD_BUFFER_ALIGNMENT;

	//PDEBUG("Allocate size = %d bytes\n", uAllocSize);

	 if(__STB_GRAPHICS_MEM_SIZE>0)  //we are using a specific graphics memory pool
    {
       pSurface->hBuffer = pm_alloc_physical_justify(gpGraphicsPMRoot,uAllocSize, OSD_ADDRESS_ALIGNMENT);
       PDEBUG("gfx_osi_create_surface: allocated pSurface->hBuffer addr = 0c%8.8x (logical)= 0x%8.8x (physical)=0x%8.8x\n",pSurface->hBuffer,pSurface->hBuffer->pLogical,pSurface->hBuffer->uPhysical);        
    }           
    else
    { 
       pSurface->hBuffer = os_alloc_physical_justify(uAllocSize, OSD_ADDRESS_ALIGNMENT);
    }


    if(NULL == pSurface->hBuffer)
    {
        PDEBUG("Failed to alloc %d buffer for new surface!\n", uAllocSize);
        pSurface->hBuffer = NULL;
        return -1;
    }

    PDEBUG("Buffer allocated = %08x \n", pSurface->hBuffer);

    if(pSurface->pPalette) // check if we should adjust palette
    {
        uNumPal = 1<<pSurface->plane[0].uPixelSize;
        pSurface->pPalette = MALLOC(uNumPal * sizeof(GFX_PALETTE_T));
        if(NULL == pSurface->pPalette)
        {
            PDEBUG("Failed to alloc %d buffer for new surface!\n", uNumPal* sizeof(GFX_PALETTE_T));
            if(__STB_GRAPHICS_MEM_SIZE>0)  //we are using a specific graphics memory pool
            {
               pm_free_physical(gpGraphicsPMRoot,pSurface->hBuffer);
            }           
            else
            {
              os_free_physical(pSurface->hBuffer);
            }  
            pSurface->hBuffer = NULL;
            return -1;
        }
        PDEBUG("Palette allocated = %08x \n", pSurface->pPalette);
    }


    pSurface->uPlaneConfig = uPlaneConfig;
    pSurface->uAttr = __GFX_SURFACE_ALLOC;

    if(__STB_GRAPHICS_MEM_SIZE>0)  //we are using a specific graphics memory pool
       pSurface->uBufferBase = os_get_physical_address(pSurface->hBuffer)-__STB_GRAPHICS_MEM_BASE_ADDR+guGraphicsVideoOffset;
    else
       pSurface->uBufferBase = os_get_physical_address(pSurface->hBuffer) - os_get_physical_base() + guGraphicsVideoOffset;

    PDEBUG("Buffer base = %08x \n", pSurface->uBufferBase);

    for(i=0; i<uNumBuf; i++)
    {
        pSurface->plane[i].pBuffer = pSurface->plane[i].uBMLA + (BYTE *)os_get_logical_address(pSurface->hBuffer);
        PDEBUGE("Plane %d buffer = %08x %08x + %08x\n", i, pSurface->plane[i].pBuffer, pSurface->plane[i].uBMLA, os_get_logical_address(pSurface->hBuffer));
    }

    PDEBUG("Init OSD control headers\n");
    pSurface->attachedDev = GFX_VDEV_NULL;

    // the initial clip region
    pSurface->clip.x1 = pSurface->clip.y1 = 0;
    pSurface->clip.x2 = uWidth-1;
    pSurface->clip.y2 = uHeight-1;

    // initialize bounding clip
    pSurface->bound = pSurface->clip;


    // then we will config for the initial parameters for osd control block
    // although this is not nessesary for normal gfx surface, but we just do it.
    if(cursor)
    {
        STB_OSD_CURSOR_CONTROL_BLOCK_T *pCntl;

        pSurface->uAttr |= __GFX_SURFACE_OSDCURSOR;
        pCntl = (STB_OSD_CURSOR_CONTROL_BLOCK_T *)os_get_logical_address(pSurface->hBuffer);
        PDEBUG("pCntl = %08x\n", pCntl);
        pSurface->pPal = (BYTE *)pCntl + sizeof(STB_OSD_CURSOR_CONTROL_BLOCK_T);
        PDEBUG("pSurface->pPal = %08x\n", pSurface->pPal);

        _OS_MEMSET(pCntl, 0, sizeof(STB_OSD_CURSOR_CONTROL_BLOCK_T));
        gfx_osi_reset_surface_palette(pSurface);
        pCntl->region_hsize = pSurface->plane[0].uAllocWidth * pSurface->plane[0].uPixelSize / 32;
        pCntl->color_resolution = (uPlaneConfig&0x01) ? 1 : 0;
        pCntl->region_vsize = pSurface->plane[0].uAllocHeight/2;
        pCntl->pixel_resolution = (uPlaneConfig&0x04) ? 1 : 0;
        pCntl->extlink_addr = pSurface->plane[0].uBMLA >> 2;
    }
    else
    {
        if(uHeaderSize > 0)
        {
            STB_OSD_GI_CONTROL_BLOCK_T *pCntl;
            pCntl = (STB_OSD_GI_CONTROL_BLOCK_T *)os_get_logical_address(pSurface->hBuffer);
            PDEBUG("pCntl = %08x\n", pCntl);
            _OS_MEMSET(pCntl, 0, sizeof(STB_OSD_GI_CONTROL_BLOCK_T));
            if(pSurface->pPalette)
            {
                pSurface->pPal = (BYTE *)pCntl + sizeof(STB_OSD_GI_CONTROL_BLOCK_T);
                PDEBUG("pSurface->pPal = %08x\n", pSurface->pPal);
                gfx_osi_reset_surface_palette(pSurface);
            }
            if(IS_OSD_SURFACE_CLUT(uPlaneConfig))
                pCntl->color_table_update = 1;
            else
                pCntl->color_table_update = 0;  // direct color mode, default to no per pixel blend control
            pCntl->region_hsize = pSurface->plane[0].uAllocWidth * pSurface->plane[0].uPixelSize / 32;
            pCntl->high_color = (uPlaneConfig&0x02) ? 1 : 0;
            pCntl->link_addr = 0;   // self loop
            pCntl->color_resolution = (uPlaneConfig&0x01) ? 1 : 0;
            pCntl->region_vsize = pSurface->plane[0].uAllocHeight/2;
            pCntl->pixel_resolution = (uPlaneConfig&0x04) ? 1 : 0;
            pCntl->extlink_addr = pSurface->plane[0].uBMLA >> 2;
            pCntl->h_ext_2 = 1;     // we have ext 2
            pCntl->h_fir_scaling = 0xF; // enable horizontal scaling; use custom ratio -- BJC 102102
            if(uNumBuf > 1)
            {
                pCntl->chroma_lnk_en = 1;
                pCntl->chroma_lnk = pSurface->plane[1].uBMLA >> 2;
            }
            if(twinheader && uNumBuf > 2)   // only for 32bit color mode
            {
                pSurface->uBufferBase2 = pSurface->uBufferBase + 
                    ((uHeaderSize + OSD_BMLA_ALIGNMENT-1) / OSD_BMLA_ALIGNMENT) * OSD_BMLA_ALIGNMENT;
                pCntl = (STB_OSD_GI_CONTROL_BLOCK_T *)((BYTE *)os_get_logical_address(pSurface->hBuffer)
                    + pSurface->uBufferBase2 - pSurface->uBufferBase);     // move to next header
                PDEBUG("Twinheader pCntl= %08x, base= %08x\n", pCntl, pSurface->uBufferBase2);
                _OS_MEMSET(pCntl, 0, sizeof(STB_OSD_GI_CONTROL_BLOCK_T));
                pCntl->region_hsize = pSurface->plane[2].uAllocWidth * pSurface->plane[2].uPixelSize / 32;
                pCntl->high_color = (uPlaneConfig&0x20) ? 1 : 0;
                pCntl->link_addr = 0;   // self loop
                pCntl->color_resolution = (uPlaneConfig&0x10) ? 1 : 0;
                pCntl->region_vsize = pSurface->plane[2].uAllocHeight/2;
                pCntl->pixel_resolution = (uPlaneConfig&0x40) ? 1 : 0;
                pCntl->extlink_addr = pSurface->plane[2].uBMLA >> 2;
                pCntl->h_ext_2 = 1;     // we have ext 2
                pSurface->twinheader = 1;
            }
        }
    }

    PDEBUGE("Surface (x,y,bpl,bpp)= %d [%8.8x], %d [%8.8x], %d [%8.8x], %d\n", 
        pSurface->plane[0].uAllocWidth,  pSurface->plane[0].uAllocWidth,
        pSurface->plane[0].uAllocHeight, pSurface->plane[0].uAllocHeight,
        pSurface->plane[0].uBytePerLine, pSurface->plane[0].uBytePerLine, 
        pSurface->plane[0].uPixelSize);


    return 0;
}


INT gfx_osi_destroy_surface(GFX_SURFACE_T * pSurface)
{
    // first, a rough check of the parameters
    if(!gfx_osi_pSurface_valid(pSurface) || (GFX_VDEV_NULL != pSurface->attachedDev)) 
    {
        PDEBUG("Invalid parameter or attached surface !\n");
        return -1;    // we cann't do it
    }

    if(pSurface->hBuffer) 
    {
       if(__STB_GRAPHICS_MEM_SIZE>0)  //we are using a specific graphics memory pool
          pm_free_physical(gpGraphicsPMRoot,pSurface->hBuffer);
       else
          os_free_physical(pSurface->hBuffer);
    }

    if(pSurface->pPalette) FREE(pSurface->pPalette);

    gfx_osi_init_surface_t(pSurface);

    return 0;
}

INT gfx_osi_reconfig_surface_dimension(GFX_SURFACE_T *pSurface, UINT uNewWidth, UINT uNewHeight)
{
    UINT uNumBuf, uBytePerLine;
    UINT cursor=0, twinheader=0;

    // first, a rough check of the parameters
    if(!gfx_osi_pSurface_valid(pSurface)) 
    {
        PDEBUG("Invalid parameter!\n");
        return -1;    // we cann't do it
    }

    if( uNewWidth < 4 || uNewHeight < 2
        || uNewWidth > GFX_SURFACE_MAX_WIDTH || uNewHeight > GFX_SURFACE_MAX_HEIGHT) 
    {
        PDEBUG("Invalid parameter: width=%d, height=%d !\n", uNewWidth, uNewHeight);
        return -1;    // we cann't do it
    }

    __gfx_osi_adjust_surface_wh(pSurface->uPlaneConfig, &uNewWidth, &uNewHeight, &uBytePerLine);
    
    if(uBytePerLine * uNewHeight > pSurface->plane[0].uAllocSize)
    {
        PDEBUG("New size too large: width=%d, height=%d !\n", uNewWidth, uNewHeight);
        return -1;    // we cann't do it
    }

    uNumBuf = GET_GFX_SURFACE_SUBPLANES(pSurface->uPlaneConfig);

    //ok, we now try to alloc the surface
    switch(pSurface->uPlaneConfig)
    {

    case GFX_SURFACE_CURSOR4BPP_RGB:   
    case GFX_SURFACE_CURSOR4BPP_YCBCR: 
    case GFX_SURFACE_CURSOR2BPP_RGB:   
    case GFX_SURFACE_CURSOR2BPP_YCBCR: 
        cursor = 1;

    case GFX_SURFACE_CLUT8BPP_ARGB:
    case GFX_SURFACE_CLUT8BPP_AYCBCR: 
    case GFX_SURFACE_CLUT4BPP_ARGB:   
    case GFX_SURFACE_CLUT4BPP_AYCBCR: 
    case GFX_SURFACE_CLUT2BPP_ARGB:   
    case GFX_SURFACE_CLUT2BPP_AYCBCR: 
    case GFX_SURFACE_ARGB_8888:       
    case GFX_SURFACE_RAW8BPP :
    case GFX_SURFACE_RAW1BPP :
        pSurface->plane[0].uAllocWidth = uNewWidth;
        pSurface->plane[0].uAllocHeight = uNewHeight;
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        break;

    case GFX_SURFACE_CURSOR4BPPP_RGB:   
    case GFX_SURFACE_CURSOR4BPPP_YCBCR: 
    case GFX_SURFACE_CURSOR2BPPP_RGB:   
    case GFX_SURFACE_CURSOR2BPPP_YCBCR: 
        cursor = 1;

    case GFX_SURFACE_CLUT8BPPP_ARGB:
    case GFX_SURFACE_CLUT8BPPP_AYCBCR: 
    case GFX_SURFACE_CLUT4BPPP_ARGB:  
    case GFX_SURFACE_CLUT4BPPP_AYCBCR:
    case GFX_SURFACE_CLUT2BPPP_ARGB:   
    case GFX_SURFACE_CLUT2BPPP_AYCBCR: 
        pSurface->plane[0].uAllocWidth = uNewWidth/2;
        pSurface->plane[0].uAllocHeight = uNewHeight;
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        break;


    case GFX_SURFACE_AYCBCR_422_8888: 
        // A
        pSurface->plane[2].uAllocWidth = uNewWidth;
        pSurface->plane[2].uAllocHeight = uNewHeight;
        pSurface->plane[2].uBytePerLine = uBytePerLine;
        twinheader = 1;
        // fall through
    case GFX_SURFACE_YCBCR_422_888:
        // Y
        pSurface->plane[0].uAllocWidth = uNewWidth;
        pSurface->plane[0].uAllocHeight = uNewHeight;
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        // UV
        pSurface->plane[1].uAllocWidth = uNewWidth;
        pSurface->plane[1].uAllocHeight = uNewHeight;
        pSurface->plane[1].uBytePerLine = uBytePerLine;
        break;

    case GFX_SURFACE_AYCBCR_420_8888: 
        // A
        pSurface->plane[2].uAllocWidth = uNewWidth;
        pSurface->plane[2].uAllocHeight = uNewHeight;
        pSurface->plane[2].uBytePerLine = uBytePerLine;
        twinheader = 1;
        // fall through
    case GFX_SURFACE_YCBCR_420_888:
        // Y
        pSurface->plane[0].uAllocWidth = uNewWidth;
        pSurface->plane[0].uAllocHeight = uNewHeight;
        pSurface->plane[0].uBytePerLine = uBytePerLine;
        // UV
        pSurface->plane[1].uAllocWidth = uNewWidth;
        pSurface->plane[1].uAllocHeight = uNewHeight/2;
        pSurface->plane[1].uBytePerLine = uBytePerLine;
        break;

    default:
        PDEBUG("Invalid plane config 0x%8.8x !\n", pSurface->uPlaneConfig);
        return -1;
    }

    PDEBUG("Parameter: numbuf = %d, width=%d, height=%d\n", uNumBuf, uNewWidth, uNewHeight);


    // the initial clip region
    pSurface->clip.x1 = pSurface->clip.y1 = 0;
    pSurface->clip.x2 = uNewWidth-1;
    pSurface->clip.y2 = uNewHeight-1;

    // initialize bounding clip
    pSurface->bound = pSurface->clip;


    // then we will config for the initial parameters for osd control block
    // although this is not nessesary for normal gfx surface, but we just do it.
    if(cursor)
    {
        STB_OSD_CURSOR_CONTROL_BLOCK_T *pCntl;

        pCntl = (STB_OSD_CURSOR_CONTROL_BLOCK_T *)os_get_logical_address(pSurface->hBuffer);
        PDEBUG("pCntl = %08x\n", pCntl);
        pCntl->region_hsize = pSurface->plane[0].uAllocWidth * 2 / pSurface->plane[0].uPixelSize;
        pCntl->region_vsize = pSurface->plane[0].uAllocHeight/2;
    }
    else
    {
        if(IS_SURFACE_OSD_COMP(pSurface->uPlaneConfig))
        {
            STB_OSD_GI_CONTROL_BLOCK_T *pCntl;
            pCntl = (STB_OSD_GI_CONTROL_BLOCK_T *)os_get_logical_address(pSurface->hBuffer);
            PDEBUG("pCntl = %08x\n", pCntl);
            pCntl->region_hsize = pSurface->plane[0].uAllocWidth * 2 / pSurface->plane[0].uPixelSize;
            pCntl->region_vsize = pSurface->plane[0].uAllocHeight/2;
            if(twinheader && uNumBuf > 2)   // only for 32bit color mode
            {
                pCntl = (STB_OSD_GI_CONTROL_BLOCK_T *)((BYTE *)os_get_logical_address(pSurface->hBuffer)
                    + pSurface->uBufferBase2 - pSurface->uBufferBase);     // move to next header
                PDEBUG("Twinheader pCntl= %08x, base= %08x\n", pCntl, pSurface->uBufferBase2);
                pCntl->region_hsize = pSurface->plane[2].uAllocWidth * 2 / pSurface->plane[2].uPixelSize;
                pCntl->region_vsize = pSurface->plane[2].uAllocHeight/2;
            }
        }
    }

    PDEBUGE("Surface (x,y,bpl,bpp)= %d [%8.8x], %d [%8.8x], %d [%8.8x], %d\n", 
        pSurface->plane[0].uAllocWidth,  pSurface->plane[0].uAllocWidth,
        pSurface->plane[0].uAllocHeight, pSurface->plane[0].uAllocHeight,
        pSurface->plane[0].uBytePerLine, pSurface->plane[0].uBytePerLine, 
        pSurface->plane[0].uPixelSize);


    return 0;
}


INT gfx_osi_get_surface_physical_parm(GFX_SURFACE_PHYSICAL_PARM_T *pParm, GFX_SURFACE_T *pSurface)
{
    UINT uNumBuf, i;
    // first, a rough check of the parameters
    if(!gfx_osi_pSurface_valid(pSurface) || NULL == pParm) 
    {
        PDEBUG("Invalid parm !\n");
        return -1;    // we cann't do it
    }

    uNumBuf = GET_GFX_SURFACE_SUBPLANES(pSurface->uPlaneConfig);

    _OS_MEMSET(pParm, 0, sizeof(GFX_SURFACE_PHYSICAL_PARM_T));

    pParm->uGFXBufferPhysicalBase = os_get_physical_address(pSurface->hBuffer);
    pParm->uGFXBufferPhysicalSize = os_get_actual_physical_size(pSurface->hBuffer);

    for(i=0; i<uNumBuf; i++) 
    {
        pParm->uPlanePhysicalBaseAddr[i] = pSurface->plane[i].uBMLA + pParm->uGFXBufferPhysicalBase;
        pParm->uPlanePhysicalOffset[i] = 0;
        pParm->uPlanePhysicalSize[i] = ((pSurface->plane[i].uAllocHeight*pSurface->plane[i].uBytePerLine + OSD_BUFFER_ALIGNMENT - 1) / OSD_BUFFER_ALIGNMENT)*OSD_BUFFER_ALIGNMENT ;
    }
    return 0;
}



//////////////////////////////////////////////////////////////////////////////
// Color space conversion helpers
//////////////////////////////////////////////////////////////////////////////

void gfx_osi_rgb2ycbcr(BYTE r, BYTE g,  BYTE b,  BYTE *y, BYTE *cb, BYTE *cr)
{
    // Y  =  0.257*R + 0.504*G + 0.098*B + 16
    // CB = -0.148*R - 0.291*G + 0.439*B + 128
    // CR =  0.439*R - 0.368*G - 0.071*B + 128
    *y  = (BYTE)((8432*(ULONG)r + 16425*(ULONG)g + 3176*(ULONG)b + 16*32768)>>15);
    *cb = (BYTE)((128*32768 + 14345*(ULONG)b - 4818*(ULONG)r -9527*(ULONG)g)>>15);
    *cr = (BYTE)((128*32768 + 14345*(ULONG)r - 12045*(ULONG)g-2300*(ULONG)b)>>15);
}

void gfx_osi_ycbcr2rgb(BYTE y, BYTE cb,  BYTE cr,  BYTE *r, BYTE *g, BYTE *b)
{
    // R = 1.164*(Y - 16) + 1.596*(CR - 128)
    // G = 1.164*(Y - 16) - 0.813*(CR - 128) - 0.391*(CB - 128)
    // B = 1.164*(Y - 16)                    + 2.018*(CB - 128)

    // R = 1.164*Y + 1.596*CR - 222.4[1.164*16 + 1.592*128]
    // G = 1.164*Y + 135.488[0.813*128 + 0.391*128 - 1.164*16] - 0.813*CR - 0.391*CB
    // B = 1.164*Y + 2.018*CB - 276.928[1.164*16 + 2.018*128]
    int rr, gg, bb, my;

    my = 38142*(int)y;

    rr = (my + 52298*(int)cr - 7287603)>>15;

    if(rr > 255) *r = 255;
    else if(rr < 0) *r = 0;
    else *r = (BYTE)rr;

    gg = (my + 4439671 - 26640*cr - 12812*cb)>> 15;

    if(gg > 255) *g = 255;
    else if(gg < 0) *g = 0;
    else *g = (BYTE)gg;
    
    bb = (my + 66126*cb - 9074377)>>15;

    if(bb > 255) *b = 255;
    else if(bb < 0) *b = 0;
    else *b = (BYTE)bb;

}

