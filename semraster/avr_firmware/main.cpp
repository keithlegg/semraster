/*
   Firmware for Scanning Electron Micorscope
   Keith Legg Sep 2013- May 2015

   Sends data to an MCP DAC chip to adjust the electron XY beams
   reads an ADC value from the sensor in 10 bit resolution  
   sends each value (pixel) as 2 bytes over UART in little-endian 
   that re-assembles an image from the two bytes per pixel.

   Developed on an ATMega328p 

  ################################

   NOTES - 

       python ORD() may not process bytes the same way termios on C does.
       in some cases a "10" may trigger a false end of line (and I think it has)
       oddly python seems to work fine 
*/


#define F_CPU 16000000UL

#include <avr/io.h>
//#include <avr/wdt.h> //watchdog for reset cmd
#include <util/delay.h>

#include <avr/sfr_defs.h>
#include <avr/interrupt.h>

#include "SPI.h"
#include "SPI.cpp"

#include "DAC_MCP49xx.h"
#include "DAC_MCP49xx.cpp"



//UART STUFF
#define FOSC 16000000UL

#define BAUD 57600

#define MYUBRR FOSC/16/BAUD-1

/***********************/
//pin assigments 
#define SS_PIN 10
#define LDAC_PIN 7
//RGB LED ON PORTD        
#define LED1 3 // green 
#define LED2 4 // red
#define LED3 5 // blue

/***********************/

//uart commands 
#define SET_RES 0x61          //a   
#define SET_STEP_SIZE 0x62    //b 
#define SET_START_X 0x63      //c  
#define SET_START_Y 0x64      //d
#define SET_END_X 0x65        //e 
#define SET_END_Y 0x66        //f 
#define CMD_TEST_ADC 0x67     //g 
#define CMD_TEST_XFER8 0x68   //h 
#define CMD_TEST_XFER10 0x69  //i 
#define CMD_TEST_FSCAN 0x6a   //j 

//#define CMD_NXT_PXL 0x74    //??

#define CMD_ECHO 0x71         //q
#define CMD_RST_SCAN 0x72     //r 
#define CMD_START_SCAN 0x73   //s 

#define CMD_START_HSCAN 0x75  //u 
#define CMD_START_VSCAN 0x76  //v 
#define CMD_SHOW_RPT 0x77     //w 
#define CMD_CAL_HSCOPE 0x78   //x 
#define CMD_CAL_VSCOPE 0x79   //y 
#define CMD_ASK_STEPSIZ 0x7a  //z 

/***********************/

#define BIT_ON 0x30 //logic high
#define BIT_OFF 0x31 //logic low

#define CHAR_TERM 0xa //terminator byte - numeric "10" - ascii newline

/***********************************************/

//scan region settings
uint8_t tl_x = 0;
uint8_t tl_y = 0;
uint8_t br_x = 64;
uint8_t br_y = 64;

uint8_t step_size = 0; //1=1024,2=512,4=256,8=128,16=64,32=32,64=16,128=8
uint8_t siz_idx   = 0;
   

/***********************************************/

// page 183 of datasheet
void USART_Init( unsigned int ubrr)
{
    UBRR0H = (unsigned char)(ubrr>>8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);
}


//*******************************************//

//TEST DATA/PIXEL TRANMISSION 
void test_8bit_xfer(void)
{
    PORTD = 0x00;     
    PORTD = (1<<LED3);//turn on blue

    USART_Init(MYUBRR);

    int x = 0;
    int y = 0;
  
    //in this case step_size is NOT A STEP but the RESOLUTION
    for(x=0;x<256;x++)
    {
        if (x<=256){
            while ( !( UCSR0A & (1<<UDRE0)) );
            UDR0 = x; 
            PORTD ^= (1<<LED2); //XOR red = purple (mixed with blue) 
        }
    }
    
    //send newline signaling this row is done scanning
    while ( !( UCSR0A & (1<<UDRE0)) );
    UDR0 = CHAR_TERM;
    PORTD = 0x00;//leds off
}

