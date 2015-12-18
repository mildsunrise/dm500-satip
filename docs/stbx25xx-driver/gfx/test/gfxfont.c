/*----------------------------------------------------------------------------+
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
|       IBM CONFIDENTIAL
|       (C) COPYRIGHT IBM CORPORATION 1999, 2001 
+-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| Author:    Yudong Yang
| Component: 
| File:      gfxfont.c
| Purpose:   Font rendering interface functions (using freetype2 library).
| Changes:
| Date:     Author Comment:
| -----     ------ --------
| 08-Apr-02  YYD   Created
+----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include <freetype.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


#include "gfxlib.h"




int gfx_load_font_freetype2(
   int fdGfxDev, 
   GFX_FONT_INFO_T *pFont,
   GFX_FONT_ENCODING_T encoding,
   unsigned int charStart,
   unsigned int charEnd,
   unsigned int uWidth,
   unsigned int uHeight,
   char * pFontPath
   )
{
    FT_Library ft_library;
    FT_Face face;
    int rtn;
   
    if(!pFont || charStart > charEnd || !uWidth || !uHeight)
    {
        fprintf(stderr, "Invalid parameters !\n");
        return -1;
    }
    
    memset(pFont, 0, sizeof(GFX_FONT_INFO_T));

    if(!pFontPath)
        pFontPath = "/usr/share/fonts/helv.ttf";
    
    rtn = FT_Init_FreeType(&ft_library);
    if(rtn)
    {
        fprintf(stderr, "Failed to init FreeType2 library!\n");
        return -1;
    }
    
    rtn = FT_New_Face(ft_library, pFontPath, 0, &face);
    if(rtn)
    {
        fprintf(stderr, "Failed to load font '%s', rtn= %d !\n", pFontPath, rtn);
        goto clean_up1;
    }
    
    //if(face->flags & FT_FACE_FLAG_SCALABLE) // is scalable font
    //{
        rtn = FT_Set_Pixel_Sizes( face, uWidth, uHeight);
        if(rtn)
        {
            fprintf(stderr, "Failed to set font size to %dx%d, rtn= %d !\n", pFontPath, rtn);
            goto clean_up;
        }
    //}
    //else    // look for a near size
    //{
    //    int i;
    //    for(i=0; i<
    //}

#if 0
    // look for available charmap
    {
        FT_CharMap  found = 0;
        FT_CharMap  charmap;
        int         n;
        
        switch(encoding)
        {
            case GFX_FONT_ASCII:
               charmap = ft_encoding_latin_2;
               break;
            case GFX_FONT_GB2312:
               charmap = ft_encoding_gb2312;
               break;
            case GFX_FONT_UNICODE:
               charmap = ft_encoding_unicode;
               break;
        }
        for ( n = 0; n < face->num_charmaps; n++ )
        {
          if (face->charmaps[n]->encoding == charmap)
          {
            found = charmap;
            break;
          }
        }
        if(!found)
        {
            fprintf(stderr, "Failed to find font encoding %d !\n", encoding);
            rtn = -1;
            goto clean_up;
        }
        FT_Set_Charmap( face, found );
    }
#endif
    
    // try to get included glyphs
    
    {
        int i,j, c;
        int xp, yp, xmax, ymax, t;

        FT_Glyph        glyph;                                         
        FT_BitmapGlyph  glyph_bitmap;                                  

        int *xadv, *yadv;
        int *cmap;

        pFont->uWidth    = malloc(sizeof(unsigned int)*(charEnd - charStart+1)*3);
        if(!pFont->uWidth)
        {
            fprintf(stderr, "Failed to allocate buffer!\n");
            free(cmap);
            rtn = -1;
            goto clean_up;
        }
        pFont->uPosX     = pFont->uWidth  + charEnd - charStart+1;
        pFont->uPosY     = pFont->uPosX + charEnd - charStart+1;

        cmap = malloc(face->num_glyphs * 2 * sizeof(int) + (charEnd - charStart+1)  * sizeof(int));

        if(!cmap)
        {
            fprintf(stderr, "Failed to alloc working buffer!\n");
            free(pFont->uWidth);
            rtn = -1;
            goto clean_up;
        }

        xadv = cmap + charEnd - charStart+1;
        yadv = xadv + face->num_glyphs;
    
        memset(cmap, 0xff, (charEnd - charStart+1)  * sizeof(int)); 
        memset(xadv, 0, face->num_glyphs * 2 * sizeof(int));

        xmax = ymax = 0;
        for(c=charStart; c<=charEnd; c++)
        {
            int glyph_index = FT_Get_Char_Index(face, c);
            FT_GlyphSlot  slot;
            
            rtn = FT_Load_Glyph( 
                  face,          /* handle to face object */
                  glyph_index,   /* glyph index           */
                  FT_LOAD_DEFAULT );  /* load flags, see below */
            if(rtn)  continue;
            cmap[c-charStart] = glyph_index;
