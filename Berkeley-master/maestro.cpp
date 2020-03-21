/* Time Daemon sends it's time to all connected nodes to adjust average time 
   The port number is passed as an argument */
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

using namespace std;

void error(const char *msg) 
{
	perror(msg);
	exit(1);
}

char buffer[256];
int n;

int relojLogico = 0, total = 0, promedio = 0, cnt = 0, t=0;

void Berkeley(long newsockfd)
{
	bzero(buffer,256);

	stringstream ss, ss1, ss2;
	ss << relojLogico;
	// Se convierte el valor del reloj a String
	string tmpstr1 = ss.str();			
	// Ahora se convierte de String a const char *
	strcpy(buffer,tmpstr1.c_str());				
	
	// Se envia el tiempo del reloj logico conectado a la computadora
	n = write((long)newsockfd,buffer,strlen(buffer));	
	if (n < 0) error("ERROR escribiendo al socket");

	bzero(buffer,256);
	//Reading the specific time difference of connected machines
	//Se lee la diferencia de tiempo entre las computadoras conectadas
	read((long)newsockfd,buffer,255);			
	cout << "Diferencia horaria de la máquina '" << newsockfd << "' : " << buffer << endl;
	
	ss1 << buffer;
	string tmpstr2 = ss1.str();
	
	//converting Time Daemon's clock from char array to integer value
	//Se convierte el valor del tiempo del reloj de char array a entero
	int diff = atoi(tmpstr2.c_str());	

	//Adding all time differences
	//Se suman las diferencias entre los tiempos
	total = total + diff;			

	sleep(2);

	//Se obtiene el promedio de las diferencias en los tiempos
	promedio = total/(cnt+1);			

	//Se calcula el tiempo promedio de ajuste para cada reloj
	int adj_clock = promedio - diff;		

	bzero(buffer,256);
	
	ss2 << adj_clock;
	string tmpstr3 = ss2.str();

	//Se convierte el tiempo de ajuste a const char *
	strcpy(buffer,tmpstr3.c_str());		

	//Se envia el tiempo de ajuste a su respectiva maquina
	n = write((long)newsockfd,buffer,strlen(buffer));	
	if (n < 0) error("ERROR escribiendo al socket");
}


void *NewConnection(void *newsockfd) //La funcion de los hilos para cada una de las peticiones de los clientes
{
	if((long)newsockfd < 0) 
		error("ERROR al aceptar");

	
	cout << "Conectado a la maquina: " << (long)newsockfd << endl;
	cnt++;

	while(cnt != t)
	{
		continue;
	}

	Berkeley((long)newsockfd);

	close((long)newsockfd);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

	long sockfd, newsockfd[10], numPuerto;
	socklen_t clilen;
	
	struct sockaddr_in serv_addr, cli_addr;

	//hilos para manejar solicitudes de clientes
	pthread_t hilos[10];	

	//Iniciando la función aleatoria con la hora actual como entrada
	srand(time(0));						
	// Definir el rango de números aleatorios del 5 al 30
	relojLogico = (rand()%25) + 5;				

	if (argc < 2) {
		fprintf(stderr,"ERROR, no se ha asignado puerto\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sockfd < 0) 
		error("ERROR al abrir socket");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	numPuerto = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(numPuerto);
	
	int reuse = 1;

    // Para reutilizar la dirección del socket en caso de bloqueos y fallas
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
		perror("setsockopt(SO_REUSEADDR) ha fallado");

	#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
		perror("setsockopt(SO_REUSEPORT) ha fallado");
	#endif

	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR durante binding");

	listen(sockfd,10);
	clilen = sizeof(cli_addr);

	cout << "Ingrese el número total de máquinas para conectar: ";
	cin >> t;

	cout << "Hora del reloj logico: " << relojLogico << endl;

	int sock_index = 0;

	for(int i=0; i < t; i++)
	{
		newsockfd[sock_index] = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);

		pthread_create(&hilos[i], NULL, NewConnection, (void *)newsockfd[sock_index]);
		
		sock_index=(sock_index+1)%100;
	}
	
	for(int i=0; i<t; i++)
	{
		int rc = pthread_join(hilos[i], NULL);
		if(rc)
		{
			printf("Error al unir hilo :\n");
			cout << "Error: " << rc << endl;
			exit(-1);
		}
	
	}

	cout << "Ajuste del reloj: " << promedio << endl;
	relojLogico = relojLogico + promedio;

	cout << "My Adjusted clock: " << relojLogico << endl;

	close(sockfd);
	return 0; 
	pthread_exit(NULL);
}