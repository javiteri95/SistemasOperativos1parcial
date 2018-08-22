#include "csapp.h"
//#include "projecto.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

int generate_random_id();
void quitNewCharacterLineInput(char *str);

int main(int argc, char **argv)
{
	int clientfd;
	char *port;
	char *host, buf[MAXLINE];
	char archivo[MAXLINE];
	rio_t rio;
	//size_t n;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
		exit(0);
	}
	host = argv[1];
	port = argv[2];
	//Nombre del archivo
	printf("Ingrese nombre de archivo a enviar\n");
	fgets(archivo, sizeof(archivo), stdin);
	quitNewCharacterLineInput(archivo);
	printf("Su archivo es %s\n",archivo);

	//archivo = argv[3];
	clientfd = Open_clientfd(host, port);
	Rio_readinitb(&rio, clientfd);
	//strcpy(buf, archivo);

	/*
	printf("buf es %s\n", buf );
	Rio_writen(clientfd,buf,sizeof(buf));
	bzero((char *)&buf,sizeof(buf));
	Rio_readlineb(&rio,buf,sizeof(buf));
	Fputs(buf, stdout);
	
	if(strcmp(buf, "0\n") == 0){
		printf("No encontro el archivo\n");
	}else if(strcmp(buf, "1\n") == 0){
		printf("El archivo existe\n");
		FILE *archivoCreado;
		archivoCreado = fopen(archivo, "r");
		while(1){
			Rio_readlineb(&rio,buf,sizeof(buf));
			if ( strcmp(buf,"fin\n") == 0 ){
				break;
			}
			fputs(buf, archivoCreado);

		}
		fclose(archivoCreado);

	}
	*/
	char *finalLinea = "\n";
	int identificadorCliente = generate_random_id();
	char* identificadorClienteStr = (char*) malloc(MAXLINE);
	sprintf(identificadorClienteStr, "%d", identificadorCliente);
	Rio_writen(clientfd, identificadorClienteStr , strlen(identificadorClienteStr));
	Rio_writen(clientfd, finalLinea , strlen(finalLinea));
	Rio_writen(clientfd, archivo , strlen(archivo));
	Rio_writen(clientfd, finalLinea , strlen(finalLinea));
	FILE *archivoFile;
	char line[MAXLINE];
	archivoFile = fopen(archivo,"r");
	while ( fgets(line, sizeof(line), archivoFile) != NULL){
	 	Rio_writen(clientfd, line , strlen(line));
	}
	Rio_writen(clientfd, finalLinea , strlen(finalLinea));
	char *final = "fin\n";
	Rio_writen(clientfd, final , strlen(final));

	Rio_readlineb(&rio, buf, sizeof(buf));
	printf("mensaje es: \n");
	while(1){
		Rio_readlineb(&rio, buf, sizeof(buf));
		if (strcmp(buf, "Fin (comunicacion)\n") == 0){
			break;
		}
		printf("%s", buf);
	}

	fclose(archivoFile);

	free(identificadorClienteStr);
	Close(clientfd);
	exit(0);
}

int generate_random_id(){
	srand(time(NULL));   // should only be called once
	int r = rand();      // returns a pseudo-random integer between 0 and RAND_MAX
	return r;
}

void quitNewCharacterLineInput(char *str){
	/* Remove trailing newline, if there. */
    if ((strlen(str)>0) && (str[strlen (str) - 1] == '\n'))
        str[strlen (str) - 1] = '\0';
}

