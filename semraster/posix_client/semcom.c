/***************************************************************/
/*
   Serial port interface to scanning electron microscope 
   
   Author   - Keith Legg 
   Created  - April 21, 2015 
   Modified - May 10, 2015 

   functions that start with sc_ are wrapper functions around sem_com
   sem_com was meant to be use seiral timeout but for now notimeout 
   is a drop is substitute until I get my shiz fixed.
*/
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>      //EXIT_SUCCESS

#include <fcntl.h>       //?serial related 
#include <termios.h>     //serial port API 
#include <unistd.h>      //for getcwd, read 
#include <string.h>      //strcmp
#include <errno.h>

#include <math.h> //sin  

#include <time.h>


#include "semcom.h"      //woah! somehow it was working without this!?
#include "framebuffer.h" // raster operations
#include "image_util.h"  // experimental features, etc 

//#include <sys/select.h>   // ???????????
   
/***************************************/
extern int scan_res;
extern int scanner_busy; //global lock for scanner 
extern int BUFFER_SIZE;  
extern char SERIAL_PORT_ID[];

#define SERIAL_BAUD B57600

int stepsize = 0; //auto detected from device 

/***************************************/

int* create_sem_PixelBuffer(int size){
    int *chr;
    chr =  ( int *) malloc( size * sizeof( int )  );
    return chr; 
}

unsigned char* create_sem_SerialBuffer(int size){
    unsigned char *chr;
    chr =  ( unsigned char *) malloc( size * sizeof( unsigned char)  );
    return chr; 
}

/***************************************/
//allocate memory on heap for large arrays 

int * pixel_buffer    = create_sem_PixelBuffer( BUFFER_SIZE );

/***************************************/
/* because the buffers are filled up chronologically, the newest 
   data gets sent to the back. When the second conversion function 
   renders the 10 bit values and pixels , the image apears flipped 
   (newest region on bottom). This function "flips" the buffer to 
   correct that artifact */

void reverse_sem_PixelBuffer(int* buffer, int totalsize){
    int i,count =0;
    int* tmp = create_sem_PixelBuffer(totalsize);
    //copy the first buffer
    for(i=0;i<totalsize;i++){
        tmp[i]=buffer[i];   
    };

    //now reverse the original
    for(i=totalsize;i>0;i--){
        buffer[count]=tmp[i];count++;    
    };

    free(tmp);
}


/***************************************************************/
/* this is the function that performs an image scan 
  it buffers the data to disk as BMP format
  it is used for both single scans and in video mode
*/

void sc_scan_image(char *filename)
{
    scanner_busy = 1;
    sc_get_resolution();   
    //fflush(stdout); 

    int use_res = scan_res;
    int width  = use_res;
    int height = use_res;

    RGBType* imageBuffer  = createBuffer24(width, height);
    RGBType* pixItr = 0;
 
    sc_run_fullscan(pixel_buffer, use_res);//assumes square image
    reverse_sem_PixelBuffer(pixel_buffer, (use_res*use_res) ) ;
 
    int idx = 0;
    int count = 0;
    int lumin = 0;
    int y = 0;
    int x = 0;

    int divAmt = 4;//divide 10bit values by 4 to get it to 8 bit numeric range    
  
    /***************/
    for(y=0;y<height;y++)
    {  
        for(x=0;x<width;x++)
        {
            idx = (y*width)+x;
            pixItr  = &( imageBuffer[idx] );//pointer to XY output image 
            lumin = (int)pixel_buffer[idx]/divAmt;//pointer received serial ADC data
            pixItr->r = lumin;
            pixItr->g = lumin;
            pixItr->b = lumin; 
        }
    }
    /***************/
    saveBMP_24bit(imageBuffer, filename, width, height );
    free(imageBuffer);

    scanner_busy = 0;//free global lock on image refresh 
}


/***************************************************************/

/*
  debug untested!
*/

void do_flush( unsigned char *serial_buffer, char *SERIAL_PORT)
{
    struct termios tio;
    struct termios stdio;
    struct termios old_stdio;    
    int tty_fd;
    fd_set rdset;
    
    // tcgetattr( STDOUT_FILENO, &old_stdio);
    // memset(&stdio,0,sizeof(stdio));
    // stdio.c_iflag=0;
    // stdio.c_oflag=0;
    // stdio.c_cflag=0;
    // stdio.c_lflag=0;
    // stdio.c_cc[VMIN]=1;
    // stdio.c_cc[VTIME]=0;
    // tcsetattr( STDOUT_FILENO, TCSANOW, &stdio );
    // tcsetattr( STDOUT_FILENO, TCSAFLUSH, &stdio );
    // fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // make the reads non-blocking
    
    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;// 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;

    tty_fd=open(SERIAL_PORT, O_RDWR | O_NONBLOCK);     
    cfsetospeed(&tio, SERIAL_BAUD);      
    cfsetispeed(&tio, SERIAL_BAUD);  
    tcsetattr(tty_fd, TCSANOW, &tio);
 
    //do we want to flush here? debug  
    if (tcflush(tty_fd, TCIOFLUSH) == 0){
        //printf("The input and output queues have been flushed.n");
    }
    else{
        perror("tcflush error");
    }
    close(tty_fd);
    //tcsetattr( STDOUT_FILENO, TCSANOW, &old_stdio);    
}


