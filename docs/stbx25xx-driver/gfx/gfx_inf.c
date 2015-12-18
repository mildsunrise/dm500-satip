//vulcan/drv/gfx/gfx_inf.c
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
//  Linux device driver interface of GFX controls 
//Revision Log:   
//  Nov/06/2001                          Created by YYD
//  Oct/21/2002   Added IOCTL dispatch handlers for IOC_GFX_SWAP_SURFACE,
//                    IOC_GFX_DMABLT, IOC_GFX_DMABLT_WAIT, and IOC_GFX_PMALLOC.
//                Added initialization of gfx atom
//                Allow mmap() of alien memory regions, but cache them.    BJC

#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <asm/system.h>
#include <linux/string.h>

#include <linux/fs.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/mm.h> 
#include <asm/pgtable.h>

#include "gfx/gfx_inf.h"
#include "os/os-generic.h"

#include "gfx_inf_helper.h"

#include "gfx_atom.h" // BJC 102102

//#define __GFX_INF_DEBUG

#ifdef __GFX_INF_DEBUG
    #define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#ifndef NULL
    // My local NULL definition
    #define NULL ((void *)0)
#endif

#define GFX_DRIVER_NAME   "STBx25xx/03xxx GFX"

#include "os/pversion.h"


#define GFX_INF_HANDLE_HOOKED       1
#define GFX_INF_HANDLE_SHARED       2

#ifdef MODULE

MODULE_AUTHOR("Yudong Yang / IBM CRL");
MODULE_DESCRIPTION("OSD Graphics driver for IBM STBx25xx/03xxx Chip");

#endif

static int gfx_inf_open(struct inode *inode, struct file *file);
static int gfx_inf_release(struct inode *inode, struct file *file);

static int gfx_inf_ioctl(struct inode *inode, 
                     struct file *filp, 
                     unsigned int ioctl_cmd, 
                     unsigned long ioctl_parm);

static int gfx_inf_mmap(struct file *file, struct vm_area_struct *vma);


static struct file_operations GfxFops = 
{
    open:       gfx_inf_open,
    release:    gfx_inf_release,      /* a.k.a. close */
    ioctl:      gfx_inf_ioctl,
    mmap:       gfx_inf_mmap
};


// just dummy open 
static int gfx_inf_open(struct inode *inode, struct file *file)
{
    BYTE *pHandleAttrs;

    PDEBUG("filep=0x%8.8x\n", (unsigned int)file);

    pHandleAttrs = (BYTE *) MALLOC(GFX_DEFAULT_SURFACES*sizeof(BYTE));

    if(!pHandleAttrs)
    {
        PFATAL("Failed to allocate private file handle data !\n");
        return -EIO;
    }
    memset(pHandleAttrs, 0, GFX_DEFAULT_SURFACES*sizeof(BYTE));
    file->private_data = pHandleAttrs;

    MOD_INC_USE_COUNT;  // MODULE

    return 0;
}

// just dummy close
static int gfx_inf_release(struct inode *inode, struct file *file)
{
    PDEBUG("filep=0x%8.8x\n", (unsigned int)file);

    if(file->private_data)
    {
        BYTE *pHandleAttrs = (BYTE *)file->private_data;
        int i;

        for(i=0; i<GFX_DEFAULT_SURFACES; i++)
        {
            if(pHandleAttrs[i] & GFX_INF_HANDLE_HOOKED)
                gfx_inf_h_destroy_surface(i);
        }
        FREE(pHandleAttrs);
    }
    MOD_DEC_USE_COUNT;

    return 0;
}


