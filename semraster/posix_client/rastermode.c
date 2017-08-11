/*
   Keith Legg April 15,2015
     module to render raster data in a ribbon for focusing the image
*/

#include <stdio.h> 
#include <stdlib.h>        


/************/

#ifdef __linux__
#include <GL/glut.h>     // Header File For The GLUT Library 
#include <GL/gl.h>       // Header File For The OpenGL32 Library
#include <GL/glu.h>      // Header File For The GLu32 Library
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

/************/

//these 3 are somewhat locked together
#include "point_op.h"    // vector operations
#include "framebuffer.h" // raster operations
#include "image_util.h"  // experimental features, etc 
/*****/
#include "semcom.h"      // serial port related

extern void LoadGLTextures(char* filename); 
extern void fake_scope_data(int *buffer, int stepsize, int use_2d_mode);
extern void update_scanres();

extern int scan_res;
extern int scanner_busy;

extern int pauseRefresh;

extern float scope_scx;

extern GLuint texture[1]; 

/***************************************/
/*
even though there is a width and height - assume image is square.
*/

void dump_ribbon_buffer(char *filename,
                          int *scanBuffer,
                          int width, int height)
{

    int tmp = 0;

    //dump binary data to disk 
    FILE *file_ptr;    
    file_ptr=fopen(filename, "wb");                
    fwrite(scanBuffer, 1, width*2, file_ptr);
    int fclose(FILE *file_ptr);

}


/***************************************/
/*
  takes a single row of pixels and renders a band of B&W pixels
  - TODO -  make the white taller than the black like a sonic display 

  even though there is a width and height - assume image is square.
*/

void render_ribbon_buffer(char *filename,
                          int *scanBuffer,
                          int width, int height)
{
    int x = 0;
    int y = 0;

    //char image_name[]   = "diagnostic.bmp"; 
    RGBType* pixBuffer  = createBuffer24(width, height);
    RGBType* pixItr = 0;
    int*    scanPtr = 0;
    int tmp = 0;
    //int midband = height/2;
    //int heigth_val = 40; //the value shown in white 

    for (y=0;y<height;y++)
    {
       //this renders a band in the middle of the image
       //if(y>(midband-heigth_val) &&y<(midband+heigth_val) ){   
          for ( x=0; x<width; x++ )
          {
              pixItr  = &( pixBuffer [(y*width) + x] );
              tmp = (int)scanBuffer[x]*.25; 

              if (tmp>=0&&tmp<=255)
              {
                  pixItr->r = tmp;
                  pixItr->g = tmp;
                  pixItr->b = tmp;
              }else{
                  pixItr->r = 0;
                  pixItr->g = 0;
                  pixItr->b = 0;                
              }

          }
      //}//band clamp
    }

    //fillbuffer24( pixBuffer, width, height, &bgcolor);
    saveBMP_24bit(pixBuffer, filename, width, height );
    free(pixBuffer); 
 
}

/***************************************/
/*
   mode 0 = horizontal
   mode 1 = vertical    
*/

void scan_ribbon(int size, int mode){ 
  	int scan_data[1048576] = {0};
    scanner_busy = 1;//debug this is a test 
  	//fake_scope_data(scan_data, size, 0); 
    //sc_get_h_sweep(scan_data, size);
    if (mode==0){
        //printf("horizontal scan \n");
  	    sc_get_h_sweep(scan_data, size);
    }else{
        //printf("vertical scan \n");      
        sc_get_v_sweep(scan_data, size);     
    }

    char filename[] = "ribbon.bmp";
  	render_ribbon_buffer(filename, scan_data, size, size/5 );
    LoadGLTextures(filename);
    scanner_busy = 0;//debug this is a test 
    /*****/
    /*
     char binfile[] = "ribbon.bin";
  	 dump_ribbon_buffer(binfile, scan_data, size, size );
    */
}

/***************************************/
void scan_full_image(int size){ 
    scanner_busy = 1;//debug this is a test 

    char filename[] = "video.bmp";
    sc_scan_image(filename);
    LoadGLTextures(filename);
    scanner_busy = 0;//debug this is a test 
} 

/***************************************/

/*
  cobbled together display callback to create a quad polygon with a UV map 
*/

void ribbonCallback(void)
{
    //update_scanres(); // NOT IN THE LOOP 

    if (!pauseRefresh)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear The Screen And The Depth Buffer
        glLoadIdentity();  

        glTranslatef(0.0f, 0.0f, scope_scx*-3.0f);\

        glBindTexture(GL_TEXTURE_2D, texture[0]);   // choose the texture to use.

        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_QUADS); 
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // BL texture and quad
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // BR texture and quad
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // TR texture and quad
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // TL texture and quad
        glEnd();  

        glutSwapBuffers();
        
        if(scanner_busy==0){
            scan_ribbon(scan_res, 0);
        }

    }
}

/***************************************/
void raster_idle(void) {
    glEnable(GL_DEPTH_TEST);// enable depth testing
    glutPostRedisplay(); // marks the current window as needing to be redisplayed. 
}

/***************************************/

void videoCallback(void)
{
    if (!pauseRefresh)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear The Screen And The Depth Buffer
        glLoadIdentity();  

        glTranslatef(0.0f, 0.0f, scope_scx*-3.0f);\

        glBindTexture(GL_TEXTURE_2D, texture[0]);   // choose the texture to use.

        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_QUADS); 
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // BL texture and quad
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // BR texture and quad
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // TR texture and quad
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // TL texture and quad
        glEnd();  

        glutSwapBuffers();
        
        if(scanner_busy==0){
            scan_full_image(scan_res);
        }

    }
}


