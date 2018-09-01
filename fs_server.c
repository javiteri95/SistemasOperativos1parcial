#include "csapp.h"
#include "string.h"
#include "projecto.h"
#include <time.h>
#include <sys/resource.h>
#include <sys/times.h> 

void createFileAndSaveIt(int connfd, informacion_cliente* infoUsuario);
void quitNewCharacterLineInput(char *str);
char* compilesAndExecuteFile(informacion_cliente* infoUsuario);
char* definirNombreEjecutable(char* nombreOriginal);
void* hiloAdministrador(void *arg);
//char* readFile(char *filename);
int readArchivo(char* path, char* resultado, int resultadoSize);

sem_t mutex;
informacion_cliente tablaUsuarios[10000];
pid_t pids[10000];
int orden_llegada = 0;
int counterPids = 0;
pthread_t tid;

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
	time_t when;
	pid_t pid;
	int status;
	int statusThread;

	statusThread = pthread_create(&tid, NULL, &hiloAdministrador, NULL);

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
		//printf("esperando conexión servidor...\n");
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		pid = Fork();
		if (pid == 0){

			Close(listenfd);

			//time(&when);

			/* Determine the domain name and IP address of the client */
			hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
						sizeof(clientaddr.sin_addr.s_addr), AF_INET);
			haddrp = inet_ntoa(clientaddr.sin_addr);
			printf("server connected to %s (%s)\n", hp->h_name, haddrp);

			sem_wait (&mutex);
			pid_t localPid = getpid();
			printf("local pid: %d\n", localPid);
			pids[counterPids] = localPid;
			counterPids++;
			orden_llegada++;
			time_t timestamp_sec; /* timestamp in second */
  			time(&timestamp_sec);  /* get current time; same as: timestamp_sec = time(NULL)*/
			int timestamp = (int) timestamp_sec;
			informacion_cliente infoUsuario;
			infoUsuario.tiempo_envio = timestamp;
			infoUsuario.orden_llegada = orden_llegada;
			tablaUsuarios[orden_llegada] = infoUsuario;
			


			sem_post (&mutex);


			

			createFileAndSaveIt(connfd, &infoUsuario);
			Close(connfd);
			exit(3);

		}else{ //parent
			printf("\nenter here\n");
			printf("pid: %d\n", pid);
			pids[counterPids] = pid;
			counterPids++;			
			printf("counterPids: %d\n", counterPids);
			for ( int i = 0;i < counterPids; i++){
				printf("procesos activos: %d\n", pids[i]);
			}
			/*
			end_id = waitpid(pid, &status, 0 );
			//end_id = waitpid(pid, &status, WNOHANG|WUNTRACED );
			printf("end_id:%d\npid: %d\ncounterPid: %d\n", end_id, pid, counterPids);
			for ( int i = 0;i < counterPids; i++){
				printf("procesos activos: %d\n", pids[i]);
			}
			time(&when);
			if (end_id == -1) {            // error calling waitpid       
				perror("waitpid error");
				exit(EXIT_FAILURE);
			}
			else if (end_id == 0) {        // child still running         
				time(&when);
				printf("Parent waiting for child at %s", ctime(&when));
				//sleep(1);
			}
			else if (end_id == pid) {  // child ended                 
				if (WIFEXITED(status))
					printf("Child ended normally\n");
				else if (WIFSIGNALED(status))
					printf("Child ended because of an uncaught signal\n");
				else if (WIFSTOPPED(status))
					printf("Child process has stopped\n");
				exit(EXIT_SUCCESS);
			}
			*/


		}


	}
	exit(0);
}


