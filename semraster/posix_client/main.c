// cd /keith/avr/semraster/src/

/***************************************************************/  

/*
***************************************************************
  *** SEMRASTER ***
  Author   - Keith Legg 
  Created  - March 22, 2015
  Modified - May 10, 2015

***************************************************************
  you can pass in a resolution as an agrument to make screen bigger 

  TODO: 
      -make a SEM binary image format 
        |---->
           |->file header
             |-> resolution (int 8-1024 squared)
             |-> image region scanned 
             |-> ? date scanned ?  
             |-> buffer for metadata 
             |-> buffer for metadata 
        |->pixel data (10 bit in 2 bytes) 
      |
      |-make python tools to load and process sem format 
      |-

    ////////// ////////// /////////////
    #to compile on linux you need GLUT
    On ubuntu you can get it with:

      sudo apt-get update
      sudo apt-get install build-essential
      sudo apt-get install freeglut3-dev

    #when compiling you need to add an '-lglut' as a comand line argument to gcc. 
    gcc test.c -lglut
*/

/***************************************************************/  

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>      //for getcwd 

#include <math.h> //sin  

/***************************************************************/

#ifdef __linux__
#define OUTPUT_BINARY "/keith/image.bin"
#include <GL/glut.h>     // Header File For The GLUT Library 
#include <GL/gl.h>       // Header File For The OpenGL32 Library
#include <GL/glu.h>      // Header File For The GLu32 Library
#endif

/***************************************************************/

//OSX related 
//to get serial port in OSX 'ls /dev/tty.*'  

#include <string.h>      //for memset 
#ifdef __APPLE__
#define OUTPUT_BINARY "/Users/klegg/image.bin"
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


extern float scope_scx;
extern float scope_scy;  
extern int linethick;
extern float x_scale;
extern float y_scale;
extern float offsetX;
extern float offsetY;


/********************************/ 

#include "point_op.h"     // vector operations
#include "framebuffer.h"  // raster operations
#include "image_util.h"   // experimental features, etc 
////////
#include "semcom.h"       // serial port related 
#include "scopemode.h"    // experimental features, etc 
#include "rastermode.h"   // experimental features, etc 
#include "file_io.h"

typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define VIEWING_DISTANCE_MIN  3.0

#define SEM_CONFIG_FILE "sem.cfg"

//#define TERMINATING_BYTE 0xa

/***************************************************************/
/***************************************************************/
//GUI properties

int window;                  // The number of our GLUT window 
GLuint texture[1];           // storage for one texture  

int isFullScreen = 0;        //are in in fullscreen?
int pauseRefresh = 0;        //dont update real time

int g_Width  = 600;   // Initial window size square
int g_Height = 600; 

//used for texture loading - my new framebuffer will replace this - DEBUG 
typedef struct Image{
    unsigned int sizeX;
    unsigned int sizeY;
    char *data;
} Image ;

//popup menu stuff
static BOOL g_bButton1Down     = FALSE;
static BOOL g_bLightingEnabled = TRUE;
static BOOL g_bFillPolygons    = TRUE;
static BOOL g_bTexture         = FALSE;

static int g_yClick = 0;
static GLfloat g_fViewDistance = 3 * VIEWING_DISTANCE_MIN;

enum {
  MENU_ABOUT = 1,
  MENU_RESTORE,
  MENU_SCOPEMODE,
  MENU_VIDEOMODE, 
  MENU_RIBBONMODE,    
  MENU_EXIT
};

/***************************************************************/
//geometry globals 

float xrot, yrot, zrot;// floats for x rotation, y rotation, z rotation 


/***************************************************************/
//SEM scan properties

int scan_res = 0;
int scanner_busy = 0;


/***************************************/
//makes a mandelbrot set image 
static void create_Image(char *filename){
  
   int w = scan_res;
   int h = scan_res;

   RGBAType *pixels  = createBuffer32(w, h);
   //fillbuffer32( pixels, locwidth, locheight, &bgcolor);
   createTestImage(pixels, w, h, -0.802, -0.177, 0.011, 110);
   saveBMP_24bit(copyBuffer24(pixels, w, h) , filename, w, h );

}

