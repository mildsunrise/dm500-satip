//vulcan/drv/gfx/osd_inf_test.c
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
//  TEST of OSD controls
//Revision Log:   
//  Oct/10/2001                          Created by YYD
//  Oct/17/2001                 Merged with gfx  by YYD
//  Jun/07/2002            Ported to Vulcan arch by YYD

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
#include <linux/init.h>
#include <linux/selection.h>    /* for color_table */
#include <linux/sched.h>   // for jiffie


#include "gfx_surface.h"
#include "osd_osi.h"
#include "gfx_osi.h"

//#include "scrman/scrman_osi.h"


#define __DRV_DEBUG
#include "os/drv_debug.h"

#define OSD_DEFAULT_XRES    640
#define OSD_DEFAULT_YRES    440

#define OSD_DEFAULT_BUF_XRES    640
#define OSD_DEFAULT_BUF_YRES    440

#define OSD_DEFAULT_TV_LEFT_MARGIN  30
#define OSD_DEFAULT_TV_UPPER_MARGIN 30

#define OSD_DEFAULT_BPP     32

// global parameters

MODULE_PARM(osd_scr_x, "i");
MODULE_PARM_DESC(osd_scr_x, "Specify the viewable graphics screen display horizontal size");
MODULE_PARM(osd_scr_y, "i");
MODULE_PARM_DESC(osd_scr_y,  "Specify the viewable graphics screen display vertical size");
MODULE_PARM(osd_gfx_bpp, "i");
MODULE_PARM_DESC(osd_gfx_bpp, "The bit per pixel of graphics surface. Valid 8(index color), 16(YUV422) and 32(AYUV422)");
MODULE_PARM(osd_tv_left, "i");
MODULE_PARM_DESC(osd_tv_left, "Adjust for TV horizontal timing offset.");
MODULE_PARM(osd_tv_upper, "i");
MODULE_PARM_DESC(osd_tv_upper, "Adjust for TV vertical timing offset.");


MODULE_AUTHOR("Yudong Yang");
MODULE_DESCRIPTION("IBM STBx25xx Graphics Tester Driver");

static INT osd_scr_x = -1;
static INT osd_scr_y = -1;
static INT osd_buf_x = -1;
static INT osd_buf_y = -1;
static INT osd_gfx_bpp = OSD_DEFAULT_BPP;
static INT osd_tv_left = OSD_DEFAULT_TV_LEFT_MARGIN;
static INT osd_tv_upper = OSD_DEFAULT_TV_UPPER_MARGIN;


static char *osd_fb_dev_name = "STB GFXTEST"; // MAX 15 CHAR

static GFX_SURFACE_T osd_fb_surface;
static GFX_VISUAL_DEVICE_ID_T osd_fb_device = GFX_VDEV_OSDGFX;


static void osd_inf_delay(unsigned int ticks)
{
    ULONG new_jif = jiffies + ticks;
    while(new_jif > jiffies) ;
}

static UINT osd_inf_rand(void)
{
    static UINT seed = 698;
    return seed = (seed*1237) % 16001;
}


static void osd_inf_debug_fill_fb(GFX_SURFACE_T *osd_test_surface)
{
    UINT i,j, x, y, mx, my;

    y = osd_test_surface->plane[0].uAllocHeight;
    x = osd_test_surface->plane[0].uAllocWidth;

    gfx_osi_fillBLT(osd_test_surface, 0, 0, x, y, 0);
    my = 1;
    for(j=0; j<y; j+=my)
    {
        mx = 1;
        for(i=0; i<x; i+=mx)
        {
            UINT32 c = ((mx*my)&0x01) ? 0xffffffff : 0x00000000;
            gfx_osi_fillBLT(osd_test_surface, i, j, mx, my, c);
            mx++;
        }
        my++;
    }
}

static void osd_inf_clear_fb(GFX_SURFACE_T *osd_test_surface, UINT32 color)
{
    gfx_osi_fillBLT(osd_test_surface, 0, 0,
        osd_test_surface->plane[0].uAllocWidth, osd_test_surface->plane[0].uAllocHeight,  color);
}

