#define _GNU_SOURCE
#include "csapp.h"
#include "string.h"
#include "projecto.h"
#include <time.h>
#include <sys/resource.h>
#include <sys/times.h> 
#include <sched.h> 

void createFileAndSaveIt(int connfd, informacion_cliente* infoUsuario);
void quitNewCharacterLineInput(char *str);
char* compilesAndExecuteFile(informacion_cliente* infoUsuario);
char* definirNombreEjecutable(char* nombreOriginal);
void* hiloAdministrador(void *arg);
void* hiloLog(void *arg);
void manejadorSennales(int pid, int sennal);
void  SIGINT_handler(int);   
void  SIGQUIT_handler(int);
void clean_stdin(void);
int readArchivo(char* path, char* resultado, int resultadoSize);
void generarEstadisticas(int pidLocal, int modo);
int generarYSetearAfinidad(int pid);
void signal_handler(int test);

sem_t mutex;
sem_t mutexAfinity;
sem_t mutexLog;
sem_t mutexCounterProcess;
informacion_cliente tablaUsuarios[10000];
pid_t pids[10000];
static int booleansAfinity[10000];
int orden_llegada = 0;
int counterPids = 0;
pthread_t tid,tid2;
int time_limit = 100;
int pages_limits = 5000;
int process_counter = 0;

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
	sem_init(&mutexAfinity,1,1);
	sem_init(&mutexLog,1,1);
	sem_init(&mutexCounterProcess,1,1);
	//time_t when;
	pid_t pid;
	//int status;
	int statusThread;
	int logThread;

	signal(SIGUSR1, signal_handler);
	statusThread = pthread_create(&tid, NULL, &hiloAdministrador, NULL);
	logThread = pthread_create(&tid2, NULL, &hiloLog, NULL);
	//trashThread = pthread_create(&tid2, NULL, &hiloRecogedor, NULL);

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
			//printf("server connected to %s (%s)\n", hp->h_name, haddrp);

			sem_wait (&mutex);
			pid_t localPid = getpid();
			//printf("local pid: %d\n", localPid);
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
			time_t rawtime;
			struct tm * timeinfo;
			time ( &rawtime );
			timeinfo = localtime ( &rawtime );
			char* timeInfo = strdup(asctime (timeinfo));
			quitNewCharacterLineInput(timeInfo);
			FILE *pFile2;
			pFile2 = fopen("logFile_warnings.txt","a");
			sem_wait(&mutexLog);
			fprintf(pFile2,"%s: Process with pid %d has finished correctly\n",timeInfo, getpid());
			sem_post(&mutexLog);
			fclose(pFile2);
			kill(getppid(),SIGUSR1);
			exit(3);

		}else{ //parent
			//printf("\nenter here\n");
			//printf("pid: %d\n", pid);
			pids[counterPids] = pid;
			counterPids++;			
			//printf("counterPids: %d\n", counterPids);
			for ( int i = 0;i < counterPids; i++){
				//printf("procesos activos: %d\n", pids[i]);
			}
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

	/*
	printf("identificador Cliente: %s \n", identificadorCliente);
	printf("archivoNombre: %s\n", archivoNombre);
	printf("nombreArchivo: %s\n", nombreArchivo);*/
	
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
	//printf("este es mensaje: %s \n", mensaje);

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

	//printf("nombreOriginal: %s\n", nombreOriginal);

	strcpy(rutaEjecutable, "Executables/");
	strcat(rutaEjecutable, usuarioStr);
	strcat(rutaEjecutable, "-");
	strcat(rutaEjecutable, nombreOriginal);
	strcat(rutaEjecutable, "out");

	//printf("ruta ejecutable es: %s\n", rutaEjecutable);
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
	
	
	//printf("este es comando compilacion: %s\n", comandoCompilacion);

	int status = system(comandoCompilacion);

	if( status != 0){
		mensaje = "Error in compilation \n";
	}else{
		strcpy(comandoEjecucion, "./");
		strcat(comandoEjecucion, rutaEjecutable);
		//printf("este es comando ejecucion: %s\n", comandoEjecucion);
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
	int booleanoOpcion2 = 0;
	//sched_setaffinity() http://www.tutorialspoint.com/unix_system_calls/sched_setaffinity.htm
	//logs 
	//struct struct rusage http://pubs.opengroup.org/onlinepubs/000095399/functions/getrusage.html
	//logs cada cierto tiempo

	while (1){
		int opcion1 = 0;
		int opcion2 = 0;
		int opcion3 = 0;
		printf("A continuación las opciones\n");
		printf("1) Ver pids de los procesos involucrados\n");
		printf("2) Ver información de un proceso\n");
		printf("3) Parar o Matar un proceso específico\n");
		printf("4) Cualquier otra opción para salir\n");
		scanf("%d", &opcion1);
		clean_stdin();
		printf("su opcion fue: %d\n", opcion1);
		if (opcion1 == 1){
			sem_wait(&mutex);
			int numberProcess = counterPids;
			sem_post(&mutex);
			int counterProcess = 1;
			for (int i = 0; i < numberProcess; i++){
				sem_wait(&mutex);
				if (pids[i] >= 0){
					printf("proceso %d: pid %d \n", counterProcess , pids[i]);
					counterProcess++;
				}				
				sem_post(&mutex);
			}
		}else if (opcion1 == 2){
			printf("Ingrese el PID del proceso\n");
			scanf("%d", &opcion2);
			clean_stdin();
			int pidLocal;
			int numberProcess = counterPids;
			for (int i = 0; i < numberProcess; i++){				
				if (pids[i] == opcion2){
					printf("Su proceso existe\n");
					sem_wait(&mutex);
					pidLocal = pids[i];
					sem_post(&mutex);
					booleanoOpcion2 = 1;
					break;
				}
			}
			if (booleanoOpcion2){
				generarEstadisticas(pidLocal, 0);
			}else{
				printf("No existe proceso con ese PID\n");
			}
		}else if (opcion1 == 3){
			printf("Ingrese el PID del proceso\n");
			scanf("%d", &opcion2);
			clean_stdin();
			int pidLocal;
			int numberProcess = counterPids;
			for (int i = 0; i < numberProcess; i++){				
				if (pids[i] == opcion2){
					printf("Su proceso existe\n");
					sem_wait(&mutex);
					pidLocal = pids[i];
					sem_post(&mutex);
					booleanoOpcion2 = 1;
					break;
				}				
			}
			if (booleanoOpcion2){
				printf("ingrese señal a enviar\n");
				printf("1) SIG_INT\n");
				printf("2) SIG_STP\n");
				printf("3) SIG_CONT\n");
				scanf("%d",&opcion3);
				clean_stdin();
				manejadorSennales(pidLocal, opcion3);
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

void manejadorSennales(int pid, int sennal){
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char* timeInfo = strdup(asctime (timeinfo));
	quitNewCharacterLineInput(timeInfo);
	FILE *pFile2;
	pFile2 = fopen("logFile_warnings.txt","a");
	if (sennal == 1){
		int returnStatus = 0;
		kill(pid, SIGINT);
        printf("Sent a SIGINT signal to process with PID: %d\n", pid);
		waitpid(pid, &returnStatus, 0);
		int numberProcess = counterPids;
		for (int i = 0; i < numberProcess; i++){
			
			if (pids[i] == pid){
				sem_wait(&mutex);
				pids[i] = -1;
				sem_post(&mutex);
				break;
			}
			
		}
		sem_wait(&mutexLog);
		fprintf(pFile2,"%s: Process with pid %d has been finished with SIGINT signal\n",timeInfo, pid);
		sem_post(&mutexLog);
		kill(getpid(),SIGUSR1);
		

	}else if(sennal == 2){
		kill(pid, SIGSTOP);
        printf("Sent a SIGSTOP signal to process with PID: %d\n", pid);
		sem_wait(&mutexLog);
		fprintf(pFile2,"%s: Process with pid %d has been paused with SIGSTOP signal\n",timeInfo, pid);
		sem_post(&mutexLog);

	}else if (sennal == 3){
		kill(pid, SIGCONT );
        printf("Sent a SIGCONT signal to process with PID: %d\n", pid);
		sem_wait(&mutexLog);
		fprintf(pFile2,"%s: Process with pid %d has been continued with SIGCONT signal\n",timeInfo, pid);
		sem_post(&mutexLog);

	}else{
		printf("No existe ninguna de esas opciones\n");
	}
	fclose(pFile2);

}

void clean_stdin(void) {
    //printf("Se va a limpiar el buffer.\n");
    int c;
    do {
     c = getchar();
    }while (c != '\n' && c != EOF);
}

void* hiloLog(void *arg){
	int counterProcesos2 = 0;
	while(1){
		//printf("entrando\n");
		int numberProcess = counterPids;
		int counterProcess = 1;
		for (int i = 0; i < numberProcess; i++){
			if (pids[i] >= 0){
				counterProcess++;
				generarEstadisticas(pids[i], 1);
				if (booleansAfinity[i] != 1){
					int valorDeSetear = generarYSetearAfinidad(pids[i]);
					if (valorDeSetear == 1){
						sem_wait(&mutexAfinity);
						booleansAfinity[i] = 1;
						sem_post(&mutexAfinity);

					}

				}

			}
		}
		sleep(10);
		counterProcesos2++;
		if (counterProcesos2 > 5){
			time_t rawtime;
			struct tm * timeinfo;
			time ( &rawtime );
			timeinfo = localtime ( &rawtime );
			char* timeInfo = strdup(asctime (timeinfo));
			quitNewCharacterLineInput(timeInfo);
			FILE *pFile;		
			pFile = fopen("logFile_warnings.txt","a");
			sem_wait(&mutexLog);
			fprintf(pFile,"%s: %d process finished in last minute \n",timeInfo, process_counter);
			sem_post(&mutexLog);
			fclose(pFile);
			counterProcesos2 = 0;
			sem_wait(&mutexCounterProcess);
			process_counter = 0;
			sem_post(&mutexCounterProcess);
			
		}
				
			
	}

}

void generarEstadisticas(int pidLocal, int modo){
	/*
	modo 0 -> x consola
	modo 1 -> a archivo
	*/
	//printf("estoy entrando aqui\n");
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
		char* estado;
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
		3 - estado %c
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
		for (int i = 0; i < 23 ; i++){
			token = strsep(&stringArchivo2, " ");
			if ( i == 2){
				estado = strdup(token);
			}else if (i == 9){
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

		time_t rawtime;
		struct tm * timeinfo;
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		char* timeInfo = strdup(asctime (timeinfo));
		quitNewCharacterLineInput(timeInfo);
		

		if (!modo){
			printf("Current local time and date: %s", asctime (timeinfo));
			printf("\nProcess information\n");
			printf("State: %s\n", estado);
			printf("Total minor Faults: %lu\n", minorFaults + minorFaultsWait);
			printf("Total mayor faults: %lu\n", mayorFaults + mayorFaultsWait);
			printf("Total Page Faults: %lu\n", totalPageFaults);
			printf("Total time spent in kernel mode: %lu seconds \n", (kernelmode + kernelmodeWait)/ (sysconf (_SC_CLK_TCK)) );
			printf("Total time spent in user mode: %lu seconds \n", (usermode + usermodeWait)/ (sysconf (_SC_CLK_TCK)) );
			printf("Total time of process been scheduled: %lu seconds \n", totalmode);
			printf("Total virtual memory: %lu\n\n", virtualMemory);
		}else{
			FILE *pFile;		
			pFile = fopen("logFile.txt","a");
			fprintf(pFile,"Current local time and date: %s", asctime (timeinfo));
			fprintf(pFile,"Process information with pid %d\n",pidLocal);
			fprintf(pFile,"State: %s\n", estado);
			fprintf(pFile,"Total minor Faults: %lu\n", minorFaults + minorFaultsWait);
			fprintf(pFile,"Total mayor faults: %lu\n", mayorFaults + mayorFaultsWait);
			fprintf(pFile,"Total Page Faults: %lu\n", totalPageFaults);
			fprintf(pFile,"Total time spent in kernel mode: %lu seconds \n", (kernelmode + kernelmodeWait)/ (sysconf (_SC_CLK_TCK)) );
			fprintf(pFile,"Total time spent in user mode: %lu seconds \n", (usermode + usermodeWait)/ (sysconf (_SC_CLK_TCK)) );
			fprintf(pFile,"Total time of process been scheduled: %lu seconds \n", totalmode);
			fprintf(pFile,"Total virtual memory: %lu\n\n", virtualMemory);
			fclose(pFile);
		}

		if (totalPageFaults > pages_limits){
			FILE *pFile2;
			pFile2 = fopen("logFile_warnings.txt","a");
			sem_wait(&mutexLog);
			fprintf(pFile2,"%s: Process with pid %d has now more than %d page faults: %lu total page faults \n",timeInfo, pidLocal, pages_limits, totalPageFaults);
			sem_post(&mutexLog);
			fclose(pFile2);
		}

		if (totalmode > time_limit){
			FILE *pFile2;
			pFile2 = fopen("logFile_warnings.txt","a");
			sem_wait(&mutexLog);
			fprintf(pFile2,"%s: Process with pid %d has been scheduled for more than %d seconds: %lu total seconds \n",timeInfo, pidLocal, time_limit, totalmode);
			sem_post(&mutexLog);
			fclose(pFile2);
		}


	}else{
		printf("algo malo sucedio\n");
	}
	
	free(pidElegido);
	free(pathArchivo);
	free(stringRespuesta);
	

}

int generarYSetearAfinidad(int pid){
	// cpu_set_t: This data set is a bitset where each bit represents a CPU.
	cpu_set_t cpuset;
	cpu_set_t cpuset2;
	int valueReturn = 1;

	int get_affinity = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);
	if (get_affinity != 0){
		valueReturn = 0;
	}else{
		int numberCores = CPU_COUNT(&cpuset);
		CPU_ZERO(&cpuset2);
		int assignedCore = (counterPids - 1) % numberCores;
		CPU_SET(assignedCore, &cpuset2);
		int set_result = sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset2);
		if (set_result != 0) {
		
			valueReturn = 0;
		}else{
			time_t rawtime;
			struct tm * timeinfo;
			time ( &rawtime );
			timeinfo = localtime ( &rawtime );
			char* timeInfo = strdup(asctime (timeinfo));
			quitNewCharacterLineInput(timeInfo);
			FILE *pFile;		
			pFile = fopen("logFile_warnings.txt","a");
			sem_wait(&mutexLog);
			fprintf(pFile,"%s: Process with pid %d has been assigned to core %d \n",timeInfo, pid, assignedCore);
			sem_post(&mutexLog);
			fclose(pFile);
		}

	}
	return valueReturn;
}

void signal_handler(int test){
	sem_wait(&mutexCounterProcess);
	process_counter++;
	sem_post(&mutexCounterProcess);
	//printf("he recibido señal\n");
}




