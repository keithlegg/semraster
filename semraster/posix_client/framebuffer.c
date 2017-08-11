#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <cmath>

#include "framebuffer.h"   
#include "point_op.h"  

#include <string.h> //for memcopy


//R-G-B Clamp dark value - used in determining what the 8 bit cutoff is for "dark"
short clampRGB = 112; 

/*****************************/

void abort_(const char * s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}


/*****************************/

//10Bit SEM binary dump format 

SEM_Type* create_Sem_Buffer(int w, int h){
    SEM_Type *pxl;
    pxl =  ( SEM_Type *) malloc( (w*h) * sizeof( SEM_Type)  );
    return pxl; 
}

/*****************************/
/*  this allocates memory for a 2D (linear array indexed by " (Y*width)+X " 
    BWI - Black White image - 1 bit  
*/

BWI_Type* createBuffer1(int w, int h){
    BWI_Type *pxl;
    pxl =  ( BWI_Type *) malloc( (w*h) * sizeof( BWI_Type)  );
    return pxl; 
}

/*****************************/
/*  this allocates memory for a 2D (linear array indexed by " (Y*width)+X " 
    GSI = Gray Scale Image - 8 bit  
*/

GSI_Type* createBuffer8(int w, int h){
    GSI_Type *pxl;
    pxl =  ( GSI_Type *) malloc( (w*h) * sizeof( GSI_Type)  );
    return pxl; 
}

/*****************************/
/*  this allocates memory for a 2D (linear array indexed by " (Y*width)+X " 
    RGBA = Red Green Blue Alpha - 32 bit  
*/

RGBAType* createBuffer32(int w, int h){
    RGBAType *pxl;
    pxl =  ( RGBAType *) malloc( (w*h) * sizeof( RGBAType)  );
    return pxl; 
}

/*****************************/
/*  this allocates memory for a 2D (linear array indexed by " (Y*width)+X " 
    RGB = Red Green Blue - 24 bit 
*/

RGBType* createBuffer24(int w, int h){
    RGBType *pxl;
    pxl =  ( RGBType *) malloc( (w*h) * sizeof( RGBType)  );
    return pxl; 
}

/*****************************/


RGBType newRgb(int r, int g, int b){
    RGBType color;

    color.r = r;
    color.g = g;
    color.b = b;

    return color;
}

RGBAType newRgba(int r, int g, int b, int a){
    RGBAType color;

    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;

    return color;
}



/*****************************/
RGBType* copyBuffer24( RGBAType *pixels, int w, int h )
{

   RGBAType *pixItr1 = 0;  
   RGBType  *pixItr2 = 0;    

   RGBType *output = createBuffer24(w,h);

   for (int y = 0; y < h; y++)
   {     
       for (int x = 0; x < w; x++)
       {  
           pixItr1 = &( pixels  [(y*w)+x] );
           pixItr2 = &( output  [(y*w)+x] );

           pixItr2->r = pixItr1->r;       
           pixItr2->g = pixItr1->g;  
           pixItr2->b = pixItr1->b;  
       }
   }
  return output;

}

/*****************************/

RGBType* copyBuffer24( RGBType *pixels, int w, int h )
{

   RGBType *pixItr1 = 0;  
   RGBType *pixItr2 = 0;    

   RGBType *output = createBuffer24(w,h);

   for (int y = 0; y < h; y++)
   {     
       for (int x = 0; x < w; x++)
       {  
           pixItr1 = &( pixels  [(y*w) + x] );
           pixItr2 = &( output  [(y*w) + x] );

           pixItr2->r = pixItr1->r;       
           pixItr2->g = pixItr1->g;  
           pixItr2->b = pixItr1->b;  

       }
   }
  return output;

}


RGBAType* copyBuffer32( RGBAType *pixels, int w, int h )
{

   RGBAType  *pixItr1 = 0;  
   RGBAType *pixItr2 = 0;    

   RGBAType *output = createBuffer32(w,h);

   for (int y = 0; y < h; y++)
   {     
       for (int x = 0; x < w; x++)
       {  
           pixItr1 = &( pixels  [(y*w)+x] );
           pixItr2 = &( output  [(y*w)+x] );

           pixItr2->r = pixItr1->r;       
           pixItr2->g = pixItr1->g;  
           pixItr2->b = pixItr1->b;  
           pixItr2->a = pixItr1->a;  

       }
   }
  return output;

}

