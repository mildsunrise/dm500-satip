//vulcan/drv/gfx/osd_osi.c
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
//  OSI function of OSD controls 
//Revision Log:   
//  Sept/28/2001                          Created by YYD
//  Oct/17/2001            Merged with GFX driver by YYD
//  Jun/05/2002            Ported to Vulcan OSD   by YYD
//  Oct/21/2002            Set CHFSR to 1x horizontal scaling by default
//                         Add handling of OSD_DISP_CNTL_ANIM,
//                           OSD_DISP_CNTL_ANIMR, and OSD_DISP_CNTL_CHFSR in
//                           osd_osi_set_display_parm() and 
//                           osd_osi_get_display_parm()                -- BJC

#include "os/os-types.h"
#include "os/pm-alloc.h"
#include "os/os-sync.h"
#include "os/os-generic.h"
#include "gfx_surface.h"
#include "osd_atom.h"
#include "osd_osi.h"
#include "osd_local.h"

// #define __OSD_OSI_DEBUG
#ifdef __OSD_OSI_DEBUG
    #define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif

//////////////////////////////////////////////////////////////////////////////
// Graphics device interfaces
//////////////////////////////////////////////////////////////////////////////

#define __OSD_GRAPH_DEVICE_INIT       (0x00000001)
#define __OSD_GRAPH_DEVICE_ENABLED    (0x00000002)

typedef struct __OSD_GRAPH_DEVICE_STRUCTURE__
{
    GFX_SURFACE_T *pSurface;
    UINT uAttr;
} OSD_GRAPH_DEVICE_T;


static OSD_GRAPH_DEVICE_T gOSDDev[GFX_NUMOF_VISUAL_DEVICES];

static MUTEX_T *gOSDMutex;

static INT osd_osi_init_device_hw(GFX_VISUAL_DEVICE_ID_T graphDev)
{

    gOSDDev[graphDev].uAttr = __OSD_GRAPH_DEVICE_INIT;
    gOSDDev[graphDev].pSurface = NULL;

    switch(graphDev)
    {
    case GFX_VDEV_OSDCUR:
        osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_CURSOR);
        osd_atom_set_plane_control(OSD_PLANE_A_BMLA,  OSD_PLANE_CURSOR|0);
        osd_atom_set_plane_control(OSD_PLANE_A_422_AVERAGING, OSD_PLANE_CURSOR|0);

        break;
    case GFX_VDEV_OSDGFX:
        osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_GRAPHICS);
        osd_atom_set_plane_control(OSD_PLANE_A_BMLA,  OSD_PLANE_GRAPHICS|0);
        osd_atom_set_plane_control(OSD_PLANE_A_422_AVERAGING, OSD_PLANE_GRAPHICS|0);
        osd_atom_set_plane_control(OSD_PLANE_G_PER_COLOR_BLEND_MODE, OSD_PLANE_GRAPHICS | 0);
        break;

    case GFX_VDEV_OSDIMG:
        osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_IMAGE);
        osd_atom_set_plane_control(OSD_PLANE_A_BMLA,  OSD_PLANE_IMAGE|0);
        break;

    default:
        PDEBUG("Invalid device ID!\n");
    }

    return 0;
}


static INT osd_osi_deinit_device_hw(GFX_VISUAL_DEVICE_ID_T graphDev)
{

    if(gOSDDev[graphDev].pSurface) 
    {
        GFX_SURFACE_T *pCurr = gOSDDev[graphDev].pSurface;
        while(pCurr)
        {
            GFX_SURFACE_T *pNext = pCurr->pNextAttach;
            if(graphDev != pCurr->attachedDev)
            {
                PDEBUG("Attached surface shows that it is not attached to me!\n");
            }
            // anyway, detach it
            pCurr->attachedDev = GFX_VDEV_NULL;
            pCurr->pNextAttach = NULL;
            pCurr = pNext;
        }
        gOSDDev[graphDev].pSurface = NULL;
    }
    gOSDDev[graphDev].uAttr = 0;

    switch(graphDev)
    {
    case GFX_VDEV_OSDCUR:
        osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_CURSOR);
        break;

    case GFX_VDEV_OSDGFX:
        osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_GRAPHICS);
        osd_atom_set_plane_control(OSD_PLANE_G_PER_COLOR_BLEND_MODE, OSD_PLANE_GRAPHICS | 0);
        break;

    case GFX_VDEV_OSDIMG:
        osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_IMAGE);
        break;

    default:
        PDEBUG("Invalid device ID!\n");
    }

    return 0;
}