static int gfx_inf_ioctl(struct inode *inode, 
                     struct file *filp, 
                     unsigned int ioctl_cmd, 
                     unsigned long ioctl_parm) 
{
    int rtn = -1;

    PDEBUG("cmd magic=0x%2.2x, id=%d, mode=0x%2.2x, parm= 0x%8.8x\n", _IOC_TYPE(ioctl_cmd), _IOC_NR(ioctl_cmd), _IOC_DIR(ioctl_cmd), (UINT)ioctl_parm);
    switch(ioctl_cmd)
    {
    case IOC_GFX_CREATE_SURFACE:
        {
            GFX_CREATE_SURFACE_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_create_surface(&parm);
                if(rtn >= 0)
                {
                    put_user(parm.hSurface, &((GFX_CREATE_SURFACE_PARM_T *)ioctl_parm)->hSurface);
                    if(filp->private_data)  // hook it by default
                    {
                        ((BYTE *)filp->private_data)[parm.hSurface] = GFX_INF_HANDLE_HOOKED;
                    }
                }
                else
                {
                    PDEBUG("Failed to create surface\n");
                }
            }
            else
            {
                rtn = -1;
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_DESTROY_SURFACE:
        {
            rtn = gfx_inf_h_destroy_surface((int)ioctl_parm);
            if(!rtn && filp->private_data)
            {
                ((BYTE *)filp->private_data)[(int)ioctl_parm] = 0;
            }
            break;
        }

    case IOC_GFX_GET_SUBPLANE_PSEUDO_SURFACE:
        {
            GFX_GET_SUBPLANE_PSEUDO_SURFACE_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_get_subplane_pseudo_surface(&parm);
                if(rtn >= 0)
                {
                    put_user(parm.hSurface, &((GFX_GET_SUBPLANE_PSEUDO_SURFACE_PARM_T *)ioctl_parm)->hSurface);
                    if(filp->private_data)  // hook it by default
                    {
                        ((BYTE *)filp->private_data)[parm.hSurface] = GFX_INF_HANDLE_HOOKED;
                    }
                }
                else
                {
                    PDEBUG("Failed to create surface\n");
                }
            }
            else
            {
                rtn = -1;
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_HOOK_SURFACE:
        {
            if((int)ioctl_parm  >= 0 && (int)ioctl_parm < GFX_DEFAULT_SURFACES && filp->private_data)
            {
                ((BYTE *)filp->private_data)[(int)ioctl_parm] |= GFX_INF_HANDLE_HOOKED;
                rtn = 0;
            }
            else
                rtn = -1;
            break;
        }

    case IOC_GFX_UNHOOK_SURFACE:
        {
            if((int)ioctl_parm  >= 0 && (int)ioctl_parm < GFX_DEFAULT_SURFACES && filp->private_data)
            {
                ((BYTE *)filp->private_data)[(int)ioctl_parm] &= ~GFX_INF_HANDLE_HOOKED;
                rtn = 0;
            }
            else
                rtn = -1;
            break;
        }

    case IOC_GFX_GET_SURFACE_INFO :
        {
            GFX_SURFACE_INFO_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            get_user(parm.hSurface, &((GFX_SURFACE_INFO_T *)ioctl_parm)->hSurface);
            rtn = gfx_inf_h_get_surface_info(&parm);
            if(rtn >= 0 )
            {
                if(copy_to_user((void *)ioctl_parm, &parm, sizeof(parm)))
                    rtn = -1;
            }
        }

    case IOC_GFX_LOCK_SURFACE:
        {
            GFX_SURFACE_INFO_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            get_user(parm.hSurface, &((GFX_SURFACE_INFO_T *)ioctl_parm)->hSurface);
            rtn = gfx_inf_h_lock_surface(&parm);
            if(rtn >= 0 )
            {
                if(copy_to_user((void *)ioctl_parm, &parm, sizeof(parm)))
                    rtn = -1;
            }
        }
        break;

    case IOC_GFX_UNLOCK_SURFACE:
        {
            rtn = gfx_inf_h_unlock_surface((int)ioctl_parm);
            break;
        }

    case IOC_GFX_GET_SURFACE_PALETTE:
        {
            GFX_SURFACE_ACCESS_PALETTE_PARM_T parm, tparm;
            GFX_PALETTE_T pal[256];
            
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                if(!parm.pPalette)
                {
                    PDEBUG("Input parm.pPalette is NULL\n");
                    break;
                }
                tparm = parm;
                tparm.pPalette = pal;
                rtn = gfx_inf_h_get_palette(&tparm);
                if(rtn)
                    break;
                if(copy_to_user(parm.pPalette, tparm.pPalette, parm.uCount*sizeof(GFX_PALETTE_T)))
                {
                    PDEBUG("Failed to get palette\n");
                    rtn = -1;
                }
                else
                    rtn = 0;
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }
        
    case IOC_GFX_SET_SURFACE_PALETTE:
        {
            GFX_SURFACE_ACCESS_PALETTE_PARM_T parm, tparm;
            GFX_PALETTE_T pal[256];

            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                tparm = parm;
                tparm.pPalette = pal;
                if(!parm.pPalette)
                {
                    PDEBUG("Input parm.pPalette is NULL\n");
                    break;
                }
                if(copy_from_user(tparm.pPalette, parm.pPalette, parm.uCount*sizeof(GFX_PALETTE_T)) == 0)
                {
                    rtn = gfx_inf_h_set_palette(&tparm);
                }
                else
                {
                    PDEBUG("Failed to set palette\n");
                }
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_GET_SHARED_SURFACE:
        {
            rtn = gfx_inf_h_get_shared_surface();
            break;
        }

    case IOC_GFX_SET_SHARED_SURFACE:
        {
            rtn = gfx_inf_h_set_shared_surface((int)ioctl_parm);
            break;
        }

    case IOC_GFX_ATTACH_SURFACE:
        {
            GFX_SURFACE_VDEV_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_attach_surface(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_DETACH_SURFACE:
        {
            GFX_SURFACE_VDEV_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_detach_surface(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    /*
     * Detach one surface and attach another; may be synchronized with OSD
     * animation interrupt -- BJC 102102
     */
    case IOC_GFX_SWAP_SURFACE:
        {
            GFX_2SURFACE_VDEV_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm,
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_swap_surface(&parm);
                copy_to_user((void *)ioctl_parm, &parm, sizeof(parm));
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }


    case IOC_GFX_GET_SURFACE_DISP_PARM:
        {
            GFX_SURFACE_DISPLAY_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            get_user(parm.hSurface, &((GFX_SURFACE_DISPLAY_T *)ioctl_parm)->hSurface);
            rtn = gfx_inf_h_get_surface_display_parm(&parm);
            if(rtn >= 0)
            {
                if(copy_to_user((void *)ioctl_parm, &parm,
                    sizeof(parm)))
                    rtn = -1;
            }
            break;
        }

    case IOC_GFX_SET_SURFACE_DISP_PARM:
        {
            GFX_SURFACE_DISPLAY_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_set_surface_display_parm(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_SET_CURSOR_ATTRIBUTE:
        {
            GFX_CURSOR_ATTRUBUTE_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_set_cursor_attributes(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_MOVE_CURSOR:
        {
            GFX_COORDINATE_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_move_cursor(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_REPORT_CURSOR:
        {
            GFX_COORDINATE_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            rtn = gfx_inf_h_report_cursor_position(&parm);
            if(rtn >= 0)
            {
                if(copy_to_user((void *)ioctl_parm, &parm,
                    sizeof(parm)))
                    rtn = -1;
            }
            break;
        }

    case IOC_GFX_GET_SCREEN_INFO:
        {
            GFX_SCREEN_INFO_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            rtn = gfx_inf_h_get_screen_info(&parm);
            if(rtn >= 0)
            {
                if(copy_to_user((void *)ioctl_parm, &parm,
                    sizeof(parm)))
                    rtn = -1;
            }
            break;
        }

    case IOC_GFX_SET_SCREEN_INFO:
        {
            GFX_SCREEN_INFO_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_set_screen_info(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_SET_DISPLAY_CONTROL:
        {
            GFX_DISPLAY_CONTROL_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_set_display_control(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_GET_DISPLAY_CONTROL:
        {
            GFX_DISPLAY_CONTROL_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_get_display_control(&parm);
                if(rtn >= 0)
                {
                    if(copy_to_user((void *)ioctl_parm, &parm,
                        sizeof(parm)))
                        rtn = -1;
                }
            }
            break;
        }

    case IOC_GFX_SET_VISUAL_DEVICE_CONTROL:
        {
            GFX_VISUAL_DEVICE_CONTROL_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_set_visual_device_control(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_GET_VISUAL_DEVICE_CONTROL:
        {
            GFX_VISUAL_DEVICE_CONTROL_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_get_visual_device_control(&parm);
                if(rtn >= 0)
                {
                    if(copy_to_user((void *)ioctl_parm, &parm,
                        sizeof(parm)))
                        rtn = -1;
                }
            }
            break;
        }

    case IOC_GFX_SET_SURFACE_CLIP_RECT:
        {
            GFX_SET_CLIP_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_set_surface_clip_rect(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            if(rtn >= 0)
            {
                if(copy_to_user((void *)ioctl_parm, &parm,
                    sizeof(parm)))
                    rtn = -1;
            }
            break;
        }

    case IOC_GFX_BITBLT:
        {
            GFX_BITBLT_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_bitBLT(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_ADV_BITBLT:
        {
            GFX_ADV_BITBLT_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_advancedBitBLT(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_FILLBLT:
        {
            GFX_FILLBLT_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_fillBLT(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_ADV_FILLBLT:
        {
            GFX_ADV_FILLBLT_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_advancedFillBLT(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_BLEND:
        {
            GFX_BLEND_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_blend(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_ADV_BLEND:
        {
            GFX_ADV_BLEND_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_advancedBlend(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_COLORKEY:
        {
            GFX_COLORKEY_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_colorKey(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    case IOC_GFX_RESIZE:
        {
            GFX_RESIZE_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm, 
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_resize(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    /*
     * Allocate physically contiguous memory suitable for use by graphics hardware or DMA controller.
     * For use with cached mmap() to allow user space application to set up memory prior to the transfer.
     * -- BJC 102102
     */
     case IOC_GFX_PMALLOC:
        {
            GFX_PMALLOC_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm,
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_pmalloc(&parm);
                copy_to_user((void *)ioctl_parm, &parm, sizeof(parm));		
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    /*
     *  Transfer video frame from system memory to frame buffer using a DMA controller.
     *  -- BJC 102102
     */
    case IOC_GFX_DMABLT:
        {
            GFX_DMABLT_PARM_T parm;
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            if(copy_from_user(&parm, (void *)ioctl_parm,
                sizeof(parm)) == 0)
            {
                rtn = gfx_inf_h_dmablt(&parm);
            }
            else
            {
                PDEBUG("copy from user fail!\n");
            }
            break;
        }

    /*
     *  If an asynchronous mode of transfer was used for DMABLT, wait for it
     *  to finish. -- BJC 102102
     */
    case IOC_GFX_DMABLT_WAIT:
        {
            if(!ioctl_parm)
            {
                PDEBUG("Invalid IOCTL parm\n");
                break;
            }
            rtn = gfx_inf_h_dmablt_wait();
            break;
        }

    case IOC_GFX_WAIT_FOR_COMPLETE:
        {
            rtn = gfx_inf_h_wait_for_engine((int)ioctl_parm);
            break;    
        }    
    
    case IOC_GFX_RESET_ENGINE:
        {
            rtn = gfx_inf_h_reset_engine();
            break;
        }

    case IOC_GFX_SET_ENGINE_MODE:
        {
            rtn = gfx_inf_h_set_engine_mode((int)ioctl_parm);
            break;    
        }

    case IOC_GFX_GET_ENGINE_MODE:
        {
            rtn = gfx_inf_h_get_engine_mode();
            break;    
        }

    default:
        PDEBUG("Invalid IOCTL cmd magic=0x%2.2x, id=%d, mode=0x%2.2x\n", _IOC_TYPE(ioctl_cmd), _IOC_NR(ioctl_cmd), _IOC_DIR(ioctl_cmd));
        break;
    }

    if(rtn < 0)
    {
        PDEBUG("call failed\n");
    }

    return rtn;
}



static int gfx_inf_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long size;
    
    if(vma->vm_flags & VM_EXEC)
        return -EPERM;
    
    size = vma->vm_end - vma->vm_start;
    PDEBUG("mmap size = 0x%8.8lx\n", size);
    if(size == 0)
        return EINVAL;

    // To make things safer, I'll check if it is my baby
    // Aliens are simply rejected.  
    // The cost of some slowdown should be acceptable
    if(_gfx_inf_h_validate_surface_address(vma->vm_pgoff << PAGE_SHIFT, size))
    {
// BJC 102102        PDEBUG("Invalid surface address specified\n");
// BJC 102102        return -EPERM;  // don't permit it
        PDEBUG("mmaping alien memory region with caching enabled\n"); // BJC 102102
    } else {
// BJC102102        vma->vm_page_prot.pgprot |= _PAGE_NO_CACHE;	// map without caching
    }

    vma->vm_flags |= VM_SHM | VM_IO ; // | VM_DONTCOPY | VM_DONTEXPAND;        // we are just not in system memory
    
    if(remap_page_range(vma->vm_start, vma->vm_pgoff << PAGE_SHIFT, 
        size, vma->vm_page_prot))
        return -EAGAIN;
    return 0;
}

static devfs_handle_t devfs_handle;

static int __init gfx_inf_init(void)
{
    int rtn = gfx_inf_h_init(GFX_MAX_ALLOWED_SURFACES);

    // print the driver verision info for futher reference
    PVERSION(GFX_DRIVER_NAME);

    if(rtn < 0) 
    {
        PFATALE("GFX: Failed to initialize device!\n"); 
        return -1;
    }

    if (devfs_register_chrdev(GFX_DEV_MAJOR, GFX_DRIVER_NAME, &GfxFops) < 0)
    {
        gfx_inf_h_deinit(2);
        PFATALE("GFX: Failed to register device!\n"); 
        return -1;
    }

    devfs_handle = devfs_find_handle(NULL, "stbgfx",
                                0, 0, DEVFS_SPECIAL_CHR,0);
    
    if(devfs_handle == NULL)
    {
      
      devfs_handle = devfs_register(NULL, "stbgfx", DEVFS_FL_DEFAULT,
                                GFX_DEV_MAJOR, 0,
                                S_IFCHR | S_IRUSR | S_IWUSR,
                                &GfxFops, NULL);
    }
    else
      devfs_handle = NULL;
      
    /*
     *  Initialize the dma module -- BJC 102102
     */    
    if(gfx_atom_init() != 0) {
        PFATALE("GFX: Failed to initialize graphics atom!\n");
    }
    return 0;
}

static void __exit gfx_inf_deinit(void)
{
    devfs_unregister_chrdev(GFX_DEV_MAJOR, GFX_DRIVER_NAME);
    if(devfs_handle != NULL)
      devfs_unregister(devfs_handle);
    gfx_inf_h_deinit(2);
    gfx_atom_deinit();  // BJC 102102
}

module_init(gfx_inf_init);
module_exit(gfx_inf_deinit);