#if 1
            // extract glyph image                                         
            rtn = FT_Get_Glyph( face->glyph, &glyph );                  
                                                                       
            // convert to a bitmap (default render mode + destroy old)    
            if ( glyph->format != ft_glyph_format_bitmap )                
            {                                                             
                rtn = FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal,
                                        0, 1 );                       
                if ( rtn ) continue; // glyph unchanged                          
            }                                                         

            glyph_bitmap = (FT_BitmapGlyph)glyph;                 
            xadv[glyph_index] = face->glyph->advance.x >> 6;
            if(glyph_bitmap->left < 0)
            {
                if(xadv[glyph_index] < glyph_bitmap->bitmap.width-glyph_bitmap->left)  // some glyphs has adv = 0;
                   xadv[glyph_index] = glyph_bitmap->bitmap.width-glyph_bitmap->left;
            }
            else
            {
                if(xadv[glyph_index] < glyph_bitmap->bitmap.width+glyph_bitmap->left)  // some glyphs has adv = 0;
                   xadv[glyph_index] = glyph_bitmap->bitmap.width+glyph_bitmap->left;
            }
//            yadv[glyph_index] = face->glyph->advance.y >> 6;
            yadv[glyph_index] = (face->glyph->linearVertAdvance + 0xffff)>>16; //slot->metrics.vertAdvance>>6;
            // discard glyph image (bitmap or not)
            FT_Done_Glyph( glyph );                      
#else            
            slot = face->glyph;

            cmap[c-charStart] = glyph_index;
            xadv[glyph_index] = (slot->linearHoriAdvance + 0xffff)>>16; // slot->metrics.horiAdvance>>6;
            yadv[glyph_index] = (slot->linearVertAdvance + 0xffff)>>16; //slot->metrics.vertAdvance>>6;
