import sys
import sem_io


"""
TODO:
  ADD AUTO- DRYRUN MODE IF NO SERIAL CONNETION ESTABLISHED

#FIRMWARE COMMANDS
#a - send file
#b - nothing
#c - send byte stream
#d - advance next byte
#e - exit bytestrem mode

"""


##
class CLI(object):

    def __init__(self):
        self.SEM_IO      = sem_io.device()
        self.command     = ''
        self.split       = [] 
        self.lastcommand = ""
        ## ## ## 
        self.HISTORY     = [] #command history

    def cpr(self, strdat):
        print( str(strdat)+'\n' )

    def cpr_ns(self, strdat):
        print( strdat )  
    ##
    def sem_console(self):  

        while self.command !='quit':
            if self.command != 'help' :
                self.cpr_ns('\n\n')
            
            self.cpr('## ## ## ## ## ENTER A COMMAND ## ## ## ## ## ##')
            self.lastcommand = self.command
            
            #python2
            if sys.version_info[0] < 3:
                self.command = str(raw_input() )
            #python3
            else:
                self.command = str(input() )

            self.split   = self.command.split(" ")

            ###########
            if self.command == 'exit' or self.command == 'x': 
                break
            if self.split[0] == 'exit' or self.split[0] == 'x': 
                break

            if self.command == 'about' or self.split[0] == 'about': 
                self.cpr_ns( '--> Keith Legg, Perihelionvfx  2013-2015 <--')
            
            if self.split[0] == 'history' or self.command == 'history': 
                self.cpr_ns( self.HISTORY )
            else:
                self.HISTORY.append( str(self.command) ) 

            ########
            if self.command == 'help': 
                self.cpr_ns('## ## ## ## ##       HELP       ## ## ## ## ## ##')   
                self.cpr_ns('  set_colormode      - clamped, falsergb         ')
                self.cpr_ns('  step             - cycle image resolutions     ')
                self.cpr_ns('  scan             - runs a full scan            ')
                self.cpr_ns('  show             - view device internals       ')
                self.cpr_ns('  echo             - (single char)-test SEM I/O  ')
                self.cpr_ns('  ---------------------------------------------  ')                

                self.cpr('\n\n')

            ########
            if self.command == 'scan' : 
                self.SEM_IO.clear_buffers()
                self.SEM_IO.full_scan()
                self.SEM_IO.process()#dodump=True
                self.SEM_IO.write_framebuffer()
            ########
            if self.command == 'xferscan' : 
                self.SEM_IO.full_scan(True)
                self.SEM_IO.process(dodump=True)
                count = 0

            ###########
            if self.command == 'raw_highbytes' : 
                self.SEM_IO.clear_buffers()
                #self.SEM_IO.full_scan()
                self.SEM_IO.receive_sample_data()                
                self.SEM_IO.fb_write_high_bytes()

            ###########
            if self.command == 'raw_lowbytes' : 
                #self.SEM_IO.clear_buffers()
                #self.SEM_IO.full_scan()
                #self.SEM_IO.fb_write_low_bytes()                
                self.cpr_ns( 'disabled use highbytes instead.')

            ###########                          
            if self.command == 'testimg': 
                self.SEM_IO.clear_buffers()
                self.SEM_IO.receive_sample_data_2d()
                self.SEM_IO.process()
                self.SEM_IO.write_framebuffer()
            ###########   
            
            # DEBUG UNFINSHED        
            
            if self.command == 'xfer8': 
                self.SEM_IO.test_uart_tx_rx8()
            
            if self.command == 'xfer10': 
                self.SEM_IO.test_uart_tx_rx10()
   
            ###########             
            if self.command == 'testbin' : 
                """ DEBUG UNFINSHED """
                self.SEM_IO.clear_buffers()
                self.SEM_IO.receive_stream_data()
                #self.SEM_IO.process() 
                self.SEM_IO.write_raw_binary()
            ###########
            if self.command == 'genx' : 
                """ DEBUG UNFINSHED generate sample data gradient on X"""
                self.SEM_IO.clear_buffers()
                self.SEM_IO.generate_sample_data()
                self.SEM_IO.write_framebuffer()
            if self.command == 'geny' : 
                """ DEBUG UNFINSHED generate sample data gradient on Y"""                
                self.SEM_IO.clear_buffers()
                self.SEM_IO.generate_sample_data()
                self.SEM_IO.write_framebuffer()
            ###########
            if self.command == 'step' : 
                self.cpr_ns( '\n' )
                self.SEM_IO.advance_stepsize()
                self.SEM_IO.show()
            ###########
            if self.command == 'show' :
                self.SEM_IO.show()
            ###########
            if self.command == 'echo' :
                self.cpr('requires 1 argument')
            ###########                
            if self.split[0] == 'echo' : 
                self.cpr_ns( 'sending %s to device'%self.split[1] )
                self.SEM_IO.test_for_echo( self.split[1] )
            ###########
            if self.split[0] == 'set_device' : 
                self.cpr_ns( 'changing device to %s'%self.split[1] )
                self.SEM_IO.set_device( self.split[1] )
            
            if self.split[0] == 'set_colormode' : 
                self.cpr_ns( 'setting color mode to %s'%self.split[1] )
                self.SEM_IO.set_colormode( self.split[1] )


####################
sem_cli = CLI()
sem_cli.sem_console()