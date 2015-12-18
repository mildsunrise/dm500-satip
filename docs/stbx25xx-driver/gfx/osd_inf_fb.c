//vulcan/drv/gfx/osd_inf_fb.c
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
//  Linux frame buffer interface of OSD controls 
//Revision Log:   
//  Oct/10/2001                          Created by YYD
//  Oct/17/2001                 Merged with gfx  by YYD

/* The necessary header files */
#include <linux/config.h>
#include <linux/version.h>

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/selection.h>    /* for color_table */
#include <linux/sched.h>   // for jiffie

#ifndef CONFIG_FB
  #error "Your kernel is not configed for Frame buffer support !"
#endif

#include <video/fbcon.h>
#ifdef CONFIG_FBCON_CFB8
  #include <video/fbcon-cfb8.h>
#endif

#ifdef CONFIG_FBCON_CFB16
  #include <video/fbcon-cfb16.h>
#endif

#ifdef CONFIG_FBCON_CFB32
  #include <video/fbcon-cfb32.h>
#endif

#include "gfx_inf_helper.h"
//#include "scrman/scrman_osi.h"

//#define __OSD_FB_DEBUG

#ifdef __OSD_FB_DEBUG
    #define __DRV_DEBUG
#endif
#include "os/drv_debug.h"

#define FB_DRIVER_NAME   "STBx25xx/03xxx Framebuffer"

#include "os/pversion.h"



#define OSD_DEFAULT_XRES    640
#define OSD_DEFAULT_YRES    440

#define OSD_DEFAULT_BUF_XRES    OSD_DEFAULT_XRES
#define OSD_DEFAULT_BUF_YRES    OSD_DEFAULT_YRES

#define OSD_DEFAULT_TV_LEFT_MARGIN  30
#define OSD_DEFAULT_TV_UPPER_MARGIN 30

#define OSD_DEFAULT_BPP    8 

// global parameters

#ifdef MODULE

MODULE_PARM(osd_scr_x, "i");
MODULE_PARM_DESC(osd_scr_x, "Specify the viewable graphics screen display horizontal size");
MODULE_PARM(osd_scr_y, "i");
MODULE_PARM_DESC(osd_scr_y,  "Specify the viewable graphics screen display vertical size");
MODULE_PARM(osd_tv_left, "i");
MODULE_PARM_DESC(osd_tv_left, "Adjust for TV horizontal timing offset.");
MODULE_PARM(osd_tv_upper, "i");
MODULE_PARM_DESC(osd_tv_upper, "Adjust for TV vertical timing offset.");

MODULE_PARM(osd_gfx_shared, "i");
MODULE_PARM_DESC(osd_gfx_shared, "Enable sharing of frame buffer with other GFX apps. [1: yes], 0: no.");

MODULE_PARM(osd_gfx_pat, "i");
MODULE_PARM_DESC(osd_gfx_pat, "Show a test pattern for 5 seconds before enable frame buffer. 1: yes, [0: no]");

MODULE_PARM(osd_gfx_af, "i");
MODULE_PARM_DESC(osd_gfx_af, "Set the antiflicker correction levels of frame buffer screen. 0: no, 1:minimal, [2:medium], 3:maximum");

MODULE_PARM(osd_gfx_afdt, "i");
MODULE_PARM_DESC(osd_gfx_afdt, "Set the antiflicker detection level. 0: all, 1:>16, 2:>32, [3:>64]");


MODULE_AUTHOR("Yudong Yang");
MODULE_DESCRIPTION("IBM STBx25xx/03xxx Graphics Framebuffer Driver");  

#endif

static INT osd_scr_x = -1;
static INT osd_scr_y = -1;
static INT osd_buf_x = -1;
static INT osd_buf_y = -1;
static INT osd_gfx_bpp = OSD_DEFAULT_BPP;
static INT osd_tv_left = OSD_DEFAULT_TV_LEFT_MARGIN;
static INT osd_tv_upper = OSD_DEFAULT_TV_UPPER_MARGIN;

static INT osd_gfx_pat = 0;
static INT osd_gfx_shared = 1;

static INT osd_gfx_af = 2;
static INT osd_gfx_afdt = 3;

