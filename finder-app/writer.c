/*
* Application entry point
*
* This file takes writes the input string into the file whose path is specified
* 
* Inputs: File path and input string
*
* Author: Jahnavi Pinnamaneni; japi8358@colorado.edu
*/


#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main (int argc, char * argv[])
{
	openlog(NULL, 0, LOG_USER);
	printf("Number of arguments %d\r\n", argc);
	if(argc != 3)
	{
		printf("ERROR: the file requires two arguments\r\n");
		printf("The arguments should be in the following order\r\n");
		printf("1. The full path to a file \r\n");
		printf("2. Text string that is to written to the file\r\n");
		syslog(LOG_ERR, "Invalid number of arguments: %d", argc-1);
		return 1;
	}
	
	char * filepath = argv[1];
	char * str      = argv[2];
	
	FILE *fptr;
	fptr = fopen(filepath,"w+");
	
	if(fptr == NULL)
   	{
      		printf("Error: file not opened properly");   
      		exit(1);             
   	}
   	
   	fprintf(fptr,"%s",str);
   	syslog(LOG_DEBUG, "Writing %s to %s", str, filepath);
   	
   	fclose(fptr);
   	closelog();
   	
   	return 0;
}

