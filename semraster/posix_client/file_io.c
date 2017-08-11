/*

    a home for file read and write operations
    read and set global configuration 
    serial port, etc 

    Author  - Keith Legg
    Created - May 10, 2015

*/


#include <stdio.h>
#include <stdlib.h> //exit()
#include <stdarg.h>
#include <string.h> //strcmp
#include <ctype.h>  //tolower, etc


int BUFFER_SIZE = 2098176;            //maxiumum possible size = 1024*1024*2
char SERIAL_PORT_ID[] = "";
char IMAGE_OUTPUT_PATH[] = "/keith";

/***************************************/
void strip( char *s )
{
    s[strcspn ( s, "\n" )] = '\0';
}

/***************************************/
void split( char *s )
{
    char * pch;
    printf ("Splitting string \"%s\" into tokens:\n",s);
    pch = strtok (s," ,.-");
    while (pch != NULL)
    {
      printf ("%s\n",pch);
      pch = strtok (NULL, " ,.-"); 
    }
}

/***************************************/
void read_config( char *filename )
{
    FILE *stream;
    char *line = NULL;

    size_t len = 0;
    ssize_t read;

    //open a file 
    stream = fopen("sem.cfg", "r");
    if (stream == NULL){
       exit(EXIT_FAILURE);
    }

   
    char command_buffer[32];
    char first_tok[32];
    char second_tok[32];
    char line_copy[80];    
    unsigned int tok_count = 0;

    while ((read = getline(&line, &len, stream)) != -1) {

        if(read>1){
            //printf("Retrieved line of length %zu :\n", read);
            //printf("---> %s", line);
            tok_count=0;
            strip(line);//remove \n
            //////////////
            char * pch;
            pch = strtok (line," ");
            while (pch != NULL)
            {
                //store the first segment  
                if(tok_count==0){
                    strcpy(first_tok, pch);
                }
                //store the second token from line and parse
                if(tok_count==1){
                    strcpy(second_tok, pch);
                    /////// 
                    strcpy(command_buffer, "serial_port");
                    if( strcmp(command_buffer, first_tok) == 0)
                    {
                       printf("# setting serial port %s\n", second_tok);
                       strcpy(SERIAL_PORT_ID, second_tok);
                    } 
                    /////// 
                    strcpy(command_buffer, "outfile_path");
                    if( strcmp(command_buffer, first_tok) == 0)
                    {
                       //printf("# setting file output path %s\n", second_tok);
                       //strcpy(IMAGE_OUTPUT_PATH, second_tok);                        
                    }  
                    /////// 
                }                
                /************/
                pch = strtok (NULL, " "); //tokenize line
                tok_count++;//count number of tokens per line
            }//iterate tokens
        
        }//if line has data (other than \n)

    }//iterate all bytes in file 

    free(line);
    fclose(stream);
}


/***************************************/

struct rec
{
    int x,y,z;
};


int testLoadBinary(void)
{
    int counter;
    FILE *ptr_myfile;
    struct rec my_record;

    ptr_myfile=fopen("test.bin","rb");
    if (!ptr_myfile)
    {
        printf("Unable to open file!");
        return 1;
    }
    for ( counter=1; counter <= 10; counter++)
    {
        fread(&my_record,sizeof(struct rec),1,ptr_myfile);
        printf("%d\n",my_record.x);
    }
    
    fclose(ptr_myfile);
    return 0;

}

