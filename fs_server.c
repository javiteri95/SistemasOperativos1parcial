#include "csapp.h"
#include "string.h"
#include "projecto.h"

void createFileAndSaveIt(int connfd);
void quitNewCharacterLineInput(char *str);
void compilesAndExecuteFile(informacion_cliente* infoUsuario);

int main(int argc, char **argv)
{
	int listenfd, connfd;
	unsigned int clientlen;
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp, *port;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	port = argv[1];

	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

		if(Fork() == 0){

			Close(listenfd);

			/* Determine the domain name and IP address of the client */
			hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
						sizeof(clientaddr.sin_addr.s_addr), AF_INET);
			haddrp = inet_ntoa(clientaddr.sin_addr);
			printf("server connected to %s (%s)\n", hp->h_name, haddrp);
			createFileAndSaveIt(connfd);
			Close(connfd);
			exit(0);
		}

		/* Determine the domain name and IP address of the client */
		/*
		hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
					sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		haddrp = inet_ntoa(clientaddr.sin_addr);
		printf("server connected to %s (%s)\n", hp->h_name, haddrp);

		
		Close(connfd);
		*/
	}
	exit(0);
}


void createFileAndSaveIt(int connfd){
	int counter = 0;
	size_t n;
	char buf[MAXLINE];
	rio_t rio;
	struct stat fileStat;
	char *temp;
	char *nombreArchivo = (char*) malloc(MAXLINE);
	informacion_cliente infoUsuario;

	Rio_readinitb(&rio, connfd);

	Rio_readlineb(&rio,buf,sizeof(buf));
	char* identificadorCliente = (char*) malloc(MAXLINE);
	strcpy(identificadorCliente, buf);
	infoUsuario.identificador_usuario = atoi(identificadorCliente);



	Rio_readlineb(&rio,buf,sizeof(buf));
	char* archivoNombre = (char*) malloc(MAXLINE);
	strcpy(archivoNombre, buf);
	strcpy(nombreArchivo, "Files/");
	quitNewCharacterLineInput(identificadorCliente);
	quitNewCharacterLineInput(archivoNombre);
	strcat(nombreArchivo, identificadorCliente);
	strcat(nombreArchivo, "-");
	strcat(nombreArchivo, archivoNombre);
	infoUsuario.ruta_archivo_fuente = nombreArchivo;

	printf("identificador Cliente: %s \n", identificadorCliente);
	printf("archivoNombre: %s\n", archivoNombre);
	printf("nombreArchivo: %s\n", nombreArchivo);

	/*
	while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
		
		//quitNewCharacterLineInput(buf);
		
		if((stat(buf,&fileStat) < 0))  {  
		  	printf("no existe el archivo %s\n", buf );
		  	temp = "0\n";
		  	Rio_writen(connfd, temp, 2*sizeof(char));
	        //return -1;
	 	}else if ((stat(buf,&fileStat) >= 0)) {
	 		counter=1;
	 		printf("existe el archivo %s\n", buf);
	 		strcpy(nombreArchivo,buf);
	 		temp = "1\n";
	 		Rio_writen(connfd, temp, 2*sizeof(char));
	 		FILE *archivo;
	 		char line[MAXLINE];
	 		archivo = fopen(nombreArchivo,"r");
	 		while ( fgets(line, sizeof(line), archivo) != NULL){
	 			Rio_writen(connfd, line , strlen(line));
	 		}
	 		char *final = "fin\n";
	 		Rio_writen(connfd, final , strlen(final));
	 	}
		 


	 	
	 	

	 }*/


	
	FILE *archivoCreado;
	
	archivoCreado = fopen(nombreArchivo, "w");
	while(counter < 50000){
		Rio_readlineb(&rio,buf,sizeof(buf));
		if ( strcmp(buf,"fin\n") == 0 ){
			break;
		}
		fputs(buf, archivoCreado);
		counter++;
	}
	fclose(archivoCreado);

	//compilesAndExecuteFile(&infoUsuario);

	free(nombreArchivo);
	free(archivoNombre);
	free(identificadorCliente);

 
}

void compilesAndExecuteFile(informacion_cliente* infoUsuario){
	char* rutaArchivo;
}

void quitNewCharacterLineInput(char *str){
	/* Remove trailing newline, if there. */
    if ((strlen(str)>0) && (str[strlen (str) - 1] == '\n'))
        str[strlen (str) - 1] = '\0';
}




