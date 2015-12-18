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
|       STB04XXX VXWORKS EVALUATION KIT SOFTWARE
|       (C) COPYRIGHT IBM CORPORATION 1999, 2001 
+-----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    Mark Detrick
| Component: 
| File:      g2d_draw
| Purpose:   2D graphics draw functions.
| Changes:
| Date:     Author Comment:
| -----     ------ --------
| 19-Mar-01  MSD   This is a total rewrite of the osd draw functions to
|                  support Pallas 2D graphics requirements.
| 19-Jul-01  MAL   Comment/spacing clean-up. Fixed repeated ycbcrFillBLT() arc in 
|                  g2d_drawCircle(). Added missing ycbcrFillBLT() calls for g2d_drawLine()
|                  g2d_drawHorizontalLine(), and g2d_drawPixel(). Removed internal
|                  function drawLine()-only g2d_drawPoly() used this, so it was changed
|                  to use g2d_drawLine(). Fixed coordinate limit check in g2d_drawLine().
|                  Added coordinate limit check to g2d_drawHorizontalLine().
| 02-Aug-01  MAL   Added draw_line() internal function to be used by g2d_drawPoly()
|                  and draw_rectangle() to be used by g2d_drawRectangle(). Added
|                  auto-update feature to g2d_drawLine() and g2d_drawPixel().
|                  Changed draw_line behavior to draw X1, Y1 as first pixel in line
|                  and X2, Y2 as extent of line. Optimized draw_line() to use a
|                  single bitBLT for vertical and horizontal lines. Removed clipping
|                  from g2d_drawRectangle(). Improved limit checking for g2d_drawPoly().
|                  Added pContext NULL pointer check to all functions. Fixed
|                  g2d_drawCircle() to always have a center pixel with a radius that
|                  does not include center pixel. Improved function description headers.
| 08-Aug-01  MAL   Changed all graphics library function names to be lower case
|                  separated by underscores.
| 06-Sep-01  MSD   Added support for 1 bit per pixel fixed width font.
| 10-Sep-01  MSD   Changed the order of the penColor and bkgColor in the calls in
|                  g2d_draw_font().
| 11 Sep-01  MAL   Fixed auto-update rectangle calculation in g2d_draw_circle().
| 05 Apr-02  YYD   Ported to Pallas Linux gfx driver
+----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include "gfxlib.h"

#define GFX_DRAW_TIMEOUT    30000   // 30 senconds

