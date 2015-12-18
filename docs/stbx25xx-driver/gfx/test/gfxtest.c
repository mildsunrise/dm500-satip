//pallas/drv/gfx/test/gfxtest.c
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
//  test of GFX controls 
//Revision Log:   
//  Nov/07/2001                             Created by YYD
//  Jun/28/2002            Changed for Vulcan/Vesta by YYD

#include <stdlib.h> // for rand
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <float.h>
#include <math.h>


#include "gfxlib.h"

#include "penguin4.h"

#define H1_X    scr.uWidth
#define H1_Y    scr.uHeight
#define H1_CFG  GFX_SURFACE_AYCBCR_422_8888

#define H2_X    scr.uWidth
#define H2_Y    scr.uHeight
#define H2_CFG  GFX_SURFACE_AYCBCR_422_8888

GFX_SCREEN_INFO_T scr;

void gfx_fill_test_pattern(int fgfx, int hSurface, GFX_SURFACE_LOCK_INFO_T *pSurface)
{
    UINT i,j, x, y, mx, my;

    y = pSurface->plane[0].uHeight;
    x = pSurface->plane[0].uWidth;

    gfx_fillBLT(fgfx, hSurface, 0, 0, x, y, 0);
    my = 1;
    for(j=0; j<y; j+=my)
    {
        mx = 1;
        for(i=0; i<x; i+=mx)
        {
            UINT32 c = ((mx*my)&0x01) ? 0xffffffff : 0x00000000;
            gfx_fillBLT(fgfx, hSurface, i, j, mx, my, c);
            mx++;
        }
        my++;
    }
}


