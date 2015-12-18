//vulcan/drv/gfx/gfx_inf_helper.c
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
//  Helper function for INF layer of GFX controls 
//Revision Log:   
//  Nov/05/2001                                                    Created by YYD
//  Nov/08/2001                                      Enabled surface locks by YYD
//  Nov/22/2001       Fixed a variable name typing bug in gfx_inf_h_init() by YYD
//  Oct/21/2002    Added dispatch handlers for GFX_DISP_CNTL_OSDIRQ and
//                     GFX_DISP_CNTL_OSDHSR in gfx_inf_set/get_display_control()
//                 Added gfx_inf_h_swap_surface() to detach one surface and
//                     attach another, optionally synchronized with the OSD
//                     animation interrupt for double-buffered operation.    
//                 Added gfx_inf_h_dmablt() to initiate a DMA "block transfer".
//                 Added gfx_inf_h_dmablt_wait() to wait for an async DMABLT.
//                 Added gfx_inf_h_pmalloc() to allocate phys contig memory.  BJC



#include "os/os-types.h"
#include "os/os-sync.h"
#include "os/os-generic.h"
#include "os/pm-alloc.h"
#include "gfx_surface.h"
#include "gfx_osi.h"
#include "osd_osi.h"

#include "gfx_inf_helper.h"
#include "gfx_atom.h" // BJC 102101


//#define __GFX_INF_HELPER_DEBUG

#ifdef __GFX_INF_HELPER_DEBUG
    #define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif



// local data structure
typedef struct __GFX_HANDLE_T_STRUCTURE__
{
    GFX_SURFACE_T  surface;
    UINT    uLocks;
    GFX_SURFACE_PHYSICAL_PARM_T phy;
} GFX_HANDLE_T;

static GFX_HANDLE_T   *gpHandles;
static UINT guMaxHandles;
static UINT guFreeHandles;
static UINT guLockedHandles;

static GFX_SCREEN_INFO_T gScreenInfo;

static int ghShared = -1;        // a globally shared surface handle which can be get by other apps

static MUTEX_T *gGFXMutex;

static int gWait_for_finish;

extern GFX_SWAP_SURFACE_T osdanim_swap; // setup by gfx_inf_h_swap_surface() -- BJC 102102