#endif

            pFont->uWidth[c-charStart] = xadv[glyph_index];

            //printf("char %d (%c), glyph %3d : xadv = %d yadv = %d\n", c, c, glyph_index, 
            //  xadv[glyph_index], yadv[glyph_index]);


            if(xadv[glyph_index] > xmax )
                xmax = xadv[glyph_index];

            if(yadv[glyph_index] > ymax )
                ymax = yadv[glyph_index];

        }
        
        //printf("xmax = %d, ymax = %d\n", xmax, ymax);

        // now calculate the layout of bitmapped glyphs

        pFont->uMaxWidth = xmax;
        pFont->uHeight   = ymax;
        pFont->uMaxChar  = charEnd;
        pFont->uMinChar  = charStart;


        xmax = 0; ymax = 0; xp = yp = 0;
        for(i=0; i<face->num_glyphs; i++)
        {
            if(xadv[i] == 0 && yadv[i] == 0)  continue;  // unused slots
            if(xadv[i] + xp >= GFX_SURFACE_MAX_WIDTH)  // next row
            {
                // printf("next line\n");
                xp = 0;
                yp += pFont->uHeight;
            }
            t = xadv[i];
            xadv[i] = xp;   yadv[i] = yp;
            xp += (t&1) ? t+1 : t ;     // compensate for the even pixel alignment of YUV422 planes
            if(xp > xmax) xmax = xp;
        }

        ymax = yp + pFont->uHeight;

        // now xmax * ymax is the required bitmap
        for(c=charStart; c<=charEnd; c++)
        {
            pFont->uPosX[c-charStart] = xadv[cmap[c-charStart]];
            pFont->uPosY[c-charStart] = yadv[cmap[c-charStart]];
            // printf("%3d  : posx, posy = %4d, %4d\n", c, pFont->uPosX[c-charStart],  pFont->uPosY[c-charStart]);
        }

        // create surface as xmax * ymax

        // printf("xdim = %d, ydim = %d \n", xmax, ymax);
        
        pFont->hFont = gfx_create_surface(fdGfxDev, xmax, ymax, GFX_SURFACE_RAW8BPP, GFX_VDEV_NULL, 0);
        if(pFont->hFont < 0)
        {
            fprintf(stderr, "Failed to create 8 bit font surface !\n");
            free(cmap);
            free(pFont->uWidth);
            rtn =  -1;
            goto clean_up;
        }
        pFont->hMonoFont = gfx_create_surface(fdGfxDev, xmax, ymax, GFX_SURFACE_RAW1BPP, GFX_VDEV_NULL, 0);
        if(pFont->hMonoFont < 0)
        {
            fprintf(stderr, "Failed to create 1 bit font surface!\n");
            free(cmap);
            free(pFont->uWidth);
            rtn =  -1;
            goto clean_up;
        }
        pFont->hScratch = gfx_create_surface(fdGfxDev, pFont->uMaxWidth*2, pFont->uHeight, GFX_SURFACE_ARGB_8888, GFX_VDEV_NULL, 0xffffffff);
        if(pFont->hScratch < 0)
        {
            fprintf(stderr, "Failed to create font scratch surface !\n");
            gfx_destroy_surface(fdGfxDev, pFont->hMonoFont);
            gfx_destroy_surface(fdGfxDev, pFont->hFont);
            free(cmap);
            free(pFont->uWidth);
            rtn =  -1;
            goto clean_up;
        }

        if(gfx_lock_surface(fdGfxDev, pFont->hFont, &pFont->infoFont) < 0)
        {
            fprintf(stderr, "Failed to lock font surface !\n");
            gfx_destroy_surface(fdGfxDev, pFont->hScratch);
            gfx_destroy_surface(fdGfxDev, pFont->hMonoFont);
            gfx_destroy_surface(fdGfxDev, pFont->hFont);
            free(cmap);
            free(pFont->uWidth);
            rtn =  -1;
            goto clean_up;
        }

        if(gfx_lock_surface(fdGfxDev, pFont->hMonoFont, &pFont->infoMono) < 0)
        {
            fprintf(stderr, "Failed to lock font surface !\n");
            gfx_unlock_surface(fdGfxDev, pFont->hFont, &pFont->infoFont);
            gfx_destroy_surface(fdGfxDev, pFont->hScratch);
            gfx_destroy_surface(fdGfxDev, pFont->hMonoFont);
            gfx_destroy_surface(fdGfxDev, pFont->hFont);
            free(cmap);
            free(pFont->uWidth);
            rtn =  -1;
            goto clean_up;
        }
        
        for(i=0; i<face->num_glyphs; i++)
        {
            // load glyph
            if(xadv[i] == 0 && yadv[i] == 0)  continue;  // unused slots
            
            rtn = FT_Load_Glyph( face, i, FT_LOAD_DEFAULT );     
            if(rtn) continue;                                                            
            // extract glyph image                                         
            rtn = FT_Get_Glyph( face->glyph, &glyph );                  
                                                                       
            // convert to a bitmap (default render mode + destroy old)    
            if ( glyph->format != ft_glyph_format_bitmap )                
            {                                                             
                rtn = FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal,
                                        0, 1 );                       
                if ( rtn ) continue; // glyph unchanged                          
            }                                                         
                                                                   
            // access bitmap content by typecasting                
            glyph_bitmap = (FT_BitmapGlyph)glyph;                 
                                                               
            // do funny stuff with it, like blitting/drawing     
            {
                int x,y, real_x, real_y;
                unsigned char *pbmp;
                if(glyph_bitmap->left > 0)
                    real_x = xadv[i] +  glyph_bitmap->left; // + 4;
                else
                    real_x = xadv[i]; // + 4;
                
                real_y = yadv[i] + ((face->size->metrics.ascender+0x3f)>>6) - glyph_bitmap->top;
                pbmp = glyph_bitmap->bitmap.buffer;
         
                // printf("glyph %d,  realx = %d  realy = %d  bmp->left = %d bmp->top = %d row = %d  col = %d\n", i, real_x, real_y, glyph_bitmap->left, glyph_bitmap->top, glyph_bitmap->bitmap.rows, glyph_bitmap->bitmap.width);
                for(y=0; y<glyph_bitmap->bitmap.rows; y++)
                {
                    unsigned char *ppix = (unsigned char *)pFont->infoFont.plane[0].pPlane 
                                    + (y+real_y)*pFont->infoFont.plane[0].uBytePerLine + real_x; 
                    unsigned char *ppixb = (unsigned char *)pFont->infoMono.plane[0].pPlane 
                                    + (y+real_y)*pFont->infoMono.plane[0].uBytePerLine; 
                    for(x=0; x<glyph_bitmap->bitmap.width; x++)
                    {
                        //printf("%2x ", pbmp[x]);
                        ppix[x] = pbmp[x];
                        ppixb[(real_x+x)>>3] |= (pbmp[x]&0x80) >> ((real_x+x)&7);
                    }
                    //printf("\n");
                    pbmp +=glyph_bitmap->bitmap.pitch; 
                }
            }
            // discard glyph image (bitmap or not)
            FT_Done_Glyph( glyph );                      
        }
        // yehoo, we are here !
        rtn = 0;
        goto normal_rtn;
    }

