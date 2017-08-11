
/*
  #include "SPI.h"

  Arduino SPI for AVR
  tuned for atmega 328p 
  Ported By Keith Legg 

*/

SPIClass SPI;


void SPIClass::begin() {

  DDRB |= (1<<2)|(1<<3)|(1<<5);    // SCK, MOSI and SS as outputs
  DDRB &= ~(1<<4);                 // MISO as input
  SPCR |= (1<<MSTR);               // Set as Master
  //SPCR |= (1<<SPR0)|(1<<SPR1);     // divided clock by 128
  SPCR |= (1<<SPE); 

}

void SPIClass::end() {
  SPCR &= ~_BV(SPE);
}


void SPIClass::setBitOrder(uint8_t bitOrder)
{
  //if(bitOrder == LSBFIRST) {
    SPCR |= _BV(DORD);
  //} else {
  //  SPCR &= ~(_BV(DORD));
  //}
}




void SPIClass::setDataMode(uint8_t mode)
{
  SPCR = (SPCR & ~SPI_MODE_MASK) | mode;
}


void SPIClass::setClockDivider(uint8_t rate)
{
  SPCR = (SPCR & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK);
  SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK);
}






