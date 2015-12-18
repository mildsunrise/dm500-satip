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
//  Nov/07/2001                          Created by YYD

#include <stdlib.h> // for rand
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <float.h>
#include <math.h>


#include "gfxlib.h"


int main(int argc, char **argv)
{
    int rtn = 0;
    int fgfx;
    int h1, i;
    int cnt, sel;
    unsigned int pcfg, af, afdt, afen;

    GFX_PALETTE_T   pal[256];       // for palette color only
    GFX_SURFACE_DISPLAY_T disp;
    GFX_SCREEN_INFO_T scr, scr1;


    fgfx = h1 = -1;

    fgfx = gfx_open();

    if(fgfx < 0)
    {
        fprintf(stderr, "Failed to open gfx device '%s'\n", GFX_DEV_NAME);
        rtn=-1; goto GFX_end;
    }

    gfx_set_engine_mode(fgfx, 0); // enable async mode

    gfx_get_screen_info(fgfx, &scr1);

    scr.uHeight = 400;
    scr.uWidth = 512;
    scr.uLeft = 80;
    scr.uUpper = 40;

    gfx_set_screen_info(fgfx, &scr);


    do
    {
        do
        {
            sel = -1;
            printf("Select depth: 1:8 bit,  2: true color, 0:quit test -> ");
            cnt = scanf("%d", &sel);
            fflush(stdin);
            if(cnt > 0)
            {
                switch(sel)
                {
                case 1:
                    pcfg = GFX_SURFACE_CLUT8BPP_ARGB;
                    break;
                case 2:
                    pcfg = GFX_SURFACE_YCBCR_422_888;
                    break;
                case 0:
                    goto GFX_end;
                default:
                    sel = -1;
                }
            }
        }
        while(sel < 0);

        h1 = gfx_create_surface(fgfx, scr.uWidth, scr.uHeight, pcfg, GFX_VDEV_OSDGFX, 0);
        if(h1 < 0)
        {
            fprintf(stderr,"Failed to create gfx surface 1\n");
            rtn=-1; goto GFX_end;
        }

        for(i=0; i<256; i++)
            pal[i].a = 255,  pal[i].r = pal[i].g =pal[i].b = i;

        gfx_set_surface_palette(fgfx, h1, 0, 256, pal);

        gfx_attach_surface(fgfx, h1, GFX_VDEV_OSDGFX);


        for(i=0; i<16; i++)
        {
            gfx_fillBLT(fgfx, h1, i*scr.uWidth/16, 0, scr.uWidth/16, scr.uHeight,  gfx_plane_pixel_color(pcfg, 255, i*17, i*17, i*17));
        }

        for(i=0; i<scr.uHeight/4; i+=8)
        {
            gfx_fillBLT(fgfx, h1, 0, i, scr.uWidth, 1, 0xffffffff);
        }

        for(; i<scr.uHeight*3/8; i+=8)
        {
            gfx_fillBLT(fgfx, h1, 0, i, scr.uWidth, 3, 0xffffffff);
        }

        for(; i<scr.uHeight/2; i+=8)
        {
            gfx_fillBLT(fgfx, h1, 0, i, scr.uWidth, 5, 0xffffffff);
        }

#ifndef  FREETYPE2_SUPPORT
        for(; i<scr.uHeight*3/4; i+=8)
        {
            gfx_fillBLT(fgfx, h1, 0, i, scr.uWidth, 1, 0);
        }

        for(; i<scr.uHeight*7/8; i+=8)
        {
            gfx_fillBLT(fgfx, h1, 0, i, scr.uWidth, 3, 0);
        }

        for(; i<scr.uHeight; i+=8)
        {
            gfx_fillBLT(fgfx, h1, 0, i, scr.uWidth, 5, 0);
        }


        gfx_wait_for_engine(fgfx, -1);

#else
        {
            GFX_FONT_INFO_T  font;
            int y = scr.uHeight/2;
            for(i=8; i<40; i+=4)
            {
                if(!gfx_load_font_freetype2(fgfx, &font, GFX_FONT_ASCII, 0, 127, i, i, "helv.ttf"))
                {
                    gfx_draw_ascii_string(fgfx, h1, &font, "TEST Antiflicker 123@#$AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz4567890,.;:'\"", 0, y, 0xffffffff, 0, GFX_DRAW_FONT_ANTIALIAS);
                    y += font.uHeight;
                    gfx_free_font(fgfx, &font);
                }
            }
        }  
#endif

        do
        {
            
            do
            {
                printf("Enable antiflicker: 1: yes,  0: No ->");
                cnt = scanf("%u", &afen);
                fflush(stdin);
            }
            while(cnt <= 0);
            
            if(afen)
            {
                gfx_set_display_attr(fgfx, GFX_DISP_CNTL_EDAF, 1);
                
                do
                {
                    printf("Antiflicker detect threshold: 0--3 ->");
                    cnt = scanf("%u", &afdt);
                    fflush(stdin);
                }
                while(cnt <= 0 || afdt < 0 || afdt > 3);
                gfx_set_display_attr(fgfx, GFX_DISP_CNTL_AFDT, afdt);
                
                do
                {
                    printf("Antiflicker correction level: 0--3 ->");
                    cnt = scanf("%u", &af);
                    fflush(stdin);
                }
                while(cnt <= 0 || af < 0 || af > 3);
                gfx_get_surface_display_parm(fgfx, h1, &disp);
                disp.bFlickerCorrect = af;
                gfx_set_surface_display_parm(fgfx, h1, &disp);
            }
            else
            {
                gfx_set_display_attr(fgfx, GFX_DISP_CNTL_EDAF, 0);
            }
            
            do
            {
                sel = -1;
                printf("Select : 1: change af settings,  2: change color depth, 0:quit ->");
                cnt = scanf("%d", &sel);
                fflush(stdin);
            }
            while(cnt <= 0 || sel < 0 || sel > 2 );

        } while(sel == 1);

        gfx_destroy_surface(fgfx, h1);
        h1 = -1;

    } while(sel != 0);        


GFX_end:

    if(fgfx >= 0)
    {
        gfx_wait_for_engine(fgfx, 30000);
        if(h1 >= 0) gfx_destroy_surface(fgfx, h1);
        gfx_set_screen_info(fgfx, &scr1);
        gfx_close(fgfx);
    }
    return rtn;
}