void osd_inf_fill_color_bar(GFX_SURFACE_T *pSurface)
{
    UINT i, x, y, pix;

    //if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig)) return ; // palette plane is not supported

    pix = pSurface->plane[0].uPixelSize;

    if(IS_GFX_SURFACE_YUV(pSurface->uPlaneConfig) && !IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig))
        pix = 32;   // fill color is argb for yuv

    y = pSurface->plane[0].uAllocHeight;
    x = pSurface->plane[0].uAllocWidth;

    for(i=0; i<8; i++)
    {
        UINT32  fc, r, g, b, a;
        r = (i&1) ? 255:64;
        g = (i&2) ? 255:64;
        b = (i&4) ? 255:64;
        a = (i&1) ? 255:128;      // semi trans
        if(pix == 32)
        {
            fc = (r<<16) | (g<<8) | (b) | (a<<24);
        }
        else if (pix <= 8)
        {
            fc = i<<5;
        }
        else
        {
            r >>= 8-pSurface->plane[0].r.uNumbits;
            g >>= 8-pSurface->plane[0].g.uNumbits;
            b >>= 8-pSurface->plane[0].b.uNumbits;
            a >>= 8-pSurface->plane[0].a.uNumbits;

            fc =  (r<<pSurface->plane[0].r.uOffset) 
                | (g<<pSurface->plane[0].g.uOffset) 
                | (b<<pSurface->plane[0].b.uOffset) 
                | (a<<pSurface->plane[0].a.uOffset) ;
        }

        gfx_osi_fillBLT(pSurface, i*x/8, 0,
            x/8, y, fc);
    }
    gfx_osi_run_engine(1);
}