static struct fb_var_screeninfo osd_fb_var_default = 
{
    xres:           OSD_DEFAULT_XRES,
    yres:           OSD_DEFAULT_YRES,
    xres_virtual:   OSD_DEFAULT_BUF_XRES,
    yres_virtual:   OSD_DEFAULT_BUF_YRES,
    xoffset:        0,
    yoffset:        0,
    bits_per_pixel: OSD_DEFAULT_BPP,
    grayscale:      0,
    red:            {0, 8, 0},
    green:          {0, 8, 0},
    blue:           {0, 8, 0},
    transp:         {0, 8, 0},
    nonstd:         0,
    activate:       0,
    height:         -1,
    width:          -1,
    accel_flags:    0,
    /* Timing */
    pixclock:       0, //74074,
    left_margin:    0,
    right_margin:   0,
    upper_margin:   0,
    lower_margin:   0,
    hsync_len:      0, //108,
    vsync_len:      0, //25,
    sync:           0,
    vmode:          FB_VMODE_NONINTERLACED
};

static char *osd_fb_dev_name = "STB OSD"; // MAX 15 CHAR

static struct fb_info osd_fb_info;
static struct display osd_fb_disp;
static struct display_switch osd_fb_sw;

#if defined(FBCON_HAS_CFB16) || defined(FBCON_HAS_CFB32)
static union {
    u16 cfb16[16];
    u32 cfb32[16];
} fbcon_cmap;
#endif

static GFX_SURFACE_LOCAL_INFO_T fb_surface_info;
static GFX_SURFACE_INFO_T fb_surface_physical;
static GFX_VISUAL_DEVICE_ID_T osd_fb_device = GFX_VDEV_OSDGFX;
static int hFbSurface= -1;

static int currcon   = 0;
static int video_cmap_len = 256;


static void osd_inf_debug_fill_fb(GFX_SURFACE_LOCAL_INFO_T *osd_test_surface)
{
    UINT i,j, k, bpl, x, y, pix, mx, my, nx, ny;
    BYTE *pBuf;
    
    printk(KERN_INFO "Frame buffer information:\n   logical= 0x%8.8x\n", 
		    (UINT)osd_test_surface->plane[0].pPlane);
    pBuf = (BYTE *)osd_test_surface->plane[0].pPlane;
    bpl = osd_test_surface->plane[0].uBytePerLine; 
    y = osd_test_surface->plane[0].uHeight;
    pix = osd_test_surface->plane[0].uPixelSize;
    x = osd_test_surface->plane[0].uWidth;

    printk(KERN_INFO "  width= %u, height= %u,  byte per line= %u,  pixel size=%u\n", x, y, bpl, pix);
    
    pix >>= 3;
    if(pix == 0) return;
    
    my = 1;
    for(j=0; j<y; j+=my)
    {
        for(ny=0; ny<my && j+ny < y; ny++)
        {
            UINT lp = (j+ny)*bpl;
            mx = 1;
            for(i=0; i<x; i+=mx)
            {
                BYTE c = ((mx*my)&0x01) ? 0xff : 0;
                for(nx=0; nx<mx && i+nx < x; nx++)
                {
                    for(k=0; k<pix;k++, lp++)
                        *(pBuf + lp) = c;
                }
                mx++;
            }
        }
        my++;
    }
}

static void osd_inf_clear_fb(GFX_SURFACE_LOCAL_INFO_T *osd_test_surface)
{
    UINT bpl, y;
    
    bpl = osd_test_surface->plane[0].uBytePerLine; 
    y = osd_test_surface->plane[0].uHeight;
    memset(osd_test_surface->plane[0].pPlane, 0, bpl*y);
}

    
static int osd_inf_fb_pan_display(struct fb_var_screeninfo *var, int con,
                              struct fb_info *info)
{
    GFX_SURFACE_DISPLAY_T disp;

    if (var->yoffset+var->yres > var->yres_virtual)
        return -EINVAL;

    if (var->xoffset+var->xres > var->xres_virtual)
        return -EINVAL;

    disp.hSurface = hFbSurface;
    gfx_inf_h_get_surface_display_parm(&disp);
    disp.uStartX = var->xoffset;
    disp.uStartY = var->yoffset;
    disp.uWinWidth = var->xres;
    disp.uWinHeight = var->yres;
    disp.uWinX = 0;
    disp.uWinY = 0;