/************************/

//TEST DATA/PIXEL TRANMISSION 
void test_10bit_xfer(void)
{
    PORTD = 0x00;     
    PORTD = (1<<LED3);//turn on blue

    USART_Init(MYUBRR);

    int x = 0;
    int y = 0;
    uint8_t low_byte = 0;
    uint8_t high_byte = 0;

    //in this case step_size is NOT A STEP but the RESOLUTION
    for(x=0;x<1024;x=x+step_size)
    {
        if(x<256){
            //lsb first- we send TWO bytes for each pixel 
            while ( !( UCSR0A & (1<<UDRE0)) );
            UDR0 = x; 
            //msb last
            while ( !( UCSR0A & (1<<UDRE0)) );
            UDR0 = 0; 
        }        
        if(x>=256){
            //high byte, shift all bits 8 places right
            high_byte = (uint8_t)(x >> 8);
            //low byte, clear the high byte
            low_byte = (uint8_t)(x & 0x00FF);

            while ( !( UCSR0A & (1<<UDRE0)) );
            UDR0 = low_byte; 
            while ( !( UCSR0A & (1<<UDRE0)) );
            UDR0 = high_byte; 
           
        }

        PORTD ^= (1<<LED2); //XOR red = purple (mixed with blue) 
    }
    
    //debug - not sure this is even right - may get rid of this 
    //send newline signaling this row is done scanning
    while ( !( UCSR0A & (1<<UDRE0)) );
    UDR0 = CHAR_TERM;
    
    PORTD = 0x00;//leds off

}


//*******************************************//
/*
   dry run scan - iterate the positions and send coordinates over uart 
   does not move XY beams or run ADC
*/

void full_scan_test(uint8_t step_size)
{
     PORTD = 0x00;       //clear leds  
     PORTD |= (1<<LED3); //blue led on 

     int a = 0;
     int b = 0;
     uint8_t low_byte = 0;
     uint8_t high_byte = 0;

     //image resolution = 1024/step_size
     for (a=0;a<1024;a=a+step_size)
     {
         for (b=0;b<1024;b=b+step_size)
         {
             if(b<256){
                  //lsb first- we send TWO bytes for each pixel 
                  while ( !( UCSR0A & (1<<UDRE0)) );
                  UDR0 = b; 
                  //msb last
                  while ( !( UCSR0A & (1<<UDRE0)) );
                  UDR0 = 0; 
             }        
             if(b>=256){
                  //high byte, shift all bits 8 places right
                  high_byte = (uint8_t)(b >> 8);
                  //low byte, clear the high byte
                  low_byte = (uint8_t)(b & 0x00FF);

                  while ( !( UCSR0A & (1<<UDRE0)) );
                  UDR0 = low_byte; 
                  while ( !( UCSR0A & (1<<UDRE0)) );
                  UDR0 = high_byte; 
                 
             }
             PORTD ^= (1<<LED2); //XOR red = purple (mixed with blue) 
         }//B-Y
         
         //send newline signaling this row is done scanning
         while ( !( UCSR0A & (1<<UDRE0)) );
         UDR0 = CHAR_TERM; 

     }//A-X

     PORTD = 0x00;  //LEDS OFF 
}


/***********************************************/

static uint8_t USART_receive(void)
{
    while (!(UCSR0A & (1 << RXC0))) {/*Busy wait.*/}
    return UDR0;
}

/***********************************************/

void USART_Transmit( unsigned char data )
{
  while ( !( UCSR0A & (1<<UDRE0)) );
  UDR0 = data;
}

/***********************************************/

void print_byte( uint8_t data){
   uint8_t i = 0;

   for (i=0; i<=7; i++) {
       //if ( !!(data & (1 << ii)) ){  // LSB
       if ( !!(data & (1 << (7 - i))) ){  // MSB
           USART_Transmit( BIT_OFF );
       }else{
           USART_Transmit( BIT_ON );
       }
    }
}

