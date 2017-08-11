""" test of binary data processing """

import sys
import sem_io

imagepath = '/keith/image.bin' #'/Users/klegg/image.bin'

sem = sem_io.device(True)

if len(sys.argv)>1:
    longedge = int(sys.argv[1])
else:
	longedge = 256

#print "longedge %s" % longedge

sem.read_raw_binary(imagepath, longedge)
sem.process()
sem.write_framebuffer()