/***************************************************************/

/*
   this works but you have to know EXACTLY how many bytes you want 
   this has NO TIMEOUT or ERROR CHECKING !! - (CHECKSUM , ETC ETC )
*/

void sem_com_notimeout( unsigned char *serial_buffer, int readsize, 
                        char *SERIAL_PORT, char *command )
{
    struct termios tio;
    struct termios stdio;
    struct termios old_stdio;    
    int tty_fd;
    fd_set rdset;

    unsigned char c='D';
    
    tcgetattr( STDOUT_FILENO, &old_stdio);
    memset(&stdio,0,sizeof(stdio));
    stdio.c_iflag=0;
    stdio.c_oflag=0;
    stdio.c_cflag=0;
    stdio.c_lflag=0;
    stdio.c_cc[VMIN]=1;
    stdio.c_cc[VTIME]=0;
    tcsetattr( STDOUT_FILENO, TCSANOW, &stdio );
    tcsetattr( STDOUT_FILENO, TCSAFLUSH, &stdio );
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);       // make the reads non-blocking

    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;

    tty_fd=open(SERIAL_PORT, O_RDWR | O_NONBLOCK);     
    cfsetospeed(&tio, SERIAL_BAUD);      
    cfsetispeed(&tio, SERIAL_BAUD);  

    tcsetattr(tty_fd, TCSANOW, &tio);
 
    //do we want to flush here? debug  
    if (tcflush(tty_fd, TCIOFLUSH) == 0){
        //printf("The input and output queues have been flushed.n");
    }
    else{
        perror("tcflush error");
    }

    //send command to SEM 
    write(tty_fd, command, 1);
    usleep( 80000 ); //wait a bit for SEM to respond

    //poll the serial hardware, count the bytes coming back 
    int count = 0; 
    while(count<readsize){
      if (read(tty_fd, &c, 1)>0)
      {
          //printf("%c", serial_buffer[count]); //this shows it works  

          serial_buffer[count] = c;            
          count++;
      }
    }

    close(tty_fd);
    tcsetattr( STDOUT_FILENO, TCSANOW, &old_stdio);    
}


/***************************************************************/

/*
   use sem_send() for unidirectional communication,
   use sem_com() to read a buffer back
   
   Warning: In this program the VMIN and VTIME flags are ignored because the O_NONBLOCK flag is set.
   http://en.wikibooks.org/wiki/Serial_Programming/termios#Opening.2FClosing_a_Serial_Device


*/

void sem_send( char *SERIAL_PORT, char *command )
{
    struct termios tio;
    struct termios stdio;
    struct termios old_stdio;
    int tty_fd;

    tcgetattr( STDOUT_FILENO, &old_stdio);

    memset( &stdio, 0, sizeof(stdio) );
    stdio.c_iflag=0;
    stdio.c_oflag=0;
    stdio.c_cflag=0;
    stdio.c_lflag=0;
    stdio.c_cc[VMIN]=1;
    stdio.c_cc[VTIME]=0;
    tcsetattr(STDOUT_FILENO,TCSANOW,&stdio);
    tcsetattr(STDOUT_FILENO,TCSAFLUSH,&stdio);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);  // make the reads non-blocking

    memset( &tio,0, sizeof(tio) );
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;    // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;

    tty_fd=open(SERIAL_PORT, O_RDWR | O_NONBLOCK);      

    cfsetospeed(&tio, SERIAL_BAUD);    
    cfsetispeed(&tio, SERIAL_BAUD);     
    tcsetattr( tty_fd, TCSANOW, &tio);
    ///////////////
    fd_set set;
    struct timeval timeout;
    int rv;

    FD_ZERO(&set); // clear the set  
    FD_SET(tty_fd, &set); // add our file descriptor to the set 

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    tcflush(tty_fd, TCIOFLUSH);
    write(tty_fd, command, 1);
  
    //////////////////////
    //close connection and return STDIO 
    close(tty_fd);
    tcsetattr( STDOUT_FILENO, TCSANOW, &old_stdio);
}

