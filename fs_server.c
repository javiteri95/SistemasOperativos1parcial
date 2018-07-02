#include "csapp.h"
#include "string.h"
#include "projecto.h"

void createFileAndSaveIt(int connfd);
void quitNewCharacterLineInput(char *str);
char* compilesAndExecuteFile(informacion_cliente* infoUsuario);
char* definirNombreEjecutable(char* nombreOriginal);

sem_t mutex;
informacion_cliente tablaUsuarios[10000];
int orden_llegada = 0;

int main(int argc, char **argv)
{
	int listenfd, connfd;
	unsigned int clientlen;
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp, *port;
	struct stat st = {0};
	//sem_wait (&mutex);
	//sem_post (&mutex);
	sem_init(&mutex,1, 1);

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	if (stat("./Files", &st) == -1) {
		mkdir("./Files", 0700);
	}

	if (stat("./Executables", &st) == -1) {
		mkdir("./Executables", 0700);
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

			sem_wait (&mutex);
			orden_llegada++;
			
			time_t timestamp_sec; /* timestamp in second */
  			time(&timestamp_sec);  /* get current time; same as: timestamp_sec = time(NULL)*/
			int timestamp = (int) timestamp_sec;

			sem_post (&mutex);


			

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
	sem_wait (&mutex);
	infoUsuario.identificador_usuario = atoi(identificadorCliente);
	sem_post (&mutex);



	Rio_readlineb(&rio,buf,sizeof(buf));
	char* archivoNombre = (char*) malloc(MAXLINE);
	strcpy(archivoNombre, buf);
	strcpy(nombreArchivo, "Files/");
	quitNewCharacterLineInput(identificadorCliente);
	quitNewCharacterLineInput(archivoNombre);
	strcat(nombreArchivo, identificadorCliente);
	strcat(nombreArchivo, "-");
	strcat(nombreArchivo, archivoNombre);
	sem_wait (&mutex);
	infoUsuario.ruta_archivo_fuente = nombreArchivo;
	infoUsuario.nombre_original = archivoNombre;
	sem_post (&mutex);

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

	char* mensaje = compilesAndExecuteFile(&infoUsuario);
	sem_wait (&mutex);
	infoUsuario.respuesta = mensaje;
	sem_post (&mutex);
	printf("este es mensaje: %s \n", mensaje);

	Rio_writen(connfd, "Inicio (comunicacion)\n" , strlen("Inicio (comunicacion)\n"));
	Rio_writen(connfd, mensaje , strlen(mensaje));
	Rio_writen(connfd, "Fin (comunicacion)\n" , strlen("Fin (comunicacion)\n"));

	free(nombreArchivo);
	free(archivoNombre);
	free(identificadorCliente);

 
}

char* compilesAndExecuteFile(informacion_cliente* infoUsuario){
	printf("entre aqui, en la funcion\n");
	char* rutaArchivoFuente;
	char* librerias;
	char* nombreOriginal;
	int usuario;
	char* usuarioStr = (char*) malloc(1024);
	FILE *fp;
	char path[1035];

	char* comandoCompilacion = (char*) malloc(1024);
	char* comandoEjecucion = (char*) malloc(1024);
	char* rutaEjecutable = (char*) malloc(1024);
	char* mensaje = (char*) malloc(10000);
	
	strcpy(mensaje,"" );
	sem_wait (&mutex);
	rutaArchivoFuente = infoUsuario->ruta_archivo_fuente;
	librerias = infoUsuario->banderas;
	nombreOriginal = infoUsuario->nombre_original;
	usuario = infoUsuario->identificador_usuario;
	sem_post(&mutex);
	sprintf(usuarioStr, "%d", usuario);

	printf("nombreOriginal: %s\n", nombreOriginal);

	strcpy(rutaEjecutable, "Executables/");
	strcat(rutaEjecutable, usuarioStr);
	strcat(rutaEjecutable, "-");
	strcat(rutaEjecutable, nombreOriginal);
	strcat(rutaEjecutable, "out");

	printf("ruta ejecutable es: %s\n", rutaEjecutable);
	sem_wait (&mutex);
	infoUsuario->ruta_archivo_ejecutable = rutaEjecutable;
	sem_post (&mutex);
	//strcpy(infoUsuario->ruta_archivo_ejecutable, rutaEjecutable);
	
	strcpy(comandoCompilacion, "gcc -o ");
	strcat(comandoCompilacion, rutaEjecutable);
	strcat(comandoCompilacion, " ");
	strcat(comandoCompilacion, rutaArchivoFuente);
	strcat(comandoCompilacion, " ");
	if (librerias != NULL){
		strcat(comandoCompilacion, librerias);
	}
	
	printf("este es comando compilacion: %s\n", comandoCompilacion);

	int status = system(comandoCompilacion);

	if( status != 0){
		mensaje = "Error in compilation \n";
	}else{
		strcpy(comandoEjecucion, "./");
		strcat(comandoEjecucion, rutaEjecutable);
		printf("este es comando ejecucion: %s\n", comandoEjecucion);
		strcat(mensaje,"");
		fp = popen(comandoEjecucion, "r");
		if (fp == NULL) {
			printf("Failed to run command\n" );
			exit(1);
		}

		while (fgets(path, sizeof(path)-1, fp) != NULL) {
			strcat(mensaje,path);
			//printf("este es: %s", path);
		}

		pclose(fp);

	}

	free(usuarioStr);

	return mensaje;





}

char* definirNombreEjecutable(char* nombreOriginal){
	char *nombreEjecutable = (char*) malloc(1024);
	strcpy(nombreEjecutable, "Executables/");
	strcat(nombreEjecutable, nombreOriginal);
	strcat(nombreEjecutable, "out");

	return nombreEjecutable;
}

void quitNewCharacterLineInput(char *str){
	/* Remove trailing newline, if there. */
    if ((strlen(str)>0) && (str[strlen (str) - 1] == '\n'))
        str[strlen (str) - 1] = '\0';
}