INT osd_osi_init(void)
{
    int i;

    if(gOSDMutex) return 0; // already done

    gOSDMutex =  os_create_mutex ();

    if(!gOSDMutex)
    {
        PDEBUG("Failed to create lock for resources!\n");
        return -1;
    }
    /////////////////////////
    if(os_get_mutex(gOSDMutex))
    {
        PDEBUG("Failed on mutex!\n");
        os_delete_mutex(gOSDMutex);
        gOSDMutex = NULL;
        return -1;
    }
    
    _OS_MEMSET(&gOSDDev[0], 0,sizeof(gOSDDev));

    for(i=0; i<GFX_NUMOF_VISUAL_DEVICES; i++)
        osd_osi_init_device_hw((GFX_VISUAL_DEVICE_ID_T)i);

    osd_atom_set_display_control(OSD_CNTL_AFVP,     0); // Anti-flicker video plane, 0 disable, 1 enable
    osd_atom_set_display_control(OSD_CNTL_EDAF,     0); // Enable display anti-flicker, 0 disable, 1 enable
    osd_atom_set_display_control(OSD_CNTL_AFDT,     0); // Anti-flicker detection threshold, 2 bits attr
    osd_atom_set_display_control(OSD_CNTL_VPAFC,    0); // Video plane anti-flicker correction, 2 bits attr
    osd_atom_set_display_control(OSD_CNTL_E32BCO,   0); 
    osd_atom_set_display_control(OSD_CNTL_ANIM,     0);// Animation mode, 0 no, 1 yes
    osd_atom_set_display_control(OSD_CNTL_ANIMR,    0);// Animation rate, 3 bits attr
    osd_atom_set_display_control(OSD_CNTL_CHFSR,    256);// Custom horizontal FIR scaling ratio, 9 bits attr -- BJC 102102
    osd_atom_set_display_control(OSD_CNTL_BACKCOLOR,0x0088);// black color
//    osd_atom_set_display_control(OSD_CNTL_BACKCOLOR,0x2222);// NOT black color

    os_release_mutex(gOSDMutex);
    /////////////////////////

    return gOSDMutex != NULL ? 0 : -1;
}