void createFileAndSaveIt(int connfd, informacion_cliente* infoUsuario){
	int counter = 0;
	//size_t n;
	char buf[MAXLINE];
	rio_t rio;
	//struct stat fileStat;
	//char *temp;
	char *nombreArchivo = (char*) malloc(MAXLINE);

	Rio_readinitb(&rio, connfd);

	Rio_readlineb(&rio,buf,sizeof(buf));
	char* identificadorCliente = (char*) malloc(MAXLINE);
	strcpy(identificadorCliente, buf);
	sem_wait (&mutex);
	infoUsuario->identificador_usuario = atoi(identificadorCliente);
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
	infoUsuario->ruta_archivo_fuente = nombreArchivo;
	infoUsuario->nombre_original = archivoNombre;
	sem_post (&mutex);

	printf("identificador Cliente: %s \n", identificadorCliente);
	printf("archivoNombre: %s\n", archivoNombre);
	printf("nombreArchivo: %s\n", nombreArchivo);
	
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

	char* mensaje = compilesAndExecuteFile(infoUsuario);
	sem_wait (&mutex);
	infoUsuario->respuesta = mensaje;
	sem_post (&mutex);
	printf("este es mensaje: %s \n", mensaje);

	Rio_writen(connfd, "Inicio (comunicacion)\n" , strlen("Inicio (comunicacion)\n"));
	Rio_writen(connfd, mensaje , strlen(mensaje));
	Rio_writen(connfd, "Fin (comunicacion)\n" , strlen("Fin (comunicacion)\n"));

	free(nombreArchivo);
	free(archivoNombre);
	free(identificadorCliente);
	
	while (1){

	}
	

 
}

