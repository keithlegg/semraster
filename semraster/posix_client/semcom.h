#ifndef SEMCOM_H    
#define SEMCOM_H


//10Bit SEM serial buffer

typedef struct{
    unsigned char b;
}SEM_Serial;

//SET UP SERIAL BUFFERS 
//SEM_Serial* create_sem_SerialBuffer(int size);
unsigned char* create_sem_SerialBuffer(int size);
int* create_sem_PixelBuffer(int size);
void reverse_sem_PixelBuffer(int* buffer, int totalsize);


//CORE SERIAL FUNCTIONS  
void do_flush( unsigned char *serial_buffer, char *SERIAL_PORT);

void sem_com_notimeout( unsigned char *serial_buffer, int readsize, 
                        char *SERIAL_PORT, char *command );

void sem_com( unsigned char *serial_buffer, int readsize, 
              char *SERIAL_PORT, char *command );

void sem_send( char *SERIAL_PORT, char *command );


// these tools use the above functions to operate

void sc_step_resolution(void);
int sc_get_resolution(void);

void sc_scan_image(char *filename);
void sc_run_fullscan(int *buffer, int resolution);

void sc_receive8_test(void);
void sc_get_h_receive10_test(int *buffer, int resolution);
void sc_get_v_sweep(int *buffer, int resolution);
void sc_get_h_sweep(int *buffer, int resolution);


//TESTING DEBUGGING TOOLS 
//generate fake scan data for testing or when serial is offline
void fake_scan_rawdata(unsigned char *serBuffPt, int resolution);
void fake_scan_data(int *bufferPt, int resolution, int use_2d_mode);
void run_10bittest(int resolution);
void run_scan_dump(int resolution);



#endif