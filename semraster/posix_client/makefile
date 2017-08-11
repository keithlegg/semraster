OBJ = main.o
INC = -I "./"

##################################
#to add mutiple C files 
#MOAR = framebuffer.c point_op.c
#    g++ $(OBJ) -o semraster $(INC) $(MOAR)
##################################

# file_io.c
MOAR = point_op.c framebuffer.c image_util.c semcom.c scopemode.c rastermode.c file_io.c    


semraster:$(OBJ)
	# -fno-stack-protector stops the stack smashing message - but it dont debug fer you
	g++ $(OBJ) -o semraster $(INC) $(MOAR) -lX11 -lXi -lglut -lGL -lGLU -lm -lz -lpng  -fno-stack-protector
	rm -f $(OBJ)

main.o:
	g++ -c main.c $(INC) 


clean:
	rm -f $(OBJ) semraster