/***************************************/

//int ImageLoad(char *filename, RGBType *pixels, int width, int height) 

int ImageLoad(char *filename, Image *image) 
{
    FILE *file;
    unsigned long size;         // size of the image in bytes.
    unsigned long i;            // standard counter.
    unsigned short int planes;  // number of planes in image (must be 1) 
    unsigned short int bpp;     // number of bits per pixel (must be 24)
    char temp;                  // temporary color storage for bgr-rgb conversion.

    if ((file = fopen(filename, "rb"))==NULL)
    {
        printf("File Not Found : %s\n",filename);
        return 0;
    }
    
    // seek through the bmp header, up to the width/height:
    fseek(file, 18, SEEK_CUR);

    // read the width
    if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
        printf("Error reading width from %s.\n", filename);
        return 0;
    }
    //printf("Width of %s: %lu\n", filename, image->sizeX);
    
    // read the height 
    if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
        printf("Error reading height from %s.\n", filename);
        return 0;
    }
    //printf("Height of %s: %lu\n", filename, image->sizeY);
    
    // calculate the size (assuming 24 bits or 3 bytes per pixel).
    size = image->sizeX * image->sizeY * 3;

    // read the planes
    if ((fread(&planes, 2, 1, file)) != 1) {
        printf("Error reading planes from %s.\n", filename);
        return 0;
    }
    if (planes != 1) {
        printf("Planes from %s is not 1: %u\n", filename, planes);
        return 0;
    }

    // read the bpp
    if ((i = fread(&bpp, 2, 1, file)) != 1) {
        printf("Error reading bpp from %s.\n", filename);
        return 0;
    }
    if (bpp != 24) {
        printf("Bpp from %s is not 24: %u\n", filename, bpp);
        return 0;
    }
  
    // seek past the rest of the bitmap header.
    fseek(file, 24, SEEK_CUR);

    // read the data. 
    image->data = (char *) malloc(size);
    if (image->data == NULL) {
        printf("Error allocating memory for color-corrected image data");
        return 0; 
    }

    if ((i = fread(image->data, size, 1, file)) != 1) {
        printf("Error reading image data from %s.\n", filename);
        return 0;
    }

    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
        temp = image->data[i];
        image->data[i] = image->data[i+2];
        image->data[i+2] = temp;
    }

    return 1;
}
    
/***************************************/
   
// Load Bitmaps And Convert To Textures
void LoadGLTextures(char* filename) 
{   
    
    Image *pixels;
    pixels = (Image *) malloc(sizeof(Image));
    //RGBType *pixels  = createBuffer24(100, 100); 

    if (pixels == NULL) {
        printf("Error allocating space for image");
        exit(0);
    }

    if (!ImageLoad(filename, pixels )) {
        exit(1);
    }        
    
    // Create Texture   
    glGenTextures(1, &texture[0]);
    glBindTexture(GL_TEXTURE_2D, texture[0]);   // 2d texture (x and y size)

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // scale linearly when image smalled than texture

    // 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, 
    // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
    glTexImage2D(GL_TEXTURE_2D, 0, 3, pixels->sizeX, pixels->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels->data);
}



/***************************************/

// initialize OpenGL  
void InitGL(int Width, int Height)       // We call this right after our OpenGL window is created.
{
    glEnable(GL_TEXTURE_2D);             // Enable Texture Mapping
    glClearColor(0.0f, 0.0f, .1f, 0.0f); // Clear The Background Color To Blue 
    glClearDepth(1.0);                   // Enables Clearing Of The Depth Buffer
    glDepthFunc(GL_LESS);                // The Type Of Depth Test To Do
    glEnable(GL_DEPTH_TEST);             // Enables Depth Testing
    glShadeModel(GL_SMOOTH);             // Enables Smooth Color Shading
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();                    // Reset The Projection Matrix

    //glOrtho(0,532,0,10,-100,100); //this
    //gluOrtho2D(0, 100, 0, 100); //(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
    gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);   // Calculate The Aspect Ratio Of The Window
    
    glMatrixMode(GL_MODELVIEW);
}

