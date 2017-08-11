/***************************************/
/*

   Author   - Keith Legg 
   Created  - April 15, 2015 
   Modified - May 2   , 2015

   (Beta) Homemade openGL oscillcope  

*/
/***************************************/

#include <stdio.h> 

#include "semcom.h"  // experimental features, etc

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

extern int g_Width;
extern int scan_res;

/***************************************************************/
//oscilloscope global settings (hotkey command hooks)

float scope_scx = .8; //grid and wave
float scope_scy = .8; //grid and wave

int linethick = 5;

float x_scale = 1.5;     //wave only
float y_scale = .01;    //wave only

float offsetX = 30;     //positional offset X
float offsetY = 1.3;    //positional offset Y

//extern unsigned char serial_buffer[];

/***************************************/
// callback when window is resized (which shouldn't happen in fullscreen) 
void gl_scope_reshape(int w, int h)
{
    if (h==0) {
        h=1;
    }
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);// set up the projection matrix 
    glLoadIdentity();
    glOrtho(0,532,0,10,-100,100);
    // go back to modelview matrix so we can move the objects about
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
} 
/***************************************/

void gl_scope_idle() {
    glEnable(GL_DEPTH_TEST);// enable depth testing
    glutPostRedisplay(); // marks the current window as needing to be redisplayed. 
}
/***************************************/
void gl_draw_graticule() 
{
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    glLoadIdentity(); 

    //draw waveform
    glColor3f(0,0,.6);
    float ls   = 60*scope_scx;  //grid spacing 
    float y_os =.05*scope_scx;  //y stretch

    int num = g_Width;//width of screen on x
    glLineWidth(1);
    for(unsigned int x=0;x<num;x=x+ls)
    { 
        for(unsigned int y=0;y<num;y=y+ls)
        { 
            glBegin(GL_LINE_STRIP);
              glVertex2f( x, 0 );
              glVertex2f( x, y );
            glEnd();   

            glBegin(GL_LINE_STRIP);
              glVertex2f( 0, y*y_os );
              glVertex2f( x, y*y_os );
            glEnd(); 

        }        
    }

}

/***************************************/
void gl_draw_scope_h() 
{
    gl_draw_graticule();
    glLineWidth(linethick);


    int vtxbuff[1024] = {0};
    
    /*********/ 
    //H,Output A, X
    sc_get_h_sweep( vtxbuff, scan_res );//this fills buffer with data
    
    //V,Output B, Y    
    //sc_get_v_sweep( vtxbuff, scan_res );//this fills buffer with data

    //fake_scan_data( vtxbuff, scan_res, 0 );
    /*********/
    
    //draw ground reff
    glLineWidth(2);    
    glColor3f(0, 1.0, 1.0);
    glBegin(GL_LINE_STRIP);
            glVertex2f( (float)x_scale+offsetX*scope_scx, 
                        (float)y_scale+offsetY*scope_scy 
            );
            glVertex2f( (float)(g_Width*x_scale)+offsetX*scope_scx, 
                        (float)y_scale           +offsetY*scope_scy 
            );           
    glEnd();
    
    /*********/
    glLineWidth(linethick);
    //draw waveform
    glColor3f(0,1.0,0);
    glBegin(GL_LINE_STRIP);
    
    float x_space    = 2.0;
    float y_div      = 1;//10 bit multiplied by this 

    for(int i=0;i<scan_res;i++)
    { 
        if(i<=g_Width)
        {
             glVertex2f( ((float)((x_space*i)        *x_scale) +offsetX)*scope_scx, 
                         ((float)((vtxbuff[i]*y_div)*y_scale) +offsetY)*scope_scx 
             );

            //glVertex2f( (float) x_space*i,  (float)vtxbuff[i]/10);

        }
    }
    glEnd();
    glutSwapBuffers();
}


/***************************************/
void gl_draw_scope_v() 
{
    gl_draw_graticule();
    glLineWidth(linethick);

    int vtxbuff[1024] = {0};
    
    /*********/ 
    //H,Output A, X
    sc_get_v_sweep( vtxbuff, scan_res );//this fills buffer with data
   
    //draw ground reff
    glLineWidth(2);    
    glColor3f(0, 1.0, 1.0);
    glBegin(GL_LINE_STRIP);
            glVertex2f( (float)x_scale+offsetX*scope_scx, 
                        (float)y_scale+offsetY*scope_scy 
            );
            glVertex2f( (float)(g_Width*x_scale)+offsetX*scope_scx, 
                        (float)y_scale           +offsetY*scope_scy 
            );           
    glEnd();
    
    /*********/
    glLineWidth(linethick);
    //draw waveform
    glColor3f(0,1.0,0);
    glBegin(GL_LINE_STRIP);
    
    float x_space    = 2.0;
    float y_div      = 1;//10 bit multiplied by this 

    for(int i=0;i<scan_res;i++)
    { 
        if(i<=g_Width)
        {
             glVertex2f( ((float)((x_space*i)        *x_scale) +offsetX)*scope_scx, 
                         ((float)((vtxbuff[i]*y_div)*y_scale) +offsetY)*scope_scx 
             );
        }
    }
    glEnd();
    glutSwapBuffers();
}