clean_up:
    memset(pFont, 0, sizeof(GFX_FONT_INFO_T));
normal_rtn:
    FT_Done_Face(face);

clean_up1:
    FT_Done_FreeType(ft_library);
    return rtn;
}

int gfx_free_font(int fdGfxDev, GFX_FONT_INFO_T *pFont)
{
    if(!pFont) return 0;
    if(!pFont->uWidth) return 0;
    free(pFont->uWidth);
    gfx_unlock_surface(fdGfxDev, pFont->hMonoFont, &pFont->infoMono);
    gfx_unlock_surface(fdGfxDev, pFont->hFont, &pFont->infoFont);
    gfx_destroy_surface(fdGfxDev, pFont->hScratch);
    gfx_destroy_surface(fdGfxDev, pFont->hMonoFont);
    gfx_destroy_surface(fdGfxDev, pFont->hFont);
    memset(pFont, 0, sizeof(GFX_FONT_INFO_T));
    return 0;
}

/*****************************************************************************
** Function:    gfx_draw_font
**
** Purpose:     draw a single character of a font into a region buffer
**
** Returns:      the width of font: if successful
**               -1: if an error occurs
*****************************************************************************/

int __gfx_draw_font_mono(
       int fdGfxDev, 
       int hDes, 
       unsigned int uPlaneConfig,
       GFX_FONT_INFO_T *pFont, 
       unsigned int uChar, 
       long X,
       long Y,
       unsigned int uColor,
       unsigned int uBackColor,
       unsigned int uFlag
    )
{
    int rtn;
    if(uFlag & GFX_DRAW_FONT_BACKGROUND)
    {
        GFX_PALETTE_T pal[2];
        GFX_BITBLT_PARM_T bltparm;
        pal[0] = *((GFX_PALETTE_T *)&uBackColor);
        pal[1] = *((GFX_PALETTE_T *)&uColor);
        gfx_set_surface_palette(fdGfxDev, pFont->hMonoFont, 0, 2, pal);
        bltparm.hDesSurface = hDes;
        bltparm.uDesX = X;
        bltparm.uDesY = Y;
        bltparm.uWidth = pFont->uWidth[uChar];
        bltparm.uHeight = pFont->uHeight;
        bltparm.hSrcSurface = pFont->hMonoFont;
        bltparm.uSrcX = pFont->uPosX[uChar];
        bltparm.uSrcY = pFont->uPosY[uChar];
        bltparm.alphaSelect.storedAlphaSelect = 
            (uFlag & GFX_DRAW_FONT_WITHALPHA) ? GFX_BLEND_ALPHA_FROM_SOURCE : GFX_DEST_ALPHA_FROM_DESTINATION;
        bltparm.alphaSelect.globalAlphaValue = 255;
        bltparm.enableGammaCorrection = 0;
        rtn = ioctl(fdGfxDev, IOC_GFX_BITBLT, &bltparm);
    }
    else    // transparent background
    {
        if(!IS_GFX_SURFACE_CLUT(uPlaneConfig) && IS_OSD_SURFACE_YUV(uPlaneConfig)) // YUV direct color mode
        {
            // :-( we need to draw every dirty pixel
            int x, y, posx, wdx;
            unsigned char *ppix;
            GFX_FILLBLT_PARM_T   parm;
            parm.hDesSurface = hDes;
            parm.uHeight = 1;
            parm.uFillColor = uColor;
            posx = pFont->uPosX[uChar];
            wdx = pFont->uWidth[uChar];
            for(y=0; y < pFont->uHeight; y++)
            {
                int scanlth = 0, scanx = -1; 
                ppix = (unsigned char *)pFont->infoMono.plane[0].pPlane 
                                + (pFont->uPosY[uChar]+y)*pFont->infoMono.plane[0].uBytePerLine; 
                parm.uDesY = Y+y;
                for(x=0; x<wdx; x++)
                {
                    if(ppix[(posx+x)>>3] & (0x80 >> ((posx+x)&7)))
                    {
                        scanlth ++;
                        if(scanx < 0) scanx = x;
                    }
                    else if(scanx >= 0) // fill now
                    {
                        parm.uDesX = X + scanx;
                        parm.uWidth = scanlth;
                        if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm)) return -1;
                        scanx = -1;
                        scanlth = 0;
                    }
                }
                if(scanx >= 0) // fill now
                {
                        parm.uDesX = X + scanx;
                        parm.uWidth = scanlth;
                        if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm)) return -1;
                        scanx = -1;
                        scanlth = 0;
                }
            }
        }
        else    // we could use adv_fillblt to draw font
        {
            GFX_ADV_FILLBLT_PARM_T fparm;
            fparm.hDesSurface = hDes;
            fparm.uDesX = X;
            fparm.uDesY = Y;
            fparm.uWidth = pFont->uWidth[uChar];
            fparm.uHeight = pFont->uHeight;
            fparm.hMaskSurface = pFont->hMonoFont;
            fparm.uMaskX = pFont->uPosX[uChar];
            fparm.uMaskY = pFont->uPosY[uChar];
            fparm.ROP = GFX_ROP_DISABLE;
            fparm.transparencyEnable = 1;
            fparm.enablePixelBitMask = 0;
            fparm.uFillColor = uColor;
            rtn = ioctl(fdGfxDev, IOC_GFX_ADV_FILLBLT, &fparm);
        }
    }
    return rtn;
}