/***************************************/

// callback when window is resized (which shouldn't happen in fullscreen) 
void ReSizeGLScene(int Width, int Height)
{
    
    if (Height==0)  // Prevent A Divide By Zero If The Window Is Too Small
    Height=1;

    glViewport(0, 0, Width, Height);    // Reset The Current Viewport And Perspective Transformation

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //glOrtho(0,532,0,10,-100,100); //this
    //gluOrtho2D(0, 100, 0, 100);
    gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);
    
    glMatrixMode(GL_MODELVIEW);
}//end resize callback



/***************************************/
/*
  cobbled together display callback to create a quad polygon with a UV map 
*/

void displayCallback()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear The Screen And The Depth Buffer
    glLoadIdentity();               // Reset The View

    glTranslatef(0.0f, 0.0f, -3.7f);    // move 5 units into the screen.

    glBindTexture(GL_TEXTURE_2D, texture[0]);   // choose the texture to use.

    glColor3f(1.0, 1.0, 1.0);
    //DEFINE GEOMETRY HERE 
    glBegin(GL_QUADS);//draw a four sided polygon(s)
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // BL texture and quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // BR texture and quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // TR texture and quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // TL texture and quad
    glEnd(); // done with the polygon.

    // since this is double buffered, swap the buffers to display what just got drawn.
    glutSwapBuffers();
}//end display callback 

/***************************************/
void showHelp(){
    printf("\n ##############################\n");
    printf(" SEMRASTER - Keith Legg 2013-2015\n");
    printf(" perihelionvfx@gmail.com         \n"); 
    printf(" www.perihelionvfx.com           \n");    
    printf(" ##############################\n");    
    printf(" commands are one key press   \n");
    printf(" 'esc' or 'q' to exit         \n");
    printf(" 'a' - ask for scan resolution \n");
    printf(" 'b' - step scan resolution    \n");
    printf(" 'f' - toggle fullscreen       \n");
    printf(" 's' - scope mode X axis       \n");
    printf(" 'S' - scope mode Y axis       \n");    
    printf(" 'r' - run single ribbon scan  \n");
    //printf("'p' - pause realtime scan    \n");
    
    printf(" 'shift -' - scale scope Y smaller   \n");
    printf(" 'shift +' - scale scope Y bigger    \n");
    printf(" '(' - scale scope X smaller        \n");
    printf(" ')' - scale scope X bigger         \n");

    printf(" '-' - scope zoom out all          \n");
    printf(" '+' - scope zoom in all           \n");    
    printf(" '7' - scope wave down y           \n");
    printf(" '8' - scope wave up y             \n");
    printf(" '9' - scope wave thicker          \n");
    printf(" '0' - scope wave thinner          \n");    


}


/***************************************/
void update_scanres(){
    scan_res = sc_get_resolution();
    if (scan_res==0){
        printf("\n#Serial communication error.%i \n", scan_res);
        exit (EXIT_FAILURE);
    } 
    //printf("#Setting scan resolution to %i.\n", scan_res);
}

/***************************************/


/***************************************/
void ribbonmode(void){
    update_scanres();
    
    glutDisplayFunc(&ribbonCallback);  
    glutIdleFunc(&raster_idle);
    glutReshapeFunc(&ReSizeGLScene); 

    if (isFullScreen==1){ 
       glutReshapeWindow(g_Width, g_Width);             
       glutFullScreen();  
    }else{ 
       glutReshapeWindow(g_Width, g_Width);  
    } 
}