char* compilesAndExecuteFile(informacion_cliente* infoUsuario){
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
	/*
	if (librerias != NULL){
		printf("entra aqui librerias\n");
		strcat(comandoCompilacion, librerias);
	}
	*/
	
	
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

void* hiloAdministrador(void *arg)
{
	printf("Bienvenidos al menu de administracion de procesos\n");
	printf ("_SC_CLK_TCK = %ld\n", sysconf (_SC_CLK_TCK)); 
	int booleanoOpcion2 = 0;
	//sched_setaffinity() http://www.tutorialspoint.com/unix_system_calls/sched_setaffinity.htm
	//logs 
	//struct struct rusage http://pubs.opengroup.org/onlinepubs/000095399/functions/getrusage.html
	//logs cada cierto tiempo

	while (1){
		int opcion1, opcion2;
		printf("A continuación las opciones\n");
		printf("1) Ver pids de los procesos involucrados\n");
		printf("2) Ver información de un proceso\n");
		printf("3) Parar o Matar un proceso específico\n");
		printf("4) Cualquier otra opción para salir\n");

		scanf("%d", &opcion1);
		printf("su opcion fue: %d\n", opcion1);
		//quitNewCharacterLineInput(opcion1);

		if (opcion1 == 1){
			sem_wait(&mutex);
			int numberProcess = counterPids;
			sem_post(&mutex);
			for (int i = 0; i < numberProcess; i++){
				sem_wait(&mutex);
				printf("proceso %d: pid %d \n", i , pids[i]);
				sem_post(&mutex);
			}
		} else if(opcion1 == 2){
			printf("Ingrese el PID del proceso\n");
			scanf("%d", &opcion2);
			int pidLocal;
			sem_wait(&mutex);
			int numberProcess = counterPids;
			sem_post(&mutex);
			for (int i = 0; i < numberProcess; i++){
				sem_wait(&mutex);
				if (pids[i] == opcion2){
					printf("Su proceso existe\n");
					pidLocal = pids[i];
					booleanoOpcion2 = 1;
					break;
				}
				sem_post(&mutex);
			}
			if (booleanoOpcion2){
				printf("estoy entrando aqui\n");
				char* pidElegido = (char*) malloc(MAXLINE);
				char* pathArchivo = (char*) malloc(MAXLINE);
				char* stringRespuesta = (char*) malloc(MAXLINE);
				sprintf(pidElegido, "%d", pidLocal);				
				strcpy(pathArchivo, "/proc/");
				strcat(pathArchivo, pidElegido);
				strcat(pathArchivo, "/stat");
				/*
				char *contenidoArchivo = readFile(pathArchivo);
				printf("pathArchivo: %s\n", pathArchivo);
				if (contenidoArchivo)
				{
					puts(contenidoArchivo);
					free(contenidoArchivo);
				}
				*/
				int resultadoArchivo = readArchivo(pathArchivo, stringRespuesta, MAXLINE);
				if (resultadoArchivo){
					char* stringArchivo2 = strdup(stringRespuesta);
					char* token;
					unsigned long minorFaults;
					unsigned long minorFaultsWait;
					unsigned long mayorFaults;
					unsigned long mayorFaultsWait;

					unsigned long usermode;
					unsigned long kernelmode;
					unsigned long usermodeWait;
					unsigned long kernelmodeWait;

					unsigned long virtualMemory;
					/*
					10 - numero de minor faults %lu
					11 - numero de minor faults esperando hijos %lu

					12 - numero de mayor faults %lu
					13 - numero de mayor faults esperando hijos %lu

					14 - tiempo en user mode %lu
					15 - tiempo en kernel mode %lu

					16 - tiempo en user mode esperando hijos %ld
					17 - tiempo en kernel esperando hijos %ld

					23 - virtual memory %lu

					41 - policy %u

					*/
					//printf("stringRespuesta: %s", stringRespuesta);
					for (int i = 0; i < 23 ; i++){
						token = strsep(&stringArchivo2, " ");
						if (i == 9){
							sscanf(token, "%lu", &minorFaults);
						}else if( i == 10){
							sscanf(token, "%lu", &minorFaultsWait);
						}else if (i == 11){
							sscanf(token, "%lu", &mayorFaults);
						}else if (i == 12){
							sscanf(token, "%lu", &mayorFaultsWait);
						}else if (i == 13){
							sscanf(token, "%lu", &usermode);
						}else if( i == 14){
							sscanf(token, "%lu", &kernelmode);
							//kernelmode
						}else if (i == 15){
							sscanf(token, "%lu", &usermodeWait);

						}else if (i == 16){
							sscanf(token, "%lu", &kernelmodeWait);
						}else if (i == 22){
							sscanf(token, "%lu", &virtualMemory);
						}
					}
					unsigned long totalPageFaults = minorFaults + mayorFaults + minorFaultsWait + mayorFaultsWait;
					unsigned long totalmode = (usermode + kernelmode +usermodeWait + kernelmodeWait) / (sysconf (_SC_CLK_TCK));

					printf("\nProcess information\n");
					printf("Total minor Faults: %lu\n", minorFaults + minorFaultsWait);
					printf("Total mayor faults: %lu\n", mayorFaults + mayorFaultsWait);
					printf("Total Page Faults: %lu\n", totalPageFaults);
					printf("Total time spent in kernel mode: %lu seconds \n", (kernelmode + kernelmodeWait)/ (sysconf (_SC_CLK_TCK)) );
					printf("Total time spent in user mode: %lu seconds \n", (usermode + usermodeWait)/ (sysconf (_SC_CLK_TCK)) );
					printf("Total time of process been scheduled: %lu seconds \n", totalmode);
					printf("Total virtual memory: %lu\n\n", virtualMemory);

				}else{
					printf("algo malo sucedio\n");
				}
				
				free(pidElegido);
				free(pathArchivo);
				free(stringRespuesta);

			}else{
				printf("No existe proceso con ese PID\n");
			}



		}else{
			printf("Ha salido del modo administrador\n");
			break;
		}
	}

    return NULL;
}

int readArchivo(char* path, char* resultado, int resultadoSize){
	FILE *archivo;
	archivo = fopen(path, "r");
	if (archivo){
		fgets(resultado,resultadoSize, (FILE*) archivo);
		fclose(archivo);
		return 1;
	}else{
		printf("algo malo sucedio\n");
		fclose(archivo);
		return 0;
	}
	
	
}