/*****************************************************************************
** Function:    g2d_draw_horizontal_line
**
** Purpose:     draw a horizontal line into a region buffer
**
** Input
** Parameters:  regionId: zero-based number to identify the region
**              bufferId: zero-based number to identify the region buffer
**              pContext: pointer to a struct that identifies drawing
**                 parameters, as follows:
**                 pContext->penColor: color of line
**              X1: x-coordinate of first and far left pixel of the line
**              X2: x-coordinate defining extent of the line (pixel not drawn)
**              Y: y-coordinate of line
**
** Output
** Parameters:  errorCode: indicates type of error when return value is -1
**
** Returns:      0: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_horizontal_line(
   int fdGfxDev, 
   int hDes, 
   long X1,
   long X2,
   long Y,
   unsigned int uColor 
   )
{
    GFX_FILLBLT_PARM_T   parm;
    
    if (X1 < 0 || X2 < 0 || Y < 0) return 0;
    
    parm.hDesSurface = hDes;
    parm.uDesX = X1 < X2 ? X1 : X2;
    parm.uDesY = Y;
    parm.uWidth = X1 < X2 ? X2-X1+1 : X1-X2+1;
    parm.uHeight = 1;
    parm.uFillColor = uColor;

    
    return ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) || ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT);

}


/*****************************************************************************
** Function:    g2d_draw_rectangle
**
** Purpose:     draw a rectangle into a region buffer
**
** Input
** Parameters:  regionId: zero-based number to identify the region
**              bufferId: zero-based number to identify the region buffer
**              fillFlags: indicates if the rectangle should be filled or
**                 not filled, as follows:
**                 FILL: rectangle is filled
**                 NOFILL: rectangle is not filled
**              pContext: pointer to a struct that identifies drawing
**                 parameters, as follows:
**                 pContext->penColor: color of the rectangle
**                 pContext->penWidth: width of the sides, if not filled
**              X1: x-coordinate of the upper left pixel of the rectangle
**              Y1: y-coordinate of the upper left pixel of the rectangle
**              X2: x-coordinate of the lower right extent of the rectangle
**                  (pixels not drawn)
**              Y2: y-coordinate of the lower right extent of the rectangle
**                  (pixels not drawn)
**
** Output
** Parameters:  errorCode: indicates type of error when return value is -1
**
** Returns:      0: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_rectangle(
   int fdGfxDev, 
   int hDes, 
   long X1,
   long Y1,
   long X2,
   long Y2, 
   unsigned int uColor,
   int fillMode 
   )
{
   long xMin, yMin, xMax, yMax;
   GFX_FILLBLT_PARM_T   parm;
   
   
   /* return an error if any part of the rectangle goes out of the region */
   if ((X1 < 0) ||
       (X2 < 0) ||
       (Y1 < 0) ||
       (Y2 < 0)   ){
      return(-1);
   }

   /* determine the top, bottom, left, and right of the rectangle */
   if (X1 < X2){
      xMin = X1;
      xMax = X2;
   }
   else{
      xMin = X2;
      xMax = X1;
   }
   if (Y1 < Y2){
      yMin = Y1;
      yMax = Y2;
   }
   else{
      yMin = Y2;
      yMax = Y1;
   }

    if(fillMode)
    {
        parm.hDesSurface = hDes;
        parm.uDesX = xMin ;
        parm.uDesY = yMin;
        parm.uWidth = xMax-xMin+1;
        parm.uHeight = yMax-yMin+1;
        parm.uFillColor = uColor;
        return ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm);
    }
    else
    {
        parm.hDesSurface = hDes;
        parm.uFillColor = uColor;
        parm.uDesX = xMin;
        parm.uDesY = yMin;
        parm.uWidth = xMax-xMin+1;
        parm.uHeight = 1;
        if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm)) return -1;
        parm.uDesY = yMax;
        if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm)) return -1;
        parm.uDesY = yMin;
        parm.uWidth = 1;
        parm.uHeight = yMax-yMin+1;
        if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm)) return -1;
        parm.uDesX = xMax;
        if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm)) return -1;
    }        

    return ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT);
}