int gfx_inf_h_init(unsigned int uNumSurface)
{
    if(uNumSurface == 0)
    {
        uNumSurface = GFX_DEFAULT_SURFACES;
    }

    if(gGFXMutex) return 0; // already done

    gGFXMutex =  os_create_mutex ();

    if(!gGFXMutex)
    {
        PDEBUG("Failed to create lock for resources!\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }


    if(osd_osi_init() < 0)
    {
        PDEBUG("Failed to initlized display device!\n");
        return -1;
    }

    if(gfx_osi_init() < 0)
    {
        PDEBUG("Failed to initlized graphics device!\n");
        osd_osi_deinit();
        return -1;
    }

    gScreenInfo.uUpper = OSD_DEFAULT_TV_UPPER_MARGIN;
    gScreenInfo.uLeft  = OSD_DEFAULT_TV_LEFT_MARGIN;
    gScreenInfo.uWidth = OSD_DEFAULT_XRES;
    gScreenInfo.uHeight= OSD_DEFAULT_YRES;

    gpHandles = MALLOC(uNumSurface * sizeof(GFX_HANDLE_T));

    if(NULL == gpHandles)
    {
        osd_osi_deinit();
        gfx_osi_deinit();

        os_release_mutex(gGFXMutex);
        os_delete_mutex(gGFXMutex);
        gGFXMutex = NULL;
        PDEBUG("Failed to allocate surface handle buffer!\n");
        return -1;
    }

    _OS_MEMSET(gpHandles, 0, uNumSurface * sizeof(GFX_HANDLE_T));
    
    guMaxHandles = uNumSurface;
    guFreeHandles = uNumSurface;

    guLockedHandles = 0;

    ghShared = -1;
    
    gWait_for_finish = 1;   // compatible with old app

    os_release_mutex(gGFXMutex);
    /////////////////////////////

    // set default af parms
    {
        GFX_DISPLAY_CONTROL_PARM_T parm;
        parm.parm = GFX_DISP_CNTL_EDAF;
        parm.uAttr = 1;
        gfx_inf_h_set_display_control(&parm);
        // set antiflicker detect threshold to afdt
        parm.parm = GFX_DISP_CNTL_AFDT;
        parm.uAttr = 2;
        gfx_inf_h_set_display_control(&parm);
    }

    return 0;

}


// nForce :  0      // safest mode
//           1      // stop when surfaces are locked
//           2      // continue anyway
int gfx_inf_h_deinit(int nForce)
{
    MUTEX_T *mutex;
    int rtn;

    if(!gGFXMutex) return 0;    // already done

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(guFreeHandles < guMaxHandles)
    {
        if(1 == nForce)
        {
            UINT i;
            if(guLockedHandles > 0)
            {
                os_release_mutex(gGFXMutex);
                //////////////////////////////
                PDEBUG("Some surface is locked, could not continue!\n");
                return -1;
            }

            for(i=0; i<guMaxHandles; i++)
            {
                if(gpHandles[i].uLocks > 0)
                {
                    // some thing wrong, should not happen
                    os_release_mutex(gGFXMutex);
                    //////////////////////////////
                    PFATAL("Some surface is locked but not recorded!\n");
                    return -1;
                }
                if(gfx_osi_pSurface_alloc(&gpHandles[i].surface))
                {
                    if(gpHandles[i].surface.attachedDev != GFX_VDEV_NULL)
                    {
                        osd_osi_detach_comp_gfx_surface(gpHandles[i].surface.attachedDev, &gpHandles[i].surface);
                    }
                    PDEBUG("Surface handle %d is not released!\n", i);
                    gfx_osi_destroy_surface(&gpHandles[i].surface);
                    guFreeHandles ++;
                }
            }
            // ok, lets do other stuffs
        }
        else if (2 == nForce)
        {
            UINT i;
            if(guLockedHandles > 0)
            {
                PFATAL("Some surface is locked, expecting unstability afterwards !\n");
            }

            for(i=0; i<guMaxHandles; i++)
            {
                if(gfx_osi_pSurface_alloc(&gpHandles[i].surface))
                {
                    if(gpHandles[i].surface.attachedDev != GFX_VDEV_NULL)
                    {
                        osd_osi_detach_comp_gfx_surface(gpHandles[i].surface.attachedDev, &gpHandles[i].surface);
                    }
                    PDEBUG("Surface handle %d is not released!\n", i);
                    gfx_osi_destroy_surface(&gpHandles[i].surface);
                }
            }
            // ok, lets do other stuffs
        }
        else if (0 == nForce)
        {
         os_release_mutex(gGFXMutex);
         /////////////////////////////
         PDEBUG("Some surface is not freed, could not continue!\n");
            return -1;
        }
    }

    mutex = gGFXMutex;  // save first
    gGFXMutex = NULL;       // no one can enter correctly now

    FREE(gpHandles);

    gpHandles = NULL;
    guMaxHandles = 0;
    guFreeHandles = 0;
    guLockedHandles = 0;

    ghShared = -1;

    rtn = gfx_osi_deinit();

    rtn += osd_osi_deinit();

    os_release_mutex(mutex);
    /////////////////////////////
    os_delete_mutex(mutex);

    return rtn;
}

int gfx_inf_h_set_shared_surface(int hSurface)
{
    int rtn = -1;
    if(hSurface < 0 || hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Invalid surface\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[hSurface].surface))
    {
        ghShared = hSurface;
        rtn = 0;
        PDEBUG("Shared surface set to %d\n", hSurface);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////

    return rtn;
}


int gfx_inf_h_get_shared_surface(void)
{
    return ghShared;
}


int gfx_inf_h_create_surface(GFX_CREATE_SURFACE_PARM_T *pParm)
{
    int rtn = -1;

    if(NULL == pParm || pParm->uHeight == 0 || pParm->uWidth == 0)
    {
        PDEBUG("Invalid parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(guFreeHandles > 0)
    {
        // try to find a free handle
        int i;
        for(i=0; i<guMaxHandles; i++)
        {
            if(!gfx_osi_pSurface_alloc(&gpHandles[i].surface)) break;
        }
        if(i<guMaxHandles)
        {
            gfx_osi_init_surface_t(&gpHandles[i].surface);  // clear everything
            if(gfx_osi_create_surface(&gpHandles[i].surface, pParm->graphDev, pParm->uPlaneConfig, pParm->uWidth, pParm->uHeight) >= 0)
            {
                rtn = 0;
                pParm->hSurface = i;
                gpHandles[i].uLocks = 0;
                PDEBUG("Clear the surface\n");
                gfx_osi_fillBLT(&gpHandles[i].surface, 0,0, pParm->uWidth, pParm->uHeight, pParm->uFillColor);
                gfx_osi_run_engine(1);
                PDEBUG("After Clear\n");
                guFreeHandles--;
                if(IS_SURFACE_OSD_COMP(gpHandles[i].surface.uPlaneConfig))
                {
                    if(!IS_SURFACE_CURSOR_COMP(gpHandles[i].surface.uPlaneConfig))
                    {
                        osd_osi_set_comp_gfx_surface_parm(&gpHandles[i].surface, 
                            OSD_GRAPH_SURFACE_SCREEN_OFFSET, 
                            (ULONG)(gScreenInfo.uLeft ),
                            (ULONG)(gScreenInfo.uUpper));
                    }
                    else
                    {
                        osd_osi_set_comp_gfx_surface_parm(&gpHandles[i].surface, 
                            OSD_GRAPH_SURFACE_SCREEN_OFFSET, 
                            (ULONG)0, (ULONG)0);
                        
                    }
                }
            }
            else 
            {
                PDEBUG("Failed on create surface!\n");
                rtn = -1;
            }
        }
        else
        {
            PFATAL("invalid free handle count !\n"); 
            guFreeHandles = 0;
            rtn = -1;
        }
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////

    return rtn;
}

int gfx_inf_h_destroy_surface(int hSurface)
{
    int rtn= -1;

    if(hSurface < 0 || hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[hSurface].surface))
    {
        if(gpHandles[hSurface].uLocks > 0)
        {
            PDEBUG("Surface %d is locked by %d times, destroy might be dangerous!\n", hSurface, gpHandles[hSurface].uLocks);
        }

        if(gpHandles[hSurface].surface.attachedDev != GFX_VDEV_NULL)
            osd_osi_detach_comp_gfx_surface(gpHandles[hSurface].surface.attachedDev, &gpHandles[hSurface].surface);

        rtn = gfx_osi_destroy_surface(&gpHandles[hSurface].surface);
        if(!rtn)  
        {
            guFreeHandles++;
            _OS_MEMSET(&gpHandles[hSurface], 0, sizeof(gpHandles[hSurface]));

            if(ghShared == hSurface) // make sure it is changed
                ghShared = -1;
        }
    }
    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}

// not yet publicated
int gfx_inf_h_reconfig_surface_dimension(int hSurface, UINT uNewWidth, UINT uNewHeight)
{
    int rtn = -1;

    if(hSurface < 0 || hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[hSurface].surface))
    {
        rtn = gfx_osi_reconfig_surface_dimension(&gpHandles[hSurface].surface, uNewWidth, uNewHeight);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


// not supported yet
int gfx_inf_h_get_subplane_pseudo_surface(GFX_GET_SUBPLANE_PSEUDO_SURFACE_PARM_T *pParm)
{
    int rtn = -1;

    return rtn;
}

// limited use only
int _gfx_inf_h_get_surface_local(GFX_SURFACE_LOCAL_INFO_T *pInfo)
{
    int rtn= -1;
    int hSurface;

    if(NULL == pInfo || pInfo->hSurface < 0 || pInfo->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    hSurface = pInfo->hSurface;

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[hSurface].surface))
    {
        int i;
        pInfo->uPlaneConfig = gpHandles[hSurface].surface.uPlaneConfig;
        //pInfo->pPalette = gpHandles[hSurface].surface.pPalette;
        
        for(i=0; i<GET_GFX_SURFACE_SUBPLANES(pInfo->uPlaneConfig); i++)
        {
            // should I mmap them here ? Currently I'm not doing that.
            pInfo->plane[i].pPlane = gpHandles[hSurface].surface.plane[i].pBuffer;
            pInfo->plane[i].uBytePerLine = gpHandles[hSurface].surface.plane[i].uBytePerLine;
            pInfo->plane[i].uPixelSize = gpHandles[hSurface].surface.plane[i].uPixelSize;
            pInfo->plane[i].uPixelJustify = gpHandles[hSurface].surface.plane[i].uPixelJustify;
            pInfo->plane[i].uWidth = gpHandles[hSurface].surface.plane[i].uAllocWidth;
            pInfo->plane[i].uHeight = gpHandles[hSurface].surface.plane[i].uAllocHeight;
            pInfo->plane[i].a = gpHandles[hSurface].surface.plane[i].a;
            pInfo->plane[i].r = gpHandles[hSurface].surface.plane[i].r;
            pInfo->plane[i].g = gpHandles[hSurface].surface.plane[i].g;
            pInfo->plane[i].b = gpHandles[hSurface].surface.plane[i].b;
        }
        rtn = 0;
    }
    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_get_surface_info(GFX_SURFACE_INFO_T *pInfo)
{
    int rtn= -1;
    int hSurface;

    if(NULL == pInfo || pInfo->hSurface < 0 || pInfo->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    hSurface = pInfo->hSurface;

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[hSurface].surface))
    {
        int i;

        gfx_osi_get_surface_physical_parm(&gpHandles[hSurface].phy, &gpHandles[hSurface].surface);

        pInfo->uPlaneConfig = gpHandles[hSurface].surface.uPlaneConfig;
        
        for(i=0; i<GET_GFX_SURFACE_SUBPLANES(pInfo->uPlaneConfig); i++)
        {
            // should I mmap them here ? Currently I'm not doing that.
            pInfo->plane[i].uBytePerLine = gpHandles[hSurface].surface.plane[i].uBytePerLine;
            pInfo->plane[i].uPixelSize = gpHandles[hSurface].surface.plane[i].uPixelSize;
            pInfo->plane[i].uPixelJustify = gpHandles[hSurface].surface.plane[i].uPixelJustify;
            pInfo->plane[i].uWidth = gpHandles[hSurface].surface.plane[i].uAllocWidth;
            pInfo->plane[i].uHeight = gpHandles[hSurface].surface.plane[i].uAllocHeight;
            pInfo->plane[i].a = gpHandles[hSurface].surface.plane[i].a;
            pInfo->plane[i].r = gpHandles[hSurface].surface.plane[i].r;
            pInfo->plane[i].g = gpHandles[hSurface].surface.plane[i].g;
            pInfo->plane[i].b = gpHandles[hSurface].surface.plane[i].b;

            pInfo->plane[i].plane.uBase = gpHandles[hSurface].phy.uPlanePhysicalBaseAddr[i];
            pInfo->plane[i].plane.uSize = gpHandles[hSurface].phy.uPlanePhysicalSize[i];
            pInfo->plane[i].plane.uOffset = gpHandles[hSurface].phy.uPlanePhysicalOffset[i];
        }
        rtn = 0;
    }
    os_release_mutex(gGFXMutex);
    /////////////////////////////

    return rtn;
}


int gfx_inf_h_lock_surface(GFX_SURFACE_INFO_T *pInfo)
{
    int rtn= -1;
    int hSurface;

    if(NULL == pInfo || pInfo->hSurface < 0 || pInfo->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    hSurface = pInfo->hSurface;

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[hSurface].surface))
    {
        int i;

        gfx_osi_get_surface_physical_parm(&gpHandles[hSurface].phy, &gpHandles[hSurface].surface);

        if(0 == gpHandles[hSurface].uLocks) guLockedHandles ++;
        gpHandles[hSurface].uLocks ++;

        pInfo->uPlaneConfig = gpHandles[hSurface].surface.uPlaneConfig;
        
        for(i=0; i<GET_GFX_SURFACE_SUBPLANES(pInfo->uPlaneConfig); i++)
        {
            // should I mmap them here ? Currently I'm not doing that.
            pInfo->plane[i].uBytePerLine = gpHandles[hSurface].surface.plane[i].uBytePerLine;
            pInfo->plane[i].uPixelSize = gpHandles[hSurface].surface.plane[i].uPixelSize;
            pInfo->plane[i].uPixelJustify = gpHandles[hSurface].surface.plane[i].uPixelJustify;
            pInfo->plane[i].uWidth = gpHandles[hSurface].surface.plane[i].uAllocWidth;
            pInfo->plane[i].uHeight = gpHandles[hSurface].surface.plane[i].uAllocHeight;
            pInfo->plane[i].a = gpHandles[hSurface].surface.plane[i].a;
            pInfo->plane[i].r = gpHandles[hSurface].surface.plane[i].r;
            pInfo->plane[i].g = gpHandles[hSurface].surface.plane[i].g;
            pInfo->plane[i].b = gpHandles[hSurface].surface.plane[i].b;

            pInfo->plane[i].plane.uBase = gpHandles[hSurface].phy.uPlanePhysicalBaseAddr[i];
            pInfo->plane[i].plane.uSize = gpHandles[hSurface].phy.uPlanePhysicalSize[i];
            pInfo->plane[i].plane.uOffset = gpHandles[hSurface].phy.uPlanePhysicalOffset[i];
        }
        rtn = 0;
    }
    os_release_mutex(gGFXMutex);
    /////////////////////////////

    return rtn;
}

int gfx_inf_h_unlock_surface(int hSurface)
{
    int rtn= -1;

    if(hSurface < 0 || hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[hSurface].surface)
        && gpHandles[hSurface].uLocks > 0)
    {
        gpHandles[hSurface].uLocks --;
        if(0 == gpHandles[hSurface].uLocks) 
            guLockedHandles --;
        rtn = 0;
    }
    else
    {
        PDEBUG("Surface invalid or not locked\n");
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


// this is a dirty slow process, but it is needed indeed
int _gfx_inf_h_validate_surface_address(ULONG uPlanePhysicalBaseAddr, ULONG uPlanePhysicalSize)
{
    int rtn = -1;

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(guLockedHandles > 0)
    {
        UINT i, j=0, candi=guMaxHandles;
        for(i=0; i<guMaxHandles; i++)
        {
            if(gpHandles[i].uLocks > 0)
            {
                for(j=0; j<GET_GFX_SURFACE_SUBPLANES(gpHandles[i].surface.uPlaneConfig); j++)
                {
                    PDEBUG("Checking %d [%d] = 0x%8.8x / %d \n", i, j, 
                            (UINT)gpHandles[i].phy.uPlanePhysicalBaseAddr[j],
                            (UINT)gpHandles[i].phy.uPlanePhysicalSize[j]);
                    if(gpHandles[i].phy.uPlanePhysicalBaseAddr[j] == uPlanePhysicalBaseAddr)
                    {
                        PDEBUG("Candidate %d [%d] = 0x%8.8x / %d \n", i, j, 
                            (UINT)gpHandles[i].phy.uPlanePhysicalBaseAddr[j],
                            (UINT)gpHandles[i].phy.uPlanePhysicalSize[j]);
                        candi = i;  i = guMaxHandles;
                        break;
                    }
                }
            }
        }
        // check if it fits
        // first, check if we find it, then its size is ok
        if( candi < guMaxHandles /* && 
            gpHandles[candi].phy.uPlanePhysicalSize[j] >= uPlanePhysicalSize*/)
            rtn = 0;    // yes
        else // no
        {
            PDEBUG("Failed to find 0x%8.8x / %d in my surface pool\n", (UINT)uPlanePhysicalBaseAddr, (UINT)uPlanePhysicalSize);
        }
    }
    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_attach_surface(GFX_SURFACE_VDEV_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm || pParm->hSurface < 0 || pParm->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    rtn = osd_osi_attach_comp_gfx_surface(pParm->graphDev, &gpHandles[pParm->hSurface].surface);

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_detach_surface(GFX_SURFACE_VDEV_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm || pParm->hSurface < 0 || pParm->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(GFX_VDEV_NULL == pParm->graphDev)
    {
        rtn = osd_osi_detach_comp_gfx_surface(gpHandles[pParm->hSurface].surface.attachedDev, &gpHandles[pParm->hSurface].surface);
    }
    else
    {
        rtn = osd_osi_detach_comp_gfx_surface(pParm->graphDev, &gpHandles[pParm->hSurface].surface);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


/*
 * Detach one surface and attach another, optionally synchronized with the OSD
 * animation interrupt.  If pParm->bWaitForSwap is set, this function will sleep
 * and not return until the next OSD animation interrupt occurs.
 */
int gfx_inf_h_swap_surface(GFX_2SURFACE_VDEV_PARM_T *pParm)
{
    int rtn = -1;

    /* 
     * If pParm->UseOsdAnimInt was set and pParm->bWaitForSwap was not set
     * in the last call to gfx_inf_h_swap_surface(), osdanim_swap.bReadyForSwap will still 
     * be set if the OSD animation interrupt has not occurred.  For this condition,
     * gfx_inf_h_swap_surface() may be called at a maximum frequency not to exceed 
     * the OSD animation interrupt frequency.
     */
    if(osdanim_swap.bReadyForSwap) {
        printk(KERN_INFO "gfx_inf_h_swap_surface() called before previous request finished\n");
        return -1;
    }

    if(NULL == pParm || pParm->hOldSurface < 0 || pParm->hOldSurface >= (int)guMaxHandles ||
       pParm->hNewSurface < 0 || pParm->hNewSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    if(pParm->bUseOsdAnimInt) {
        pParm->missedInts = osdanim_swap.missedInts;
        if(osdanim_swap.missedInts > 0) {
            osdanim_swap.missedInts = 0;
            PDEBUG("missed %d osd animation interrupts\n", pParm->missedInts);
        }
    } else {
        pParm->missedInts = 0;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;

    }
    if(pParm->bUseOsdAnimInt) {
        osdanim_swap.hGraphDev = pParm->graphDev;	
        osdanim_swap.hOldSurface = pParm->hOldSurface;
        osdanim_swap.hNewSurface = pParm->hNewSurface;
        sema_init(&osdanim_swap.sem, 0);
        osdanim_swap.bReadyForSwap = 1;           /* signal ready for swap */
        if(pParm->bWaitForSwap) {
            down_interruptible(&osdanim_swap.sem);    /* wait for swap to take place */
        }
        osdanim_swap.missedInts--;
        rtn = 0;	
    } else {
        rtn = 0;
	    if(pParm->hOldSurface != 0) {
            rtn |= osd_osi_detach_comp_gfx_surface(pParm->graphDev, &gpHandles[pParm->hOldSurface].surface);
        }
        rtn |= osd_osi_attach_comp_gfx_surface(pParm->graphDev, &gpHandles[pParm->hNewSurface].surface);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////

    return rtn;
}


int gfx_inf_h_get_screen_info(GFX_SCREEN_INFO_T *pInfo)
{
    if(NULL == pInfo)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    _OS_MEMCPY(pInfo, &gScreenInfo, sizeof(GFX_SCREEN_INFO_T));
    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return 0;
}


int gfx_inf_h_set_screen_info(GFX_SCREEN_INFO_T *pInfo)
{
    int i;
    
    UINT oldLeft, oldUpper;
    
    if(NULL == pInfo)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }
    
    oldLeft = gScreenInfo.uLeft;
    oldUpper = gScreenInfo.uUpper;
    
    _OS_MEMCPY(&gScreenInfo, pInfo, sizeof(GFX_SCREEN_INFO_T));

    // adjust cursor
    {
        int x,y, rtn;
        rtn =  osd_osi_get_cursor_position(&x, &y);
        if(0 == rtn)
        {
            if(x < (INT)gScreenInfo.uLeft) x = 0;
            else x = x - (INT)gScreenInfo.uLeft;
            if(y < (INT)gScreenInfo.uUpper) y = 0;
            else y = y - (INT)gScreenInfo.uUpper;

            osd_osi_set_cursor_position(x + (INT)gScreenInfo.uLeft, 
            y + (INT)gScreenInfo.uUpper);
        }
    }
    // adjust every allocated surface
    for(i=0; i<guMaxHandles; i++)
    {
        if(gfx_osi_pSurface_alloc(&gpHandles[i].surface) &&
            IS_SURFACE_OSD_COMP(gpHandles[i].surface.uPlaneConfig) &&
            !IS_SURFACE_CURSOR_COMP(gpHandles[i].surface.uPlaneConfig))
        {
            ULONG lparm1, lparm2;

            PDEBUG("Surface %d %8.8x\n", i, gpHandles[i].surface.uPlaneConfig); 
            
            osd_osi_get_comp_gfx_surface_parm(&gpHandles[i].surface, 
                OSD_GRAPH_SURFACE_SCREEN_OFFSET, &lparm1, &lparm2);
            if(lparm1 < (ULONG)oldLeft) 
            {
                lparm1 = (ULONG)gScreenInfo.uLeft;
            }
            else 
            {
                lparm1 = lparm1 - (ULONG)oldLeft + (ULONG)gScreenInfo.uLeft;
            }
            
            if(lparm2 < (ULONG)oldUpper) 
            {
                lparm2 = (ULONG)gScreenInfo.uUpper;
            }
            else 
            {
                lparm2 = lparm2 - (ULONG)oldUpper + (ULONG)gScreenInfo.uUpper;
            }
            osd_osi_set_comp_gfx_surface_parm(&gpHandles[i].surface, 
                OSD_GRAPH_SURFACE_SCREEN_OFFSET, lparm1, lparm2);
    	    PDEBUG("done\n"); 
        }
    }
    
    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return 0;
}


int gfx_inf_h_get_surface_display_parm(GFX_SURFACE_DISPLAY_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm || pParm->hSurface < 0 || pParm->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hSurface].surface)
        && IS_SURFACE_OSD_COMP(gpHandles[pParm->hSurface].surface.uPlaneConfig))
    {
        ULONG lparm1, lparm2;

        pParm->uStartX = 0;
        pParm->uStartY = 0;
        osd_osi_get_comp_gfx_surface_parm(&gpHandles[pParm->hSurface].surface, 
            OSD_GRAPH_SURFACE_FLICKER_CORRECTION, &lparm1, &lparm2);
        pParm->bFlickerCorrect = (BYTE)lparm1;

        osd_osi_get_comp_gfx_surface_parm(&gpHandles[pParm->hSurface].surface, 
            OSD_GRAPH_SURFACE_SCREEN_OFFSET, &lparm1, &lparm2);
        if((UINT)lparm1 < gScreenInfo.uLeft) 
        {
            pParm->uWinX = 0;
        }
        else 
        {
            pParm->uWinX = (UINT)lparm1 - gScreenInfo.uLeft;
        }

        if((UINT)lparm2 < gScreenInfo.uUpper) 
        {
            pParm->uWinY = 0;
        }
        else 
        {
            pParm->uWinY = (UINT)lparm2 - gScreenInfo.uUpper;
        }

        pParm->uWinWidth  = gpHandles[pParm->hSurface].surface.plane[0].uAllocWidth;
        pParm->uWinHeight = gpHandles[pParm->hSurface].surface.plane[0].uAllocHeight;
        rtn = 0;
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_set_surface_display_parm(GFX_SURFACE_DISPLAY_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm || pParm->hSurface < 0 || pParm->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hSurface].surface)
        && IS_SURFACE_OSD_COMP(gpHandles[pParm->hSurface].surface.uPlaneConfig))
    {
        PDEBUGE("Surface %d win change to (%d, %d)\n", pParm->hSurface, pParm->uWinWidth, pParm->uWinHeight);

        osd_osi_set_comp_gfx_surface_parm(&gpHandles[pParm->hSurface].surface, 
            OSD_GRAPH_SURFACE_SCREEN_OFFSET, 
            (ULONG)(gScreenInfo.uLeft  + pParm->uWinX),
            (ULONG)(gScreenInfo.uUpper + pParm->uWinY));
        
        osd_osi_set_comp_gfx_surface_parm(&gpHandles[pParm->hSurface].surface, 
            OSD_GRAPH_SURFACE_FLICKER_CORRECTION, 
            (ULONG)pParm->bFlickerCorrect, (ULONG)0);

        rtn = 0;
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_get_palette(GFX_SURFACE_ACCESS_PALETTE_PARM_T *pParm)
{
    int rtn= -1;

    if(!pParm || !pParm->pPalette ||  pParm->hSurface < 0 || pParm->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hSurface].surface)
        && IS_GFX_SURFACE_CLUT(gpHandles[pParm->hSurface].surface.uPlaneConfig))
    {
        rtn = gfx_osi_get_surface_palette(&gpHandles[pParm->hSurface].surface, pParm->pPalette, pParm->uStart, pParm->uCount);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}

int gfx_inf_h_set_palette(GFX_SURFACE_ACCESS_PALETTE_PARM_T *pParm)
{
    int rtn= -1;

    if(!pParm || !pParm->pPalette ||  pParm->hSurface < 0 || pParm->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hSurface].surface)
        && IS_GFX_SURFACE_CLUT(gpHandles[pParm->hSurface].surface.uPlaneConfig))
    {
        rtn = gfx_osi_set_surface_palette(&gpHandles[pParm->hSurface].surface, pParm->pPalette, pParm->uStart, pParm->uCount);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}



int gfx_inf_h_move_cursor(GFX_COORDINATE_T *pParm)
{
    if(NULL == pParm || pParm->nCursorX < 0 || pParm->nCursorY < 0)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    return osd_osi_set_cursor_position(pParm->nCursorX + (INT)gScreenInfo.uLeft, 
        pParm->nCursorY + (INT)gScreenInfo.uUpper);
}

int gfx_inf_h_report_cursor_position(GFX_COORDINATE_T *pParm)
{
    int rtn;
    int x, y;
    if(NULL == pParm< 0)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    rtn =  osd_osi_get_cursor_position(&x, &y);

    if(rtn == 0)
    {
        if(x < (INT)gScreenInfo.uLeft) pParm->nCursorX = 0;
        else pParm->nCursorX = x - (INT)gScreenInfo.uLeft;
        if(y < (INT)gScreenInfo.uUpper) pParm->nCursorY = 0;
        else pParm->nCursorY = y - (INT)gScreenInfo.uUpper;
    }
    return rtn;
}

int gfx_inf_h_set_cursor_attributes(GFX_CURSOR_ATTRUBUTE_PARM_T *pParm)
{
    int rtn = -1;

    if(NULL == pParm || pParm->hCursor < 0 || pParm->hCursor >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hCursor].surface)
        && IS_SURFACE_CURSOR_COMP(gpHandles[pParm->hCursor].surface.uPlaneConfig))
    {
        rtn = osd_osi_set_cursor_attributes(&gpHandles[pParm->hCursor].surface, pParm->uIndex, pParm->attr);
    }
 
    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}



int gfx_inf_h_set_display_control(GFX_DISPLAY_CONTROL_PARM_T *pParm)
{
	int rtn;

    if(NULL == pParm)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    switch(pParm->parm)
    {
    case GFX_DISP_CNTL_BACKCOLOR:  // 16 bits background color in y8:u4:v4
        return osd_osi_set_display_parm(OSD_DISP_CNTL_BACKCOLOR, pParm->uAttr);
    case GFX_DISP_CNTL_VALPHA:    // Video alpha, 8 bits attr  00 transparnet, ff opaque
        return -1;                  // not supported
    case GFX_DISP_CNTL_AVALPHA:   // Alternative video alpha, 8 bits attr  00 transparnet, ff opaque
        return -1;                  // not supported
    case GFX_DISP_CNTL_AFVP:      // Anti-flicker video plane, 0 disable, 1 enable
        return osd_osi_set_display_parm(OSD_DISP_CNTL_AFVP, pParm->uAttr);
    case GFX_DISP_CNTL_EDAF:      // Enable display anti-flicker, 0 disable, 1 enable
        return osd_osi_set_display_parm(OSD_DISP_CNTL_EDAF, pParm->uAttr);
    case GFX_DISP_CNTL_AFDT:      // Anti-flicker detection threshold, 2 bits attr
        return osd_osi_set_display_parm(OSD_DISP_CNTL_AFDT, pParm->uAttr);
    case GFX_DISP_CNTL_VPAFC:     // Video plane anti-flicker correction, 2 bits attr
        return osd_osi_set_display_parm(OSD_DISP_CNTL_VPAFC, pParm->uAttr);
    case GFX_DISP_CNTL_FP:        // Force progressive, 0 don't, 1 upsample progressive
        return -1;                  // not supported
    case GFX_DISP_CNTL_OSDIRQ:    // OSD animation interrupt enable, 1 bit attr -- BJC 102102
        if(pParm->uAttr == 1) {
            // first set animation rate to 30Fps NTSC / 25 Fps PAL
	        rtn = osd_osi_set_display_parm(OSD_DISP_CNTL_ANIMR, 0);// Animation rate, 3 bits attr
            if(rtn != 0) {
                return rtn;
		    }
		}
                
       return osd_osi_set_display_parm(OSD_DISP_CNTL_ANIM, pParm->uAttr);// Animation mode, 0 no, 1 yes
       
    case GFX_DISP_CNTL_OSDHSR:     // OSD horizontal FIR scaling ratio, 9 bits attr -- BJC 102102
        return osd_osi_set_display_parm(OSD_DISP_CNTL_CHFSR, pParm->uAttr);
	    
    default:
        return -1;
    }
}

int gfx_inf_h_get_display_control(GFX_DISPLAY_CONTROL_PARM_T *pParm)
{
    if(NULL == pParm)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    switch(pParm->parm)
    {
    case GFX_DISP_CNTL_BACKCOLOR:  // 16 bits background color in y8:u4:v4
        pParm->uAttr = osd_osi_get_display_parm(OSD_DISP_CNTL_BACKCOLOR);
        break;
    case GFX_DISP_CNTL_VALPHA:    // Video alpha, 8 bits attr  00 transparnet, ff opaque
        return -1;          // not supported
    case GFX_DISP_CNTL_AVALPHA:   // Alternative video alpha, 8 bits attr  00 transparnet, ff opaque
        return -1;          // not supported
    case GFX_DISP_CNTL_AFVP:      // Anti-flicker video plane, 0 disable, 1 enable
        pParm->uAttr = osd_osi_get_display_parm(OSD_DISP_CNTL_AFVP);
        break;
    case GFX_DISP_CNTL_EDAF:      // Enable display anti-flicker, 0 disable, 1 enable
        pParm->uAttr = osd_osi_get_display_parm(OSD_DISP_CNTL_EDAF);
        break;
    case GFX_DISP_CNTL_AFDT:      // Anti-flicker detection threshold, 2 bits attr
        pParm->uAttr = osd_osi_get_display_parm(OSD_DISP_CNTL_AFDT);
        break;
    case GFX_DISP_CNTL_VPAFC:     // Video plane anti-flicker correction, 2 bits attr
        pParm->uAttr = osd_osi_get_display_parm(OSD_DISP_CNTL_VPAFC);
        break;
    case GFX_DISP_CNTL_FP:        // Force progressive, 0 don't, 1 upsample progressive
        return -1;      // not supported
    case GFX_DISP_CNTL_OSDIRQ:    // OSD animation interrupt enable, 1 bit attr -- BJC 102102
	    pParm->uAttr = osd_osi_get_display_parm(OSD_DISP_CNTL_ANIM);// Animation mode, 0 no, 1 yes
	    break;
    case GFX_DISP_CNTL_OSDHSR:     // OSD horizontal FIR scaling ratio, 9 bits attr -- BJC 102102
        pParm->uAttr = osd_osi_get_display_parm(OSD_DISP_CNTL_CHFSR);
	    break;
    default:
        return -1;
    }
    return 0;
}


int gfx_inf_h_get_visual_device_control(GFX_VISUAL_DEVICE_CONTROL_PARM_T *pParm)
{
    if(NULL == pParm)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    switch(pParm->cntl)
    {
    case GFX_VISUAL_DEVICE_GLOBAL_ALPHA:
        pParm->uAttr = osd_osi_get_device_parm(pParm->graphDev, OSD_GRAPH_DEVICE_GLOBAL_ALPHA); 
        break;

    case GFX_VISUAL_DEVICE_ENABLE:
        pParm->uAttr = osd_osi_get_device_parm(pParm->graphDev, OSD_GRAPH_DEVICE_ENABLE); 
        break;

    default:
        return -1;
    }
    return 0;
}

int gfx_inf_h_set_visual_device_control(GFX_VISUAL_DEVICE_CONTROL_PARM_T *pParm)
{
    if(NULL == pParm)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    switch(pParm->cntl)
    {
    case GFX_VISUAL_DEVICE_GLOBAL_ALPHA:
        return osd_osi_set_device_parm(pParm->graphDev, OSD_GRAPH_DEVICE_GLOBAL_ALPHA, pParm->uAttr);

    case GFX_VISUAL_DEVICE_ENABLE:
        return osd_osi_set_device_parm(pParm->graphDev, OSD_GRAPH_DEVICE_ENABLE, pParm->uAttr); 
       
    default:
        return -1;
    }
    return 0;
}

int gfx_inf_h_wait_for_engine(int nTimeout)
{
    int rtn;
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    rtn =  gfx_osi_wait_for_engine(nTimeout);

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}

int gfx_inf_h_reset_engine(void)
{
    int rtn;
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }
        
    rtn =  gfx_osi_reset_engine();
    
    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}

int gfx_inf_h_set_engine_mode(int enable_sync)
{
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }
        
    gWait_for_finish = enable_sync ? 1 : 0;
    
    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return 0;
    
}

int gfx_inf_h_get_engine_mode()
{
    return gWait_for_finish;
}


int gfx_inf_h_set_surface_clip_rect(GFX_SET_CLIP_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm || pParm->hSurface < 0 || pParm->hSurface >= (int)guMaxHandles)
    {
        PDEBUG("Bad parm\n");
        return -1;
    }
    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hSurface].surface))
    {
        rtn = gfx_osi_set_surface_clip_region(&gpHandles[pParm->hSurface].surface, 
            &pParm->rect);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}