/***********************************************/
void print_byte_16( uint16_t data){
   uint8_t i = 0;

   for (i=0; i<=15; i++) {
       //if ( !!(data & (1 << ii)) ){  // LSB
       if ( !!(data & (1 << (15 - i))) ){  // MSB
           USART_Transmit( BIT_OFF );
       }else{
           USART_Transmit( BIT_ON );
       }
    }
}

/***********************************************/
//debug experimental - cant seem to make this work!
void USART_tx_string( char data[]  )
{
    int i = 0; 
    while(data[i] != '\0')
    {
        UDR0 = data[i++];
        while (!(UCSR0A & (1 <<UDRE0)));
    }

    while (!(UCSR0A & (1 <<UDRE0)));
    UDR0 = '\0';
}

//*******************************************//
//debug experimental 
void break_connection(){
    PORTD = 0x00;    //clear port  (LEDS) 
    USART_Transmit( 0x45 ); //E
    USART_Transmit( 0x78 ); //x
    USART_Transmit( 0x69 ); //i
    USART_Transmit( 0x54 ); //T
}

//*******************************************//
void echo_uart(){
    uint8_t buf = USART_receive();
    print_byte(buf);
}

//*******************************************//
/*
  ADPS (ADC Prescaler Select ) bits : These bits determine the division factor between the AVR clock frequency and the ADC clock frequency. 
*/
void ADC_init()
{
    //ADC clock = System Clock / System Clock Prescaler / ADC Prescaler.
    //VREFF needs to be set? NOT off of lm7805
    ADMUX = 0x00;   
    ADMUX |= (1 << REFS0);  // For Aref=AVcc;  
    ADMUX &= ~(1 << ADLAR);  
    //ADCSRA=(1<<ADPS1);  //Prescalar div factor =4
    ADCSRA|= (1 << ADPS0)|(1 << ADPS1); //Prescalar=8 - this works 
    //  //ADCSRA=(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);  //Prescalar div factor =128
    ADCSRA |= (1<<ADIE);//activate adc interrupts 
    ADCSRA |= (1 << ADEN); // (ADC Free Running Select) bit : If you want to use the Free Running Mode, 
                           // you must set this bit.
}

//*******************************************//
int ADCsingleREAD(uint8_t ch)
{
    uint16_t ADCval;
    ADCSRA |= (1 << ADEN);  
    ADCSRA |= (1 << ADSC);
    while(ADCSRA & (1 << ADSC));          
    ADCval = ADCL;
    ADCval = (ADCH << 8) + ADCval; 
    return ADCval;
}

//*******************************************//
//  scan only a single row horizontally down the middle
void vert_scan(uint8_t step_size)
{
     //TURN ON LED WHEN SCANNING
     PORTD = 0x00;       //clear leds  
     PORTD |= (1<<LED3); //blue led on 

     int a = 0;
     DAC_MCP49xx dac(DAC_MCP49xx::MCP4912, SS_PIN, LDAC_PIN);
     dac.setPortWrite(true);
     dac.setAutomaticallyLatchDual(true);

     //start beams to center XY  
     dac.outputA(512);
     dac.outputB(512);
     dac.latch(); 

     //image resolution = 1024/step_size
     for (a=0;a<1024;a=a+step_size)
     {
         dac.outputB(a);
         dac.latch(); 
         //start ADC for this pixel, let the ISR do the rest.
         ADCSRA |= (1 << ADSC);
     }
    
     //return beams to center XY  
     dac.outputA(512);
     dac.outputB(512);
     dac.latch(); 

     PORTD = 0x00;  //LEDS OFF 
}