/*****************************************************************************
** Function:    g2d_draw_polygon
**
** Purpose:     draw a polygon into a region buffer
**
** Input
** Parameters:  regionId: zero-based number to identify the region
**              bufferId: zero-based number to identify the region buffer
**              fillFlags: indicates if the polygon should be filled or
**                 not filled, as follows:
**                 FILL: polygon is filled
**                 NOFILL: polygon is not filled
**              pContext: pointer to a struct that identifies drawing
**                 parameters, as follows:
**                 pContext->penColor: color of the polygon
**              X: pointer to an array of x-coordinates of polygon vertices
**              Y: pointer to an array of y-coordinates of polygon vertices
**              numVertices: number of vertices in X and Y arrays
**
** Output
** Parameters:  errorCode: indicates type of error when return value is -1
**
** Returns:      0: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_polygon(
   int fdGfxDev, 
   int hDes, 
   long *X,
   long *Y,
   unsigned int numVertices,
   unsigned int uColor,
   int fillMode
   )
{
    unsigned long i;
    long slope, x, y, yMax, yMin, xMax, xMin;
    GFX_FILLBLT_PARM_T   parm;
    GFX_SURFACE_INFO_T sInfo;
    int rtn;
    
    sInfo.hSurface = hDes;
    
    // try to get some information regarding the surface
    rtn = ioctl(fdGfxDev, IOC_GFX_GET_SURFACE_INFO, &sInfo);
    if( rtn < 0)
    {
        return rtn;
    }

   yMax = Y[0];
   yMin = Y[0];
   xMax = X[0];
   xMin = X[0];

   for (i = 0; i < numVertices; i++)
   {
      if (Y[i] > yMax)
      {
         yMax = Y[i];
      }
      if (Y[i] < yMin)
      {
         yMin = Y[i];
      }
      if (X[i] > xMax)
      {
         xMax = X[i];
      }
      if (X[i] < xMin)
      {
         xMin = X[i];
      }
   }

    parm.hDesSurface = hDes;
    parm.uDesX = 0;
    parm.uDesY = 0;
    parm.uWidth = 0;
    parm.uHeight = 1;
    parm.uFillColor = uColor;

    if ((xMin < 0) || (xMax >= sInfo.plane[0].uWidth) || (yMin < 0) || (yMax >= sInfo.plane[0].uHeight)){
       return(-1);
    }
    
   /* If the polygon is to be filled, we need to determine the outer edges and  */
   /* then draw scanlines between the 2 outer edges                             */
   if (fillMode){
       unsigned short *Xside1, *Xside2;
       
       Xside1 = malloc(yMax*2*sizeof(unsigned short));
       if(!Xside1)  
            return -1;
       Xside2 = Xside1 + yMax;
      /* return an error if any part of the polygon falls outside the region */
      for (i = 0; i < numVertices - 1; i++){
         if (Y[i + 1] != Y[i]){
            slope = ((X[i + 1] - X[i]) << 16) / (Y[i + 1] - Y[i]);
            if (Y[i] < Y[i + 1]){
               /* going down */
               x = X[i] << 16;
               for (y = Y[i]; y < Y[i + 1]; y++){
                  Xside1[y] = x >> 16;
                  x += slope;
               }
            }
            else{
               /* going up */
               x = X[i + 1] << 16;
               for (y = Y[i + 1]; y < Y[i]; y++){
                  Xside2[y] = x >> 16;
                  x += slope;
               }
            }
         }
      }

      if (Y[0] != Y[numVertices - 1]){
         slope = ((X[0] - X[numVertices - 1]) << 16) / (Y[0] - Y[numVertices- 1]);
         if (Y[numVertices - 1] < Y[0]){
            /* going down */
            x = X[numVertices - 1] << 16;

            for (y = Y[numVertices - 1]; y < Y[0]; y++){
               Xside1[y] = x >> 16;
               x += slope;
            }
         }
         else{
            /* going up */
            x = X[0] << 16;

            for (y = Y[0]; y < Y[numVertices - 1]; y++){
               Xside2[y] = x >> 16;
               x += slope;
            }
         }
      }

      /* figure out which edge list is on the left and start drawing scanlines */
      if (Xside1[((yMin +  yMax) >> 1)] < Xside2[((yMin + yMax) >> 1)])
      {
         for (y = yMin; y < yMax; y++)
         {
            parm.uDesX = Xside1[y];
            parm.uDesY = y;
            parm.uWidth = Xside2[y] - Xside1[y];
            if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
            {
                fprintf(stderr, "gfx_draw_polygon: FillBLT failed for handle %d.\n", hDes);
                free(Xside1);
                return(-1);
            }	
         }
      }
      else
      {
         for (y = yMin; y < yMax; y++)
         {
            parm.uDesX = Xside2[y];
            parm.uDesY = y;
            parm.uWidth = Xside1[y] - Xside2[y];
            if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
            {
                fprintf(stderr, "gfx_draw_polygon: FillBLT failed for handle %d.\n", hDes);
                free(Xside1);
                return(-1);
            }	
         }
      }
      free(Xside1);
   }
   else { // not filled
      for  (i = 0; i < numVertices - 1; i++){
         /* non-horizonal lines */
         if(gfx_draw_line(fdGfxDev, hDes, X[i], Y[i], X[i + 1], Y[i + 1], uColor) != 0)
         {
            fprintf(stderr, "gfx_draw_polygon: draw_line failed for handle %d.\n", hDes);
            return(-1);
         }
      }
      if(gfx_draw_line(fdGfxDev, hDes, X[numVertices - 1], Y[numVertices - 1], X[0], Y[0], uColor) != 0)
      {
         fprintf(stderr, "gfx_draw_polygon: draw_line failed for handle %d.\n", hDes);
         return(-1);
      }
   }

   return ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT);
}