void videomode(void){
    //DEBUG - YOU REALLY DONT WANT TO RUN IN ANYTHING HIGHER THAN 64 RES
    if (scan_res>128){  
        printf("Video mode disabled if resolution > 128.");
    }

    if (scan_res<=128){        
        update_scanres();

        glutDisplayFunc(&videoCallback);  
        glutIdleFunc(&raster_idle);
        glutReshapeFunc(&ReSizeGLScene); 

        if (isFullScreen==1){ 
           glutReshapeWindow(g_Width, g_Width);             
           glutFullScreen();  
        }else{ 
           glutReshapeWindow(g_Width, g_Width);  
        } 
    }

}

void scopemode(void){
    update_scanres();
    glutDisplayFunc(&gl_draw_scope_v); //rapid firing of this == bad idea? 
    glutIdleFunc(&gl_scope_idle);
    glutReshapeFunc(&gl_scope_reshape);

    if (isFullScreen==1){ 
       glutReshapeWindow(g_Width, g_Width);             
       glutFullScreen();  
    }else{ 
       glutReshapeWindow(g_Width, g_Width);  
    }   

}
/***************************************/

/*
   // define hotkeys // 
   TODO:
      shift P - advance paused one frame
      shift S - save screenshot 


*/

void keyPressed(unsigned char key, int x, int y) 
{

    usleep(100);
    /****************************/ 
    //ESCAPE OR Q KEY
    if (key == 27 || key == 113)  
    { 
        glutDestroyWindow(window); 
        exit(0);                   
    }
    /****************************/     
    if (key == 97) //a
    { 
        update_scanres();
        printf("%i\n", scan_res);
    }
    /****************************/  
    if (key == 112) //p
    { 
        if(pauseRefresh){
            pauseRefresh = 0;    
        }else{
            pauseRefresh = 1;
        }
    }
    /****************************/  
    if (key == 98) //b
    { 
       sc_step_resolution();  
       update_scanres();     
       printf("resolution set to %i \n", scan_res);                
    }
    /****************************/   
    if (key == 104){showHelp();} //h
    /****************************/ 
    if (key == 122) //z
    { 
        //printf("you pressed z\n");
        //sem_com(OUTPUT_BINARY, SERIAL_PORT_ID, 2);       
    }
    /****************************/ 
    //if (key == 118){LoadGLTextures("generated2.bmp");} //v
    /****************************/ 
    if (key == 102) //f
    { 
        //LoadGLTextures("generated2.bmp"); 
        if (isFullScreen==0){ 
           glutFullScreen();  
           isFullScreen = 1; 
        }else{ 
           glutReshapeWindow(g_Width, g_Width); //width twice because its square
           glutPositionWindow(0,0);
           isFullScreen = 0; 
        }      
        
    }
    /****************************/ 
    if (key == 119) //w
    { 
        //if you auto sequence the images and save - you have a movie!!!
        //EXPLORE THIS IDEA!!

        //update_scanres();
           
        char filename[] = "scan_buffer.bmp";
        sc_scan_image(filename);
        LoadGLTextures(filename);       
    }
    /****************************/ 
    //test image mode - t 
    if (key == 116) //t
    { 
        //resetPerspectiveProjection();
        //setOrthographicProjection();
        //sem_com(OUTPUT_BINARY, SERIAL_PORT_ID, 4); 
        //hud_text(120, 120, 255, 0, 0, "hello"); 

        /*******/
        //if you auto sequence the images and save - you have a movie!!!
        //EXPLORE THIS IDEA!!
        /*******/
        char filename[] = "scan_buffer.bmp";
        create_Image(filename); 
        LoadGLTextures(filename);         

    }
    /****************************/ 
    //test UART mode //T 
    if (key == 84)
    {
        update_scanres();    
        run_10bittest(scan_res);
    }
        
    /****************************/ 
    //raster mode
    if (key == 99) //c
    {     

        printf("#running a binary dump of scan\n");   
        update_scanres();
        run_scan_dump(scan_res);

    }
    /****************************/ 
    //ribbon mode horizontal
    if (key == 114) //r
    { 
        ribbonmode();
        //scan_ribbon(scan_res, 0); //run once 
    }    
 
    /****************************/ 
    //video mode 
    if (key == 118){videomode();} //v

    //ribbon mode vertical
    if (key == 82){scan_ribbon(scan_res, 1);} //R   

    /****************************/ 
    //idle mode
    if (key == 100) //d
    { 
        glutDisplayFunc(&displayCallback);  
        glutIdleFunc(&displayCallback);
        glutReshapeFunc(&ReSizeGLScene);           
    }
    /****************************/     
    //scope mode H    
    if (key == 115){scopemode();}//s

    //scope mode V    
    if (key == 83) //S
    { 


    }

    /****************************/ 
    //scope dc shift up   
    if (key == 56){offsetY=offsetY+.05;} //8

    //scope dc shift down   
    if (key == 55){offsetY=offsetY-.05;} //7

    //scope lines thicker   
    if (key == 48){if (linethick<=9){linethick=linethick+1;}} //0

    //scope lines thinner    
    if (key == 57){if (linethick>1){linethick=linethick-1;}} //9

    //scope wave narrower    
    if (key == 41){x_scale=x_scale+.02;} //(

    //scope wave wider    
    if (key == 40){x_scale=x_scale-.02;} //)

    //scope wave taller    
    if (key == 43){y_scale=y_scale+.001;} //=

    //scope wave shorter    
    if (key == 95){y_scale=y_scale-.001;} //_

    //scope wave magnify    
    if (key == 61){scope_scx=scope_scx+.01;scope_scy=scope_scy+.01;} //=

    //scope wave shrink    
    if (key == 45){if (scope_scx>.2){scope_scx=scope_scx-.01;scope_scy=scope_scy-.01;}}

}