    if(gfx_inf_h_set_surface_display_parm(&disp) < 0)
    {
        PDEBUG("Failed to pan surface!\n");
        return -EINVAL;
    }

    PDEBUG("pan x = %d, pan y = %d, left = %d, upper = %d\n", var->xoffset, var->yoffset, var->left_margin, var->upper_margin);
    return 0;
}

static int osd_inf_fb_update_var(int con, struct fb_info *info)
{
    if (con == currcon) 
    {
        
        struct fb_var_screeninfo *var = &fb_display[currcon].var;
        GFX_SURFACE_DISPLAY_T disp;
        GFX_SCREEN_INFO_T  scr;

        gfx_inf_h_get_screen_info(&scr);
        scr.uLeft =var->left_margin;
        scr.uUpper = var->upper_margin;
        gfx_inf_h_set_screen_info(&scr);

        disp.hSurface = hFbSurface;
        gfx_inf_h_get_surface_display_parm(&disp);
        disp.uStartX = var->xoffset;
        disp.uStartY = var->yoffset;
        disp.uWinWidth = var->xres;
        disp.uWinHeight = var->yres;
        disp.uWinX = 0;
        disp.uWinY = 0;

        if(gfx_inf_h_set_surface_display_parm(&disp) < 0)
        {
            PDEBUG("Failed to pan surface!\n");
            return -EINVAL;
        }

        PDEBUG("con=%d, x=%d, y=%d, pan x = %d, pan y = %d, left = %d, upper = %d\n", con, var->xres, var->yres, var->xoffset, var->yoffset, var->left_margin, var->upper_margin);

        return osd_inf_fb_pan_display(var,con,info);
    }
    else 
        PDEBUG("UPD VAR nothing\n");
    return 0;
}


static int osd_inf_fb_get_fix(struct fb_fix_screeninfo *fix, int con,
             struct fb_info *info)
{
    memset(fix, 0, sizeof(struct fb_fix_screeninfo));
    strcpy(fix->id, osd_fb_dev_name);

    fix->smem_start= (unsigned int)fb_surface_physical.plane[0].plane.uBase + fb_surface_physical.plane[0].plane.uOffset; // physical address

    fix->smem_len= fb_surface_physical.plane[0].plane.uSize;  // byte per line * height
    fix->line_length= fb_surface_info.plane[0].uBytePerLine; // byte per line

    fix->type = FB_TYPE_PACKED_PIXELS;      // fixme: we may need to support multiple planes
    fix->visual = IS_GFX_SURFACE_CLUT(fb_surface_info.uPlaneConfig) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_TRUECOLOR;

    fix->xpanstep  = fb_surface_info.plane[0].uPixelJustify;    // justification
    fix->ypanstep  = 1;
    fix->ywrapstep = 0;         // no wrap

    PDEBUG("FB FIX smem_start = %08x, smem_len = %08x, line_length = %d\n", 
        fix->smem_start, fix->smem_len, fix->line_length);
    return 0;
}

static int osd_inf_fb_get_var(struct fb_var_screeninfo *var, int con,
             struct fb_info *info)
{

    //memcpy(var, &osd_fb_var_default, sizeof(struct fb_var_screeninfo));
    //fixme: the original seems to have some problems
     
    if(con==-1)
        memcpy(var, &osd_fb_var_default, sizeof(struct fb_var_screeninfo));
    else
        *var=fb_display[con].var;
    
    PDEBUG("con=%d, x=%d, y=%d, pan x = %d, pan y = %d, left = %d, upper = %d\n", con, var->xres, var->yres, var->xoffset, var->yoffset, var->left_margin, var->upper_margin);
    return 0;
}