INT osd_osi_deinit(void)
{
    int i;
    MUTEX_T *mutex;
    
    if(!gOSDMutex) return 0; // already done

    // wait for any unfinished operation
    /////////////////////////
    if(os_get_mutex(gOSDMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }
    mutex = gOSDMutex;  // save first
    gOSDMutex = NULL;       // no one can enter correctly now

    for(i=0; i<GFX_NUMOF_VISUAL_DEVICES; i++)
        osd_osi_deinit_device_hw((GFX_VISUAL_DEVICE_ID_T)i);
    
    _OS_MEMSET(&gOSDDev[0], 0, sizeof(gOSDDev));

    osd_atom_set_display_control(OSD_CNTL_AFVP,     0); // Anti-flicker video plane, 0 disable, 1 enable
    osd_atom_set_display_control(OSD_CNTL_EDAF,     0); // Enable display anti-flicker, 0 disable, 1 enable
    osd_atom_set_display_control(OSD_CNTL_AFDT,     0); // Anti-flicker detection threshold, 2 bits attr
    osd_atom_set_display_control(OSD_CNTL_VPAFC,    0); // Video plane anti-flicker correction, 2 bits attr
    osd_atom_set_display_control(OSD_CNTL_E32BCO,   0); 
    osd_atom_set_display_control(OSD_CNTL_ANIM,     0);// Animation mode, 0 no, 1 yes
    osd_atom_set_display_control(OSD_CNTL_ANIMR,    0);// Animation rate, 3 bits attr
    osd_atom_set_display_control(OSD_CNTL_CHFSR,    256);// Custom horizontal FIR scaling ratio, 9 bits attr -- BJC 102102
    osd_atom_set_display_control(OSD_CNTL_BACKCOLOR,0x0088);// black color

    os_release_mutex(mutex);
    /////////////////////////
    os_delete_mutex(mutex);

    return 0;
}

// local helper func
static void __update_device_surface(GFX_VISUAL_DEVICE_ID_T graphDev, GFX_SURFACE_T * pSurface)
{
    switch(graphDev)
    {
    case GFX_VDEV_OSDCUR:
        // set the buffer addresses for OSD
        osd_atom_set_base_address(OSD_ADDR_CURSOR_BASE, pSurface->uBufferBase >> 7);
        osd_atom_set_base_address(OSD_ADDR_CURSOR_LINK, (pSurface->uBufferBase&127) >> 5);
        osd_atom_set_plane_control(OSD_PLANE_A_BMLA, OSD_PLANE_CURSOR|1);
        osd_atom_set_plane_control(OSD_PLANE_A_ENABLE, OSD_PLANE_CURSOR);
        break;

    case GFX_VDEV_OSDGFX:
        // set the buffer addresses for OSD
        PDEBUG("OSD_ADDR_GRAPHICS_BASE is being set to 0x%8.8x...0x%8.8x\n",pSurface->uBufferBase ,pSurface->uBufferBase >> 7);
        osd_atom_set_base_address(OSD_ADDR_GRAPHICS_BASE, pSurface->uBufferBase >> 7);
        osd_atom_set_base_address(OSD_ADDR_GRAPHICS_LINK, (pSurface->uBufferBase&127) >> 5);
        osd_atom_set_plane_control(OSD_PLANE_A_BMLA, OSD_PLANE_GRAPHICS|1);
        osd_atom_set_plane_control(OSD_PLANE_A_ENABLE, OSD_PLANE_GRAPHICS);
        osd_atom_set_plane_control(OSD_PLANE_G_PER_COLOR_BLEND_MODE, OSD_PLANE_GRAPHICS | 1);
        if(pSurface->twinheader)
        {
            PDEBUG("TwinHeader surface, back ground will not be shown\n");
            osd_atom_set_base_address(OSD_ADDR_IMAGE_BASE, pSurface->uBufferBase2 >> 7);
            osd_atom_set_base_address(OSD_ADDR_IMAGE_LINK, (pSurface->uBufferBase2&127) >> 5);
            osd_atom_set_plane_control(OSD_PLANE_A_BMLA, OSD_PLANE_IMAGE|1);
            osd_atom_set_plane_control(OSD_PLANE_A_ENABLE, OSD_PLANE_IMAGE);   // should we ?
            osd_atom_set_display_control(OSD_CNTL_E32BCO, 1);
        }
        else
        {
            osd_atom_set_display_control(OSD_CNTL_E32BCO, 0);
            if(!IS_OSD_SURFACE_CLUT(pSurface->uPlaneConfig))
            {
                osd_atom_set_plane_control(OSD_PLANE_G_PER_COLOR_BLEND_MODE, OSD_PLANE_GRAPHICS | 0);
            }
        }
        break;

    case GFX_VDEV_OSDIMG:
        // set the buffer addresses for OSD
        if(osd_atom_get_display_control(OSD_CNTL_E32BCO))
        {
#if 1       // fixme: Sould we enable attaching of img while we use twinheader surface
            PDEBUG("We get TwinHeader surface attched. Attached IMG surface will not be seen!\n");
            return;
#else
            osd_atom_set_display_control(OSD_CNTL_E32BCO, 0);
#endif
        }
        osd_atom_set_base_address(OSD_ADDR_IMAGE_BASE, pSurface->uBufferBase >> 7);
        osd_atom_set_base_address(OSD_ADDR_IMAGE_LINK, (pSurface->uBufferBase&127) >> 5);
        osd_atom_set_plane_control(OSD_PLANE_A_BMLA, OSD_PLANE_IMAGE|1);
        osd_atom_set_plane_control(OSD_PLANE_A_ENABLE, OSD_PLANE_IMAGE);
        osd_atom_set_plane_control(OSD_PLANE_G_PER_COLOR_BLEND_MODE, OSD_PLANE_GRAPHICS | 0);
        break;

    default: // GFX_VDEV_NULL
	break;
    }
}


// attach a already attached surface will move it to top most.
INT osd_osi_attach_comp_gfx_surface(GFX_VISUAL_DEVICE_ID_T graphDev, GFX_SURFACE_T * pSurface)
{
    if(!gfx_osi_pSurface_valid(pSurface) || GFX_VDEV_NULL == graphDev) 
    {
        PDEBUG("Invalid surface or device %d!\n", (INT)graphDev);
        return -1;    // we cann't do it
    }
    if(!IS_SURFACE_OSD_COMP(pSurface->uPlaneConfig)) 
    {
        PDEBUG("The surface is incompatible with OSD device !\n");
        return -1;    // we cann't do it
    }

    if(GFX_VDEV_NULL != pSurface->attachedDev && graphDev != pSurface->attachedDev) 
    {
        PDEBUG("The surface is already attached to someone else !\n");
        return -1;    // we cann't do it
    }

    if(!(gOSDDev[graphDev].uAttr & __OSD_GRAPH_DEVICE_INIT)) 
    {
        PDEBUG("Device %d is not initlized!\n", (INT)graphDev);
        return -1;
    }

    // already attached device
    if(graphDev == pSurface->attachedDev)
    {
        /////////////////////////
        if(os_get_mutex(gOSDMutex))
        {
            PDEBUG("Failed on mutex!\n");
            return -1;
        }
        
        if(gOSDDev[graphDev].pSurface) 
        {
            GFX_SURFACE_T *pCurr = gOSDDev[graphDev].pSurface;
            GFX_SURFACE_T *pPrev = NULL;
            // look it up and make sure it is 
            while(pCurr && pCurr != pSurface) 
            {
                pPrev = pCurr;
                pCurr = pCurr->pNextAttach;
            }
            if(!pCurr)  // not find
            {
                os_release_mutex(gOSDMutex);
                /////////////////////////
                PDEBUG("Warning: Surface 0x%8.8x is reported to be attached to device %d but actually not!\n", (INT)pSurface, (INT)graphDev);
            }
            else // if(!pCurr)  // not find
            {
                if(pCurr != gOSDDev[graphDev].pSurface) // it is not the top most one
                {
                    // we need to adjust the links and update the surface
                    // pPrev should never be NULL at this point
                    pPrev->pNextAttach = pCurr->pNextAttach;

                    pSurface->pNextAttach = gOSDDev[graphDev].pSurface;
                    pSurface->attachedDev = graphDev;
                    gOSDDev[graphDev].pSurface = pSurface;
                }
                __update_device_surface(graphDev, pSurface);
                os_release_mutex(gOSDMutex);
                /////////////////////////
                PDEBUG("Success!\n");
                return 0;
            }
        }
        else  // if(gOSDDev[graphDev].pSurface)
        {
            os_release_mutex(gOSDMutex);
            /////////////////////////
            PDEBUG("Warning: Surface 0x%8.8x is reported to be attached to device %d but actually not!\n", (INT)pSurface, (INT)graphDev);
        }
    }

    // default attach
    // the first time attach of unattached device
    
    // check for compatibility
    switch(graphDev)
    {
    case GFX_VDEV_OSDCUR:
        if(!(pSurface->uPlaneConfig & OSD_PLANE_CURSOR))
        {
            PDEBUG("Surface is incompatible with cursor device!\n");
            return -1;
        }
        break;
        
    case GFX_VDEV_OSDGFX:
        if(!(pSurface->uPlaneConfig & OSD_PLANE_GRAPHICS))
        {
            PDEBUG("Surface is incompatible with graphics device!\n");
            return -1;
        }
        break;
        
    case GFX_VDEV_OSDIMG:
        if(!(pSurface->uPlaneConfig & OSD_PLANE_IMAGE))
        {
            PDEBUG("Surface is incompatible with still device!\n");
            return -1;
        }
        break;
        
    default:    // we could not attach it to null device
        PDEBUG("Invalid device ID!\n");
        return -1;
    }
    
    /////////////////////////
    if(os_get_mutex(gOSDMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }
    
    pSurface->pNextAttach = gOSDDev[graphDev].pSurface;
    pSurface->attachedDev = graphDev;
    gOSDDev[graphDev].pSurface = pSurface;
    
    __update_device_surface(graphDev, pSurface);

    if(GFX_VDEV_OSDGFX == graphDev && !gOSDDev[GFX_VDEV_OSDIMG].pSurface && !pSurface->twinheader)
         osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_IMAGE);   // should we ?

    os_release_mutex(gOSDMutex);
    /////////////////////////

    PDEBUG("Success!\n");

    return 0;
}


// pSurface can be NULL to detach default surface 
INT osd_osi_detach_comp_gfx_surface(GFX_VISUAL_DEVICE_ID_T graphDev, GFX_SURFACE_T * pSurface)
{
    int updateSurface = 0;
    int twinheader = 0;

    if( GFX_VDEV_NULL == graphDev || 
        (pSurface && (   !gfx_osi_pSurface_alloc(pSurface)  // psurface can be NULL
                      || pSurface->attachedDev != graphDev) ))
    {
        PDEBUG("Invalid parameter or uninitialized surface!\n");
        return -1;    // we cann't do it
    }

    if(!pSurface)   // if it is NULL, detach the top most surface
        pSurface = gOSDDev[graphDev].pSurface;

    if(!(gOSDDev[graphDev].uAttr & __OSD_GRAPH_DEVICE_INIT)) 
    {
        PDEBUG("Device %d is not initlized!\n", (INT)graphDev);
        return -1;
    }

    /////////////////////////
    if(os_get_mutex(gOSDMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }


    // detach 
    if(gOSDDev[graphDev].pSurface)
    {
        GFX_SURFACE_T *pCurr = gOSDDev[graphDev].pSurface;
        GFX_SURFACE_T *pPrev = NULL;
        // look it up
        while(pCurr && pCurr != pSurface) 
        {
            pPrev = pCurr;
            pCurr = pCurr->pNextAttach;
        }
        if(!pCurr)  // not find
        {
            os_release_mutex(gOSDMutex);
            /////////////////////////
            PDEBUG("Surface 0x%8.8x is not attached to device %d!\n", (INT)pSurface, (INT)graphDev);
            return -1;
        }

        pCurr->attachedDev = GFX_VDEV_NULL;
        if(pCurr == gOSDDev[graphDev].pSurface) //  the top most one
        {
            updateSurface = 1;
            if(GFX_VDEV_OSDGFX == graphDev && gOSDDev[graphDev].pSurface->twinheader) twinheader = 1;
            gOSDDev[graphDev].pSurface = pCurr->pNextAttach;
        }
        else // we don't need to update the surface
        {
            // pPrev should never be NULL at this point
            pPrev->pNextAttach = pCurr->pNextAttach;
        }
        pCurr->pNextAttach = NULL; // done
    }
    else
    {
        os_release_mutex(gOSDMutex);
        /////////////////////////
        PDEBUG("Device %d doesn't have a surface attached!\n", (INT)graphDev);
        return -1;
    }

    if(updateSurface)
    {
        if(NULL == gOSDDev[graphDev].pSurface)  // no one attached, so shutdown
        {
            switch(graphDev)
            {
            case GFX_VDEV_OSDCUR:
                osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_CURSOR);
                break;
                
            case GFX_VDEV_OSDGFX:
                osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_GRAPHICS);
                osd_atom_set_display_control(OSD_CNTL_E32BCO, 0);   // certainly
                if(gOSDDev[GFX_VDEV_OSDIMG].pSurface)
                    __update_device_surface(GFX_VDEV_OSDGFX, gOSDDev[GFX_VDEV_OSDGFX].pSurface);
                else
                    osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_IMAGE);   // should we ?
                break;
                
            case GFX_VDEV_OSDIMG:
                osd_atom_set_plane_control(OSD_PLANE_A_DISABLE, OSD_PLANE_IMAGE);
                if(gOSDDev[GFX_VDEV_OSDGFX].pSurface && gOSDDev[GFX_VDEV_OSDGFX].pSurface->twinheader)   // we need to update this for twinheader ones
                    __update_device_surface(GFX_VDEV_OSDGFX, gOSDDev[GFX_VDEV_OSDGFX].pSurface);
                break;
                
            default:
                PDEBUG("Invalid device ID!\n");
            }
        }
        else    // update the current surface
        {
            if(twinheader && gOSDDev[GFX_VDEV_OSDIMG].pSurface) 
                __update_device_surface(GFX_VDEV_OSDIMG, gOSDDev[GFX_VDEV_OSDIMG].pSurface);
            __update_device_surface(graphDev, gOSDDev[graphDev].pSurface);
        }
    }
    os_release_mutex(gOSDMutex);
    /////////////////////////

    PDEBUG("Success!\n");
    return 0;
}