RGBAType* copyBuffer32( RGBType *pixels, int w, int h )
{

   RGBType *pixItr1 = 0;  
   RGBAType *pixItr2 = 0;    

   RGBAType *output = createBuffer32(w,h);

   for (int y = 0; y < h; y++)
   {     
       for (int x = 0; x < w; x++)
       {  
           pixItr1 = &( pixels  [(y*w) + x] );
           pixItr2 = &( output  [(y*w) + x] );

           pixItr2->r = pixItr1->r;       
           pixItr2->g = pixItr1->g;  
           pixItr2->b = pixItr1->b;  
           pixItr2->a = 255; 
       }
   }
  return output;

}


/*****************************/

RGBAType* blitBuffer32( RGBAType *pixels, int* w, int* h, int startx, int starty,  
                        int endx, int endy )
{

   RGBAType *pixItr1 = 0;  
   RGBAType *pixItr2 = 0;    

   int new_w = (endx-startx);
   int new_h = (endy-starty);

   RGBAType *output = createBuffer32( new_w, new_h );

   int cnt_x = 0;
   int cnt_y = 0;

   for (int y = starty; y < endy; y++)
   {     
       cnt_x = 0;
       for (int x = startx; x < endx; x++)
       {  
           pixItr1 = &( pixels [(y* *w) + x] );
           pixItr2 = &( output [(cnt_y* new_w) + cnt_x] );
           
           pixItr2->r = pixItr1->r;       
           pixItr2->g = pixItr1->g;  
           pixItr2->b = pixItr1->b;  
           pixItr2->a = pixItr1->a; 
           
           cnt_x++;
       }
       cnt_y++;
   }

   *h = new_h;
   *w = new_w;

  return output;
}

/*****************************/
/*not used yet - this is for experimenting*/
RGBAType* copyBufferEveryOther32( RGBAType *pixels, int* w, int* h, int step_size )
{

   RGBAType *pixItr1 = 0;  
   RGBAType *pixItr2 = 0;    

   int new_w = *w/step_size;
   int new_h = *h/step_size;
   
   //printf("debug new size is %i %i \n", new_w, new_h );

   //RGBAType *output = createBuffer32(w,h);
   RGBAType *output = createBuffer32( new_w, new_h );

   int cnt_x = 0;
   int cnt_y = 0;

   // #pragma omp parallel for // ???
   for (int y = 0; y < *h; y=y+step_size)
   {     
       cnt_x = 0;
       for (int x = 0; x < *w; x=x+step_size)
       {  
           pixItr1 = &( pixels  [(y**w) + x] );
           
           //pixItr2 = &( output  [(cnt_y*w)+x/step_size] );
           pixItr2 = &( output  [(cnt_y*new_w) + cnt_x ] );

           pixItr2->r = pixItr1->r;       
           pixItr2->g = pixItr1->g;  
           pixItr2->b = pixItr1->b;  
           pixItr2->a = pixItr1->a;  

           cnt_x++;
       }
       cnt_y++;
   }

   *w = new_w;
   *h = new_h;

   return output;

}

/*****************************/

/*

dest_pixel [x,y] = src_pixel [x * x_scale_factor, y * y_scale_factor]
   where x/y_scale_factor is
src_size / dest_size

*/

//got this from here:
//http://www.compuphase.com/graphic/scale.htm

void ScaleLine(RGBAType *Target, RGBAType *Source, int SrcWidth, int TgtWidth)
{
  int NumPixels = TgtWidth;
  int stepPart   = SrcWidth / TgtWidth;
  int FractPart = SrcWidth % TgtWidth;
  int E = 0;

  while (NumPixels-- > 0) {
    
    *Target++ = *Source;
    Source += stepPart;

      
    E += FractPart;
    if (E >= TgtWidth) {
      E -= TgtWidth;
      Source++;
    } 
     
  }  
}


void ScaleRect(RGBAType *Target, RGBAType *Source, int SrcWidth, int SrcHeight,
               int TgtWidth, int TgtHeight)
{
  int NumPixels = TgtHeight;
  int stepHeight   = (SrcHeight / TgtHeight) * SrcWidth;
  int FractPart = SrcHeight % TgtHeight;
  int E = 0;
  RGBAType *PrevSource = NULL;

  while (NumPixels-- > 0) {
    if (Source == PrevSource) {
      memcpy(Target, Target-TgtWidth, TgtWidth*sizeof(*Target));
    } else {
      ScaleLine(Target, Source, SrcWidth, TgtWidth);
      PrevSource = Source;
    }//if 
    
    Target += TgtWidth;
    Source += stepHeight;
     
    E += FractPart;
    if (E >= TgtHeight) {
      E -= TgtHeight;
      Source += SrcWidth;
    }//if
    
  }//while
}

/*****************************/
//fill RGB buffer with a solid color - from RGBType