/*****************************************************************************
** Function:    g2d_draw_triangle
**
** Purpose:     draw a filled "upright" isocoles triangle into a region buffer
**              (triangle has horizontal bottom and equal-length left and
**              right sides)
**
** Input
** Parameters:  regionId: zero-based number to identify the region
**              bufferId: zero-based number to identify the region buffer
**              pContext: pointer to a struct that identifies drawing
**                 parameters, as follows:
**                 pContext->penColor: color of the triangle
**              X1: x-coordinate of the upper left extent of the triangle
**              Y1: y-coordinate of the upper left extent of the triangle
**              X2: x-coordinate of the lower right extent of the triangle
**                  (pixel not drawn)
**              Y2: y-coordinate of the lower right extent of the triangle
**                  (pixel not drawn)
**
** Output
** Parameters:  errorCode: indicates type of error when return value is -1
**
** Returns:      0: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_ur_triangle(
   int fdGfxDev, 
   int hDes, 
   long X1,
   long Y1,
   long X2, 
   long Y2,
   unsigned int uColor
   )
{
   long y, xMin, xMax, yMin, yMax, slope, runningSlope;
   GFX_FILLBLT_PARM_T   parm;

   /* return an error if any part of the triangle goes out of the region */
   if ((X1 < 0) ||
       (X2 < 0) ||
       (Y1 < 0) ||
       (Y2 < 0)   )
   {
      return(-1);
   }

   /* return if the triangle width or height is 0 */
   if (X1 == X2){
      return(0);
   }
   else if (Y1 == Y2){
      return(0);
   }
   /* determine the min/max of the x and y coordinates */
   if (X1 < X2){
      xMin = X1;
      xMax = X2;
   }
   else{
      xMin = X2;
      xMax = X1;
   }

   if (Y1 < Y2){
      yMin = Y1;
      yMax = Y2;
   }
   else{
      yMin = Y2;
      yMax = Y1;
   }
   /* figure out the slope of the left and right triangle edges */
   slope = ((xMax - xMin) << 15) / (yMax - yMin);
   runningSlope = 0;

    parm.hDesSurface = hDes;
    parm.uDesX = 0;
    parm.uDesY = 0;
    parm.uWidth = 0;
    parm.uHeight = 1;
    parm.uFillColor = uColor;

   /* draw each of the scanlines */
   for (y = yMin; y < yMax; y++){
      X1 = ((xMax + xMin) >> 1) - (runningSlope >> 16);
      X2 = ((xMax + xMin) >> 1) + (runningSlope >> 16);
      if(X2 < xMax)
      {
         parm.uDesX = X1;
         parm.uDesY = y;
         parm.uWidth = X2 - X1;
         if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
         {
               fprintf(stderr, "gfx_draw_triangle: fillBLT failed for handle %d.\n", hDes);
               return(-1);
         }
      }	
      runningSlope += slope;
   }

   return ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT);
}