int __gfx_draw_font_antialias(
       int fdGfxDev, 
       int hDes, 
       unsigned int uPlaneConfig,
       GFX_FONT_INFO_T *pFont, 
       unsigned int uChar, 
       long X,
       long Y,
       unsigned int uColor,
       unsigned int uBackColor,
       unsigned int uFlag
    )
{
    int rtn;

    if(IS_GFX_SURFACE_CLUT(uPlaneConfig)
#ifdef G2D_AYCBCR
        || GET_GFX_SURFACE_DATA_TYPE(uPlaneConfig) == G2D_AYCBCR 
#endif
        )
        return __gfx_draw_font_mono(fdGfxDev, hDes, uPlaneConfig, pFont, uChar, X, Y, uColor, uBackColor, uFlag);

    // fill the foreground
    {
        GFX_FILLBLT_PARM_T  fparm;
        fparm.hDesSurface = pFont->hScratch;
        fparm.uDesX = pFont->uMaxWidth;
        fparm.uDesY = 0;
        fparm.uWidth = pFont->uWidth[uChar];
        fparm.uHeight = pFont->uHeight;
        fparm.uFillColor = uColor;
        if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &fparm)) return -1;
    }

    if(GET_GFX_SURFACE_DATA_TYPE(uPlaneConfig) == G2D_ARGB_8888)    // a special case we could use dest blend
    {
        // fill the background
        if(uFlag & GFX_DRAW_FONT_BACKGROUND)
        {
            GFX_FILLBLT_PARM_T  fparm;
            fparm.hDesSurface = hDes;
            fparm.uDesX = X;
            fparm.uDesY = Y;
            fparm.uWidth = pFont->uWidth[uChar];
            fparm.uHeight = pFont->uHeight;
            fparm.uFillColor = uBackColor;
            if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &fparm)) return -1;
        }
        
        {
            GFX_ADV_BLEND_PARM_T   parm;
        
            parm.hDesSurface = hDes;
            parm.uDesX = X;
            parm.uDesY = Y;
            parm.uWidth = pFont->uWidth[uChar];
            parm.uHeight = pFont->uHeight;
            parm.hSrcSurface = pFont->hScratch;
            parm.uSrcX = pFont->uMaxWidth;
            parm.uSrcY = 0;
            parm.hAlphaSurface = pFont->hFont;
            parm.uAlphaX = pFont->uPosX[uChar];
            parm.uAlphaY = pFont->uPosY[uChar];
            parm.blendSelect.storedAlphaSelect = (uFlag & GFX_DRAW_FONT_WITHALPHA) ? GFX_DEST_ALPHA_FROM_BLEND : GFX_DEST_ALPHA_FROM_DESTINATION;
            parm.blendSelect.blendInputSelect  = GFX_BLEND_ALPHA_FROM_PATTERN;
            rtn =  ioctl(fdGfxDev, IOC_GFX_ADV_BLEND, &parm);
        }
    }
    else // we blend at scratch pad and then blt back to dest
    {
        unsigned int uwidth = IS_GFX_SURFACE_YUV(uPlaneConfig) ? ((pFont->uWidth[uChar]+1)&0xfffffffe) : pFont->uWidth[uChar];
        // fill the background scratch
        if(uFlag & GFX_DRAW_FONT_BACKGROUND)
        {
            GFX_FILLBLT_PARM_T  fparm;
            fparm.hDesSurface = pFont->hScratch;
            fparm.uDesX = 0;
            fparm.uDesY = 0;
            fparm.uWidth = uwidth;
            fparm.uHeight = pFont->uHeight;
            fparm.uFillColor = uBackColor;
            if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &fparm)) return -1;
        }
        else    // get the dest to scratch backgrond
        {
            GFX_BITBLT_PARM_T  bparm;
            bparm.hDesSurface = pFont->hScratch;
            bparm.uDesX = 0;
            bparm.uDesY = 0;
            bparm.uWidth = uwidth;
            bparm.uHeight = pFont->uHeight;
            bparm.hSrcSurface = hDes;
            bparm.uSrcX = X;
            bparm.uSrcY = Y;

#if defined(G2D_RGB_565) && defined(G2D_YCBCR)  // pallas only
            if(GET_GFX_SURFACE_DATA_TYPE(uPlaneConfig) == G2D_RGB_565 ||
               GET_GFX_SURFACE_DATA_TYPE(uPlaneConfig) == G2D_YCBCR     ) 
            {
               bparm.alphaSelect.storedAlphaSelect = GFX_DEST_ALPHA_FROM_GIVEN;
               bparm.alphaSelect.globalAlphaValue = 255;
            }
            else   
#endif
               bparm.alphaSelect.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
            if(ioctl(fdGfxDev, IOC_GFX_BITBLT, &bparm)) return -1;
            

        }

        // blend on scratch pad
        {
            GFX_ADV_BLEND_PARM_T   parm;
        
            parm.hDesSurface = pFont->hScratch;
            parm.uDesX = 0;
            parm.uDesY = 0;
            parm.uWidth = pFont->uWidth[uChar];
            parm.uHeight = pFont->uHeight;
            parm.hSrcSurface = pFont->hScratch;
            parm.uSrcX = pFont->uMaxWidth;
            parm.uSrcY = 0;
            parm.hAlphaSurface = pFont->hFont;
            parm.uAlphaX = pFont->uPosX[uChar];
            parm.uAlphaY = pFont->uPosY[uChar];
            parm.blendSelect.storedAlphaSelect = (uFlag & GFX_DRAW_FONT_WITHALPHA) ? GFX_DEST_ALPHA_FROM_BLEND : GFX_DEST_ALPHA_FROM_DESTINATION;
            parm.blendSelect.blendInputSelect  = GFX_BLEND_ALPHA_FROM_PATTERN;
            if(ioctl(fdGfxDev, IOC_GFX_ADV_BLEND, &parm)) return -1;
        }

        // blt scratch back to dest
        {
            GFX_BITBLT_PARM_T  bparm;
            bparm.hDesSurface = hDes;
            bparm.uDesX = X;
            bparm.uDesY = Y;
            bparm.uWidth = uwidth;
            bparm.uHeight = pFont->uHeight;
            bparm.hSrcSurface = pFont->hScratch;
            bparm.uSrcX = 0;
            bparm.uSrcY = 0;
            bparm.alphaSelect.storedAlphaSelect = GFX_DEST_ALPHA_FROM_SOURCE;
            rtn = ioctl(fdGfxDev, IOC_GFX_BITBLT, &bparm);
        }
    }
    
    return  rtn;
}