//*******************************************//
// scan only a single row horizontally down the middle
void horiz_scan(uint8_t step_size)
{
     //TURN ON LED WHEN SCANNING
     PORTD = 0x00;       //clear leds  
     PORTD |= (1<<LED3); //blue led on 

     int a = 0;
     DAC_MCP49xx dac(DAC_MCP49xx::MCP4912, SS_PIN, LDAC_PIN);
     dac.setPortWrite(true);
     dac.setAutomaticallyLatchDual(true);

     //start beams to center XY  
     dac.outputA(512);
     dac.outputB(512);
     dac.latch(); 

     //image resolution = 1024/step_size
     for (a=0;a<1024;a=a+step_size)
     {
         dac.outputA(a);
         dac.latch(); 
         //start ADC for this pixel, let the ISR do the rest.
         ADCSRA |= (1 << ADSC);
     }
    
     //return beams to center XY  
     dac.outputA(512);
     dac.outputB(512);
     dac.latch(); 

     PORTD = 0x00;  //LEDS OFF 
}


//*******************************************//

/*
  return a stack of pixel rows terminated by "0x0a"  to signal end of transmition
*/
void full_scan(uint8_t step_size)
{
     //TURN ON LED WHEN SCANNING
     PORTD = 0x00;       //clear leds  
     PORTD |= (1<<LED3); //blue led on 

     int a = 0;
     int b = 0;

     DAC_MCP49xx dac(DAC_MCP49xx::MCP4912, SS_PIN, LDAC_PIN);
     dac.setPortWrite(true);
     dac.setAutomaticallyLatchDual(true);

     //image resolution = 1024/step_size
     for (a=0;a<1024;a=a+step_size)
     {
         for (b=0;b<1024;b=b+step_size)
         {
             dac.outputA(a);
             dac.outputB(b);
             dac.latch(); 
             //start ADC for this pixel, let the ISR do the rest.
             ADCSRA |= (1 << ADSC);

         }//B-Y
         
         //send newline signaling this row is done scanning
         while ( !( UCSR0A & (1<<UDRE0)) );
         UDR0 = CHAR_TERM; 

     }//A-X
    
     //return beams to center XY  
     dac.outputA(512);
     dac.outputB(512);
     dac.latch(); 

     PORTD = 0x00;  //LEDS OFF 
}

//*******************************************//
/*
  blink leds to indicate a scan is about to begin
*/
void scan_flashy()
{
   PORTD = 0x00;       //clear port first  
   short dly = 50;
   short num_flash = 3;

   int a = 0;
   for (a=0;a<num_flash;a++)
   {
       PORTD = (1<<LED3); //Blue
       _delay_ms(dly); 
       
       PORTD = 0x00;       //clear port first  
       PORTD = (1<<LED2); 
       _delay_ms(dly); 

       PORTD = 0x00; 
       PORTD = (1<<LED3); 
       _delay_ms(dly); 

       PORTD = 0x00;       //clear port first  
       PORTD = (1<<LED2); 
       _delay_ms(dly); 
   }
}

//*******************************************//
/*
  blink leds to indicate SEM is alive and ready
  (this is the 2.0 firmware blink pattern)
*/
void init_flashy()
{
   PORTD = 0x00;       //clear port first  
   short dly       = 20;
   short num_flash = 5;

   int a = 0;
   for (a=0;a<num_flash;a++)
   {
       PORTD = (1<<LED1);  //green
       _delay_ms(dly); 
       
       PORTD = 0x00;     
       PORTD = (1<<LED3);  //1 - green, 2- Red , 3 - Blue
       _delay_ms(dly*2); 
     
       PORTD  = 0x00;     
       PORTD  = (1<<LED3); 
       PORTD |= (1<<LED2);      
       _delay_ms(dly); 
      
       PORTD = (1<<LED1);  //green
       _delay_ms(dly); 
       
       PORTD = 0x00;     
       PORTD = (1<<LED3);  //1 - green, 2- Red , 3 - Blue
       _delay_ms(dly*2); 
     
       PORTD  = 0x00;     
       PORTD = (1<<LED1);    
       _delay_ms(dly); 
   }
}