void osd_inf_fill_8bit_pattern(GFX_SURFACE_T *pSurface)
{
    UINT i, j, x, y, w, h;
    GFX_PALETTE_T pal[256];

    w = pSurface->plane[0].uAllocHeight;
    h = pSurface->plane[0].uAllocWidth;

#if 1
    for(i=0; i<128; i++)
    {
        pal[i].a = ((i&0x60)<<1) | ((i&0x60)>>1) | ((i&0x60)>>3) | ((i&0x60)>>5);
        pal[i].r = (i&0x30)<<2;
        pal[i].g = (i&0x0c)<<4;
        pal[i].b = (i&0x03)<<6;
    }
#else
    gfx_osi_get_surface_palette(pSurface, pal, 0, 256);
#endif
    for(j=128; j<256; j+=16)
    {
        switch(((j-128)/16)/3)
        {
        case 0:
            for(i=0; i<16; i++)
            {
                pal[i+j].a = i*17;
            }
            break;
        case 1:
            for(i=0; i<16; i++)
            {
                pal[i+j].a = 255-i*17;
            }
            break;
        case 2:
            for(i=0; i<16; i++)
            {
                pal[i+j].a = 255;
            }
            break;
        }
        switch(((j-128)/16)%3)
        {
        case 0:
            for(i=0; i<16; i++)
            {
                pal[i+j].r = i*17;
                pal[i+j].g = i*17;
                pal[i+j].b = i*17;
            }
            break;
        case 1:
            for(i=0; i<16; i++)
            {
                pal[i+j].r = 255-i*17;
                pal[i+j].g = 255-i*17;
                pal[i+j].b = 255-i*17;
            }
            break;
        case 2:
            for(i=0; i<16; i++)
            {
                pal[i+j].r = 255;
                pal[i+j].g = 255;
                pal[i+j].b = 255;
            }
            break;
        }
    }

    if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig))
    {
        gfx_osi_set_surface_palette(pSurface, pal, 0, 256);
        
        for(i=0, y=0; y<h; y+=h/16)
        {
            for(x=0; x<w; x+= w/16, i++)
            {
                gfx_osi_fillBLT(pSurface, x, y, w/16, h/16, i);
            }
        }
    }
    else
    {
        for(i=0, y=0; y<h; y+=h/16)
        {
            for(x=0; x<w; x+= w/16, i++)
            {
                gfx_osi_fillBLT(pSurface, x, y, w/16, h/16, *(UINT32 *)&pal[i]);
            }
        }
    }
}

    
static int osd_inf_test_init(void)
{
    int rtn;
    int i;
    ULONG oldjiff;
    UINT uDispWidth, uDispHeight;
    
    //if(scrman_osi_get_fmt(&uDispWidth, &uDispHeight) < 0)
    {
        uDispWidth = OSD_DEFAULT_XRES;
        uDispHeight = OSD_DEFAULT_YRES; 
    }
    
    osd_osi_set_display_parm(OSD_DISP_CNTL_BACKCOLOR, 0x80F8);

    PINFOE("Video screen size (%u, %u)\n", uDispWidth, uDispHeight);
    
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
    
    gfx_osi_init_surface_t(&osd_fb_surface);
    switch(osd_gfx_bpp)
    {
    case 8:
        rtn = gfx_osi_create_surface(&osd_fb_surface, osd_fb_device, 
            GFX_SURFACE_CLUT8BPP_ARGB, osd_buf_x, osd_buf_y);
        break;
        
    case 16:   // RGB565
        rtn = gfx_osi_create_surface(&osd_fb_surface, osd_fb_device, 
            GFX_SURFACE_YCBCR_422_888, osd_buf_x, osd_buf_y);
        break;
        
    case 32:   // ARGB8888
        rtn = gfx_osi_create_surface(&osd_fb_surface, osd_fb_device, 
            GFX_SURFACE_AYCBCR_422_8888, osd_buf_x, osd_buf_y);
        break;
        
    default:
        PFATAL("Unsupported graphics bpp config!\n");
        rtn = -EINVAL;
        break;
    }
    
    PINFOE("Surface created \n");
    
    if(rtn < 0)
    {
        if(-1 == rtn) 
        {
            PFATAL("Error in creating graphics surface\n");
            rtn = -ENOMEM;
        }
        return rtn;
    }
    
    rtn = osd_osi_set_comp_gfx_surface_parm(&osd_fb_surface, OSD_GRAPH_SURFACE_SCREEN_OFFSET, osd_tv_left, osd_tv_upper);
    
    if(rtn < 0)
    {
        gfx_osi_destroy_surface(&osd_fb_surface);
        PFATAL("Failed to config graphics surface parameters!\n");
        return -EPERM;
    }
    
    PINFOE("Clear Surface \n");
    osd_inf_clear_fb(&osd_fb_surface, 0);
    PINFOE("The video buffer is now filled with vertical bar pattern for 5 seconds!\n");
    osd_inf_debug_fill_fb(&osd_fb_surface);
    
    rtn = osd_osi_attach_comp_gfx_surface(osd_fb_device, &osd_fb_surface);
    if(rtn < 0)
    {
        gfx_osi_destroy_surface(&osd_fb_surface);
        PFATAL("Failed to attach graphics surface to gfx device!\n");
        return -EPERM;
    }
    
    
    PINFOE("%s: framebuffer at 0x%8.8x, mapped to 0x%8.8x\n",
        osd_fb_dev_name, osd_fb_surface.plane[0].uBMLA, (UINT)osd_fb_surface.plane[0].pBuffer);
    
    osd_inf_delay(500);
    
    {
        
#if 1   // simple fill
        PINFOE("\nTest fill solid rects\n");
        PINFOE("  Single command mode, 250 filles\n");
        oldjiff = jiffies;
        for(i=0; i<250; i++)
        {
            UINT x = osd_inf_rand()%osd_scr_x;
            UINT y = osd_inf_rand()%osd_scr_y;
            UINT h = osd_inf_rand()%osd_scr_x;
            UINT w = osd_inf_rand()%osd_scr_y;
            UINT c = osd_inf_rand()%255 | (osd_inf_rand()%255<<8) | (osd_inf_rand()%255<<16) | (osd_inf_rand()%255<<24);
            
            // PINFOE("Fill (x,y,w,h, c) %d %d %d %d 0x%8.8x\n", x, y, w, h, c);
            gfx_osi_fillBLT(&osd_fb_surface, x, y, w, h, c);
            gfx_osi_run_engine(1);
        }
        //        PINFOE("Now Start fill\n");
        PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
        osd_inf_delay(500);
#endif
        
#if 1       // pattern fill        
        {
            GFX_SURFACE_T  mask;
            gfx_osi_init_surface_t(&mask);
            rtn = gfx_osi_create_surface(&mask, GFX_VDEV_NULL, 
                GFX_SURFACE_RAW1BPP, osd_buf_x, osd_buf_y);
            if(rtn < 0)
            {
                PFATAL("Failed to create mask surface, test stopped!\n");
                return 0;
            }
            {
                // make pattern 4x4
                UINT bpl, y, i;
                BYTE pat=0xf0;
                
                bpl = mask.plane[0].uBytePerLine; 
                y = mask.plane[0].uAllocHeight;
                
                //memset(mask.plane[0].pBuffer, 0xff, y*bpl);
                
                
                for(i=0; i<y-3; i+=4, pat = ~pat)
                {
                    memset(mask.plane[0].pBuffer+i*bpl, pat, 4*bpl);
                }
            }
            
            PINFOE("\nTest fill pattern rects\n");
            {
                gfx_osi_advancedFillBLT(&osd_fb_surface, 1, 1, 16, 16, 0xffffffff,
                    &mask, 1, 1,  GFX_ROP_DISABLE, 0, 0x00ff0000, 0, 0xf0f0f0f0);
                
                PINFOE("  Single command mode, 250 filles\n");
                
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%osd_scr_x+1;
                    UINT y = osd_inf_rand()%osd_scr_y;
                    UINT h = osd_inf_rand()%osd_scr_x;
                    UINT w = osd_inf_rand()%osd_scr_y;
                    UINT c = osd_inf_rand()%255 | (osd_inf_rand()%255<<8) | (osd_inf_rand()%255<<16) | (osd_inf_rand()%255<<24);
                    
                    //           PINFOE("Fill (x,y,w,h, c) %d %d %d %d 0x%8.8x\n", x, y, w, h, c);
                    gfx_osi_advancedFillBLT(&osd_fb_surface, x, y, w, h, c,
                        &mask, 0, 0,  GFX_ROP_DISABLE, 0, 0x00ff0000, 0, 0xf0f0f0f0);
                    
                    gfx_osi_run_engine(1);
                }
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
            }
            gfx_osi_destroy_surface(&mask);
            osd_inf_delay(500);
        }
#endif
        
        
        
        
#if 1   // more test
        
        
#define TST_HEIGHT   256
#define TST_WIDTH   256
        
#define YUV_DEV GFX_VDEV_OSDIMG
        
#define YUV_FMT GFX_SURFACE_YCBCR_422_888
        
        
        {
            GFX_SURFACE_T  yuv, rgb, rgb_bl, rgb_8, yuv_8;
            ALPHA_SELECT alps;
            
            
            gfx_osi_init_surface_t(&yuv);
            gfx_osi_init_surface_t(&rgb);
            gfx_osi_init_surface_t(&rgb_bl);
            gfx_osi_init_surface_t(&yuv_8);
            gfx_osi_init_surface_t(&rgb_8);
            if(osd_gfx_bpp < 16)  // clut
            {
                rtn = gfx_osi_create_surface(&yuv, YUV_DEV,     // put YUV on still 
                    GFX_SURFACE_CLUT8BPP_AYCBCR, TST_WIDTH, TST_HEIGHT);
                rtn = gfx_osi_create_surface(&rgb, GFX_VDEV_NULL,
                    GFX_SURFACE_CLUT8BPP_ARGB, TST_WIDTH, TST_HEIGHT);
                osd_inf_fill_8bit_pattern(&yuv);
                osd_inf_fill_8bit_pattern(&rgb);
            }
            else
            {
                rtn = gfx_osi_create_surface(&yuv, YUV_DEV,     // put YUV on still 
                    YUV_FMT, TST_WIDTH, TST_HEIGHT);
                rtn = gfx_osi_create_surface(&rgb, GFX_VDEV_NULL,
                    GFX_SURFACE_ARGB_8888, TST_WIDTH, TST_HEIGHT);
                osd_inf_fill_color_bar(&yuv);
                osd_inf_fill_color_bar(&rgb);

                rtn = gfx_osi_create_surface(&rgb_bl, GFX_VDEV_NULL,
                    GFX_SURFACE_ARGB_8888, TST_WIDTH, TST_HEIGHT);
                osd_inf_fill_8bit_pattern(&rgb_bl);

                rtn = gfx_osi_create_surface(&yuv_8, YUV_DEV,    
                    GFX_SURFACE_CLUT8BPP_AYCBCR, TST_WIDTH, TST_HEIGHT);
                rtn = gfx_osi_create_surface(&rgb_8, YUV_DEV,    
                    GFX_SURFACE_CLUT8BPP_ARGB, TST_WIDTH, TST_HEIGHT);
                osd_inf_fill_8bit_pattern(&yuv_8);
                osd_inf_fill_8bit_pattern(&rgb_8);
            }


            if(rtn < 0)
            {
                PFATAL("Failed to create new surface, test stopped!\n");
                //fixme: may lost allocated surfaces here
                return 0;
            }
            
            
            // fill color bar
            
            
            PINFOE("  Showing the YUV test color bar\n");
            osd_osi_set_comp_gfx_surface_parm(&yuv, OSD_GRAPH_SURFACE_SCREEN_OFFSET, 0 /*100+osd_tv_left*/, 0/*100+osd_tv_left*/);
            osd_osi_detach_comp_gfx_surface(osd_fb_device, &osd_fb_surface);
            osd_osi_attach_comp_gfx_surface(YUV_DEV, &yuv);
            osd_inf_delay(200);
            osd_osi_attach_comp_gfx_surface(osd_fb_device, &osd_fb_surface);
            osd_osi_detach_comp_gfx_surface(YUV_DEV, &yuv);
            
            // show it
            PINFOE("  Showing the RGB test color bar\n");
            gfx_osi_fillBLT(&osd_fb_surface, 0,0, osd_scr_x, osd_scr_y, 0xff000000);
            gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
            osd_inf_delay(200);
            osd_inf_debug_fill_fb(&osd_fb_surface);
                
            if(osd_gfx_bpp >= 16)  // directcolor
            {
                // show it
                PINFOE("  Showing the RGB blend test pattern\n");
                gfx_osi_fillBLT(&osd_fb_surface, 0,0, osd_scr_x, osd_scr_y, 0xff000000);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb_bl, 0,0, NULL, 0);
                osd_inf_delay(200);
                osd_inf_debug_fill_fb(&osd_fb_surface);

                // show it
                PINFOE("  Showing the YUV 8 bit pattern\n");
                osd_osi_attach_comp_gfx_surface(osd_fb_device, &yuv_8);
                osd_inf_delay(200);
                osd_osi_detach_comp_gfx_surface(osd_fb_device, &yuv_8);

                PINFOE("  Showing the RGB 8 bit pattern\n");
                osd_osi_attach_comp_gfx_surface(osd_fb_device, &rgb_8);
                osd_inf_delay(200);
                osd_osi_detach_comp_gfx_surface(osd_fb_device, &rgb_8);
            }

#if 0   // bitblt
            PINFOE("\nTest bitblt / color convert\n");
            alps.globalAlphaValue = 128;
            alps.storedAlphaSelect = GFX_DEST_ALPHA_FROM_GIVEN;
            
            PINFOE("  bitBLT YUV->YUV, 250 ops\n");
            oldjiff = jiffies;
            for(i=0; i<250; i++)
            {
                UINT x = osd_inf_rand()%osd_scr_x;
                UINT y = osd_inf_rand()%osd_scr_y;
                UINT h = osd_inf_rand()%TST_HEIGHT;
                UINT w = osd_inf_rand()%TST_WIDTH;
                UINT sx = osd_inf_rand()%TST_HEIGHT;
                UINT sy = osd_inf_rand()%TST_WIDTH;
                if(gfx_osi_bitBLT(&osd_fb_surface, x, y, w, h,
                    &yuv,  sx,  sy, &alps, 0))
                {
                    PDEBUG("bitBLT failed\n");
                    break;
                }
            }
            gfx_osi_run_engine(1);
            PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
            osd_inf_delay(500);
            osd_inf_debug_fill_fb(&osd_fb_surface);
            
            if(osd_gfx_bpp >= 16)  // directcolor
            {
                PINFOE("  bitBLT  RGB -> YUV, 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%osd_scr_x;
                    UINT y = osd_inf_rand()%osd_scr_y;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_bitBLT(&osd_fb_surface, x, y, w, h,
                        &rgb,  sx,  sy, &alps, 0))
                    {
                        PDEBUG("bitBLT failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                osd_inf_delay(500);
                osd_inf_debug_fill_fb(&osd_fb_surface);
                
                PINFOE("  bitBLT YUV -> RGB, 250 ops, result will be shown after finish\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_bitBLT(&rgb_bl, x, y, w, h,
                        &yuv,  sx,  sy, &alps, 0))
                    {
                        PDEBUG("bitBLT failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb_bl, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb_bl);
                osd_inf_delay(500);
                osd_inf_debug_fill_fb(&osd_fb_surface);

                PINFOE("  bitBLT with YUV palette to YUV expansion, 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%osd_scr_x;
                    UINT y = osd_inf_rand()%osd_scr_y;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_bitBLT(&osd_fb_surface, x, y, w, h,
                        &yuv_8,  sx,  sy, &alps, 0))
                    {
                        PDEBUG("bitBLT failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                osd_inf_delay(500);
                osd_inf_debug_fill_fb(&osd_fb_surface);

                PINFOE("  bitBLT with RGB palette to YUV expansion, 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%osd_scr_x;
                    UINT y = osd_inf_rand()%osd_scr_y;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_bitBLT(&osd_fb_surface, x, y, w, h,
                        &rgb_8,  sx,  sy, &alps, 0))
                    {
                        PDEBUG("bitBLT failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                osd_inf_delay(500);
                osd_inf_debug_fill_fb(&osd_fb_surface);

                PINFOE("  bitBLT with RGB palette to RGB conversion, 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_bitBLT(&rgb_bl, x, y, w, h,
                        &rgb_8,  sx,  sy, &alps, 0))
                    {
                        PDEBUG("bitBLT failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb_bl, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb_bl);
                osd_inf_delay(500);
                
                PINFOE("  bitBLT with YUV palette to RGB conversion, 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_bitBLT(&rgb_bl, x, y, w, h,
                        &yuv_8,  sx,  sy, &alps, 0))
                    {
                        PDEBUG("bitBLT failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb_bl, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb_bl);
                osd_inf_delay(500);
            }
#endif      //biblt

#if 0       // blend
            if(osd_gfx_bpp >= 16)  // directcolor
            {
                ALPHA_BLEND_SELECT bs;

                gfx_osi_fillBLT(&osd_fb_surface, 0,0, osd_scr_x, osd_scr_y, 0xff000000);
                PINFOE("\nTest ARGB32 blend capability\n");
                osd_inf_debug_fill_fb(&rgb);
                PINFOE("  blend argb - argb, given alpha, 250 ops\n");
                bs.blendInputSelect = GFX_BLEND_ALPHA_FROM_GIVEN;
                bs.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
                bs.globalAlphaValue = 128;
                gfx_osi_blend(&rgb, 0, 0, TST_WIDTH, TST_HEIGHT,
                    &rgb_bl,  0,  0, &bs);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb);
                osd_inf_delay(300);

                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    bs.globalAlphaValue = i;
                    if(gfx_osi_blend(&rgb, x, y, w, h,
                        &rgb_bl,  sx,  sy, &bs))
                    {
                        PDEBUG("blend failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb);
                osd_inf_delay(500);

                gfx_osi_fillBLT(&osd_fb_surface, 0,0, osd_scr_x, osd_scr_y, 0xff000000);
                PINFOE("  blend argb - argb, source alpha, 250 ops\n");
                bs.blendInputSelect = GFX_BLEND_ALPHA_FROM_SOURCE;
                bs.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
                gfx_osi_blend(&rgb, 0, 0, TST_WIDTH, TST_HEIGHT,
                    &rgb_bl,  0,  0, &bs);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb);
                osd_inf_delay(300);
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_blend(&rgb, x, y, w, h,
                        &rgb_bl,  sx,  sy, &bs))
                    {
                        PDEBUG("blend failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb);
                osd_inf_delay(500);
            
                gfx_osi_fillBLT(&osd_fb_surface, 0,0, osd_scr_x, osd_scr_y, 0xff000000);
                PINFOE("  blend argb - argb, dest alpha, 250 ops\n");
                bs.blendInputSelect = GFX_BLEND_ALPHA_FROM_DESTINATION;
                bs.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
                gfx_osi_blend(&rgb, 0, 0, TST_WIDTH, TST_HEIGHT,
                    &rgb_bl,  0,  0, &bs);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb);
                osd_inf_delay(300);
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_blend(&rgb, x, y, w, h,
                        &rgb_bl,  sx,  sy, &bs))
                    {
                        PDEBUG("blend failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb);
                osd_inf_delay(500);
            
                gfx_osi_fillBLT(&osd_fb_surface, 0,0, osd_scr_x, osd_scr_y, 0xff000000);
                PINFOE("  blend argb - argb, extern alpha, 250 ops\n");
                bs.blendInputSelect = GFX_BLEND_ALPHA_FROM_PATTERN;
                bs.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
                osd_inf_fill_8bit_pattern(&rgb_8);
                gfx_osi_advancedBlend(&rgb, 0, 0, TST_WIDTH, TST_HEIGHT,
                    &rgb_bl,  0,  0, &rgb_8, 0,0, &bs);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb);
                osd_inf_delay(300);
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    UINT sx = osd_inf_rand()%TST_HEIGHT;
                    UINT sy = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_advancedBlend(&rgb, x, y, w, h,
                        &rgb_bl,  sx,  sy, &rgb_8, 0,0, &bs))
                    {
                        PDEBUG("blend failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_debug_fill_fb(&rgb);
                osd_inf_delay(500);
                osd_inf_debug_fill_fb(&osd_fb_surface);
            }
#endif


#if 1       // resize
            PINFOE("\nTest resize capability\n");
            
            osd_inf_debug_fill_fb(&osd_fb_surface);
            PINFOE("  resize YUV->YUV , 250 ops\n");
            oldjiff = jiffies;
            for(i=0; i<250; i++)
            {
                UINT x = osd_inf_rand()%osd_scr_x;
                UINT y = osd_inf_rand()%osd_scr_y;
                UINT h = osd_inf_rand()%osd_buf_y;
                UINT w = osd_inf_rand()%osd_buf_x;
                if(gfx_osi_resize(&osd_fb_surface, x, y, w, h,
                    &yuv, 0,0, TST_WIDTH, TST_HEIGHT,
                    128, 0))
                {
                    PDEBUG("resize failed\n");
                    break;
                }
            }
            gfx_osi_run_engine(1);
            PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
            osd_inf_delay(500);
            
            PINFOE("  resize RGB->YUV Batch mode, 250 ops\n");
            oldjiff = jiffies;
            for(i=0; i<250; i++)
            {
                UINT x = osd_inf_rand()%osd_scr_x;
                UINT y = osd_inf_rand()%osd_scr_y;
                UINT h = osd_inf_rand()%osd_buf_y;
                UINT w = osd_inf_rand()%osd_buf_x;
                if(gfx_osi_resize(&osd_fb_surface, x, y, w, h,
                    &rgb, 0,0, TST_WIDTH, TST_HEIGHT,
                    128, 0))
                {
                    PDEBUG("resize failed\n");
                    break;
                }
            }
            gfx_osi_run_engine(1);
            PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
            osd_inf_delay(500);
            
            if(osd_gfx_bpp >= 16)
            {
                
                PINFOE("  resize YUV->RGB Batch mode, 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_resize(&rgb, x, y, w, h,
                        &yuv, 0,0, TST_WIDTH, TST_HEIGHT,
                        128, 0))
                    {
                        PDEBUG("resize failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                // show it
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_delay(500);
                
                PINFOE("  resize RGB->YUV Batch mode, 250 ops\n");
                osd_inf_fill_8bit_pattern(&rgb);
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%osd_scr_x;
                    UINT y = osd_inf_rand()%osd_scr_y;
                    UINT h = osd_inf_rand()%osd_buf_y;
                    UINT w = osd_inf_rand()%osd_buf_x;
                    if(gfx_osi_resize(&osd_fb_surface, x, y, w, h,
                        &rgb, 0,0, TST_WIDTH, TST_HEIGHT,
                        128, 0))
                    {
                        PDEBUG("resize failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                osd_inf_debug_fill_fb(&osd_fb_surface);
                
                PINFOE("  resize YUV palette ->YUV , 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%osd_scr_x;
                    UINT y = osd_inf_rand()%osd_scr_y;
                    UINT h = osd_inf_rand()%osd_buf_y;
                    UINT w = osd_inf_rand()%osd_buf_x;
                    if(gfx_osi_resize(&osd_fb_surface, x, y, w, h,
                        &yuv_8, 0,0, TST_WIDTH, TST_HEIGHT,
                        128, 0))
                    {
                        PDEBUG("resize failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                osd_inf_delay(500);
                
                osd_inf_debug_fill_fb(&osd_fb_surface);
                PINFOE("  resize RGB palette ->YUV , 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%osd_scr_x;
                    UINT y = osd_inf_rand()%osd_scr_y;
                    UINT h = osd_inf_rand()%osd_buf_y;
                    UINT w = osd_inf_rand()%osd_buf_x;
                    if(gfx_osi_resize(&osd_fb_surface, x, y, w, h,
                        &rgb_8, 0,0, TST_WIDTH, TST_HEIGHT,
                        128, 0))
                    {
                        PDEBUG("resize failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                osd_inf_delay(500);
                
                PINFOE("  resize YUV palette ->RGB Batch mode, 250 ops\n");
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_resize(&rgb, x, y, w, h,
                        &yuv_8, 0,0, TST_WIDTH, TST_HEIGHT,
                        128, 0))
                    {
                        PDEBUG("resize failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                // show it
                osd_inf_debug_fill_fb(&osd_fb_surface);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_delay(500);
                
                PINFOE("  resize RGB palette ->RGB , 250 ops\n");
                osd_inf_fill_8bit_pattern(&rgb);
                oldjiff = jiffies;
                for(i=0; i<250; i++)
                {
                    UINT x = osd_inf_rand()%TST_WIDTH;
                    UINT y = osd_inf_rand()%TST_HEIGHT;
                    UINT h = osd_inf_rand()%TST_HEIGHT;
                    UINT w = osd_inf_rand()%TST_WIDTH;
                    if(gfx_osi_resize(&rgb, x, y, w, h,
                        &rgb_8, 0,0, TST_WIDTH, TST_HEIGHT,
                        128, 0))
                    {
                        PDEBUG("resize failed\n");
                        break;
                    }
                }
                gfx_osi_run_engine(1);
                PINFOE("    Time = %ld jiffies\n", jiffies - oldjiff);
                // show it
                osd_inf_debug_fill_fb(&osd_fb_surface);
                gfx_osi_bitBLT(&osd_fb_surface, 0,0, TST_WIDTH, TST_HEIGHT, &rgb, 0,0, NULL, 0);
                osd_inf_delay(500);
                
            }            
#endif            
            gfx_osi_destroy_surface(&yuv);
            gfx_osi_destroy_surface(&rgb);
            if(osd_gfx_bpp >= 16)
            {
                gfx_osi_destroy_surface(&yuv_8);
                gfx_osi_destroy_surface(&rgb_8);
                gfx_osi_destroy_surface(&yuv_8);
                gfx_osi_destroy_surface(&rgb_bl);
            }
        }
#endif
        
    }
    
    
    
    return 0;
}

static void osd_inf_test_deinit(void)
{
    osd_osi_detach_comp_gfx_surface(osd_fb_device, &osd_fb_surface);
    gfx_osi_destroy_surface(&osd_fb_surface);
    osd_osi_set_display_parm(OSD_DISP_CNTL_BACKCOLOR, 0x1088);
    return ;
}


/*
*  Modularization
*/


module_init(osd_inf_test_init);
module_exit(osd_inf_test_deinit);