// return font width in pixels
int gfx_draw_font(
   int fdGfxDev, 
   int hDes, 
   GFX_FONT_INFO_T *pFont, 
   unsigned int uChar, 
   long X,
   long Y,
   unsigned int uColor,
   unsigned int uBackColor,
   unsigned int uFlag
   )
{
    GFX_SURFACE_INFO_T sInfo;
    int rtn;

    if(!pFont || !pFont->uWidth)  return -1;
    if(uChar > pFont->uMaxChar || uChar < pFont->uMinChar)
        return -1;  // out of range
    uChar -= pFont->uMinChar;
    if(!pFont->uWidth[uChar]) // zero width or not encoded
    {
        return(0);
    }


    sInfo.hSurface = hDes;
    
    // try to get some information regarding the surface
    rtn = ioctl(fdGfxDev, IOC_GFX_GET_SURFACE_INFO, &sInfo);
    if( rtn < 0)
    {
        return rtn;
    }
    
    if((uFlag & GFX_DRAW_FONT_ANTIALIAS))
    {
        rtn = __gfx_draw_font_antialias(fdGfxDev, hDes, sInfo.uPlaneConfig, pFont, uChar, X, Y, uColor, uBackColor, uFlag);
    }
    else  // no anti-aliasing, we may choose to use either bit_blt, adv_fill_blt or draw pixel using fill_blt
    {
        rtn = __gfx_draw_font_mono(fdGfxDev, hDes, sInfo.uPlaneConfig, pFont, uChar, X, Y, uColor, uBackColor, uFlag);
    }
    
    if( rtn || ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT)) return -1;
    return pFont->uWidth[uChar];
}