INT osd_osi_set_device_parm(GFX_VISUAL_DEVICE_ID_T graphDev, OSD_GRAPH_DEVICE_PARM_T parm, UINT uValue)
{
    INT rtn;
    UINT uPlane=0;
    if(!(gOSDDev[graphDev].uAttr & __OSD_GRAPH_DEVICE_INIT)) 
    {
        PDEBUG("Device %d is not initlized!\n", (INT)graphDev);
        return -1;
    }

    switch(graphDev)
    {
    case GFX_VDEV_OSDCUR:
        uPlane = OSD_PLANE_CURSOR;
        break;

    case GFX_VDEV_OSDGFX:
        uPlane = OSD_PLANE_GRAPHICS;
        break;

    case GFX_VDEV_OSDIMG:
        uPlane = OSD_PLANE_IMAGE;
        break;

    default:
        PDEBUG("Invalid device ID!\n");
        os_release_mutex(gOSDMutex);
        /////////////////////////
        return -1;
    }

    /////////////////////////
    if(os_get_mutex(gOSDMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }
    switch(parm)
    {
    case OSD_GRAPH_DEVICE_ENABLE:    //   0x001 enable / 0x000 disable
        rtn = osd_atom_set_plane_control(uValue ? OSD_PLANE_A_ENABLE : OSD_PLANE_A_DISABLE, uPlane);
        break;

    default:
        rtn = -1;
        break;
    }

    os_release_mutex(gOSDMutex);
    /////////////////////////
    return rtn;
}

UINT osd_osi_get_device_parm(GFX_VISUAL_DEVICE_ID_T graphDev, OSD_GRAPH_DEVICE_PARM_T parm)
{
    UINT rtn;
    UINT uPlane=0;
    if(!(gOSDDev[graphDev].uAttr & __OSD_GRAPH_DEVICE_INIT)) 
    {
        PDEBUG("Device %d is not initlized!\n", (INT)graphDev);
        return 0;
    }

    switch(graphDev)
    {
    case GFX_VDEV_OSDCUR:
        uPlane = OSD_PLANE_CURSOR;
        break;

    case GFX_VDEV_OSDGFX:
        uPlane = OSD_PLANE_GRAPHICS;
        break;

    case GFX_VDEV_OSDIMG:
        uPlane = OSD_PLANE_IMAGE;
        break;

    default:
        PDEBUG("Invalid device ID!\n");
        os_release_mutex(gOSDMutex);
        /////////////////////////
        return 0;
    }

    /////////////////////////
    if(os_get_mutex(gOSDMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return 0;
    }
    switch(parm)
    {
    case OSD_GRAPH_DEVICE_ENABLE:    //   0x001 enable / 0x000 disable
        rtn = osd_atom_get_plane_control(OSD_PLANE_A_ENABLE, uPlane);
        break;
    default:
        rtn = 0;
        break;
    }

    os_release_mutex(gOSDMutex);
    /////////////////////////
    return rtn;
}


INT osd_osi_set_display_parm(OSD_DISPLAY_CONTROL_T parm, UINT uAttr)
{
    INT rtn;
    /////////////////////////
    if(os_get_mutex(gOSDMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    switch(parm)
    {
    case OSD_DISP_CNTL_BACKCOLOR:  // 16 bit back color
        rtn = osd_atom_set_display_control(OSD_CNTL_BACKCOLOR, uAttr);
        break;

    case OSD_DISP_CNTL_AFVP:      // Anti-flicker video plane, 0 disable, 1 enable
        rtn = osd_atom_set_display_control(OSD_CNTL_AFVP, uAttr);
        break;

    case OSD_DISP_CNTL_EDAF:      // Enable display anti-flicker, 0 disable, 1 enable
        rtn = osd_atom_set_display_control(OSD_CNTL_EDAF, uAttr);
        break;

    case OSD_DISP_CNTL_AFDT:      // Anti-flicker detection threshold, 2 bits attr
        rtn = osd_atom_set_display_control(OSD_CNTL_AFDT, uAttr);
        break;

    case OSD_DISP_CNTL_VPAFC:     // Video plane anti-flicker correction, 2 bits attr
        rtn = osd_atom_set_display_control(OSD_CNTL_VPAFC, uAttr);
        break;

    case OSD_DISP_CNTL_ANIM:     // Animation mode, 0 no, 1 yes -- BJC 102102
        rtn = osd_atom_set_display_control(OSD_CNTL_ANIM, uAttr);
        break;

    case OSD_DISP_CNTL_ANIMR:    // Animation rate, 3 bits attr -- BJC 102102
        rtn = osd_atom_set_display_control(OSD_CNTL_ANIMR, uAttr);
        break;

    case OSD_DISP_CNTL_CHFSR:    // Custom horizontal FIR scaling ratio, 9 bits attr -- BJC 102102
        rtn = osd_atom_set_display_control(OSD_CNTL_CHFSR, uAttr);
        break;

    default:
        rtn = -1;
        break;
    }
    
    os_release_mutex(gOSDMutex);
    /////////////////////////

    return rtn;
}


UINT osd_osi_get_display_parm(OSD_DISPLAY_CONTROL_T parm)
{
    UINT rtn;
    /////////////////////////
    if(os_get_mutex(gOSDMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return 0;
    }

    switch(parm)
    {
    case OSD_DISP_CNTL_BACKCOLOR:  // 16 bit back color
        rtn = osd_atom_get_display_control(OSD_CNTL_BACKCOLOR);
        break;

    case OSD_DISP_CNTL_AFVP:      // Anti-flicker video plane, 0 disable, 1 enable
        rtn = osd_atom_get_display_control(OSD_CNTL_AFVP);
        break;

    case OSD_DISP_CNTL_EDAF:      // Enable display anti-flicker, 0 disable, 1 enable
        rtn = osd_atom_get_display_control(OSD_CNTL_EDAF);
        break;

    case OSD_DISP_CNTL_AFDT:      // Anti-flicker detection threshold, 2 bits attr
        rtn = osd_atom_get_display_control(OSD_CNTL_AFDT);
        break;

    case OSD_DISP_CNTL_VPAFC:     // Video plane anti-flicker correction, 2 bits attr
        rtn = osd_atom_get_display_control(OSD_CNTL_VPAFC);
        break;

    case OSD_DISP_CNTL_ANIM:     // Animation mode, 0 no, 1 yes -- BJC 102102
        rtn = osd_atom_get_display_control(OSD_CNTL_ANIM);
        break;

    case OSD_DISP_CNTL_ANIMR:    // Animation rate, 3 bits attr -- BJC 102102
        rtn = osd_atom_get_display_control(OSD_CNTL_ANIMR);
        break;

    case OSD_DISP_CNTL_CHFSR:    // Custom horizontal FIR scaling ratio, 9 bits attr -- BJC 102102
        rtn = osd_atom_get_display_control(OSD_CNTL_CHFSR);
        break;

    default:
        rtn = 0;
        break;
    }
    
    os_release_mutex(gOSDMutex);
    /////////////////////////

    return rtn;
}



INT osd_osi_set_comp_gfx_surface_parm(GFX_SURFACE_T * pSurface, OSD_GRAPH_SURFACE_PARM_T parm, ULONG lValue1, ULONG lValue2)
{
    if(!gfx_osi_pSurface_valid(pSurface) || !IS_SURFACE_OSD_COMP(pSurface->uPlaneConfig)) 
    {
        PDEBUG("Invalid parameter or uninitialized surface!\n");
        return -1;    // we cann't do it
    }

    if(pSurface->uAttr & __GFX_SURFACE_OSDCURSOR)   // cursor
    {
        STB_OSD_CURSOR_CONTROL_BLOCK_T *pCntl;
        pCntl = os_get_logical_address(pSurface->hBuffer);
        switch(parm)
        {
        case OSD_GRAPH_SURFACE_FLICKER_CORRECTION:       // set the antiflicker filter value (v1)
            pCntl->anti_flicker = (unsigned)lValue1&0x03;   // two bits
            break;
            
        case OSD_GRAPH_SURFACE_SCREEN_OFFSET:            // the offset of surface to screen (v1 as horizontal, v2 as vertical)
            {
                UINT xoff = (UINT)lValue1;
                UINT yoff = (UINT)lValue2;
                
                if(xoff >= OSD_SURFACE_MAX_WIDTH || yoff >= OSD_SURFACE_MAX_HEIGHT)
                {
                    PDEBUG("Surface offset (%d,%d) out of range!\n", xoff, yoff);
                    return -1;
                }
                
                pCntl->start_column = xoff>>1;
                pCntl->start_column_odd = xoff&1;
                pCntl->start_row = yoff>>1;
                pCntl->start_row_odd = yoff&1;
            }
            break;
            
        case OSD_GRAPH_SURFACE_SINGLE_PALETTE:           // access a single palette (v1 as index and v2 as value)
            return gfx_osi_set_surface_palette(pSurface, (GFX_PALETTE_T *)&lValue2, (UINT)lValue1, 1);
            
        case OSD_GRAPH_SURFACE_ALL_PALETTE:              // access all palette from 0 (v1 as num of entries, v2 is used as pointer to palette array)
            // if v1 == NULL, the default palette is used
            if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig))
            {
                GFX_PALETTE_T *pPal = (GFX_PALETTE_T *)lValue2;
                UINT numpal = (UINT)lValue1;
                if(numpal > 16 && (pSurface->uAttr & __GFX_SURFACE_OSDCURSOR))
                {
                    numpal = 16;
                }
                else if(numpal > 256)
                {
                    numpal = 256;
                }
                if(NULL == pPal) // he want default palette
                {
                    return gfx_osi_reset_surface_palette(pSurface);
                }
                else
                {
                    gfx_osi_set_surface_palette(pSurface, pPal, 0, numpal);
                }
            }
            else
            {
                PDEBUG("Could not set palette on non clut planes!\n");
                return -1;
            }
            break;
        default:
            PDEBUG("Could not set the parm!\n");
            return -1;
        }


    }
    else    // image / graphics
    {
        STB_OSD_GI_CONTROL_BLOCK_T *pCntl;
        pCntl = os_get_logical_address(pSurface->hBuffer);
        
        switch(parm)
        {
        case OSD_GRAPH_SURFACE_FLICKER_CORRECTION:       // set the antiflicker filter value (v1)
            pCntl->anti_flicker = (unsigned)lValue1&0x03;   // two bits
            break;
            
        case OSD_GRAPH_SURFACE_SCREEN_OFFSET:            // the offset of surface to screen (v1 as horizontal, v2 as vertical)
            {
                UINT xoff = (UINT)lValue1;
                UINT yoff = (UINT)lValue2;
                
                if(xoff >= OSD_SURFACE_MAX_WIDTH || yoff >= OSD_SURFACE_MAX_HEIGHT)
                {
                    PDEBUG("Surface offset (%d,%d) out of range!\n", xoff, yoff);
                    return -1;
                }
                
                pCntl->start_column = xoff>>1;
                pCntl->start_row = yoff>>1;
                if(pSurface->twinheader)
                {
                    STB_OSD_GI_CONTROL_BLOCK_T *pCntl2 = (STB_OSD_GI_CONTROL_BLOCK_T *)((BYTE *)pCntl + pSurface->uBufferBase2 - pSurface->uBufferBase);
                    pCntl2->start_column = xoff>>1;
                    pCntl2->start_row = yoff>>1;
                }
            }
            break;
            
        case OSD_GRAPH_SURFACE_SINGLE_PALETTE:           // access a single palette (v1 as index and v2 as value)
            return gfx_osi_set_surface_palette(pSurface, (GFX_PALETTE_T *)&lValue2, (UINT)lValue1, 1);
            
        case OSD_GRAPH_SURFACE_ALL_PALETTE:              // access all palette from 0 (v1 as num of entries, v2 is used as pointer to palette array)
            // if v1 == NULL, the default palette is used
            if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig))
            {
                GFX_PALETTE_T *pPal = (GFX_PALETTE_T *)lValue2;
                UINT numpal = (UINT)lValue1;
                if(numpal > 16 && (pSurface->uAttr & __GFX_SURFACE_OSDCURSOR))
                {
                    numpal = 16;
                }
                else if(numpal > 256)
                {
                    numpal = 256;
                }
                if(NULL == pPal) // he want default palette
                {
                    return gfx_osi_reset_surface_palette(pSurface);
                }
                else
                {
                    gfx_osi_set_surface_palette(pSurface, pPal, 0, numpal);
                }
            }
            else
            {
                PDEBUG("Could not set palette on non clut planes!\n");
                return -1;
            }
            break;
        default:
            PDEBUG("Could not set the parm!\n");
            return -1;
        }
    }

    return 0;
}


INT osd_osi_get_comp_gfx_surface_parm(GFX_SURFACE_T *pSurface, OSD_GRAPH_SURFACE_PARM_T parm, ULONG *plValue1, ULONG *plValue2)
{
    if(!gfx_osi_pSurface_valid(pSurface) 
        || !IS_SURFACE_OSD_COMP(pSurface->uPlaneConfig) || NULL == plValue1) 
    {
        PDEBUG("Invalid parameter or uninitialized surface!\n");
        return -1;    // we cann't do it
    }
    
    if(pSurface->uAttr & __GFX_SURFACE_OSDCURSOR)   // cursor
    {
        STB_OSD_CURSOR_CONTROL_BLOCK_T *pCntl;
        pCntl = os_get_logical_address(pSurface->hBuffer);
        
        switch(parm)
        {
        case OSD_GRAPH_SURFACE_FLICKER_CORRECTION:       // get the antiflicker filter value (v1)
            *plValue1 = (ULONG)(pCntl->anti_flicker);    // two bits
            break;
            
        case OSD_GRAPH_SURFACE_SCREEN_OFFSET:            // the offset of surface to screen (v1 as horizontal, v2 as vertical)
            {
                if(NULL == plValue2)
                {
                    PDEBUG("Unexpacted input NULL pointer!\n");
                    return -1;
                }
                // xoff
                *plValue1 = ((ULONG)pCntl->start_column<<1) + pCntl->start_column_odd;
                // yoff
                *plValue2 = ((ULONG)pCntl->start_row<<1) + pCntl->start_row_odd;
            }
            break;
            
        case OSD_GRAPH_SURFACE_SINGLE_PALETTE:           // access a single palette (v1 as index and v2 as value)
            if(NULL == plValue2)
            {
                PDEBUG("Unexpacted input NULL pointer!\n");
                return -1;
            }
            if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig))
            {
                UINT index = (UINT)*plValue1;
                if(index > 255 || (index > 15 && (pSurface->uAttr & __GFX_SURFACE_OSDCURSOR)))
                {
                    PDEBUG("palette index out of range!\n");
                    return -1;
                }
                *(GFX_PALETTE_T *)plValue2  = pSurface->pPalette[index];
            }
            else
            {
                PDEBUG("Could not get palette on non clut planes!\n");
                return -1;
            }
            break;
            
        case OSD_GRAPH_SURFACE_ALL_PALETTE:              // access all palette from 0 (v1 as num of entries, v2 is used as pointer to palette array)
            if(NULL == plValue2)
            {
                PDEBUG("Unexpacted input NULL pointer!\n");
                return -1;
            }
            if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig))
            {
                GFX_PALETTE_T *pPal = (GFX_PALETTE_T *)plValue2;
                UINT numpal = (UINT)*plValue1;
                if(numpal > 16 && (pSurface->uAttr & __GFX_SURFACE_OSDCURSOR))
                {
                    numpal = 16;
                }
                else if(numpal > 256)
                {
                    numpal = 256;
                }
                _OS_MEMCPY(pPal, pSurface->pPalette, sizeof(GFX_PALETTE_T)*numpal);
            }
            else
            {
                PDEBUG("Could not get palette on non clut planes!\n");
                return -1;
            }
            break;
            
        default:
            PDEBUG("Could not get the parm!\n");
            return -1;
            
        }
    }
    else  // gaphics / image
    {
        STB_OSD_GI_CONTROL_BLOCK_T *pCntl;
        pCntl = os_get_logical_address(pSurface->hBuffer);
        
        switch(parm)
        {
        case OSD_GRAPH_SURFACE_FLICKER_CORRECTION:       // get the antiflicker filter value (v1)
            *plValue1 = (ULONG)(pCntl->anti_flicker);    // two bits
            break;
            
        case OSD_GRAPH_SURFACE_SCREEN_OFFSET:            // the offset of surface to screen (v1 as horizontal, v2 as vertical)
            {
                if(NULL == plValue2)
                {
                    PDEBUG("Unexpacted input NULL pointer!\n");
                    return -1;
                }
                // xoff
                *plValue1 = ((ULONG)pCntl->start_column<<1);
                // yoff
                *plValue2 = ((ULONG)pCntl->start_row<<1);
            }
            break;
            
        case OSD_GRAPH_SURFACE_SINGLE_PALETTE:           // access a single palette (v1 as index and v2 as value)
            if(NULL == plValue2)
            {
                PDEBUG("Unexpacted input NULL pointer!\n");
                return -1;
            }
            if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig))
            {
                UINT index = (UINT)*plValue1;
                if(index > 255 || (index > 15 && (pSurface->uAttr & __GFX_SURFACE_OSDCURSOR)))
                {
                    PDEBUG("palette index out of range!\n");
                    return -1;
                }
                *(GFX_PALETTE_T *)plValue2  = pSurface->pPalette[index];
            }
            else
            {
                PDEBUG("Could not get palette on non clut planes!\n");
                return -1;
            }
            break;
            
        case OSD_GRAPH_SURFACE_ALL_PALETTE:              // access all palette from 0 (v1 as num of entries, v2 is used as pointer to palette array)
            if(NULL == plValue2)
            {
                PDEBUG("Unexpacted input NULL pointer!\n");
                return -1;
            }
            if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig))
            {
                GFX_PALETTE_T *pPal = (GFX_PALETTE_T *)plValue2;
                UINT numpal = (UINT)*plValue1;
                if(numpal > 16 && (pSurface->uAttr & __GFX_SURFACE_OSDCURSOR))
                {
                    numpal = 16;
                }
                else if(numpal > 256)
                {
                    numpal = 256;
                }
                _OS_MEMCPY(pPal, pSurface->pPalette, sizeof(GFX_PALETTE_T)*numpal);
            }
            else
            {
                PDEBUG("Could not get palette on non clut planes!\n");
                return -1;
            }
            break;
            
        default:
            PDEBUG("Could not get the parm!\n");
            return -1;
        }
    }
    
    return 0;
}
    