static void osd_inf_fb_set_disp(int con)
{
    struct fb_fix_screeninfo fix;
    struct display *display;
    struct display_switch *sw;
   
    if (con >= 0)
        display = &fb_display[con];
    else
        display = &osd_fb_disp;    /* used during initialization */

    osd_inf_fb_get_fix(&fix, con, 0);

    memset(display, 0, sizeof(struct display));

    display->screen_base = fb_surface_info.plane[0].pPlane;  // logical address
    display->visual = fix.visual;
    display->type = fix.type;
    display->type_aux = fix.type_aux;
    display->ypanstep = fix.ypanstep;
    display->ywrapstep = fix.ywrapstep;
    display->line_length = fix.line_length;
    display->next_line = fix.line_length;
    display->can_soft_blank = 0;
    display->inverse = 0;
    osd_inf_fb_get_var(&display->var, -1, &osd_fb_info);

    switch (osd_gfx_bpp) {  // pixel size in bits
#ifdef FBCON_HAS_CFB8
    case 8:
        sw = &fbcon_cfb8;
        break;
#endif
#ifdef FBCON_HAS_CFB16
    case 16:
        sw = &fbcon_cfb16;
        display->dispsw_data = fbcon_cmap.cfb16;
        break;
#endif
#ifdef FBCON_HAS_CFB32
    case 32:
        sw = &fbcon_cfb32;
        display->dispsw_data = fbcon_cmap.cfb32;
        break;
#endif
    default:
        sw = &fbcon_dummy;
        return;
    }
    memcpy(&osd_fb_sw, sw, sizeof(*sw));
    display->dispsw = &osd_fb_sw;
/*
    if ("can not scroll") 
    {
        display->scrollmode = SCROLL_YREDRAW;
        osd_fb_sw.bmove = fbcon_redraw_bmove;
    }
*/
}

// static 
int osd_inf_fb_set_var(struct fb_var_screeninfo *var, int con,
              struct fb_info *info)
{
    // fixme, we should be able to change it
    if (var->xres_virtual   > osd_buf_x   ||
        var->yres_virtual   > osd_buf_y    ||
        var->bits_per_pixel != osd_gfx_bpp ||
        var->nonstd) 
    {
        PDEBUG("Cound not change the video mode to %dx%dx%d [%d]!\n", 
            var->xres_virtual, var->yres_virtual, var->bits_per_pixel, var->nonstd);
        return -EINVAL;
    }

    if (var->xres > var->xres_virtual   ||
        var->yres > var->yres_virtual   ||
        var->yoffset+var->yres > var->yres_virtual ||
        var->xoffset+var->xres > var->xres_virtual)
    {
        PDEBUG("Invalid var parameter !\n");
        return -EINVAL;
    }

    if ((var->activate & FB_ACTIVATE_MASK) == FB_ACTIVATE_TEST)
        return 0;

    PDEBUG("con=%d, x=%d, y=%d, pan x = %d, pan y = %d, left = %d, upper = %d\n", con, var->xres, var->yres, var->xoffset, var->yoffset, var->left_margin, var->upper_margin);
    //fixme:  the original vesafb code doesn't have this
    memcpy(&osd_fb_var_default, var, sizeof(osd_fb_var_default));

    if(con >= 0)
    {
        fb_display[con].var = *var;
    }
    if (info->changevar)
		(*info->changevar)(con);


    return osd_inf_fb_update_var(con, info);
}



// static 
int osd_inf_fb_getcolreg(unsigned regno, unsigned *red, unsigned *green,
              unsigned *blue, unsigned *transp,
              struct fb_info *fb_info)
{
    GFX_PALETTE_T   pal;
    GFX_SURFACE_ACCESS_PALETTE_PARM_T parm;

    if(!IS_OSD_SURFACE_CLUT(fb_surface_physical.uPlaneConfig) || regno > 255)
    {
        PDEBUG("Failed to get surface palette entry %d!\n", regno);
        return -1;
    }
    
    parm.hSurface = hFbSurface;
    parm.pPalette = &pal;
    parm.uCount = 1;
    parm.uStart = regno;

    if(gfx_inf_h_get_palette(&parm) < 0)
        return -1;

    *red    = (UINT)pal.r<<8;
    *green  = (UINT)pal.g<<8;
    *blue   = (UINT)pal.b<<8;
    *transp = (UINT)pal.a<<8;
    return 0;
}


// static 
int osd_inf_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
              unsigned blue, unsigned transp,
              struct fb_info *fb_info)
{

    if(regno > 255)
    {
        PDEBUG("Failed to get surface palette entry %d!\n", regno);
        return -1;
    }