//*******************************************//
/*
  need to do a lot more with this - feedback that the software can 
  auto adjust step size internally client side, etc 
*/
void shw_report()
{
     USART_Transmit(0x73);//s
     USART_Transmit(0x74);//t
     USART_Transmit(0x65);//e
     USART_Transmit(0x70);//p
     
     USART_Transmit(0x20);//space

     print_byte(step_size);
     USART_Transmit(0x20);//space         

     USART_Transmit(0x74);//t
     USART_Transmit(0x6c);//l
     USART_Transmit(0x20);//space
     
     print_byte(tl_x);
     USART_Transmit(0x20);//space         

     USART_Transmit(0x74);//t
     USART_Transmit(0x72);//r
     USART_Transmit(0x20);//space

     print_byte(tl_y);
     USART_Transmit( 0xd ); //CR

     //USART_Transmit( 0xd ); //0xd = carriage return
     //send newline byte signaling we are done 
     while ( !( UCSR0A & (1<<UDRE0)) );
     UDR0 = CHAR_TERM;

}

//*******************************************//
//tell client what resolution you are set to!
void shw_stepsize()
{
    print_byte(step_size);
}

//*******************************************//

int main (void)
{
    DDRD |= (1<<LDAC_PIN)|(1<<LED1)|(1<<LED2)|(1<<LED3);  

    ADC_init();
    USART_Init(MYUBRR);

    sei();
    init_flashy(); //tell operator you are booted up and listening for commands

    //default to 256 square resolution
    step_size = 4; //1=1024,2=512,4=256,8=128,16=64,32=32,64=16,128=8
    siz_idx   = 3;

    while(1)
    {
        //listen for a command over UART  
        while (!(UCSR0A & (1 << RXC0))) {}//Busy wait.

        //perhaps a small delay before responding???
        //_delay_us(800);

        switch(UDR0)
        {
            case CMD_ECHO: //q
                PORTD = 0x00;
                _delay_us(300);

                //USART_tx_string("SEM test of string print.");
                PORTD |= (1<<LED1); //1 - green, 2- Red , 3 - Blue

                echo_uart();
                USART_Transmit( 0x0d ); // CR          
                USART_Transmit( 0x0a ); // \n
                 
                PORTD = 0x00;
            break;

            case CMD_START_SCAN: //s
                //scan_flashy();  //DOH! takes too much time in video mode 
                PORTD = 0x00;           
                PORTD |= (1<<LED3); //Blue
                full_scan(step_size);// 1=1024,2=512,4=256,8=128,32=32,64=16,128=8 //
            break;
   
            case CMD_START_HSCAN: //u
                 PORTD = 0x00;           
                 PORTD |= (1<<LED3); //Blue
                 horiz_scan(step_size);// 1=1024,2=512,4=256,8=128,32=32,64=16,128=8 //
            break;

            case CMD_START_VSCAN: //v
                PORTD = 0x00;           
                PORTD |= (1<<LED3); //Blue
                vert_scan(step_size);// 1=1024,2=512,4=256,8=128,32=32,64=16,128=8 //
            break;

            case CMD_CAL_HSCOPE: //
              //    PORTD = 0x00;           
              //    PORTD |= (1<<LED3);  
            break;

            case CMD_CAL_VSCOPE: //
              //    PORTD = 0x00;           
              //    PORTD |= (1<<LED3);  
            break;

            case CMD_TEST_XFER8: //h - 8bit test of send/receive 
                test_8bit_xfer();
            break;

            case CMD_TEST_XFER10: //i - 10bit test of send/receive 
                test_10bit_xfer();
            break;

            case CMD_TEST_FSCAN: //j - test of sending a whole scan 
                full_scan_test(step_size);
            break;

            case CMD_RST_SCAN:
                 //experiment to hardware reset - not working 
                 //cli(); // disable interrupts
                 //wdt_enable(WDTO_15MS); // enable watchdog
                 //while(1); // wait for watchdog to reset processor
            break;

            case CMD_SHOW_RPT: //w
                shw_report();
            break;  

            case CMD_ASK_STEPSIZ: //z
                //_delay_us(300);            
                shw_stepsize();
            break; 

            case SET_RES: //a
                PORTD |= (1<<LED2); //Red 
            break;

            case SET_STEP_SIZE: //b
                //  step  = buffer 
                //  1 - 1   =  1024
                //  2 - 2   =  512
                //  3 - 4   =  256
                //  4 - 8   =  128
                //  5 - 16  =  64                
                //  6 - 32  =  32
                //  7 - 64  =  16
                //  8 - 128 =  8 

                siz_idx++;

                if (siz_idx==1){
                    step_size = 1;
                }
                if (siz_idx==2){
                    step_size = 2;
                }
                if (siz_idx==3){
                    step_size = 4;
                }

                if (siz_idx==4){
                   step_size = 8;
                }
                if (siz_idx==5){
                    step_size = 16;
                }
                if (siz_idx==6){
                    step_size = 32;
                }
                if (siz_idx==7){
                    step_size = 64;
                }
                
                if (siz_idx==8){
                    siz_idx=0;
                    step_size = 128;            
                }
            break;
        
            case  CMD_TEST_ADC: //g
                ADCSRA |= (1 << ADSC); //trigger the adc
            break;

            case SET_START_X:
                //tl_x = 128; 
            break;

            case SET_START_Y: 
                 //tl_y = 128; 
            break;

            case SET_END_X: 
                 //br_x = 512; 
            break;

            case SET_END_Y: 
            break;
        }//switch    

        //heartbeat
        PORTD = 0x00;
        PORTD |= (1<<LED2);  
        _delay_ms(20);
        PORTD = 0x00;
    }//while

}//main