//////////////////////////////////////////////////////////////////////////////
// Cursor specific interfaces
//////////////////////////////////////////////////////////////////////////////


INT osd_osi_create_cursor(GFX_SURFACE_T *pCursor, UINT uPlaneConfig, UINT uWidth, UINT uHeight)
{
    return gfx_osi_create_surface(pCursor, GFX_VDEV_OSDCUR, uPlaneConfig, uWidth, uHeight);
}

INT osd_osi_destory_cursor(GFX_SURFACE_T *pCursor)
{
    return gfx_osi_destroy_surface(pCursor);
}

INT osd_osi_attach_cursor(GFX_SURFACE_T *pCursor)
{
    return osd_osi_attach_comp_gfx_surface(GFX_VDEV_OSDCUR, pCursor);
}

INT osd_osi_detach_cursor(GFX_SURFACE_T *pCursor)
{
    return osd_osi_detach_comp_gfx_surface(GFX_VDEV_OSDCUR, pCursor);
}

INT osd_osi_set_cursor_position(INT nHorizontal, INT nVertical)
{

    if(!(gOSDDev[GFX_VDEV_OSDCUR].uAttr & __OSD_GRAPH_DEVICE_INIT)) 
    {
        PDEBUG("Cursor device is not initlized!\n");
        return -1;
    }
    if(NULL == gOSDDev[GFX_VDEV_OSDCUR].pSurface)
    {
        PDEBUG("No surface is attached to cursor device!\n");
        return -1;
    }
    if(nHorizontal < 0) nHorizontal = 0;
    if(nHorizontal > OSD_CURSOR_POSITION_RANGE) nHorizontal = OSD_CURSOR_POSITION_RANGE;
    if(nVertical < 0) nVertical = 0;
    if(nVertical > OSD_CURSOR_POSITION_RANGE) nVertical = OSD_CURSOR_POSITION_RANGE;
    
    return osd_osi_set_comp_gfx_surface_parm(gOSDDev[GFX_VDEV_OSDCUR].pSurface, OSD_GRAPH_SURFACE_SCREEN_OFFSET, nHorizontal, nVertical);
}