void fillbuffer24(RGBType *pixBuffer, int width, int height, RGBType *color)
{
    int xa = 0;
    int ya = 0;

    RGBType* pixItr = 0;

    for (ya=0;ya<height;ya++)
    {
        for (xa=0;xa<width;xa++)
        {
            pixItr = &( pixBuffer  [(ya*width) + xa] );
            
            pixItr->r = color->r;
            pixItr->g = color->g;
            pixItr->b = color->b;

        }
    }


}

/*****************************/
void fillbuffer32(RGBAType *pixBuffer, int width, int height, RGBType *color)
{
    int xa = 0;
    int ya = 0;

    RGBAType* pixItr = 0;

    for (ya=0;ya<height;ya++)
    {
        for (xa=0;xa<width;xa++)
        {
            pixItr = &( pixBuffer  [(ya*width) + xa] );
            
            pixItr->r = color->r;
            pixItr->g = color->g;
            pixItr->b = color->b;
            pixItr->a = 255;

        }
    }


}

/*****************************/
//fill RGB buffer with a solid color - from 3 integers 

void fillbuffer24(RGBType *pixBuffer, int width, int height, int rval, int gval, int bval)
{
    int xa = 0;
    int ya = 0;

    RGBType* pixItr = 0;

    for (ya=0;ya<height;ya++)
    {
        for (xa=0;xa<width;xa++)
        {
            pixItr = &( pixBuffer  [(ya*width) + xa] );
            pixItr->r = rval;
            pixItr->r = gval;
            pixItr->r = bval;

        }
    }
}

/*****************************/
// clamping functions to determine a "dark" pixel 

short scanner_darkly(RGBAType* pixel32){
   if (pixel32->r <clampRGB || pixel32->g <clampRGB || pixel32->b <clampRGB ){
     return 1;
   }
   return 0;
}

short scanner_darkly(RGBType* pixel24){
   if (pixel24->r <clampRGB || pixel24->g <clampRGB || pixel24->b <clampRGB ){
     return 1;
   }
   return 0;
}

short scanner_darkly(int *pix){
   if (pix[0]<clampRGB || pix[1]<clampRGB || pix[2]<clampRGB ){
     return 1;
   }
   return 0;
}

/*****************************/

void draw_point ( RGBAType *fb_image, int imagewidth, int pt[2], int color[3]  ){
    RGBAType* ptr = &(fb_image[(pt[1]*imagewidth)+pt[0]]);
    ptr->r= color[0];ptr->g=color[1];ptr->b=color[2];
}

void draw_point ( RGBAType *fb_image, int imagewidth, int xcoord, int ycoord, int color[3]  ){
    RGBAType* ptr = &(fb_image[(ycoord*imagewidth)+xcoord]);
    ptr->r= color[0];ptr->g=color[1];ptr->b=color[2];
}

void draw_point ( RGBAType *fb_image, int imagewidth, int xcoord, int ycoord, RGBType *color  ){
    RGBAType* ptr = &(fb_image[(ycoord*imagewidth)+xcoord]);
    ptr->r= color->r;ptr->g=color->g;ptr->b=color->b;ptr->a=255;
}

void draw_point ( RGBType *fb_image, int imagewidth, int xcoord, int ycoord, int color[3]  ){
    RGBType* ptr = &(fb_image[(ycoord*imagewidth)+xcoord]);
    ptr->r= color[0];ptr->g=color[1];ptr->b=color[2];
}

void draw_point ( RGBType *fb_image, int imagewidth, int xcoord, int ycoord, RGBType *color  ){
    RGBType* ptr = &(fb_image[(ycoord*imagewidth)+xcoord]);
    ptr->r= color->r;ptr->g=color->g;ptr->b=color->b;
}

//UNTESTED - add point width (need imageheight to do it right)
void draw_point ( RGBType *fb_image, int imagewidth, int xcoord, int ycoord, int dia, RGBType *color  ){
    for (int i=0;i<dia;i++)
    {
        if (xcoord+i<imagewidth){
            RGBType* ptr = &(fb_image[(ycoord*imagewidth)+(xcoord+i)]);
            ptr->r= color->r;ptr->g=color->g;ptr->b=color->b;
        }

        if (xcoord-i>0){
            RGBType* ptr = &(fb_image[(ycoord*imagewidth)+(xcoord-i)]);
            ptr->r= color->r;ptr->g=color->g;ptr->b=color->b;
        }
        
        if (ycoord-1>0){
            RGBType* ptr = &(fb_image[((ycoord-i)*imagewidth)+xcoord]);
            ptr->r= color->r;ptr->g=color->g;ptr->b=color->b;
        }

    }
}