/*****************************************************************************
** Function:    gfx_draw_ascii_string
**
** Purpose:     draw a string of ascii characters into a region buffer
**
** Returns:     width of string : if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_ascii_string(
   int fdGfxDev, 
   int hDes, 
   GFX_FONT_INFO_T *pFont, 
   const char *szStr,
   long X,
   long Y,
   unsigned int uColor,
   unsigned int uBackColor,
   unsigned int uFlag
   )
{
    GFX_SURFACE_INFO_T sInfo;
    int rtn;
    int xadv;
   
    if(!pFont || !pFont->uWidth || pFont->uMinChar >= 256 || !szStr)  return -1;


    sInfo.hSurface = hDes;
    
    // try to get some information regarding the surface
    rtn = ioctl(fdGfxDev, IOC_GFX_GET_SURFACE_INFO, &sInfo);
    if( rtn < 0)
    {
        return rtn;
    }

    xadv = 0;
    while(*szStr)
    {
        unsigned int uChar = (unsigned int)*szStr;
        if(uChar >= pFont->uMinChar || uChar <= pFont->uMaxChar) 
        {
            uChar -= pFont->uMinChar;
            if(pFont->uWidth[uChar]) // zero width or not encoded
            {
                if((uFlag & GFX_DRAW_FONT_ANTIALIAS))
                {
                    rtn = __gfx_draw_font_antialias(fdGfxDev, hDes, sInfo.uPlaneConfig, pFont, uChar, X+xadv, Y, uColor, uBackColor, uFlag);
                }
                else  // no anti-aliasing, we may choose to use either bit_blt, adv_fill_blt or draw pixel using fill_blt
                {
                    rtn = __gfx_draw_font_mono(fdGfxDev, hDes, sInfo.uPlaneConfig, pFont, uChar, X+xadv, Y, uColor, uBackColor, uFlag);
                }
                if(rtn) break;
                xadv += pFont->uWidth[uChar];
            }
        }
        szStr ++;
    }
    if(rtn || ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT)) return -1;
    
    return xadv;
}


/*****************************************************************************
** Function:    gfx_draw_ascii_string_rect
**
** Purpose:     draw a string of ascii characters into a rect of region buffer
**              with line wrap !!!
**
** Returns:     num of char written : if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_ascii_string_rect( int fdGfxDev, int hDes, GFX_FONT_INFO_T *pFont, const char *szStr,
   long X1,  long Y1, long X2, long Y2, int char_space, int line_space, unsigned int uColor,  unsigned int uBackColor,  unsigned int uFlag )
{
    GFX_SURFACE_INFO_T sInfo;
    int rtn, c;
    int xadv, yadv;
   
    if(!pFont || !pFont->uWidth || pFont->uMinChar >= 256 || !szStr)  return -1;

    /* return an error if part of the font is outside the region */
    if ((X1 > X2 ) ||
        (Y1 > Y2)   )
    {
        return(-1);
    }

    sInfo.hSurface = hDes;
    
    // try to get some information regarding the surface
    rtn = ioctl(fdGfxDev, IOC_GFX_GET_SURFACE_INFO, &sInfo);
    if( rtn < 0)
    {
        return rtn;
    }

    xadv = X1;
    yadv = Y1;
    c = 0;
    while(*szStr)
    {
        unsigned int uChar = (unsigned int)*szStr;
        if(uChar >= pFont->uMinChar || uChar <= pFont->uMaxChar) 
        {
            uChar -= pFont->uMinChar;
            if(pFont->uWidth[uChar]) // zero width or not encoded
            {
                if(xadv + pFont->uWidth[uChar] + char_space > X2)
                {
                    if(X1 + pFont->uWidth[uChar] + char_space > X2)
                        break;  // that's all
                    xadv = X1;
                    yadv += pFont->uHeight + line_space;
                }
                if(yadv + pFont->uHeight + line_space > Y2)
                    break;      // that's all

                if((uFlag & GFX_DRAW_FONT_ANTIALIAS))
                {
                    rtn = __gfx_draw_font_antialias(fdGfxDev, hDes, sInfo.uPlaneConfig, pFont, uChar, xadv, yadv, uColor, uBackColor, uFlag);
                }
                else  // no anti-aliasing, we may choose to use either bit_blt, adv_fill_blt or draw pixel using fill_blt
                {
                    rtn = __gfx_draw_font_mono(fdGfxDev, hDes, sInfo.uPlaneConfig, pFont, uChar, xadv, yadv, uColor, uBackColor, uFlag);
                }
                if(rtn) break;
                xadv += pFont->uWidth[uChar] + char_space;
                c++;
            }
        }
        szStr ++;
    }
    if(rtn || ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT)) return -1;
    
    return c;
}


/*****************************************************************************
** Function:    gfx_get_ascii_string_length
**
** Returns:      width of string in pixels: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_get_ascii_string_length(
   GFX_FONT_INFO_T *pFont, 
   const char *szStr
   )
{
    int rtn;
    if(!pFont || !pFont->uWidth || pFont->uMinChar >= 256 )  return -1;

    if(!szStr) return 0;
    rtn = 0;
    while(*szStr)
    {
        unsigned int uChar = (unsigned int)*szStr;
        if(uChar >= pFont->uMinChar || uChar <= pFont->uMaxChar) 
        {
            uChar -= pFont->uMinChar;
            rtn += (int)pFont->uWidth[uChar];
        }
        szStr ++;
    }
    return(rtn);
}


/*****************************************************************************
** Function:    gfx_get_ascii_string_height
**
** Purpose:     determine the height of a font in pixels
**
** Returns:     height of sting in pixels: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_get_ascii_string_height(
   GFX_FONT_INFO_T *pFont, 
   const char *szStr
   )
{
    if(!pFont || !pFont->uWidth || pFont->uMinChar >= 256)  return -1;
    return (int)pFont->uHeight;
}

