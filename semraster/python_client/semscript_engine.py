"""
  SEM API - 
  use semraster as a scripting tool instead of pyserial 

"""


class semraster_api_engine(object):
	def __init__(self):
		self.semraster_bin = '../semraster/whatever'
        self.command_stack = []

    def add_command(self, command):
    	self.command_stack.append(command)

    def execute(self):
    	for c in self.command_stack:
    		print c