    switch (osd_gfx_bpp) {  // pixel size in bits
#ifdef FBCON_HAS_CFB8
    case 8:
        {
            GFX_PALETTE_T   pal;
            GFX_SURFACE_ACCESS_PALETTE_PARM_T parm;
            parm.hSurface = hFbSurface;
            parm.pPalette = &pal;
            parm.uCount = 1;
            parm.uStart = regno;
            
            pal.r = red>>8;
            pal.g = green>>8;
            pal.b = blue>>8;
            pal.a = 0xff;	// alpha
            if(gfx_inf_h_set_palette(&parm) < 0)
                return -1;
        }
        break;
#endif
#ifdef FBCON_HAS_CFB16
    case 16:
        /* 0:5:6:5 */
        fbcon_cmap.cfb16[regno] =
            ((red   & 0xf800)      ) |
            ((green & 0xfc00) >>  5) |
            ((blue  & 0xf800) >> 11);
        break;
#endif
#ifdef FBCON_HAS_CFB32
    case 32:
        red   >>= 8;
        green >>= 8;
        blue  >>= 8;
        fbcon_cmap.cfb32[regno] = 0xff000000       |  // alpha
            (red   << osd_fb_var_default.red.offset)   |
            (green << osd_fb_var_default.green.offset) |
            (blue  << osd_fb_var_default.blue.offset);
        break;
#endif
    default:
        return -1;
    }
    return 0;
}

/*
static void do_install_cmap(int con, struct fb_info *info)
{
    if (con != currcon)
        return;
    if (fb_display[con].cmap.len)
        fb_set_cmap(&fb_display[con].cmap, 1, vesa_setcolreg, info);
    else
        fb_set_cmap(fb_default_cmap(video_cmap_len), 1, vesa_setcolreg,
                info);
}
*/


static int osd_inf_fb_get_cmap(struct fb_cmap *cmap, int kspc, int con,
               struct fb_info *info)
{
    if (con == currcon) /* current console? */
        return fb_get_cmap(cmap, kspc, osd_inf_fb_getcolreg, info);
    else if (fb_display[con].cmap.len) /* non default colormap? */
        fb_copy_cmap(&fb_display[con].cmap, cmap, kspc ? 0 : 2);
    else
        fb_copy_cmap(fb_default_cmap(video_cmap_len),
             cmap, kspc ? 0 : 2);
    return 0;
}

static int osd_inf_fb_set_cmap(struct fb_cmap *cmap, int kspc, int con,
               struct fb_info *info)
{
    int err;

    if (!fb_display[con].cmap.len) {    /* no colormap allocated? */
        err = fb_alloc_cmap(&fb_display[con].cmap,video_cmap_len,0);
        if (err)
            return err;
    }
    if (con == currcon)            /* current console? */
        return fb_set_cmap(cmap, kspc, osd_inf_fb_setcolreg, info);
    else
        fb_copy_cmap(cmap, &fb_display[con].cmap, kspc ? 0 : 1);
    return 0;
}

static struct fb_ops osd_fb_ops = 
{
    owner:          THIS_MODULE,
    fb_get_fix:     osd_inf_fb_get_fix,
    fb_get_var:     osd_inf_fb_get_var,
    fb_set_var:     osd_inf_fb_set_var,
    fb_get_cmap:    osd_inf_fb_get_cmap,
    fb_set_cmap:    osd_inf_fb_set_cmap,
    fb_pan_display: osd_inf_fb_pan_display,
};


static void do_install_cmap(int con, struct fb_info *info)
{
    if (con != currcon)
        return;
    if (fb_display[con].cmap.len)
        fb_set_cmap(&fb_display[con].cmap, 1, osd_inf_fb_setcolreg, info);
    else
        fb_set_cmap(fb_default_cmap(video_cmap_len), 1, osd_inf_fb_setcolreg,info);
}

static int osd_inf_fb_switch(int con, struct fb_info *info)
{
    /* Do we have to save the colormap? */
    if (fb_display[currcon].cmap.len)
        fb_get_cmap(&fb_display[currcon].cmap, 1, osd_inf_fb_getcolreg, info);
    
    currcon = con;
    /* Install new colormap */
    if (con == currcon)
    {
        if (fb_display[con].cmap.len)
            fb_set_cmap(&fb_display[con].cmap, 1, osd_inf_fb_setcolreg, info);
        else
            fb_set_cmap(fb_default_cmap(video_cmap_len), 1, osd_inf_fb_setcolreg,info);
    }
    osd_inf_fb_update_var(con,info);
    return 1;
}

