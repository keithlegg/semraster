
"""
   Interface to SEM prototype via pyserial.
   requires: pyserial and PIL

   Author    - Keith Legg, 
   Contact   - perihelionvfx@gmail.com 
   Created   - October 12, 2014  
   Modified  - Jan     16, 2015
   Modified  - March   21, 2015
   Modified  - May     10, 2015

"""

import os
import sys

import serial
import time

# # # # #

#python2 use this 
#import Image 
#python3 use this  
from PIL import Image 


# Mac OS: type 'ls /dev/tty.*' to see the serial ports,
SERIAL_PORT = '' #'/dev/tty.usbserial-FTWWRSIBA'  # '/dev/tty.usbserial-A400gnmx'  # 
OUTFILE_DIRECTORY = '/keith' # '/Users/klegg' # 
CONFIG_FILE = 'sem.cfg'  


RX_BUFFER = []


class device(object):
    """ interface class to communicate with SEM device via serial port 
        the following commands are recognized by the firmware:

        SET_RES        0x61 //a   
        SET_STEP_SIZE  0x62 //b 
        SET_START_X    0x63 //c  
        SET_START_Y    0x64 //d
        SET_END_X      0x65 //e 
        SET_END_Y      0x66 //f 
        CMD_ECHO       0x71 //q
        CMD_SHOW_RPT   0x77 //w 
        CMD_RST_SCAN   0x72 //r 
        CMD_START_SCAN 0x73 //s 
        CMD_TEST_ADC   0x67 //g 
        CMD_TEST_XFER  0x74 //t 

    """

    def __init__(self, dryrun=False):
        #you can use pyserial OR semraster to interface
        self.PYSERIAL       = None  #python pyserial 
        #self.SEMRASTER      = None  #custom C SEM API 
           ## ## ## ##   
        self.SERIAL_PORT    = None  #'/dev/tty.usbserial' 

        self.COLOR_MODE     = 'clamped' #'clamped' # falsergb , 
        self.TERMINATOR     = 0xb #*** suspect this does not matter as long as its not a number***
 
        self.longedge       = 0
        self.im_type        = 'png' #'bmp' #'png'        
        self.filename       = 'image'
        self.bfilename      = 'dump.bin'
        self.tfilename      = 'dump.txt'

        self.pic_count      = 0
        self.IMAGE_BUFFER   = [] #array of rows [[row], [row]]
        self.pix_vals       = []

        #self.step_sizes     = [1024,512,256,128,64,32,16,8] #[8,16,32,64,128,256,512,1024]
        self.step_idx       = 2

        self.load_config()
        #########################        
        #dryrun lets you use this class without connecting to a serial port
        if not dryrun:
            self.initsem()
    
    def scribe(self, instr):
        print(instr)

    def load_config(self):
        """ load a config file - mostly for setting the 
            serial port properties 
        """ 
        if not os.path.isfile(CONFIG_FILE): 
            sys.exit('\n\n Error opening %s/%s \n\n'%(os.getcwd(),CONFIG_FILE) )
             
        else:    
            f = open(CONFIG_FILE, 'r')
            for line in f:
               toked = line.split(' ')          
               if toked[0]=='serial_port':
                   print '#setting serial port to %s'%toked[1] 
                   self.set_device(toked[1].replace('\n',''))
               

    def initsem(self):
        """ device constructor 
            need to add feature that detects if serial port is offline or has changed         
            automatically connects to serial port - you can bypass
            by sending "True" when you instantiate this class 
        """
        self.PYSERIAL = serial.Serial(port=self.SERIAL_PORT,
                                           baudrate=57600,
                                           timeout=1)
        self.set_res()
        time.sleep(0.1)
        self.PYSERIAL.flush()
        self.PYSERIAL.flushInput()
        self.autoset_res()        
    ##----------------------##
    def shutdown(self):
        self.clear_buffers()
        self.PYSERIAL.close()
    ##----------------------##
    def tx_char(self, char):
        """ send one byte to SEM 
            need to add feature that detects if serial port is offline or has changed 
        """

        self.PYSERIAL.write( char )
    ##----------------------##
    def read_fixed_buffer(self, bufsize):
        return self.PYSERIAL.read(bufsize)        
    ##----------------------##
    def read_stream_in(self):
        """ readline seems bonky,numeric 10 character 
            can be mistaken for newline 
        """
        bufr = self.PYSERIAL.readline()
        bufr = bufr[:-1] #remove the newline
        return bufr
    ##----------------------##
    def set_device(self, devtouse=SERIAL_PORT):
        self.SERIAL_PORT = devtouse 
     
    ##----------------------##
    def set_colormode(self, colormode=None):
        if colormode!=None:
            self.COLOR_MODE = colormode  
    ##----------------------##
    def set_res(self, restouse=256):
        self.longedge = restouse 
    ##----------------------##
    def autoset_res(self):
        self.tx_char("z") 
        stepres = str(self.read_fixed_buffer(8) )
        res = 256
        if stepres=='00000001':
            res = 1024
        if stepres=='00000010':
            res = 512
        if stepres=='00000100':
            res = 256
        if stepres=='00001000':
            res = 128   
        if stepres=='00010000':
            res = 64 
        if stepres=='00100000':
            res = 32 
        if stepres=='01000000':
            res = 16  
        if stepres=='10000000':
            res = 8                                                                     
        #### 
        print '##Detected scan resolution: %s %s'%(stepres,res) 
        self.longedge = int(res) 

    ##----------------------##
    def clear_buffers(self):
        RX_BUFFER          = [] # raw serial buffer for one row (null terminated)
        self.IMAGE_BUFFER  = [] #array of rows [ [row], [row] ]
        self.pix_vals      = [] #array of rows of 10 bit (stored as 16 bit) digitized data [ [0-1024], [0-1024] ]
        
        self.PYSERIAL.flushInput()
        self.PYSERIAL.flush()
    ##----------------------##    
    def advance_stepsize (self):
        self.step_idx+=1
        if self.step_idx==8:
            self.step_idx=0
        self.tx_char('b')
        self.autoset_res()
    ##----------------------##
    def show(self):
        """ ask SEM to report status , including internal settings """
        self.scribe( 'serial port is %s'%self.SERIAL_PORT ) 
        self.PYSERIAL.write( "w"  )
        self.scribe( self.read_stream_in() )#read until done (\0)         
        #self.scribe( 'expected image size : '+str(self.step_sizes[self.step_idx]) )
        self.scribe( 'color mode is       : '+str(self.COLOR_MODE) )
        self.scribe( 'resolution is set to %s'%self.longedge )

    ##----------------------##
    def test_for_echo(self, char):
        """
           debugging tool to test send/recive. char is an ascii value to send to SEM
           SEM will return a byte of the value in eight bits  ("e"="01100101") 
        """

        self.PYSERIAL.write( "q"  )
        self.PYSERIAL.write( char )
        self.scribe( self.read_stream_in() )#read until done (\0) 

    ##----------------------##
    def full_scan(self, testmode=False):
        """
           initiate a "full scan" mode to the device - read back what it returns and store it in an array.
        """
        self.autoset_res()
        self.clear_buffers()
        #this is the whole shebang 
        if not testmode:
            self.PYSERIAL.write("s")
        #testmode returns the same amount of data, an index of the loop  
        if testmode:
            self.PYSERIAL.write("j")            
        for r in range(0,self.longedge):
            RX_BUFFER = self.PYSERIAL.readline()
            RX_BUFFER = RX_BUFFER[:-1] #remove the newline
            self.IMAGE_BUFFER.append(RX_BUFFER) #add row to image
        self.scribe( '#received '+str(len(self.IMAGE_BUFFER))+' rows of pixels \n' )
    ##----------------------##
    def process(self, dodump=None):
        """
            stitch the serialized MSB and LSB (8 bit bytes) into a sequence of 16bit values 
        """
        lowbyte  = 0 #lsb
        highbyte = 0 #msb
        voltage  = 0 #10 bit value (two bytes assembled from uart )
        ##
        dumpbuffer = []
        dumpcount = 0
        rowcount = 0 

        for row in self.IMAGE_BUFFER:
            row_vals = []
            count=0   
            rowcount=rowcount+1 
            for pix in row:
                if count==0:
                    lowbyte =( ord(pix) )
                if count%2==0:
                    lowbyte =( ord(pix) )
                #only capture on ODD bit so we have low and high 
                if count%2:
                    highbyte =( ord(pix) ) 
                    if lowbyte>=256:
                        highbyte=highbyte-1      
                    voltage = (highbyte*256)+lowbyte 
                    row_vals.append(voltage)  
                    # # # # # 
                    if dodump:
                        dumpbuffer.append( "count:%s  rawpix %s high %s low %s- output %s" % 
                            (str(count).zfill(4), str(ord(pix)).zfill(4), str(highbyte).zfill(4), str(lowbyte).zfill(4), voltage) )
           
                count=count+1
            if dodump:
                dumpbuffer.append('\n') 

            #print('process : row length ', row_vals[1], len(row_vals) )
            self.pix_vals.append(row_vals) #sorted buffer 
        
        #################################
        #raw dump of high and low bytes sent over for debugging
        if dodump:
            self.dump_buffer_text( (OUTFILE_DIRECTORY+'/pixeldumpraw.txt'),
                                    dumpbuffer)
            self.scribe( ('converted ', len(self.pix_vals) , 'rows , row size: ', len(self.pix_vals[1])) )
    ##----------------------##
    def dump_buffer_text(self, 
                         filename=('/pixeldump.txt'),
                         data=[]):
        print('writing file %s'%filename)
        newFile = open (filename, "w")
        for r in data:
            newFile.write( "%s %s" % (r, '\n') )  
        newFile.close()
    ##----------------------##
    def dump_buffer_binary(self, sembinary, res):
        """ for processing the binary file data dump from semraster """
        
        self.longedge = res #store for writing framebuffer 
        numpixels = ((res*2)+1)*(res)
        self.scribe( '\n' )  
        self.scribe( '# resolution (square): %s'% res )
        self.scribe( '# number of expected pixels is: %s'% numpixels )
        rowcnt = 0
        pxlcnt = 0

        self.IMAGE_BUFFER =[] #clear old image out
        RX_BUFFER = []

        f = open(sembinary, "rb")
        try:
            byte = f.read(1)
            while byte != "":
                byte = f.read(1)
                if byte:
                    #debug - not sure this terminator char is a good idea - may rip it out
                    if ord(byte)!=self.TERMINATOR:
                        RX_BUFFER.append(byte)              
                    if ord(byte)==self.TERMINATOR:
                        self.IMAGE_BUFFER.append(RX_BUFFER)  
                        rowcnt+=1
                        RX_BUFFER = []
                        ##
                    pxlcnt+=1    
            ##      
            #print '#received '+str(len(self.IMAGE_BUFFER))+' rows of pixels \n'
            self.scribe( '# binary contains %s rows.  '%(rowcnt+1) )
            self.scribe( '# binary contains %s pixels.'%(pxlcnt+1) ) 
        finally:
            f.close()
    ##----------------------##
    def write_raw_binary(self):
        """ debugging tool
            write image data to a pure binary file for debugging  
        """
        fname = (OUTFILE_DIRECTORY+'/'+ self.bfilename ) 
        newFile = open (fname, "wb")
        self.scribe( 'writing file %s\n'%fname )
        for byte in self.IMAGE_BUFFER:
            self.scribe( ord(byte) ) 
            #newFile.write( "%s\n" % byte )  #byte as text 
            newFile.write( byte )            #binary             
        newFile.close()
    ##----------------------##
    def write_framebuffer(self, colormode=None):
        """ take internal array of 10 bit interger values and 
            try to make a pretty picture out of them
            there is some difficulty in the mapping of 1024->255
        """ 
        if colormode!=None:
            self.COLOR_MODE = colormode

        #write image data to an image file   
        pilObj = Image.new('RGB', (self.longedge, self.longedge) ) 
        dpix   = pilObj.load()

        y=0
        for row in self.pix_vals:
            x=0     
            for pix in row:
        
                #############
                #colormodes go here 
                #############
                if self.COLOR_MODE=='clamped':
                    clval = int(pix/4)# clval = max(0, min( (pix/4), 255))
                    if clval>255:
                        clval=255
                    if clval<0:
                        clval=0
                    if pix==None:
                        clval=0   
                    #print '----------> pix x y cval dpix', pix, x, y, clval, dpix
                    dpix[ x, y ] = ( clval, clval, clval ) #clamped 
                #############                
                if self.COLOR_MODE=='falsergb':
                    dpix[ x, y ] = ( pix>>2, pix>>4, pix>>8 ) #RGB 
                
                #colormodes go here 
                #############
                x+=1 
            #    
            y+=1
        ########################    
        fnam = OUTFILE_DIRECTORY+'/'+str(self.pic_count).zfill(4)+'_'+self.filename+'.'+self.im_type 
        self.scribe( '# Saving image file: %s'%fnam ) 
        pilObj.save( fnam )
        
        self.pic_count+=1

    ##----------------------##
    def receive_terminated_stream(self):
        """ debugging tool """
        self.clear_buffers()
        self.PYSERIAL.write("t")
        self.IMAGE_BUFFER = self.PYSERIAL.readline()
        self.scribe( '#received %s bytes \n'%str(len(self.IMAGE_BUFFER)) )
    ##----------------------##
    def receive_stream_data(self):
        """ debugging tool """
        self.clear_buffers()
        self.PYSERIAL.write("t")
        for b in range(256):
             self.IMAGE_BUFFER.append(self.PYSERIAL.read() )
        self.scribe( '#received %s bytes \n'%str(len(self.IMAGE_BUFFER)) )
    ##----------------------##
    def test_uart_tx_rx8(self):
        """ debugging tool
            read 8bit (0-255) interger tansmition from SEM 
        """

        self.clear_buffers()
        self.PYSERIAL.write( "h"  )
        buffr_data = self.read_fixed_buffer(256)
        for c in buffr_data:
            self.scribe( ('#-> ',c, ord(c) ) )  

    ##----------------------##
    def test_uart_tx_rx10(self):
        """ debugging tool
            read 10bit (0-1024) interger tansmition from SEM 
        """

        self.clear_buffers()
        self.PYSERIAL.write( "i"  )
        
        #buffr_data = self.read_stream_in()
        buffr_data = self.read_fixed_buffer(256)
        for c in buffr_data:
            self.scribe( ('#-> ',c, ord(c) ) )  

    ##----------------------##
    def receive_sample_data_2d(self):
        """ debugging tool 
            send a 2D sequence of bytes 
            from the hardware and write them to a file client side 
        """
        self.clear_buffers()
        self.PYSERIAL.write("t")
        for r in range(0,self.longedge):
            RX_BUFFER =[]
            RX_BUFFER = self.PYSERIAL.readline()
            RX_BUFFER = RX_BUFFER[:-1] #remove the newline
            self.IMAGE_BUFFER.append(RX_BUFFER) #add row to image
        self.scribe( '#received %s rows of pixels \n'%str(len(self.IMAGE_BUFFER)) )

    ##----------------------##
    def generate_sample_data(self):
        """ debugging tool 
            play with pixel math , etc 
            I think it uses an as yet unbuilt testing tool on SEM 
            DEBUG UNFINISHED  
        """

        self.clear_buffers()
        imagesize = 256
        for rx in range(0,self.longedge):
            #self.IMAGE_BUFFER.append(r) 
            thisrow = [] 
            for ry in range(0,self.longedge): 
                thisrow.append(rx)
            self.pix_vals.append(thisrow)


#############################################
"""
testdev = device()
testdev.full_scan()
testdev.process()
testdev.write_framebuffer()
"""