/***************************************************************/

/*
   use sem_send() for unidirectional communication,
   use sem_com() to read a buffer back 
*/


void sem_com( unsigned char *serial_buffer, int readsize, 
                        char *SERIAL_PORT, char *command )
{
    struct termios tio;
    struct termios stdio;
    struct termios old_stdio;    
    int tty_fd;
    fd_set rdset;

    unsigned char c='D';
    
    tcgetattr( STDOUT_FILENO, &old_stdio);

    memset(&stdio,0,sizeof(stdio));
    stdio.c_iflag=0;
    stdio.c_oflag=0;
    stdio.c_cflag=0;
    stdio.c_lflag=0;
    stdio.c_cc[VMIN]=1;
    stdio.c_cc[VTIME]=0;
    tcsetattr( STDOUT_FILENO, TCSANOW, &stdio );
    tcsetattr( STDOUT_FILENO, TCSAFLUSH, &stdio );
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // make the reads non-blocking

    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;  // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;

    tty_fd=open(SERIAL_PORT, O_RDWR | O_NONBLOCK);      
    cfsetospeed(&tio, SERIAL_BAUD);      
    cfsetispeed(&tio, SERIAL_BAUD);  

    tcsetattr(tty_fd, TCSANOW, &tio);
   
    //do we want to flush here? debug  
    tcflush(tty_fd, TCIOFLUSH);

    //send command to SEM 
    write(tty_fd, command, 1);
    usleep( 80000 ); //wait a bit for SEM to respond

    //count the bytes coming back and exit when there 
    int count = 0; 
    while(count<readsize){
      if (read(tty_fd, &c, 1)>0)
      {
          //write(STDOUT_FILENO, &c, 1);  
          //printf("%x -", c);          
          serial_buffer[count] = c;            
          count++;
      }
    }

    close(tty_fd);
    tcsetattr( STDOUT_FILENO, TCSANOW, &old_stdio);    
}

/***************************************************************/
/***************************************************************/

void sc_step_resolution(void)
{
    sem_send( SERIAL_PORT_ID, "b");
}

/***************************************/
/*
  activate SEM 8 bit count up test and receive
*/

void sc_receive8_test(void)
{
   int resolution = 256;
   
   unsigned char *serial_buffer = create_sem_SerialBuffer(BUFFER_SIZE);
   int * pixel_buffer = create_sem_PixelBuffer(BUFFER_SIZE);

   sem_com_notimeout(serial_buffer, resolution, SERIAL_PORT_ID, "h" );

   int count = 0;
   char msb  = 0;
   char lsb  = 0;

   //sew it together and cache it
   for(int z=0;z< resolution ;z++)
   {
       printf("- %i ", serial_buffer[z]&0xff);
       pixel_buffer[count] = serial_buffer[z]&0xff ;count++;
   }
   printf("total count is %i \n", count );
   
   free(serial_buffer);

}

/***************************************/
/*
   perform UART RX and assemble 10 bit int values 
*/

void sc_get_h_receive10_test(int *buffer, int resolution)
{
   
   unsigned char *serial_buffer = create_sem_SerialBuffer(BUFFER_SIZE);   
   //sem_com( serial_buffer, resolution*2, SERIAL_PORT_ID, "i" );  
   sem_com_notimeout(serial_buffer, resolution*2, SERIAL_PORT_ID, "i" );

   int count = 0;
   char msb = 0;
   char lsb = 0;

   //sew it together and cahce it
   for(int z=0;z< resolution*2 ;z++)
   {
       //ODD - MSB 
       if (z%2!=0)
       {
          //printf("ms %i ", lsb&0xff );  
          msb = serial_buffer[z];
       }
       
       //EVEN - LSB
       if (z%2==0)
       {
          //printf("ls %i ", msb&0xff );
          lsb =  serial_buffer[z];
          buffer[count] = (msb << 8 ) | (lsb & 0xff);count++;
       }

   }
    //printf("total count is %i \n", count );
    free(serial_buffer);
}

/***************************************/
void run_10bittest(int resolution)
{
    int foo_buffer[1024] = {0};
    sc_get_h_receive10_test(foo_buffer, 1024); 
    for(int a = 0;a<1024;a++){
       printf("[%i-%i] ", a, foo_buffer[a]);
    }
}

/***************************************/
/*
   perform a horizontal ribbon scan and fill buffer  
*/