/* 0 unblank, 1 blank, 2 no vsync, 3 no hsync, 4 off */

static void osd_inf_fb_blank(int blank, struct fb_info *info)
{
    /* Not supported */
}


static int osd_inf_fb_init(void)
{
    int rtn;
    UINT uDispWidth, uDispHeight;
    GFX_CREATE_SURFACE_PARM_T surfc;
    GFX_SURFACE_DISPLAY_T disp;

    // print the driver verision info for futher reference
    PVERSION(FB_DRIVER_NAME);

    
    //if(scrman_osi_get_fmt(&uDispWidth, &uDispHeight) < 0)
    {
        uDispWidth = OSD_DEFAULT_XRES;
        uDispHeight = OSD_DEFAULT_YRES; 
    }
    
    PDEBUGE("Video screen size (%u, %u)\n", uDispWidth, uDispHeight);
    
    if(osd_scr_x < 0)   // default
        osd_scr_x = uDispWidth;
    
    if(osd_scr_y < 0)   // default 
        osd_scr_y = uDispHeight;
    
    if(osd_buf_x < 0)   // default
        osd_buf_x = OSD_DEFAULT_BUF_XRES;
    if(osd_buf_x < osd_scr_x)   // at least
        osd_buf_x = osd_scr_x;
    
    if(osd_buf_y < 0)   // default
        osd_buf_y = OSD_DEFAULT_BUF_YRES;
    if(osd_buf_y < osd_scr_y)   // at least
        osd_buf_y = osd_scr_y;
    
    if(osd_tv_left < 0)
        osd_tv_left = 0;
    
    if(osd_tv_upper < 0)
        osd_tv_upper = 0;
    
    if(osd_gfx_bpp < 8)
        osd_gfx_bpp = OSD_DEFAULT_BPP;
    
//    if(osd_gfx_alpha<0 ||osd_gfx_alpha>255) osd_gfx_alpha = 255;
    
    surfc.uWidth = osd_buf_x;
    surfc.uHeight = osd_buf_y;
    surfc.uFillColor = 0;
    surfc.graphDev = osd_fb_device;
    switch(osd_gfx_bpp)
    {
#ifdef FBCON_HAS_CFB8
    case 8:
        surfc.uPlaneConfig = GFX_SURFACE_CLUT8BPP_ARGB;
        rtn = gfx_inf_h_create_surface(&surfc);
        osd_fb_var_default.bits_per_pixel = 8;
        osd_fb_var_default.red.offset = 0;
        osd_fb_var_default.red.length = 8;
        osd_fb_var_default.red.msb_right = 0;
        osd_fb_var_default.green.offset = 0;
        osd_fb_var_default.green.length = 8;
        osd_fb_var_default.green.msb_right = 0;
        osd_fb_var_default.blue.offset = 0;
        osd_fb_var_default.blue.length = 8;
        osd_fb_var_default.blue.msb_right = 0;
        osd_fb_var_default.transp.offset = 0;
        osd_fb_var_default.transp.length = 8;
        osd_fb_var_default.transp.msb_right = 0;
        video_cmap_len = 256;
        break;
#endif
        
#ifdef FBCON_HAS_CFB16
#ifdef GFX_SURFACE_RGB_565
    case 16:   // RGB565
        surfc.uPlaneConfig = GFX_SURFACE_RGB_565;
        rtn = gfx_inf_h_create_surface(&surfc);
        osd_fb_var_default.bits_per_pixel = 16;
        osd_fb_var_default.red.offset = 11;
        osd_fb_var_default.red.length = 5;
        osd_fb_var_default.red.msb_right = 0;
        osd_fb_var_default.green.offset = 5;
        osd_fb_var_default.green.length = 6;
        osd_fb_var_default.green.msb_right = 0;
        osd_fb_var_default.blue.offset = 0;
        osd_fb_var_default.blue.length = 5;
        osd_fb_var_default.blue.msb_right = 0;
        osd_fb_var_default.transp.offset = 0;
        osd_fb_var_default.transp.length = 0;
        osd_fb_var_default.transp.msb_right = 0;
        video_cmap_len = 16;
        break;
#else
        PINFOE("16 bpp color depth is not supported by hardware\n");
        rtn = -EINVAL;
        break;
#endif
#endif
        
#ifdef FBCON_HAS_CFB32
#ifdef GFX_SURFACE_ARGB_8888    
    case 32:   // ARGB8888
        surfc.uPlaneConfig = GFX_SURFACE_ARGB_8888;
        rtn = gfx_inf_h_create_surface(&surfc);
        osd_fb_var_default.bits_per_pixel = 32;
        osd_fb_var_default.red.offset = 16;
        osd_fb_var_default.red.length = 8;
        osd_fb_var_default.red.msb_right = 0;
        osd_fb_var_default.green.offset = 8;
        osd_fb_var_default.green.length = 8;
        osd_fb_var_default.green.msb_right = 0;
        osd_fb_var_default.blue.offset = 0;
        osd_fb_var_default.blue.length = 8;
        osd_fb_var_default.blue.msb_right = 0;
        osd_fb_var_default.transp.offset = 24;
        osd_fb_var_default.transp.length = 8;
        osd_fb_var_default.transp.msb_right = 0;
        video_cmap_len = 16;
        break;
#else
        PINFOE("32 bpp color depth is not supported by hardware\n");
        rtn = -EINVAL;
        break;
#endif
#endif
        
    default:
        PFATAL("Unsupported graphics bpp config!\n");
        rtn = -EINVAL;
        break;
    }
    
    if( rtn < 0)
    {
        PFATAL("Error in creating graphics surface\n");
        rtn = -ENOMEM;
        return rtn;
    }
    
    hFbSurface = surfc.hSurface;

    fb_surface_info.hSurface = hFbSurface;
    fb_surface_physical.hSurface = hFbSurface;

    gfx_inf_h_lock_surface(&fb_surface_physical);  // to get it's physical address
    gfx_inf_h_unlock_surface(hFbSurface);
    rtn = _gfx_inf_h_get_surface_local(&fb_surface_info);
    if(rtn < 0)
    {
        gfx_inf_h_destroy_surface(hFbSurface);
        PFATAL("Error in creating graphics surface\n");
        rtn = -ENOMEM;
        return rtn;
    }
    
    osd_fb_var_default.xres = osd_scr_x;
    osd_fb_var_default.yres = osd_scr_y;
    osd_fb_var_default.xres_virtual = osd_buf_x;
    osd_fb_var_default.yres_virtual = osd_buf_y;
    osd_fb_var_default.xoffset = 0;
    osd_fb_var_default.yoffset = 0;
    osd_fb_var_default.grayscale = 0;
    osd_fb_var_default.nonstd = 0;
    osd_fb_var_default.activate = FB_ACTIVATE_NOW;
    osd_fb_var_default.accel_flags = 0;

    {
        GFX_SCREEN_INFO_T  scr;
        scr.uLeft =osd_tv_left;
        scr.uUpper = osd_tv_upper;
        scr.uWidth = osd_scr_x;
        scr.uHeight = osd_scr_y;
        gfx_inf_h_set_screen_info(&scr);
    }
    
    disp.hSurface = hFbSurface;
    gfx_inf_h_get_surface_display_parm(&disp);
    disp.uStartX = 0;
    disp.uStartY = 0;
    disp.uWinWidth = osd_scr_x;
    disp.uWinHeight = osd_scr_y;
    disp.uWinX = 0;
    disp.uWinY = 0;
    disp.bFlickerCorrect = osd_gfx_af;
    
    if(gfx_inf_h_set_surface_display_parm(&disp) < 0)
    {
        gfx_inf_h_destroy_surface(hFbSurface);
        PFATAL("Failed to config graphics surface parameters!\n");
        return -EPERM;
    }

    
    {
        GFX_SURFACE_VDEV_PARM_T vdev;
        vdev.hSurface = hFbSurface;
        vdev.graphDev = osd_fb_device;
        
        rtn = gfx_inf_h_attach_surface(&vdev);
    }
    if(rtn < 0)
    {
        gfx_inf_h_destroy_surface(hFbSurface);
        PFATAL("Failed to attach graphics surface to gfx device!\n");
        return -EPERM;
    }

    if(rtn < 0)
    {
        GFX_SURFACE_VDEV_PARM_T vdev;
        vdev.hSurface = hFbSurface;
        vdev.graphDev = osd_fb_device;
        
        rtn = gfx_inf_h_detach_surface(&vdev);
        gfx_inf_h_destroy_surface(hFbSurface);
        PFATAL("Failed to set graphics surface parm!\n");
        return -EIO;
    }

    {
        GFX_DISPLAY_CONTROL_PARM_T parm;
        // enable antiflicker on OSD display
        parm.parm = GFX_DISP_CNTL_EDAF;
        parm.uAttr = 1;
        gfx_inf_h_set_display_control(&parm);
        // set antiflicker detect threshold to afdt
        parm.parm = GFX_DISP_CNTL_AFDT;
        parm.uAttr = osd_gfx_afdt;
        gfx_inf_h_set_display_control(&parm);
    }
    
    if(osd_gfx_pat) // show the pattern for 5 seconds
    {
        printk(KERN_INFO "The video buffer is now filled with vertical bar pattern for 5 seconds!\n");
        osd_inf_debug_fill_fb(&fb_surface_info);
        {
            ULONG new_jif = jiffies + 500;
            while(new_jif > jiffies) ;
        }
        printk(KERN_INFO "Now you will not see the vertical bar if frame buffer is working!\n");
    }
    
    
    printk(KERN_INFO "%s: framebuffer  mapped to 0x%8.8x\n",
        osd_fb_dev_name, (UINT)fb_surface_info.plane[0].pPlane);
    
    /* some dummy values for timing to make fbset happy */
    osd_fb_var_default.pixclock     = 74074;
    osd_fb_var_default.left_margin  = osd_tv_left;
    osd_fb_var_default.right_margin = 0;
    osd_fb_var_default.upper_margin = osd_tv_upper;
    osd_fb_var_default.lower_margin = 0;
    osd_fb_var_default.hsync_len    = 108;
    osd_fb_var_default.vsync_len    = 25;
    
    strcpy(osd_fb_info.modename, osd_fb_dev_name);
    osd_fb_info.changevar = NULL;
    osd_fb_info.node = -1;
    osd_fb_info.fbops = &osd_fb_ops;
    osd_fb_info.disp=&osd_fb_disp;
    osd_fb_info.switch_con=&osd_inf_fb_switch;
    osd_fb_info.updatevar=&osd_inf_fb_update_var;
    osd_fb_info.blank=&osd_inf_fb_blank;
    osd_fb_info.flags=FBINFO_FLAG_DEFAULT;
    osd_inf_fb_set_disp(-1);
    
    
    if (register_framebuffer(&osd_fb_info)<0)
    {
        GFX_SURFACE_VDEV_PARM_T vdev;
        vdev.hSurface = hFbSurface;
        vdev.graphDev = osd_fb_device;
        
        rtn = gfx_inf_h_detach_surface(&vdev);
        gfx_inf_h_destroy_surface(hFbSurface);
        PFATAL("Failed to register frame buffer!\n");
        return -EINVAL;
    }
    printk(KERN_INFO "fb%d: %s frame buffer device\n",
        GET_FB_IDX(osd_fb_info.node), osd_fb_info.modename);

    if(osd_gfx_shared)
    {
        PINFOE("Shared surface handle is set to %d.\n", hFbSurface);
        gfx_inf_h_set_shared_surface(hFbSurface);
    }
    return 0;
}

static void osd_inf_fb_deinit(void)
{
    GFX_SURFACE_VDEV_PARM_T vdev;
    if (unregister_framebuffer(&osd_fb_info)<0)
    {
        PFATAL("Failed to unregister frame buffer!\n");
        return ;
    }

    vdev.hSurface = hFbSurface;
    vdev.graphDev = osd_fb_device;
    
    gfx_inf_h_detach_surface(&vdev);
    gfx_inf_h_destroy_surface(hFbSurface);

    return ;
}


/*
 *  Modularization
 */


module_init(osd_inf_fb_init);
module_exit(osd_inf_fb_deinit);