/*****************************************************************************
** Function:    g2d_draw_circle
**
** Purpose:     draw a circle into a region buffer
**
** Input
** Parameters:  regionId: zero-based number to identify the region
**              bufferId: zero-based number to identify the region buffer
**              fillFlags: indicates if the circle should be filled or
**                 not filled, as follows:
**                 FILL: circle is filled
**                 NOFILL: circle is not filled
**              pContext: pointer to a struct that identifies drawing
**                 parameters, as follows:
**                 pContext->penColor: color of the circle
**              X: x-coordinate of center pixel
**              Y: y-coordinate of center pixel
**              radius: radius in pixels (not including center pixel)
**
** Output
** Parameters:  errorCode: indicates type of error when return value is -1
**
** Returns:      0: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_circle(
   int fdGfxDev, 
   int hDes, 
   long X, 
   long Y,
   long radius,
   unsigned int uColor,
   int fillMode
   )
{
   long x, y, D, length0, length1;
   GFX_FILLBLT_PARM_T   parm;

   /* return an error if any part of the circle falls outside the region */
   if (radius < 0 /* || (X - radius) < 0 || (Y - radius) < 0*/ )
   {
      return(-1);
   }
   D = 3 - (radius << 1);
   x = radius;
   y = 0;
 	length0 = 1;
 	length1 = 1;

    parm.hDesSurface = hDes;
    parm.uDesX = 0;
    parm.uDesY = 0;
    parm.uWidth = 0;
    parm.uHeight = 1;
    parm.uFillColor = uColor;

   while (y <= x)
   {
      if (fillMode)
      {
         if(x>0){
            length0=(X+x)-(X-x)+1;
            length1=(X+y)-(X-y)+1;
         }
         else{
            length0 = 1;
            length1 = 1;
         }
      }

      parm.uDesX = X-x;
      parm.uDesY = Y-y;
      parm.uWidth = length0;
      
      if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
      {
	         fprintf(stderr, "gfx_draw_circle: fillBLT 0 failed\n\r");
            return(-1);
      }				       
      parm.uDesY = Y+y;
      if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
      {
	         fprintf(stderr, "gfx_draw_circle: fillBLT 1 failed\n\r");
            return(-1);
      }				       

      if (!fillMode)
      {
          parm.uDesX = X+x;
          parm.uDesY = Y-y;
          parm.uWidth = 1;
          if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
          {
               fprintf(stderr, "gfx_draw_circle: fillBLT 2 failed\n\r");
               return(-1);
          }
          parm.uDesY = Y+y;
          if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
          {
               fprintf(stderr, "gfx_draw_circle: fillBLT 3 failed\n\r");
               return(-1);
          }
      }
      if(y!=x)
      {	       
          parm.uDesX = X-y;
          parm.uDesY = Y-x;
          parm.uWidth = length1;
          if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
          {
	           fprintf(stderr, "gfx_draw_circle: fillBLT 4 failed\n\r");
               return(-1);
          }				       
          parm.uDesY = Y+x;
          if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
          {
               fprintf(stderr, "gfx_draw_circle: fillBLT 5 failed\n\r");
               return(-1);
          }
          if(!fillMode)
          {
              parm.uDesX = X+y;
              parm.uDesY = Y-x;
              parm.uWidth = 1;
              if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
              {
                  fprintf(stderr, "gfx_draw_circle: fillBLT 6 failed\n\r");
                  return(-1);
              }
              parm.uDesY = Y+x;
              if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm) != 0)
              {
                  fprintf(stderr, "gfx_draw_circle: fillBLT 7 failed\n\r");
                  return(-1);
              }
          }
      }
      if (D < 0){
         /* update the decision variable */
         D = D + (y << 2) + 6;
      }
      else{
         /* update the decision variable */
         D = D + ((y - x) << 2) + 10;
         x--;
      }
      y++;
   }

   return ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT);
}