/*********************************************/

/* 
  Interrupt to send 2 bytes over serial port when ADC conversion is finished 
  this is a 10-bit number split in two 8 bit blocks. 
*/

//ISR for when ADC is complete - send two bytes little endian
ISR(ADC_vect) {
    PORTD |= (1<<LED2); //Red
      
    while ( !( UCSR0A & (1<<UDRE0)) );
    UDR0 = ADCL; 
    while ( !( UCSR0A & (1<<UDRE0)) );
    UDR0 = ADCH;           

    PORTD ^= (1<<LED2); //XOR red = purple (mixed with blue) 
}


/***********************************************/
/***********************************************/
/***********************************************/
//debug experimental

uint8_t* hex_decode(const char *in, size_t len,uint8_t *out)
{
    unsigned int i, t, hn, ln;
    for (t = 0,i = 0; i < len; i+=2,++t) 
    {
        hn = in[i] > '9' ? in[i] - 'A' + 10 : in[i] - '0';
        ln = in[i+1] > '9' ? in[i+1] - 'A' + 10 : in[i+1] - '0';
        out[t] = (hn << 4 ) | ln;
    }
    return out;
}

/***********************************************/
//debug experimental 
char gethexnib(void) {
  char a;
  a = USART_receive(); USART_Transmit(a);
  if(a >= 'a') {
      return (a - 'a' + 0x0a);
  } else if(a >= '0') {
      return(a - '0');
  }
  return a;
}

/***********************************************/
//debug experimental 
char gethex(void) {
  return (gethexnib() << 4) + gethexnib();
}

/***********************************************/
//debug experimental 
void puthex(char ch) {
  char ah;

  ah = ch >> 4;
  if(ah >= 0x0a) {
      ah = ah - 0x0a + 'a';
  } else {
      ah += '0';
  }
  
  ch &= 0x0f;
  if(ch >= 0x0a) {
      ch = ch - 0x0a + 'a';
  } else {
      ch += '0';
  }
  
  USART_Transmit(ah);
  USART_Transmit(ch);
}