void sc_get_h_sweep(int *buffer, int resolution)
{
   unsigned char *serial_buffer = create_sem_SerialBuffer(BUFFER_SIZE);
   sem_com_notimeout(serial_buffer, resolution*2, SERIAL_PORT_ID, "u" );

   int count = 0;
   char msb = 0;
   char lsb = 0;

   //sew it together and insert int value into *buffer 
   for(int z=1; z<(resolution*2); z++)
   {
       //even-lsb
       if(z%2==0)
       {
          lsb =  serial_buffer[z];
       }//lsb

       //odd-msb
       if (z%2==1)//EQUALS 1???? NOT 0 ???
       {
          msb = serial_buffer[z];
          int val = 0;
          val = (msb << 8 ) | (lsb & 0xff);
          //printf("z %i lsb %x msb %x  val %i count %i\n",z, lsb&0xff , msb, val, count);
          buffer[count] = val;count++;
       }//msb
   
   }//loop

    free(serial_buffer);
}
/***************************************/
/*
   DEBUG GET RID OF THIS AND MAKE IT AN ARG TO OTHER FUNC ABOVE 
   perform veritcal ribbon scan and fill buffer  
*/

void sc_get_v_sweep(int *buffer, int resolution)
{
   unsigned char *serial_buffer = create_sem_SerialBuffer(BUFFER_SIZE); 
   sem_com_notimeout(serial_buffer, resolution*2, SERIAL_PORT_ID, "v" );

   int count = 0;
   char msb = 0;
   char lsb = 0;

   //sew it together and cahce it
   for(int z=1; z<(resolution*2); z++)
   {
       if(z%2==0)
       {
          lsb =  serial_buffer[z];
       }//lsb

       if (z%2==1)//EQUALS 1???? NOT 0 ???
       {
          msb = serial_buffer[z];
          int val = 0;
          val = (msb << 8 ) | (lsb & 0xff);
          //printf("z %i lsb %x msb %x  val %i count %i\n",z, lsb&0xff , msb, val, count);
          buffer[count] = val;count++;
       }//msb
   }//loop
   free( serial_buffer );
}

/***************************************/
/*
   Initiate a 2D image SEM scan - capture all data and sew it into 10 bit int values 
*/

void sc_run_fullscan(int *buffer, int resolution)
{
   unsigned char *serial_buffer = create_sem_SerialBuffer(BUFFER_SIZE);
   int numPixels = resolution*((resolution*2)+1);

   /******/
   sem_com_notimeout(serial_buffer, numPixels, SERIAL_PORT_ID, "s" );
   //sem_com_notimeout(serial_buffer, numPixels, SERIAL_PORT_ID, "j" );//test data
   /******/

   int frbuff_count = 0;
   int tobuff_count = 0;

   char msb = 0;
   char lsb = 0;
   int y = 0;
   int x = 0;

   int idx = 0;
   unsigned int val = 0;

    //sew it together and cache it
    for(y=0; y<resolution; y++)
    {
        for(x=0; x<(resolution*2)+1; x++)
        {
            idx  = (y*(resolution*2))+x;
   
            if(x%2==0)
            {
                lsb =  serial_buffer[ frbuff_count ];frbuff_count++;
            }

            if (x%2==1) 
            {
                msb = serial_buffer[ frbuff_count ];frbuff_count++;
                val = (msb << 8 ) | (lsb & 0xff);
                buffer[tobuff_count] = val;tobuff_count++;
            }
        }//loop x
    }//loop y

    free(serial_buffer);//temporary buffer for UART data
}


/***************************************/
/*
   run a scan and dump in proprietary binary format 
*/

void run_scan_dump(int resolution)
{
    //DUMP RAW UART DATA TO BINARY FILE  
    int numPixels = resolution*((resolution*2)+1);
    int* pixel_buffer = create_sem_PixelBuffer(numPixels);
    unsigned char* serial_buffer = create_sem_SerialBuffer(numPixels);    
    printf("# resolution %i numpixels %i numbytes %i\n", resolution, (resolution*resolution), numPixels );
 
    sem_com_notimeout(serial_buffer, numPixels, SERIAL_PORT_ID, "s" );
    //sem_com_notimeout(serial_buffer, numPixels, SERIAL_PORT_ID, "j" );

    char* filename = "scan_dump.bin";
    FILE *file_ptr;    
    file_ptr = fopen(filename, "wb");                
    fwrite(serial_buffer, 1, numPixels, file_ptr);
    int fclose(FILE *file_ptr);
    
    printf("# finished writing file %s \n", filename);
    
    //free(pixel_buffer); //dont free it off the heap
    free(serial_buffer);

}

/***************************************/
/*
   ask SEM to report its resolution settings 
*/

