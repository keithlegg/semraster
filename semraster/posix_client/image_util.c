
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include "framebuffer.h"

/***************************************************************/


/* 
   experimental file format exporter - works but still buggy 

   -TO USE -  

   RGBAType *pixels = read_png_create_buffer32( INFILE );
   RGBType *pixels2 =  cvt32bit_24bit(pixels, width, height);   
   saveBMP_24bit ( pixels2 , OUTFILE, width, height) ;
   free(pixels); free(pixels2);


*/

void saveBMP_24bit ( RGBType *data, const char *filename, int w, int h) {

    FILE *f;
    int k = w*h;
    int s = 4*k;
    int filesize = 54 +s;

    int dpi = 300;
    double factor = 39.375;

    int m = (int)factor;
    int ppm = dpi*m;

    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0 ,0,0,0,0 , 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,24,0};

    bmpfileheader[2] = (unsigned char) (filesize);
    bmpfileheader[3] = (unsigned char) (filesize>>8);
    bmpfileheader[4] = (unsigned char) (filesize>>16);
    bmpfileheader[5] = (unsigned char) (filesize>>24);

    bmpinfoheader[4] = (unsigned char) (w);
    bmpinfoheader[5] = (unsigned char) (w>>8);
    bmpinfoheader[6] = (unsigned char) (w>>16);
    bmpinfoheader[7] = (unsigned char) (w>>24);

    bmpinfoheader[8]  = (unsigned char) (h);
    bmpinfoheader[9]  = (unsigned char) (h>>8);
    bmpinfoheader[10] = (unsigned char) (h>>16);
    bmpinfoheader[11] = (unsigned char) (h>>24);

    bmpinfoheader[21] = (unsigned char) (s);
    bmpinfoheader[22] = (unsigned char) (s>>8);
    bmpinfoheader[23] = (unsigned char) (s>>16);
    bmpinfoheader[24] = (unsigned char) (s>>24);

    bmpinfoheader[25] = (unsigned char) (ppm);
    bmpinfoheader[26] = (unsigned char) (ppm>>8);
    bmpinfoheader[27] = (unsigned char) (ppm>>16);
    bmpinfoheader[28] = (unsigned char) (ppm>>24);

    bmpinfoheader[29] = (unsigned char) (ppm);
    bmpinfoheader[30] = (unsigned char) (ppm>>8);
    bmpinfoheader[31] = (unsigned char) (ppm>>16);
    bmpinfoheader[32] = (unsigned char) (ppm>>24);

    f = fopen( filename,"wb");
    fwrite( bmpfileheader, 1, 14, f);
    fwrite( bmpinfoheader, 1, 40, f);

    for (int i = 0; i < k;i++){
       RGBType *rgba = &(data[i]);

       double red   = rgba->r*255;
       double green = rgba->g*255;
       double blue  = rgba->b*255;

       unsigned char color[3] = { (int)floor(blue),(int)floor(green),(int)floor(red) };
       fwrite (color, 1,3,f);
    }
    fclose(f);

}


/************************************/

void loadBMP_24bit( RGBType *data, const char *filename, int *w, int *h){
    FILE *file;
    unsigned long size;                 // size of the image in bytes.
    unsigned long i;                    // standard counter.
    unsigned short int planes;          // number of planes in image (must be 1) 
    unsigned short int bpp;             // number of bits per pixel (must be 24)
    char temp;                          // temporary color storage for bgr-rgb conversion.
   
    char *imageData = 0;  // temporary color storage

    if ((file = fopen(filename, "rb"))==NULL){printf("File Not Found : %s\n",filename);}
    
    // seek through the bmp header, up to the width/height:
    fseek(file, 18, SEEK_CUR);

    // read the width
    if ((i = fread(w, 4, 1, file)) != 1) {printf("Error reading width from %s.\n", filename);}
    printf("Width of %s: %i\n", filename, *w);
    
    // read the height 
    if ((i = fread(h, 4, 1, file)) != 1) {printf("Error reading height from %s.\n", filename);}
    printf("Height of %s: %i\n", filename, *h);
    
    // calculate the size (assuming 24 bits or 3 bytes per pixel).
    size = *w * *h *3;

    // read the planes
    if ((fread(&planes, 2, 1, file)) != 1) {printf("Error reading planes from %s.\n", filename);}
    if (planes != 1) {printf("Planes from %s is not 1: %u\n", filename, planes);}

    // read the bpp
    if ((i = fread(&bpp, 2, 1, file)) != 1){printf("Error reading bpp from %s.\n", filename);}
    if (bpp != 24){ printf("Bpp from %s is not 24: %u\n", filename, bpp);}
    
    // seek past the rest of the bitmap header.
    fseek(file, 24, SEEK_CUR);

    // read the data. 
    imageData = (char *) malloc(size);
    if (imageData == NULL){ printf("Error allocating memory for color-corrected image data");}
    if ((i = fread(imageData, size, 1, file)) != 1) {printf("Error reading image data from %s.\n", filename);}

    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
      temp = imageData[i];
      //printf("%s",temp);
      //data[i]   = image->data[i+2];
      //data[i+2] = temp;
      
    }

    //fclose
    //free

}
   


/************************************/


/*
  This creates a test image for file exporitng , etc. It creates a Mandelbrot set "test" image. 

  -TO USE- 
  //createTestImage(pixels, width, height, -0.802, -0.177, 0.011, 110);

*/

inline void setRGB(RGBAType *ptr, float val)
{
    /* map a float value to an RGB color */

    int v = (int)(val * 767);
    if (v < 0) v = 0;
    if (v > 767) v = 767;
    int offset = v % 256;

    if (v<256) {
        ptr->r = 0; ptr->g = 0; ptr->b = offset;
    }
    else if (v<512) {
         ptr->r = 255-offset; ptr->g = (int)offset/2; ptr->b = 0;
    }
    else {
        ptr->r = offset; ptr->g = 255; ptr->b = 0;
    }
}

void createTestImage(RGBAType *buffer, int width, int height, float xS, float yS, float rad, int maxIteration)
{
    int xPos, yPos;
    float minMu = maxIteration;
    float maxMu = 0;

    for (yPos=0 ; yPos<height ; yPos++)
    {
        float yP = (yS-rad) + (2.0f*rad/height)*yPos;

        for (xPos=0 ; xPos<width ; xPos++)
        {
            float xP = (xS-rad) + (2.0f*rad/width)*xPos;

            int iteration = 0;
            float x = 0;
            float y = 0;

            while (x*x + y*y <= 4 && iteration < maxIteration)
            {
                float tmp = x*x - y*y + xP;
                y = 2*x*y + yP;
                x = tmp;
                iteration++;
            }

            if (iteration < maxIteration) {
                float modZ = sqrt(x*x + y*y);
                float mu = iteration - (log(log(modZ))) / log(2);
                if (mu > maxMu) maxMu = mu;
                if (mu < minMu) minMu = mu;

                RGBAType* pixPtr = &( buffer[(yPos * width) + xPos]);
                setRGB(pixPtr, ( (mu- minMu) / (maxMu - minMu) ) );
            }
            else {
                RGBAType* pixPtr = &( buffer[(yPos * width) + xPos]);
                setRGB(pixPtr, 0);
            }
        }
    }
}