/*****************************/

void threshold (RGBAType *pixbuffer, int imagewidth, int imageheight, int threshval) 
/*
    32 bit threshold
*/
{
    RGBAType* pix = 0;

    for(int i=0; i<imageheight; i++)
    {
        for(int j=0; j<imagewidth; j++) 
        {
            pix = &( pixbuffer [(i*imagewidth)+j]);
            
            //thresholded output
            if (pix->r<threshval || pix->g<threshval || pix->b<threshval )
            {
                  pix->r =  0;
                  pix->g =  0;
                  pix->b =  0;
                  pix->a =  255;   
            }else{
                  pix->r =  255;
                  pix->g =  255;
                  pix->b =  255;
                  pix->a =  255;  
            }
        }//imagewidth
     }//imageheight
}

/*****************************/

void threshold (RGBType *pixbuffer, int imagewidth, int imageheight, int threshval) 
/*
    24 bit threshold
*/
{
    RGBType* pix = 0;

    for(int i=0; i<imageheight; i++)
    {
        for(int j=0; j<imagewidth; j++) 
        {
            pix = &( pixbuffer [(i*imagewidth)+j]);
            
            //thresholded output
            if (pix->r<threshval || pix->g<threshval || pix->b<threshval )
            {
                  pix->r =  0;
                  pix->g =  0;
                  pix->b =  0;
            }else{
                  pix->r =  255;
                  pix->g =  255;
                  pix->b =  255;
            }
        }//imagewidth
     }//imageheight
}

/*****************************/
// guassian blur funciton with optional threshold 
void gaussBlur (RGBAType *pixbuffer, RGBAType *pix2buffer, int imagewidth, int imageheight, int r, bool do_threshold, int threshval) 
{
    int w = imagewidth;
    int h = imageheight;

    RGBAType* im1_pix = 0;
    RGBAType* im2_pix = 0;

    int rs = (int)ceil(r * .5); //significant radius
    for(int i=0; i<h; i++)
    {

        for(int j=0; j<w; j++) 
        {

            int rval = 0;
            int gval = 0;
            int bval = 0; 
            int x = 0;
            int y = 0;

            int wsum = 0; 

            //fake "convolution kernel" iterate around the pixel at radius R 
            for(int iy = i-rs; iy<(i+rs+1); iy++)
            {
                for(int ix = j-rs; ix<(j+rs+1); ix++) 
                {
                    x = min(w-1, max(0, ix)); //clamp the extents/corner x
                    y = min(h-1, max(0, iy)); //clamp the extents/corner y

                    //long dsq = (ix-j)*(ix-j)+(iy-i)*(iy-i);
                    //int wght = exp( -dsq / (2*r*r) ) / (int)(PI*2*r*r);
                    
                    int wght = 1; //full weighted average
                    
                    wsum += wght;
  
                    im1_pix = &( pixbuffer [(y*imagewidth) + x]);
                    rval += im1_pix->r;
                    gval += im1_pix->g;
                    bval += im1_pix->b; 
                    //dont care about alpha - set to 255 

                }//kernel X
           }//kernel Y

           im2_pix = &( pix2buffer [(i*imagewidth) + j] );

           //paste the color to the output framebuffer
           if (!do_threshold){

               //printf( "rval %i gval %i bval%i \n", (rval/wsum), (gval/wsum), (bval/wsum) ) ;

               im2_pix->r = (int)(rval/wsum);
               im2_pix->g = (int)(gval/wsum);
               im2_pix->b = (int)(bval/wsum);
               im2_pix->a = 255;
           
           }

           if (do_threshold){
               //thresholded output
               if ((int)(rval/wsum)<threshval || (int)(gval/wsum)<threshval || (int)(bval/wsum)<threshval )
               {
                  im2_pix->r =  0;
                  im2_pix->g =  0;
                  im2_pix->b =  0;
                  im2_pix->a =  255;   
               }else{
                  im2_pix->r =  255;
                  im2_pix->g =  255;
                  im2_pix->b =  255;
                  im2_pix->a =  255;  
               }
           }
        }//width
     }//height
}

/*****************************/

void draw_square( RGBAType *row_pt, int width, int tl[2], int br[2], int color[3] )
{
  int plot_x = 0;
  int plot_y = 0;
  int px     = 0;

  int tl_x = tl[0];
  int tl_y = tl[1];
  int br_x = br[0];
  int br_y = br[1];
  
  for ( px =tl_x; px <br_x; px++){ draw_point(row_pt, width, px, br_y, color); }
  for ( px =tl_x; px <br_x; px++){ draw_point(row_pt, width, px, tl_y, color); }
  for ( px =tl_y; px <=br_y; px++){ draw_point(row_pt, width, br_x, px, color); }
  for ( px =tl_y; px <=br_y; px++){ draw_point(row_pt, width, tl_x, px, color); }
}