INT osd_osi_get_cursor_position(INT *pnHorizontal, INT *pnVertical)
{
    int rtn;
    ULONG x, y;

    if(!(gOSDDev[GFX_VDEV_OSDCUR].uAttr & __OSD_GRAPH_DEVICE_INIT)) 
    {
        PDEBUG("Cursor device is not initlized!\n");
        return -1;
    }
    if(NULL == gOSDDev[GFX_VDEV_OSDCUR].pSurface)
    {
        PDEBUG("No surface is attached to cursor device!\n");
        return -1;
    }
    rtn = osd_osi_get_comp_gfx_surface_parm(gOSDDev[GFX_VDEV_OSDCUR].pSurface, OSD_GRAPH_SURFACE_SCREEN_OFFSET, &x, &y);

    if(pnHorizontal)  *pnHorizontal = (INT)x;
    if(pnVertical) *pnVertical = (INT)y;

    return rtn;
}



INT osd_osi_set_cursor_attributes(GFX_SURFACE_T *pCursor, UINT uIndex, GFX_CURSOR_ATTRIBUTE_T attr)
{
    STB_OSD_C_PALETTE_T *pPal;
    UINT32 val = (UINT32)attr;

    if(!gfx_osi_pSurface_valid(pCursor) ||  !(pCursor->uAttr & __GFX_SURFACE_OSDCURSOR) || uIndex >= (1<<pCursor->plane[0].uPixelSize))
    {
        PDEBUG("Invalid parameter or uninitialized surface!\n");
        return -1;    // we cann't do it
    }
    pPal = pCursor->pPal;
    pPal[uIndex].peep   = (val&2) ? 1 : 0;
    pPal[uIndex].steady = (val&1) ? 1 : 0;
    return 0;
}