int gfx_inf_h_bitBLT(GFX_BITBLT_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm 
        || pParm->hSrcSurface < 0 || pParm->hSrcSurface >= (int)guMaxHandles 
        || pParm->hDesSurface < 0 || pParm->hDesSurface >= (int)guMaxHandles )
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hDesSurface].surface) &&
        gfx_osi_pSurface_alloc(&gpHandles[pParm->hSrcSurface].surface)   )
    {
        rtn = gfx_osi_bitBLT(&gpHandles[pParm->hDesSurface].surface, 
            pParm->uDesX, pParm->uDesY, pParm->uWidth, pParm->uHeight,
            &gpHandles[pParm->hSrcSurface].surface,
            pParm->uSrcX, pParm->uSrcY,
            &pParm->alphaSelect, pParm->enableGammaCorrection);
        if(rtn == 0) 
            rtn = gfx_osi_run_engine(gWait_for_finish);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}



int gfx_inf_h_advancedBitBLT(GFX_ADV_BITBLT_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm 
        || pParm->hSrcSurface  < 0 || pParm->hSrcSurface  >= (int)guMaxHandles 
        || pParm->hDesSurface  < 0 || pParm->hDesSurface  >= (int)guMaxHandles 
        || pParm->hMaskSurface >= (int)guMaxHandles )
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    if(pParm->hMaskSurface >= 0 && !gfx_osi_pSurface_alloc(&gpHandles[pParm->hMaskSurface].surface))
    {
        PDEBUG("Bad mask\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hDesSurface].surface) &&
        gfx_osi_pSurface_alloc(&gpHandles[pParm->hSrcSurface].surface))
    {
        rtn = gfx_osi_advancedBitBLT(&gpHandles[pParm->hDesSurface].surface, 
            pParm->uDesX, pParm->uDesY, pParm->uWidth, pParm->uHeight,
            &gpHandles[pParm->hSrcSurface].surface,
            pParm->uSrcX, pParm->uSrcY,
            pParm->hMaskSurface >= 0 ? &gpHandles[pParm->hMaskSurface].surface : NULL, 
            pParm->uMaskX, pParm->uMaskY,
            pParm->ROP, pParm->enablePixelBitMask, pParm->uPixelBitMask,
            &pParm->alphaSelect);
        if(rtn == 0) 
            rtn = gfx_osi_run_engine(gWait_for_finish);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_fillBLT(GFX_FILLBLT_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm 
        || pParm->hDesSurface  < 0 || pParm->hDesSurface  >= (int)guMaxHandles )
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hDesSurface].surface) )
    {
        rtn = gfx_osi_fillBLT(&gpHandles[pParm->hDesSurface].surface, 
            pParm->uDesX, pParm->uDesY, pParm->uWidth, pParm->uHeight,
            pParm->uFillColor);
        if(rtn == 0) 
            rtn = gfx_osi_run_engine(gWait_for_finish);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_advancedFillBLT(GFX_ADV_FILLBLT_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm 
        || pParm->hDesSurface  < 0 || pParm->hDesSurface  >= (int)guMaxHandles 
        || pParm->hMaskSurface >= (int)guMaxHandles )
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    if(pParm->hMaskSurface >= 0 && !gfx_osi_pSurface_alloc(&gpHandles[pParm->hMaskSurface].surface))
    {
        PDEBUG("Bad mask\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hDesSurface].surface))
    {
        rtn = gfx_osi_advancedFillBLT(&gpHandles[pParm->hDesSurface].surface, 
            pParm->uDesX, pParm->uDesY, pParm->uWidth, pParm->uHeight,
            pParm->uFillColor,
            pParm->hMaskSurface >= 0 ? &gpHandles[pParm->hMaskSurface].surface : NULL, 
            pParm->uMaskX, pParm->uMaskY,
            pParm->ROP, pParm->transparencyEnable, pParm->uBackGroundColor,
            pParm->enablePixelBitMask, pParm->uPixelBitMask );
        if(rtn == 0) 
            rtn = gfx_osi_run_engine(gWait_for_finish);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_blend(GFX_BLEND_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm 
        || pParm->hSrcSurface < 0 || pParm->hSrcSurface >= (int)guMaxHandles 
        || pParm->hDesSurface < 0 || pParm->hDesSurface >= (int)guMaxHandles )
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hDesSurface].surface) &&
        gfx_osi_pSurface_alloc(&gpHandles[pParm->hSrcSurface].surface)   )
    {
        rtn = gfx_osi_blend(&gpHandles[pParm->hDesSurface].surface, 
            pParm->uDesX, pParm->uDesY, pParm->uWidth, pParm->uHeight,
            &gpHandles[pParm->hSrcSurface].surface,
            pParm->uSrcX, pParm->uSrcY,
            &pParm->blendSelect);
        if(rtn == 0) 
            rtn = gfx_osi_run_engine(gWait_for_finish);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_advancedBlend(GFX_ADV_BLEND_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm 
        || pParm->hSrcSurface  < 0 || pParm->hSrcSurface  >= (int)guMaxHandles 
        || pParm->hDesSurface  < 0 || pParm->hDesSurface  >= (int)guMaxHandles 
        || pParm->hAlphaSurface >= (int)guMaxHandles )
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    if(pParm->hAlphaSurface >= 0 && !gfx_osi_pSurface_alloc(&gpHandles[pParm->hAlphaSurface].surface))
    {
        PDEBUG("Bad alpha \n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hDesSurface].surface) &&
        gfx_osi_pSurface_alloc(&gpHandles[pParm->hSrcSurface].surface))
    {
        rtn = gfx_osi_advancedBlend(&gpHandles[pParm->hDesSurface].surface, 
            pParm->uDesX, pParm->uDesY, pParm->uWidth, pParm->uHeight,
            &gpHandles[pParm->hSrcSurface].surface,
            pParm->uSrcX, pParm->uSrcY,
            pParm->hAlphaSurface >= 0 ? &gpHandles[pParm->hAlphaSurface].surface : NULL, 
            pParm->uAlphaX, pParm->uAlphaY,
            &pParm->blendSelect);
        if(rtn == 0) 
            rtn = gfx_osi_run_engine(gWait_for_finish);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;

}


int gfx_inf_h_colorKey(GFX_COLORKEY_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm 
        || pParm->hSrcSurface < 0 || pParm->hSrcSurface >= (int)guMaxHandles 
        || pParm->hDesSurface < 0 || pParm->hDesSurface >= (int)guMaxHandles )
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hDesSurface].surface) &&
        gfx_osi_pSurface_alloc(&gpHandles[pParm->hSrcSurface].surface)   )
    {
        rtn = gfx_osi_colorKey(&gpHandles[pParm->hDesSurface].surface, 
            pParm->uDesX, pParm->uDesY, pParm->uWidth, pParm->uHeight,
            &gpHandles[pParm->hSrcSurface].surface,
            pParm->uSrcX, pParm->uSrcY,
            &pParm->colorKeySelect, &pParm->alphaSelect);
        if(rtn == 0) 
            rtn = gfx_osi_run_engine(gWait_for_finish);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}