void gfx_fill_color_bar(int fgfx, int hSurface, GFX_SURFACE_LOCK_INFO_T *pSurface)
{
    UINT i,j, k, bpl, x, y, pix, mx, my, nx, ny;

    if(IS_GFX_SURFACE_CLUT(pSurface->uPlaneConfig)) return ; // palette plane is not supported

    pix = pSurface->plane[0].uPixelSize;

    if(IS_GFX_SURFACE_YUV(pSurface->uPlaneConfig))
        pix = 32;   // fill color is argb for yuv

    y = pSurface->plane[0].uHeight;
    x = pSurface->plane[0].uWidth;

    for(i=0; i<8; i++)
    {
        UINT32  fc, r, g, b, a;
        r = (i&1) ? 255:64;
        g = (i&2) ? 255:64;
        b = (i&4) ? 255:64;
        a = 0x192;      // semi trans
        if(pix == 32)
        {
            fc = (r<<16) | (g<<8) | (b) | (a<<24);
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

        gfx_fillBLT(fgfx, hSurface, i*x/8, 0,
            x/8, y, fc);
    }
}

void drawing(int fgfx, int h2, unsigned int uPlaneConfig)
{
    int i, j;
    GFX_RECT_T clip;

   clip.x1 = 50;
   clip.y1 = 20;
   clip.x2 = 400;
   clip.y2 = 250;


#if 1
   gfx_set_surface_clip_rect(fgfx, h2, &clip);
   fprintf(stderr, "Old clip x1,y1, x2, y2 = %d, %d, %d, %d\n", clip.x1,clip.y1,clip.x2,clip.y2);
    fprintf(stderr, "drawing on surface %d \n", h2);
    for (i=10; i<200; i+=2) 
        gfx_draw_horizontal_line(fgfx, h2, 10+i, 300+i, i*2,  gfx_plane_pixel_color(uPlaneConfig, 255, (i*20)&0xff, (255-i*40)&0xff, (128+i*3)&0xff)); 
    sleep(1);
    for (i=10; i<200; i+=4) 
        gfx_draw_rectangle(fgfx, h2, 10+i, 10+i,  i*4, i*4,  gfx_plane_pixel_color(uPlaneConfig, 255, (i*20)&0xff, (255-i*40)&0xff, (128+i*3)&0xff), i&8); 
    sleep(1);
    {
        long x[64], y[64];
        for(i=49; i>=3; i--)
        {
           for(j=0; j<i; j++)
           {
               x[j] = 200 + (100 + 4*i) * cos((double)j/i*3.14159*2);
               y[j] = 200 + (100 + 4*i) * sin((double)j/i*3.14159*2);
           }
           gfx_draw_polygon(fgfx, h2, x, y, i, gfx_plane_pixel_color(uPlaneConfig, 255, (i*20)&0xff, (255-i*40)&0xff, (128+i*3)&0xff), i&1);
        }
    }
    sleep(1);
    for (i=200; i>0; i-=2) 
        gfx_draw_circle(fgfx, h2, 250, 200,  i*2,  gfx_plane_pixel_color(uPlaneConfig, 255, (i*20)&0xff, (255-i*40)&0xff, (128+i*3)&0xff), i&2); 

    sleep(1); 
    
    {
        int x,y;
        for(i=49; i>=3; i--)
        {
            for(j=0; j<i; j++)
            {
                x = 220 + (100 + 4*i) * cos((double)j/i*3.14159*2+i);
                y = 220 + (100 + 4*i) * sin((double)j/i*3.14159*2+i);
                gfx_draw_line(fgfx, h2, 220, 220, x, y,  gfx_plane_pixel_color(uPlaneConfig, 255, (i*20)&0xff, (255-i*40)&0xff, (128+i*3)&0xff));             
            }
        } 
    }

   /*clip.x1 = 50;
   clip.y1 = 20;
   clip.x2 = 400;
   clip.y2 = 250;
*/
   gfx_set_surface_clip_rect(fgfx, h2, &clip);
#endif

#ifdef FREETYPE2_SUPPORT
    
    {
        GFX_FONT_INFO_T  font;
        
        if(!gfx_load_font_freetype2(fgfx, &font, GFX_FONT_ASCII, 32, 127, 40, 40, "helv.ttf"))
        {
            int x,y, lth;
            
            {
                GFX_SURFACE_INFO_T si;
                char *pstr = "Other Surface";
                gfx_get_surface_info(fgfx, h2, &si);
                switch(si.uPlaneConfig)
                {
                case GFX_SURFACE_CLUT8BPP_ARGB:
                    pstr = "GFX_SURFACE_CLUT8BPP_ARGB"; break;
                case GFX_SURFACE_CLUT8BPP_AYCBCR:
                    pstr = "GFX_SURFACE_CLUT8BPP_AYCBCR"; break;
#ifdef GFX_SURFACE_RGB_565
                case GFX_SURFACE_RGB_565:
                    pstr = "GFX_SURFACE_RGB_565"; break;
#endif
                case GFX_SURFACE_AYCBCR_422_8888:
                    pstr = "GFX_SURFACE_AYCBCR_422_8888"; break;
                case GFX_SURFACE_YCBCR_422_888:
                    pstr = "GFX_SURFACE_YCBCR_422_888"; break;
                case GFX_SURFACE_AYCBCR_420_8888:
                    pstr = "GFX_SURFACE_AYCBCR_420_8888"; break;
                case GFX_SURFACE_YCBCR_420_888:
                    pstr = "GFX_SURFACE_YCBCR_420_888"; break;
                case GFX_SURFACE_ARGB_8888:
                    pstr = "GFX_SURFACE_ARGB_8888"; break;
#ifdef GFX_SURFACE_AYCBCR_8888
                case GFX_SURFACE_AYCBCR_8888:
                    pstr = "GFX_SURFACE_AYCBCR_8888"; break;
#endif
                }
                gfx_draw_ascii_string(fgfx, h2, &font, pstr, 10, 10, 
                    gfx_plane_pixel_color(uPlaneConfig, 255, 255, 255, 0),
                    gfx_plane_pixel_color(uPlaneConfig, 255, 0, 0, 128),
                    GFX_DRAW_FONT_BACKGROUND|GFX_DRAW_FONT_ANTIALIAS|GFX_DRAW_FONT_WITHALPHA);
            }
            lth = gfx_draw_ascii_string(fgfx, h2, &font, "AA with BACKGND", 10, 10+font.uHeight, 
                gfx_plane_pixel_color(uPlaneConfig, 255, 255, 255, 0), 
                gfx_plane_pixel_color(uPlaneConfig, 255, 0, 0, 128), 
                GFX_DRAW_FONT_BACKGROUND|GFX_DRAW_FONT_ANTIALIAS|GFX_DRAW_FONT_WITHALPHA);
            
            gfx_draw_ascii_string(fgfx, h2, &font, "AA without BACKGND", 10+lth+4, 10+font.uHeight, 
                gfx_plane_pixel_color(uPlaneConfig, 255, 255, 0, 0), 0, 
                GFX_DRAW_FONT_ANTIALIAS|GFX_DRAW_FONT_WITHALPHA);
            
            lth = gfx_draw_ascii_string(fgfx, h2, &font, "No AA with BACKGND", 10, 10+font.uHeight*2, 
                gfx_plane_pixel_color(uPlaneConfig, 255, 255, 255, 0), 
                gfx_plane_pixel_color(uPlaneConfig, 255, 0, 0, 128), 
                GFX_DRAW_FONT_BACKGROUND | GFX_DRAW_FONT_WITHALPHA);
            
            gfx_draw_ascii_string(fgfx, h2, &font, "No AA without BACKGND", 10+lth+4, 10+font.uHeight*2, 
                gfx_plane_pixel_color(uPlaneConfig, 255, 255, 0, 0), 0, GFX_DRAW_FONT_WITHALPHA) ;
            
            x = 0;
            y = 0;
            for(i=32; i<=127; i++)
            {
                int rtn = gfx_draw_font(fgfx, h2, &font, i, 10 + x, 10 + y + font.uHeight*3, 
                    gfx_plane_pixel_color(uPlaneConfig, 255, (i*20)&0xff, (255-i*40)&0xff, (128+i*3)&0xff),
                    gfx_plane_pixel_color(uPlaneConfig, 128, (i*37)&0xff, (255-i*56)&0xff, (128+i*7)&0xff), 
                    ((i&1) ? 0 : GFX_DRAW_FONT_WITHALPHA | GFX_DRAW_FONT_BACKGROUND) | ((i&2) ? 0 : GFX_DRAW_FONT_ANTIALIAS));
                if(rtn > 0)
                {
                    x+= rtn+2;
                    if(x > 512) { x = 0;  y += gfx_get_ascii_string_height(&font, " ")+2; }
                }
            }
            sleep(5);
            gfx_free_font(fgfx, &font);
        }
    }
#endif
    sleep(1); 
}


int main(int argc, char **argv)
{
    int rtn = 0;
    int fgfx;

    pid_t   pid_child = -1;

    int h0, h1, h2, hmask, halpha, hcursor, hyuv;
    GFX_SURFACE_LOCK_INFO_T s0, s1, s2, smask, salpha, scursor, syuv;

    GFX_PALETTE_T   pal[256];       // for palette color only


    if(argc > 1)
    {
       if(argv[1][0] == '-' && argv[1][1] == 'h')
       {
         fprintf(stderr, "Usage: gfxtest [bmp24-1 [bmp24-2 [bmp8(alpha)]]]\n");
         return 0;
       }
    }

    srand(34321);

    fgfx = h0 = h1 = h2 = hmask = halpha = hcursor = hyuv = -1;

    s0.uPlaneConfig = s1.uPlaneConfig = s2.uPlaneConfig = 
            smask.uPlaneConfig = salpha.uPlaneConfig = scursor.uPlaneConfig = 
            syuv.uPlaneConfig = 0;

    fgfx = gfx_open();

    if(fgfx < 0)
    {
        fprintf(stderr, "Failed to open gfx device '%s'\n", GFX_DEV_NAME);
        rtn=-1; goto GFX_end;
    }

    gfx_set_engine_mode(fgfx, 0); // enable async mode

    gfx_get_screen_info(fgfx, &scr);

    printf("Screen x = %d, y = %d\n", scr.uWidth, scr.uHeight);

#if 0
    if(argc > 2)
    {
        int hbmp = gfx_LoadBMP_Surface(fgfx, NULL, NULL, NULL, argv[2], 255);
	if(hbmp >= 0)
        {
             gfx_attach_surface(fgfx, hbmp, GFX_VDEV_OSDGFX);
	     printf("Woo, bmp works!\n");
	     getchar();
	     gfx_detach_surface(fgfx, hbmp, GFX_VDEV_OSDGFX);
	     gfx_bitBLT(fgfx, hyuv, 0, 0, 1024, 1024, hbmp, 0, 0, NULL, 0);
             gfx_destroy_surface(fgfx, hbmp);
	}
    }

    if(argc > 4)
    {
        int hbmp = gfx_LoadTGA32b_Surface(fgfx, NULL, NULL, NULL, argv[4]);
	if(hbmp >= 0)
        {
             gfx_attach_surface(fgfx, hbmp, GFX_VDEV_OSDGFX);
	     printf("Woo, tga works !\n");
	     getchar();
	     gfx_detach_surface(fgfx, hbmp, GFX_VDEV_OSDGFX);
	     gfx_bitBLT(fgfx, hyuv, 0, 0, 1024, 1024, hbmp, 0, 0, NULL, 0);
             gfx_destroy_surface(fgfx, hbmp);
	}
    }
#endif

    h1 = gfx_create_surface(fgfx, H1_X, H1_Y, H1_CFG, GFX_VDEV_NULL, 0);
    if(h1 < 0)
    {
        fprintf(stderr,"Failed to create gfx surface 1\n");
        rtn=-1; goto GFX_end;
    }

    h2 = gfx_create_surface(fgfx, H2_X, H2_Y, H2_CFG, GFX_VDEV_NULL, 0);
    if(h2 < 0)
    {
        fprintf(stderr,"Failed to create gfx surface 2\n");
        rtn=-1; goto GFX_end;
    }

    hyuv = gfx_create_surface(fgfx, 640, 480, GFX_SURFACE_YCBCR_422_888, GFX_VDEV_OSDGFX, gfx_plane_pixel_color(GFX_SURFACE_AYCBCR_422_8888, 255, 0, 0, 255));
    if(hyuv < 0)
    {
        fprintf(stderr,"Failed to create gfx surface 2\n");
        rtn=-1; goto GFX_end;
    }

    // mask is 1 bpp only
    hmask = gfx_create_surface(fgfx, H2_X, H2_Y, GFX_SURFACE_RAW1BPP, GFX_VDEV_NULL, 0);
    if(hmask < 0)
    {
        fprintf(stderr,"Failed to create gfx surface mask\n");
        rtn=-1; goto GFX_end;
    }

    // alpha can also use 1 bpp
    halpha = gfx_create_surface(fgfx, H2_X, H2_Y, GFX_SURFACE_CLUT8BPP_ARGB, GFX_VDEV_NULL, 0);
    if(halpha < 0)
    {
        fprintf(stderr,"Failed to create gfx surface alpha\n");
        rtn=-1; goto GFX_end;
    }

    // cursor can be 1 to 4 bpp
	// dirty fix for stb03xxx and stbx2xxx
	// cursor width is no more than 64 in 4bpp mode
    hcursor = gfx_create_surface(fgfx, PENGUIN4_X/2, PENGUIN4_Y/2, GFX_SURFACE_CURSOR4BPP_RGB, GFX_VDEV_NULL, 0);
    if(hcursor < 0)
    {
        fprintf(stderr,"Failed to create gfx surface cursor\n");
        rtn=-1; goto GFX_end;
    }

    h0 = gfx_get_shared_surface(fgfx);

    if(h0 < 0)
    {
        fprintf(stderr, "Linux Frame buffer driver is not loaded !\n");
    }

    if(h0 >= 0 && gfx_lock_surface(fgfx, h0, &s0) < 0)
    {
        fprintf(stderr,"Failed to lock Linux framebuffer surface\n");
        rtn=-1; goto GFX_end;
    }
    else if(h0 >= 0)
    {
        fprintf(stderr, "Framebuffer is %d x %d x %d bpp\n", 
            s0.plane[0].uWidth, s0.plane[0].uHeight, s0.plane[0].uPixelSize);
    }

    if(gfx_lock_surface(fgfx, h1, &s1) < 0)
    {
        fprintf(stderr,"Failed to lock gfx surface 1\n");
        rtn=-1; goto GFX_end;
    }

    if(gfx_lock_surface(fgfx, h2, &s2) < 0)
    {
        fprintf(stderr,"Failed to lock gfx surface 2\n");
        rtn=-1; goto GFX_end;
    }

    if(gfx_lock_surface(fgfx, hmask, &smask) < 0)
    {
        fprintf(stderr,"Failed to lock gfx surface mask\n");
        rtn=-1; goto GFX_end;
    }
    if(gfx_lock_surface(fgfx, halpha, &salpha) < 0)
    {
        fprintf(stderr,"Failed to lock gfx surface alpha\n");
        rtn=-1; goto GFX_end;
    }

    if(gfx_lock_surface(fgfx, hcursor, &scursor) < 0)
    {
        fprintf(stderr,"Failed to lock gfx surface cursor\n");
        rtn=-1; goto GFX_end;
    }

    if(gfx_lock_surface(fgfx, hyuv, &syuv) < 0)
    {
        fprintf(stderr,"Failed to lock gfx surface yuv\n");
        rtn=-1; goto GFX_end;
    }

    {
       int i, x, y;

       gfx_set_surface_palette(fgfx, hcursor, 0, 16, penguin4pal);

       for(i=0; i<16; i++)
       {
           gfx_set_cursor_attributes(fgfx, hcursor, i, GFX_CURSOR_NO_PEEP_STEADY);
       }
	   //resample the large penguin
       for(y=0; y< PENGUIN4_Y/2; y++)
	   {
		   for(x=0; x<PENGUIN4_X/2; x++)
		   {
				((char *)scursor.plane[0].pPlane)[x+y*PENGUIN4_X/4] = (((char *)penguin4b)[x*2 + y*PENGUIN4_X]&0xf0) | ((((char *)penguin4b)[x*2 + 1 + y*PENGUIN4_X]&0xf0) >> 4);
		   }
	   }
       // memcpy(scursor.plane[0].pPlane, penguin4b, sizeof(penguin4b));
    }

    gfx_attach_surface(fgfx, hcursor, GFX_VDEV_OSDCUR);


    // fork a child process to take care of the cursor
    if((pid_child = fork()) == 0)  // child
    {
        int x, x0, y, k;

#define MAX_STEP    150
#define FACTOR      (MAX_STEP*MAX_STEP/4*MAX_STEP)

        while(1)
        {
            k = MAX_STEP;
            for(x0= -MAX_STEP/2; x0<(INT)scr.uWidth;)
            {
                for(x = x0; x < x0+k && x < (INT)scr.uWidth; x+=3)
                {
                    y = scr.uHeight/2 - (x-x0)*(x0+k - x)*k*scr.uHeight/2/FACTOR;
                    gfx_move_cursor(fgfx, x, y);
                    usleep(7*1000);
                }
                x0 += k;
                k = k*29/32;
                if(k < 10)  k = 10;
            }
        }
    }


    // generate mask
    {
        // make pattern 4x4
        UINT bpl, y, i;
        BYTE pat=0xf0;
        
        bpl = smask.plane[0].uBytePerLine; 
        y = smask.plane[0].uHeight;
        
        for(i=0; i<y-3; i+=4, pat = ~pat)
        {
            memset(smask.plane[0].pPlane+i*bpl, pat, 4*bpl);
        }
    }

    
    fprintf(stderr, "Show surface yuv and wait for 2 seconds\n");
    gfx_attach_surface(fgfx, hyuv, GFX_VDEV_OSDIMG);  // display it
drawing(fgfx, hyuv, syuv.uPlaneConfig);
    sleep(2);
    gfx_detach_surface(fgfx, hyuv, GFX_VDEV_OSDIMG);  // hide it


    fprintf(stderr, "Show surface 1 and wait for 2 seconds\n");
    gfx_attach_surface(fgfx, h1, GFX_VDEV_OSDGFX);  // display it

//    gfx_move_cursor(fgfx, H1_X/2, H1_Y/2);

drawing(fgfx, h1, s1.uPlaneConfig);

    gfx_fill_color_bar(fgfx, h1, &s1);

    if(argc > 1)
    {
        int w,h;
        int hbmp = gfx_LoadBitmap_Surface(fgfx, NULL, &w, &h, argv[1], 255);

        if(hbmp < 0)
        {
            fprintf(stderr, "Failed to load bitmap '%s' into surface 1\n", argv[1]);
        }
        gfx_bitBLT(fgfx, h1, 0,0, w, h, hbmp, 0,0, NULL, 0);
        gfx_destroy_surface(fgfx, hbmp);
    }

    sleep(2);


    gfx_attach_surface(fgfx, h2, GFX_VDEV_OSDGFX);  // display it
    fprintf(stderr, "Show surface 2 and wait for 2 seconds\n");


drawing(fgfx, h2, s2.uPlaneConfig);

    gfx_fill_test_pattern(fgfx, h2, &s2);

    if(argc > 2)
    {
        int w,h;
        int hbmp = gfx_LoadBitmap_Surface(fgfx, NULL, &w, &h, argv[2], 255);

        if(hbmp < 0)
        {
            fprintf(stderr, "Failed to load bitmap '%s' into surface 1\n", argv[2]);
        }
        gfx_bitBLT(fgfx, h2, 0,0, w, h, hbmp, 0,0, NULL, 0);
        gfx_destroy_surface(fgfx, hbmp);
    }

    sleep(2);

    gfx_attach_surface(fgfx, halpha, GFX_VDEV_OSDGFX);  // display it
    fprintf(stderr, "Show surface alpha and wait for 2 seconds\n");

drawing(fgfx, halpha, salpha.uPlaneConfig);

    // generate alpha pattern
    {
        UINT bpl, y, i;
        
        bpl = salpha.plane[0].uBytePerLine; 
        y = salpha.plane[0].uHeight;

        for(i=0; i<y; i++)
        {
            memset(salpha.plane[0].pPlane+i*bpl, i%0xff, bpl);
        }
    }

    if(argc > 3)
    {
        GFX_PALETTE_T	pal[256];
        if(gfx_LoadBMP8b(&salpha, pal, 0,0, salpha.plane[0].uWidth, salpha.plane[0].uHeight, argv[3], 0, 0, 255))
        {
            fprintf(stderr, "Failed to load bitmap '%s' into alpha surface\n", argv[3]);
        }
        else
        {
    //        int i;
    //        for(i=0; i<256; i++) pal[i].r = pal[i].g = pal[i].b = i, pal[i].a=255;
            gfx_set_surface_palette(fgfx, halpha, 0, 255, pal);
        }
    }


    sleep(2);

    // this is a test for reattachment feature, and is not needed if you
    // just want to detach h1. 
    fprintf(stderr, "Show surface 2 and wait for 1 seconds\n");
    gfx_attach_surface(fgfx, h2, GFX_VDEV_OSDGFX);  
    // reattaching h2 should be handled correctly

    sleep(1);

    gfx_detach_surface(fgfx, h1, GFX_VDEV_OSDGFX);  // hide it
    gfx_detach_surface(fgfx, halpha, GFX_VDEV_OSDGFX);  // hide it

    fprintf(stderr, "random fill 300 \n");
    {
        int i;
        for(i=0; i<300; i++)
        {
            UINT x = rand()%H1_X;
            UINT y = rand()%H1_Y;
            UINT h = rand()%H1_X + 20;
            UINT w = rand()%H1_Y + 20;
            UINT c = rand()%255 | (rand()%255<<8) | (rand()%255<<16) | (rand()%255<<24);
            
            //           PINFOE("Fill (x,y,w,h, c) %d %d %d %d 0x%8.8x\n", x, y, w, h, c);
            gfx_fillBLT(fgfx, h2, x, y, w, h, c);
        }
        gfx_wait_for_engine(fgfx, 30000);
    }

    fprintf(stderr, "random masked fill 300 \n");

    {
        int i;
        for(i=0; i<300; i++)
        {
            UINT x = rand()%H1_X;
            UINT y = rand()%H1_Y;
            UINT h = rand()%H1_X + 20;
            UINT w = rand()%H1_Y + 20;
            UINT c = rand()%255 | (rand()%255<<8) | (rand()%255<<16) | (rand()%255<<24);
            
            //           PINFOE("Fill (x,y,w,h, c) %d %d %d %d 0x%8.8x\n", x, y, w, h, c);
            gfx_advancedFillBLT(fgfx, h2, x, y, w, h, c,
                hmask, 0, 0,  GFX_ROP_DISABLE, 1, 0x00ff0000, 0, 0xf0f0f0f0);
        }
        gfx_wait_for_engine(fgfx, 30000);
    }

    sleep(2);


    fprintf(stderr, "bitBLT 1 to 2: random 300\n");
    
    {
        int i;
        ALPHA_SELECT as;
        as.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
        as.globalAlphaValue = 0xff; // not used
        for(i=0; i<300; i++)
        {
            UINT x = rand()%H2_X;
            UINT y = rand()%H2_Y;
            UINT h = rand()%H2_X+20;
            UINT w = rand()%H2_Y+20;
            UINT sh = rand()%H1_X;
            UINT sw = rand()%H1_Y;

            gfx_bitBLT(fgfx, h2, x, y,
                        h, w,
                        h1, sh, sw, &as, 1);
        }
        gfx_wait_for_engine(fgfx, 30000);
    }

    fprintf(stderr, "masked bitBLT 1 to 2: random 300\n");
    
    {
        int i;
        ALPHA_SELECT as;
        as.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
        as.globalAlphaValue = 0xff; // not used
        for(i=0; i<300; i++)
        {
            UINT x = rand()%H2_X;
            UINT y = rand()%H2_Y;
            UINT h = rand()%H2_X+20;
            UINT w = rand()%H2_Y+20;
            UINT sh = rand()%H1_X;
            UINT sw = rand()%H1_Y;

            gfx_advancedBitBLT(fgfx, h2, x, y,
                        h, w,
                        h1, sh, sw, 
                        hmask, 0, 0,
                        GFX_ROP_DISABLE,
                        0, 0xf0f0f0f0,
                        &as);
        }
        gfx_wait_for_engine(fgfx, 30000);
    }

#if 0       //unsupported
    fprintf(stderr, "masked bitBLT 1 to 2 with pix mask: random 300\n");
    
    {
        int i;
        ALPHA_SELECT as;
        as.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
        as.globalAlphaValue = 0xff; // not used
        for(i=0; i<300; i++)
        {
            UINT x = rand()%H2_X;
            UINT y = rand()%H2_Y;
            UINT h = rand()%H2_X+20;
            UINT w = rand()%H2_Y+20;
            UINT sh = rand()%H1_X;
            UINT sw = rand()%H1_Y;

            gfx_advancedBitBLT(fgfx, h2, x, y,
                        h, w,
                        h1, sh, sw, 
                        hmask, 0, 0,
                        GFX_ROP_DISABLE,
                        1, 0xf000fff0,
                        &as);
        }
        gfx_wait_for_engine(fgfx, 30000);
    }

    sleep(2);
#endif


#if 1   // only supported for RGB8888 
    {
        int hrgb1, hrgb2;
        hrgb1 = hrgb2 = -1;
        hrgb1 = gfx_create_surface(fgfx, H1_X, H1_Y, GFX_SURFACE_ARGB_8888, GFX_VDEV_NULL, 0);
        hrgb2 = gfx_create_surface(fgfx, H2_X, H2_Y, GFX_SURFACE_ARGB_8888, GFX_VDEV_NULL, 0);
        if(hrgb1 >= 0 && hrgb2 >= 0)
        {
            gfx_bitBLT(fgfx, hrgb1, 0,0, H1_X, H1_Y, h1, 0,0, NULL, 0);
            gfx_bitBLT(fgfx, hrgb2, 0,0, H2_X, H2_Y, h2, 0,0, NULL, 0);
            
            fprintf(stderr, "blend 1 to 2: random 50\n");
            
            {
                int i;
                ALPHA_BLEND_SELECT bs;
                bs.blendInputSelect = GFX_BLEND_ALPHA_FROM_GIVEN;
                bs.storedAlphaSelect = GFX_DEST_ALPHA_FROM_DESTINATION;
                
                for(i=0; i<50; i++)
                {
                    UINT x = rand()%H2_X;
                    UINT y = rand()%H2_Y;
                    UINT h = rand()%H2_X+20;
                    UINT w = rand()%H2_Y+20;
                    UINT sh = rand()%H1_X;
                    UINT sw = rand()%H1_Y;
                    
                    bs.globalAlphaValue = rand()%255; 
                    
                    gfx_blend(fgfx, hrgb2, x, y,
                        h, w,
                        hrgb1, sh, sw, &bs);
                }
                gfx_wait_for_engine(fgfx, 30000);
            }
            gfx_bitBLT(fgfx, h2, 0,0, H2_X, H2_Y, hrgb2, 0,0, NULL, 0);

            sleep(2);
            
            fprintf(stderr, "blend with external alpha 1 to 2: random 50\n");
            
            {
                int i;
                ALPHA_BLEND_SELECT bs;
                bs.blendInputSelect = GFX_BLEND_ALPHA_FROM_PATTERN;
                bs.storedAlphaSelect = GFX_DEST_ALPHA_FROM_GIVEN;
                bs.globalAlphaValue = 0xff; // not used
                for(i=0; i<50; i++)
                {
                    UINT x = rand()%H2_X;
                    UINT y = rand()%H2_Y;
                    UINT h = rand()%H2_X+20;
                    UINT w = rand()%H2_Y+20;
                    UINT sh = rand()%H1_X;
                    UINT sw = rand()%H1_Y;
                    
                    gfx_advancedBlend(fgfx, hrgb2, x, y, h, w,
                        hrgb1, sh, sw, 
                        halpha, 0, 0, &bs);
                }
                gfx_wait_for_engine(fgfx, 30000);
            }
            gfx_bitBLT(fgfx, h2, 0,0, H2_X, H2_Y, hrgb2, 0,0, NULL, 0);
            
            sleep(2);
        }
        else
        {
            fprintf(stderr, "Failed to create RGB blend surface\n");
        }
        if(hrgb1 >= 0)
            gfx_destroy_surface(fgfx, hrgb1);
        if(hrgb2 >= 0)
            gfx_destroy_surface(fgfx, hrgb2);
    }

#endif



#if 0   // not supported
    fprintf(stderr, "colorKey from 1 to 2: random 300\n");
    
    {
        int i, x, y;
        COLOR_KEY_SELECT cs;
        ALPHA_SELECT as;
        GFX_SURFACE_DISPLAY_T dp;

        UINT dx, dy;
        
        dx = H1_X/4; dy = H1_Y/4;

        gfx_set_surface_clip_rect(fgfx, h1, NULL);  // disable clipping on h1
        gfx_fillBLT(fgfx, h1, 0, 0, dx, dy, 0x8000ff00);
        gfx_advancedFillBLT(fgfx, h1, 4, 4, dx-8, dy-8, 0xffff00ff,
                            hmask, 0, 0, GFX_ROP_DISABLE, 1, 0xff000000, 0, 0xf0f0f0f0);
        memset(&dp, 0, sizeof(dp));
	dp.uWinX = scr.uWidth/2 - dx/2;
        dp.uWinY = scr.uHeight/2 - dy/2;
        dp.uWinWidth = dx;
        dp.uWinHeight = dy;
        gfx_set_surface_display_parm(fgfx, h1, &dp);
        gfx_attach_surface(fgfx, h1, GFX_VDEV_OSDGFX);
        sleep(1);
        gfx_detach_surface(fgfx, h1, GFX_VDEV_OSDGFX);

        as.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
        as.globalAlphaValue = 0xff; // not used
        cs.colorKeyCompareSelect = GFX_COLORKEY_COMPARE_SOURCE;
        cs.colorKeyCompareValue = 0xffff00ff;
        cs.colorKeyOutputSelect = GFX_COLORKEY_OUTPUT_KEEP;
        cs.replacementColor = 0xffff0000;

        x = dp.uWinX;
        y = dp.uWinY;

        gfx_set_surface_clip_rect(fgfx, h1, NULL);  // disable clipping on h1
        for(i=0; i<300; i++)
        {
            int xinc = rand()%10 - 5;
            int yinc = rand()%10 - 5;
            x += xinc;
            y += yinc; 
            if(x < 0) x = 0;
            if(y < 0) y = 0;
            // save the block before do
	    gfx_bitBLT(fgfx, h1, dx, 0, dx, dy, h2, x, y, &as, 0);
            gfx_colorKey(fgfx, h2, x, y, dx, dy,
                        h1, 0, 0, 
                        &cs, &as);
            usleep(100*100);
            // restore it
            gfx_bitBLT(fgfx, h2, x, y, dx, dy, h1, dx, 0 , &as, 0);
        }
        gfx_wait_for_engine(fgfx, 30000);
    }

    sleep(2);

#endif
    
    fprintf(stderr, "resize from 1 to 2: random 300\n");
   
    {
        int i, r;
        for(i=0; i<300; i++)
        {
            UINT x = rand()%H2_X/2;
            UINT y = rand()%H2_Y/2;
            UINT h = rand()%H2_X + 4;
            UINT w = rand()%H2_Y + 4;
            UINT sx = rand()%H1_X/2;
            UINT sy = rand()%H1_Y/2;
            UINT sh = rand()%H1_X +4;
            UINT sw = rand()%H1_Y +4;

            // printf("%3d: %3d, %3d,  %3d, %3d -> %3d, %3d,  %3d, %3d ", i, sx, sy, sh, sw, x, y, h, w, r); fflush(stdout);
            r= gfx_resize(fgfx, h2, x, y, h, w,
                        h1, sx, sy, sh, sw, 
                        0xc0, 1);
            // printf("[%d]\n", r); fflush(stdout);
        }
    }

    sleep(2);

    fprintf(stderr, "That's all folks\n");

    gfx_detach_surface(fgfx, h2, GFX_VDEV_OSDGFX);  // hide it
    gfx_detach_surface(fgfx, hcursor, GFX_VDEV_OSDCUR);

    /*
    gfx_SaveBMP24b("save24.bmp", &s2, 0,0, s2.plane[0].uWidth, s2.plane[0].uHeight);

    {
        GFX_PALETTE_T pal[256];
        gfx_get_surface_palette(fgfx, halpha, 0, 256, pal);
        gfx_SaveBMP8b("save8.bmp", &salpha, pal, 0,0, salpha.plane[0].uWidth, salpha.plane[0].uHeight);
    }
    */

GFX_end:

    if(fgfx >= 0)
    {
        gfx_wait_for_engine(fgfx, 30000);
        if(s0.uPlaneConfig) gfx_unlock_surface(fgfx, h0, &s0);
        if(s1.uPlaneConfig) gfx_unlock_surface(fgfx, h1, &s1);
        if(s2.uPlaneConfig) gfx_unlock_surface(fgfx, h2, &s2);
        if(smask.uPlaneConfig) gfx_unlock_surface(fgfx, hmask, &smask);
        if(salpha.uPlaneConfig) gfx_unlock_surface(fgfx, halpha, &salpha);
        if(scursor.uPlaneConfig) gfx_unlock_surface(fgfx, hcursor, &scursor);

        if(syuv.uPlaneConfig) gfx_unlock_surface(fgfx, hyuv, &syuv);

        if(h1 >= 0) gfx_destroy_surface(fgfx, h1);
        if(h2 >= 0) gfx_destroy_surface(fgfx, h2);
        if(hyuv >= 0) gfx_destroy_surface(fgfx, hyuv);
        if(hmask >= 0) gfx_destroy_surface(fgfx, hmask);
        if(halpha >= 0) gfx_destroy_surface(fgfx, halpha);
        if(hcursor >= 0) gfx_destroy_surface(fgfx, hcursor);

        if (pid_child > 0)
        {
            kill(pid_child, SIGQUIT);
            wait(NULL);
        }

        gfx_close(fgfx);
    }
    return rtn;
}