int sc_get_resolution(void)
{
    unsigned char *serial_buffer = create_sem_SerialBuffer(BUFFER_SIZE);
    //sem_com(          serial_buffer, 8, SERIAL_PORT_ID, "z" );  
    sem_com_notimeout(serial_buffer, 8, SERIAL_PORT_ID, "z" );

    char status_buffer[8]; 
    //strncpy(serial_buffer, status_buffer, 8);
    for(int z=0;z<9;z++){
        status_buffer[z] = serial_buffer[z];
    }
    /////////////////

    if(strcmp( "00000001", status_buffer)==0)
    {
        stepsize = 1024;           
    }
    if(strcmp( "00000010", status_buffer)==0)
    {
        stepsize = 512; 
    }
    if(strcmp( "00000100", status_buffer)==0)
    {
        stepsize = 256; 
    }
    if(strcmp( "00001000", status_buffer)==0)
    {
        stepsize = 128; 
    }                
    if(strcmp( "00010000", status_buffer)==0)
    {
        stepsize = 64; 
    }
    if(strcmp( "00100000", status_buffer)==0)
    {
        stepsize = 32; 
    }   
    if(strcmp( "01000000", status_buffer)==0)
    {
        stepsize = 16; 
    } 
    if(strcmp( "10000000", status_buffer)==0)
    {
        stepsize = 8; 
    }  
    free(serial_buffer);
    return stepsize;
}

/***********************************************/
/*
   generate 2 bytes per pixel  fake image data
*/

void fake_scan_rawdata(unsigned char *serBuffPt, int resolution)
{
    //int use_2d_mode = 0; 
    int count = 0;  
    int x = 0;
    int y = 0;

    for(y=0;y<resolution;y++)
    {
        for(x=0;x<resolution;x++)
        {
            if(x<256)
            {
                // we send TWO bytes little endian for each pixel !
                serBuffPt[count] = x;count++; //lsb first             
                serBuffPt[count] = 0;count++; //msb is 0 if <256
            }        
            if(x>=256)
            {
                //lsb first - clear the high byte 
                serBuffPt[count] = (x & 0x00FF);count++;
                //msb last - shift all bits 8 places right
                serBuffPt[count] = x>>8;count++; 
            }

         }

         //add 0xa terminator (this may be removed later)
         serBuffPt[count] = 0xa;count++;
    }
 
}

/***************************************/
/*
   just generate some fake scanned data for testing 
*/

void fake_scan_data(int *bufferPt, int resolution, int use_2d_mode)
{
   unsigned char *serial_buffer = create_sem_SerialBuffer(BUFFER_SIZE);

   //int use_2d_mode = 0; 
   int count = 0;  
   
   int lsb = 0;
   int msb = 0;
   int x = 0;
   int y = 0;
 
   count=0;

   /************/   
   if(use_2d_mode==0) //1D MODE
   {
     for(int z=0;z<stepsize;z++)
     {
         bufferPt[z] = serial_buffer[z]; 
     }
   } 

   /************/
   if(use_2d_mode==1) //2D MODE 8bit
   {
       //printf("fake_scope_data 2d mode\n");
       for(y=0;y<stepsize;y++)
       {    
           for(x=0;x<stepsize;x++)
           {
               //count == (y*stepsize)+x   //??
               bufferPt[count] = serial_buffer[count]; //1d data to 2d image --2d is (y*stepsize)+x 
               count++;
           }
       }
   }

   /************/
   if(use_2d_mode==2) //2D MODE 10bit
   {
        
        //STACK SMASHING HAS BEEN TRACED TO HERE!!??
        fake_scan_rawdata(&serial_buffer[0], resolution); //sets - serial_buffer

        //would be nice to have a gradient at all resolutions - TODO  
        //int step_amt = 1024/resolution;
        //printf("step amount is %i \n", step_amt);

        int step = 0;
        //now sew 2 bytes per pixel into a 10 bit value per pixel
        for (y=0;y<resolution;y++)
        {
            step = 0;
            for (x=0;x<(resolution*2)+1;x++)
            {
                if (x<resolution*2)
                {
                    if ( x%2==0 ){           
                        lsb = serial_buffer[step];
                    }

                    if ( x%2==1 ){
                        msb = serial_buffer[step];            
                        //printf( "msb %i lsb %i \n", msb , lsb );

                        bufferPt[count] = (msb << 8 ) | (lsb & 0xff);                         
                        //printf("val %i \n ", bufferPt[count]);
                        count++;
                    }
                }//strip off terminator
                step++;//=step+step_amt;
            }//x
       }//y
   }//2d mode   

}




