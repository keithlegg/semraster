# semraster
experimental interface to a scanning electron microscope. 


The hardware is an FTDI 2232H breakout running as a simple serial port.
This is wired to a 328P chip (woefully underpowered for this application, I know) 
The 328P is wired to a DAC chip to drive the XY beams
The sensor on the SEM is read back in through the ADC.
When the ADC interrupt fires, it sends the 10 bit luminance value over the serial port as two bytes.

There is a python script that uses pyserial and PIL to read the bytes in pairs of two and assemble a picture.
There is also some experimental C code that opens a serial port, talks to the hardware and can read back the picture.
It uses Open GL to display raster data and graphics.

There are some goodies like an oscilscope view, and more.