/*****************************************/
void MouseButton(int button, int state, int x, int y)
{
    // Respond to mouse button presses.
    // If button1 pressed, mark this state so we know in motion function.

    if (button == GLUT_LEFT_BUTTON)
    {
        g_bButton1Down = (state == GLUT_DOWN) ? TRUE : FALSE;
        g_yClick = y - 3 * g_fViewDistance;
    }
}

/*****************************************/
void MouseMotion(int x, int y)
{
  // If button1 pressed, zoom in/out if mouse is moved up/down.

  if (g_bButton1Down)
    {
      g_fViewDistance = (y - g_yClick) / 3.0;
      if (g_fViewDistance < VIEWING_DISTANCE_MIN)
         g_fViewDistance = VIEWING_DISTANCE_MIN;
      glutPostRedisplay();
    }
}





void show_help(void){

        printf(" *****************************     \n");
        printf("  Keith Legg  2013-2015            \n");
        printf("  Usage:  semraster <modes>        \n");
        printf("    gui        - run GL gui        \n");
        printf("    ribbondump -                   \n");
        printf("    scandump   -                   \n");         
        printf(" *****************************     \n");


}
/***************************************************************/

void SelectFromMenu(int idCommand)
{
  switch (idCommand)
    {
        case MENU_ABOUT:
          show_help();
          break;

        case MENU_RESTORE:
          glutDisplayFunc(&displayCallback);  
          glutIdleFunc(&displayCallback);
          glutReshapeFunc(&ReSizeGLScene);  

          break;      

        case MENU_SCOPEMODE:
          scopemode();
          break;    

        case MENU_RIBBONMODE:
          ribbonmode();
          break;

        case MENU_VIDEOMODE:
          videomode();
          break;

        case MENU_EXIT:
          exit (0);
          break;
    }

    // Almost any menu selection requires a redraw
    glutPostRedisplay();
}


/*****************************************/

int BuildPopupMenu (void)
{
    int menu;

    menu = glutCreateMenu (SelectFromMenu);
    glutAddMenuEntry ("help"       , MENU_ABOUT);
    glutAddMenuEntry ("idle mode"  , MENU_RESTORE);
    glutAddMenuEntry ("scope mode" , MENU_SCOPEMODE);
    glutAddMenuEntry ("ribbon mode", MENU_RIBBONMODE);        
    glutAddMenuEntry ("video mode" , MENU_VIDEOMODE);
    glutAddMenuEntry ("exit  "     , MENU_EXIT);

    return menu;
}