int gfx_inf_h_resize(GFX_RESIZE_PARM_T *pParm)
{
    int rtn= -1;

    if(NULL == pParm 
        || pParm->hSrcSurface < 0 || pParm->hSrcSurface >= (int)guMaxHandles 
        || pParm->hDesSurface < 0 || pParm->hDesSurface >= (int)guMaxHandles )
    {
        PDEBUG("Bad parm\n");
        return -1;
    }

    /////////////////////////////
    if(os_get_mutex(gGFXMutex))
    {
        PDEBUG("Failed on mutex!\n");
        return -1;
    }

    if(gfx_osi_pSurface_alloc(&gpHandles[pParm->hDesSurface].surface) &&
        gfx_osi_pSurface_alloc(&gpHandles[pParm->hSrcSurface].surface)   )
    {
        rtn = gfx_osi_resize(&gpHandles[pParm->hDesSurface].surface, 
            pParm->uDesX, pParm->uDesY, pParm->uDesWidth, pParm->uDesHeight,
            &gpHandles[pParm->hSrcSurface].surface,
            pParm->uSrcX, pParm->uSrcY, pParm->uSrcWidth, pParm->uSrcHeight,
            pParm->destAlpha, pParm->enableGammaCorrection);
        if(rtn == 0) 
            rtn = gfx_osi_run_engine(gWait_for_finish);
    }

    os_release_mutex(gGFXMutex);
    /////////////////////////////
    return rtn;
}

/*
 * Dispatch dmablt request. -- BJC 102102
 */
int gfx_inf_h_dmablt(GFX_DMABLT_PARM_T *pParm)
{
    gfx_atom_dmablt_handle_request(pParm);
    return 0;
}

/*
 * Dispatch dmablt wait request. -- BJC 102102
 */
int gfx_inf_h_dmablt_wait(void)
{
    gfx_atom_dmablt_wait_handle_request();
    return 0;
}

/*
 * Dispatch pmalloc request. -- BJC 102102
 */
int gfx_inf_h_pmalloc(GFX_PMALLOC_PARM_T *pParm)
{
    return gfx_atom_pmalloc_handle_request(pParm);
}