/*****************************************************************************
** Function:    g2d_draw_line
**
** Purpose:     draw a line into a region buffer
**
** Input
** Parameters:  regionId: zero-based number to identify the region
**              bufferId: zero-based number to identify the region buffer
**              pContext: pointer to a struct that identifies drawing
**                 parameters, as follows:
**                 pContext->penColor: color of the line
**              X1: x-coordinate of the first pixel of the line
**              Y1: y-coordinate of the first pixel of the line
**              X2: x-coordinate of the extent of the line (pixel not drawn)
**              Y2: y-coordinate of the extent of the line (pixel not drawn)
**
** Output
** Parameters:  errorCode: indicates type of error when return value is -1
**
** Returns:      0: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_line(
   int fdGfxDev, 
   int hDes, 
   long X1, 
   long Y1, 
   long X2,
   long Y2,
   unsigned int uColor
   )
{
   long xMin, yMin, xMax, yMax, xDelta, yDelta, x, y, D, xInc, yInc;

   GFX_SURFACE_INFO_T sInfo;
   GFX_FILLBLT_PARM_T   parm;


   int BltWidth=1;
   int BltHeight=1;
   int done=0, rtn;


    sInfo.hSurface = hDes;
    
    // try to get some information regarding the surface
    rtn = ioctl(fdGfxDev, IOC_GFX_GET_SURFACE_INFO, &sInfo);
    if( rtn < 0)
    {
        return rtn;
    }

    parm.hDesSurface = hDes;
    parm.uFillColor = uColor;
    
   /* determine the min and max x and y values */
   if (X1 < X2){
      xMin = X1;
      xMax = X2;
   }
   else{
      xMin = X2;
      xMax = X1;
   }
   if (Y1 < Y2){
      yMin = Y1;
      yMax = Y2;
   }
   else{
      yMin = Y2;
      yMax = Y1;
   }
   x = X1;
   if (X1 < X2){
      xInc = 1;
   }
   else if (X1 > X2){
      xInc = -1;
   }
   else{
      xInc = 0;
   }
   y = Y1;
   if (Y1 < Y2){
   yInc = 1;
   }
   else if (Y1 > Y2){
      yInc = -1;
   }
   else{
      yInc = 0;
   }
   xDelta = xMax - xMin;
   yDelta = yMax - yMin;

   if (xDelta > yDelta){    
      D = (yDelta << 1);
      for(x = X1; (x != X2) && (done == 0); x+=xInc){
         /* ensure "real pixel" y-coordinate stays within the region */
         if((y >= 0) && (y < sInfo.plane[0].uHeight))
         {
            if(yInc == 0)
            {
               BltWidth=xMax-xMin;
               BltHeight=1;
               done=1;
               x=xMin;
               if(X2==xMin)
               {
                  x++;
               }
            }
            parm.uDesX = x;
            parm.uDesY = y;
            parm.uWidth = BltWidth;
            parm.uHeight = BltHeight;
            if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm))
            {
                return -1;
            }
         }
         if (D < 0){
            D += (yDelta << 1);
         }
         else{
            D += ((yDelta - xDelta) << 1);
            y += yInc;
         }
      }
   }
   else
   {
      D = (xDelta << 1);
      for(y = Y1; (y != Y2) && (done == 0); y+=yInc)
      {
         /* ensure "real pixel" x-coordinate stays within the region */
         if((x >= 0) && (x < sInfo.plane[0].uWidth))
         {
            /* if a vertical line, draw it using a single blit */
            if(xInc == 0)
            {
               BltWidth=1;
               BltHeight=yMax-yMin;
               done=1;
               y=yMin;
               if(Y2==yMin)
               {
                  y++;
               }
            }
            parm.uDesX = x;
            parm.uDesY = y;
            parm.uWidth = BltWidth;
            parm.uHeight = BltHeight;
            if(ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm))
            {
                return -1;
            }
         }
         if (D < 0){
            D += (xDelta << 1);
         }
         else{
            D += ((xDelta - yDelta) << 1);
            x += xInc;
         }
      }
   }

   return ioctl(fdGfxDev, IOC_GFX_WAIT_FOR_COMPLETE, GFX_DRAW_TIMEOUT);

}


/*****************************************************************************
** Function:    g2d_draw_pixel
**
** Purpose:     draw a pixel into a region buffer
**
** Input
** Parameters:  regionId: zero-based number to identify the region
**              bufferId: zero-based number to identify the region buffer
**              pContext: pointer to a struct that identifies drawing
**                 parameters, as follows:
**                 pContext->penColor: color of the pixel
**              X: x-coordinate of the pixel
**              Y: y-coordinate of the pixel
**
** Output
** Parameters:  errorCode: indicates type of error when return value is -1
**
** Returns:      0: if successful
**              -1: if an error occurs
*****************************************************************************/
int gfx_draw_pixel(
   int fdGfxDev, 
   int hDes, 
   long X, 
   long Y,
   unsigned int uColor
   )
{
   GFX_FILLBLT_PARM_T   parm;

   /* return an error if the pixel is out of the region */
   if ((X < 0) || (Y < 0))
   {
      return(-1);
   }

    parm.hDesSurface = hDes;
    parm.uDesX = X;
    parm.uDesY = Y;
    parm.uWidth = 1;
    parm.uHeight = 1;
    parm.uFillColor = uColor;
    return ioctl(fdGfxDev, IOC_GFX_FILLBLT, &parm);
}

