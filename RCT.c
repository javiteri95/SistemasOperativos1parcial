#include <stdio.h>     
#include <stdlib.h> 
#include "csapp.h"

int main(){
    //printf("hols mundo RTC \n");
    pid_t pid ;
    int status;
    char *const parmList[] = { NULL};
    char* message = (char*) malloc(sizeof(char) *6);
    system(" gcc -o ejemplo1 ejemplo1.c");
		
	if((pid = Fork()) == 0){

		if (execve("./ejemplo1" ,parmList , NULL)<0){
			printf("ERROR\n");
				
		}
	}

	if (waitpid(pid, &status, 0 ) < 0){
		message=strdup("ERROR\n");
	}else{
		message=strdup("OK\n");
	}

    printf("%s", message);
    free(message);

    
}