/*****************************/

void draw_fill_square( RGBAType *row_pt, int width, int x_orig, int y_orig, int dia, int color[3])
{
   int tl[2] = {0};
   int br[2] = {0};

   for (int a=0;a<dia;a++){

       tl[0] = x_orig-a;
       tl[1] = y_orig-a;
       
       br[0] = x_orig+a;
       br[1] = y_orig+a;

       draw_square( row_pt, width, tl, br, color );

   }

}


/*****************************/
void draw_line( RGBAType *fb_image, int imagewidth, int x1, int y1, int x2, int y2, RGBType *color) 
{
    int delta_x(x2 - x1);
    // if x1 == x2, then it does not matter what we set here
    signed char const ix((delta_x > 0) - (delta_x < 0));
    delta_x = abs(delta_x) << 1;
         int delta_y(y2 - y1);
    // if y1 == y2, then it does not matter what we set here
    signed char const iy((delta_y > 0) - (delta_y < 0));
    delta_y = abs(delta_y) << 1;
    draw_point(fb_image, imagewidth, x1, y1, color );
    
    if (delta_x >= delta_y)
    {
        // error may go below zero
        int error(delta_y - (delta_x >> 1));
        while (x1 != x2)
        {
            if ((error >= 0) && (error || (ix > 0)))
            {
                error -= delta_x;
                y1 += iy;
            }
            // else do nothing
            error += delta_y;
            x1 += ix;
            draw_point(fb_image, imagewidth, x1, y1, color);
        }
    }
    else
    {
        // error may go below zero
        int error(delta_x - (delta_y >> 1));
        while (y1 != y2)
        {
            if ((error >= 0) && (error || (iy > 0)))
            {
                error -= delta_y;
                x1 += ix;
            }
            // else do nothing
            error += delta_x;
            y1 += iy;
            draw_point(fb_image, imagewidth, x1, y1, color );
        }
    }
}

/*****************************/

void draw_line( RGBType *fb_image, int imagewidth, int x1, int y1, int x2, int y2, RGBType *color)  
{
    int delta_x(x2 - x1);
    // if x1 == x2, then it does not matter what we set here
    signed char const ix((delta_x > 0) - (delta_x < 0));
    delta_x = abs(delta_x) << 1;
         int delta_y(y2 - y1);
    // if y1 == y2, then it does not matter what we set here
    signed char const iy((delta_y > 0) - (delta_y < 0));
    delta_y = abs(delta_y) << 1;
    draw_point(fb_image, imagewidth, x1, y1, color );
    
    if (delta_x >= delta_y)
    {
        // error may go below zero
        int error(delta_y - (delta_x >> 1));
        while (x1 != x2)
        {
            if ((error >= 0) && (error || (ix > 0)))
            {
                error -= delta_x;
                y1 += iy;
            }
            // else do nothing
            error += delta_y;
            x1 += ix;
            draw_point(fb_image, imagewidth, x1, y1, color);
        }
    }
    else
    {
        // error may go below zero
        int error(delta_x - (delta_y >> 1));
        while (y1 != y2)
        {
            if ((error >= 0) && (error || (iy > 0)))
            {
                error -= delta_y;
                x1 += ix;
            }
            // else do nothing
            error += delta_x;
            y1 += iy;
            draw_point(fb_image, imagewidth, x1, y1, color );
        }
    }
}

/*****************************/
/*
 draw a linear, non-periodic poly line from a list of points 
*/
void draw_poly_line ( RGBAType *fb_image, int imagewidth, pix_coord *vertices, int numpts, RGBType *color )
{
    int i =0;
    if (numpts<2){
      return;
    }  

    for (i=1;i<numpts;i++ )
    {
        draw_line( fb_image, imagewidth, vertices[i-1].x, vertices[i-1].y, vertices[i].x, vertices[i].y, color );      
    }

}

/*****************************/
/*
 transform a vector to a point and draw it. 
*/
void draw_vector ( RGBAType *fb_image, int imagewidth, vector2d vec, int xpos, int ypos, RGBType *color )
{
    int start_x = xpos;
    int start_y = ypos;
    int end_x   = (int)vec.x + ypos;
    int end_y   = (int)vec.y + ypos;

    draw_line( fb_image, imagewidth, start_x, start_y, end_x, end_y, color );      

}