/***************************************/
void openGlMain(int screenSize)
{

    //these are fake parameters to pass to glutinit 
    char *myargv [1];
    int myargc=1;
    myargv [0]=strdup ("Myappname");
    glutInit(&myargc, myargv);
    // you can find documentation at http://reality.sgi.com/mjk/spec3/spec3.html   
    //glutInit(myargc, myargv);  

    /*******/

     //Depth buffer, RGBA color, Double buffer , Alpha    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);  

    glutInitWindowSize(screenSize, screenSize);  //window size
    // the window starts at the upper left corner of the screen  
    glutInitWindowPosition(0, 0);  
   
    window = glutCreateWindow("SEM raster display"); //create an opengl window 

    glutDisplayFunc(&displayCallback);//register display callback       
    // Even if there are no events, redraw our gl scene.  
    glutIdleFunc(&displayCallback);

    glutReshapeFunc(&ReSizeGLScene);  //register window resize callback 
    glutKeyboardFunc(&keyPressed);    // Register key pressed callback 
    
    InitGL(screenSize, screenSize); // Initialize window. 
    
    ///////////////////////////
    
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseMotion);
  
    // Create our popup menu
    BuildPopupMenu();
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();// Start Event Processing Engine   
   
}



/***************************************/
void parseArgs(int argc, char **argv)
{
    if (argc < 2){
        //abort_("Usage: semraster <mode> ");
        printf("# no mode specified. starting gui mode.");
        update_scanres();
        openGlMain(g_Width); //start up openGL         
    }

    char runmode[10];
    /************/   
    strcpy(runmode, "help");
    if( strcmp(argv[1], runmode) == 0)
    {
       show_help();
    }    
    strcpy(runmode, "helpapi");
    if( strcmp(argv[1], runmode) == 0)
    {
        printf(" *****************************      \n");
        printf("  API commands                      \n");
        printf(" //  rx_stream -                    \n"); 
        printf(" //  rx_byte   -                    \n"); 
        printf(" //  tx_byte   -                    \n");   
        printf("     show      -                    \n");   
        printf("     step      -                    \n"); 
        printf("     flush     -                    \n"); 
        printf(" *****************************      \n");
    }   
 
    /************/   
    strcpy(runmode, "gui");
    if( strcmp(argv[1], runmode) == 0)
    {
        update_scanres();
        openGlMain(g_Width); //start up openGL 
    }
    /************/   
    strcpy(runmode, "flush");
    if( strcmp(argv[1], runmode) == 0)
    {
      //flush serial port 
    }    
    /************/   
    strcpy(runmode, "scan");
    if( strcmp(argv[1], runmode) == 0)
    {
        update_scanres();
        char filename[] = "scan_buffer.bmp";
        sc_scan_image(filename);
    }
    /************/       
    strcpy(runmode, "show");
    if( strcmp(argv[1], runmode) == 0)
    {
       update_scanres();     
       printf("%i \n", scan_res); 
    }       
    /************/   
    strcpy(runmode, "step");
    if( strcmp(argv[1], runmode) == 0)
    {
       sc_step_resolution();  
       update_scanres();     
       printf("%i \n", scan_res); 
    }    
    /************/
    strcpy(runmode, "ribbondump");
    if( strcmp(argv[1], runmode) == 0)
    {
        update_scanres();
        scan_ribbon(scan_res, 0);
    }   
    /************/   
    strcpy(runmode, "scandump");
    if( strcmp(argv[1], runmode) == 0)
    {
        update_scanres();
        printf("#running a binary dump of scan\n");   
        run_scan_dump(scan_res);
    }
    /************/

}

/***************************************/
/***************************************/
int main(int argc, char **argv) 
{  
    
    //do_flush( unsigned char *serial_buffer, char *SERIAL_PORT)
    char config[] = "sem.cfg";
    read_config(config);

    parseArgs(argc, argv); //new args with mode added 
    return 1;
}

