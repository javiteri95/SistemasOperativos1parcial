#include <stdio.h>     
#include <stdlib.h> 
#include "csapp.h"

int main(){
    //printf("hols mundo RTC \n");
	/*
    pid_t pid ;
    int status;
    char *const parmList[] = { NULL};
    char* message = (char*) malloc(sizeof(char) *6);
    status = system("gcc -o ejemplo1 ejemplo1.c");
	printf("este es status: %d \n", status);
	*/
	
	FILE *fp;
	char path[1035];
	dup2(STDOUT_FILENO, STDERR_FILENO);
	fp = popen("gcc -o ejemplo1 ejemplo1-mal.c", "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}

	printf("hola universo\n");
	while (fgets(path, sizeof(path)-1, fp) != NULL) {
		printf("este es: %s", path);
	}

	pclose(fp);

	
	/*	
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
	*/

	/*
    printf("%s", message);
    free(message);
	*/

	return 0;

    
}