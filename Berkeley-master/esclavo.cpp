#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
using namespace std;

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[])
{
	/**
		sockfd - Esta variable almacenan los valores devueltos por la llamada 
	 			al sistema de socket y la llamada al sistema de aceptación.
		numPuerto - Almacena el número de puerto en el que el servidor acepta conexiones. 
		numCart - es el valor de retorno para las llamadas read () y write (); 
				es decir, contiene el número de caracteres leídos o escritos.
	 */
	long sockfd, numPuerto, numCart;

	/*
		serv_addr - es una estructura que contiene una dirección de internet. 
					Esta estructura se define en <netinet / in.h>. 
					La variable serv_addr contendrá la dirección del servidor
	*/
	struct sockaddr_in serv_addr;
	
	/*
		La variable server es un puntero a una estructura de tipo hostent. 
		Esta estructura se define en el archivo de encabezado netdb.h, 
		su definicion completa estara en el archvio de documentación.
	*/
	struct hostent *server;

	// El reloj logico que manejara nuestro cliente 
	int relojLogico = 0;

	// El servidor lee los caracteres de la conexión del socket en este búfer.
	char buffer[256];

	/*	El usuario debe pasar el número de puerto en el que el 
		servidor aceptará conexiones como argumento. 
	*/
	if(argc < 2) {
		fprintf(stderr,"uso puerto %s\n", argv[0]);
		exit(0);
	}
	
	/*	El número de puerto en el que el servidor escuchará las conexiones se 
		pasa como argumento, y esta declaración utiliza la función atoi () para 
		convertir esto de una cadena de dígitos a un entero.
	*/
	numPuerto = atoi(argv[1]);

    // Iniciando la función aleatoria con la hora actual como entrada
	srand(time(0));						
    // Definiendo el rango de numeros random desde 5 al 30
	relojLogico = (rand()%25) + 5;				

	/* 	La llamada al sistema socket () crea un nuevo socket. Se necesitan tres argumentos.

		El primero es el dominio de dirección del socket. El dominio de Internet para cualquiera 
		de los dos hosts en Internet (AF_INET). 
		
		El segundo argumento es el tipo de socket. Recuerde que hay dos opciones aquí, 
		un socket de flujo en el que los caracteres se leen en un flujo continuo como si 
		fueran de un archivo o tubería, y un socket de datagrama, en el que los mensajes 
		se leen en fragmentos. Las dos constantes simbólicas son SOCK_STREAM y SOCK_DGRAM. 
		
		El tercer argumento es el protocolo. Si este argumento es cero (y siempre debería 
		serlo excepto en circunstancias inusuales), el sistema operativo elegirá el protocolo 
		más apropiado. Escogerá TCP para sockets de flujo y UDP para sockets de datagramas.
	
	*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	/*	La llamada al sistema de socket devuelve una entrada en la tabla de descriptores de 
		archivo (es decir, un entero pequeño). Si la llamada del socket falla, devuelve -1. 
		En este caso, el programa muestra un mensaje de error y sale.
	*/
	if(sockfd < 0) 
		error("ERROR abriendo socket");
	

	/*	La funcion gethostbyname() toma un nombre como argumento y devuelve un puntero a un 
		servidor que contiene información sobre ese host. El campo contiene la dirección IP. 
		Si esta estructura es NULL, el sistema no pudo localizar un host con este nombre.
	*/
	server = gethostbyname("localhost");
	if(server == NULL) {
		fprintf(stderr,"ERROR, no hay tal host\n");
		exit(0);
	}

	/*	Este código establece los campos en serv_addr. Gran parte es lo mismo que en el servidor. 
		La función bzero () establece todos los valores en un búfer a cero. Toma dos argumentos, 
		el primero es un puntero al búfer y el segundo es el tamaño del búfer. Por lo tanto, esta 
		línea inicializa serv_addr a ceros.
		
		Sin embargo, debido a que el servidor de server-> h_addr es una cadena de caracteres, usamos 
		la función: bcopy().
		Copia bytes de longitud de "(char *)server->h_addr" a "char *)&serv_addr.sin_addr.s_addr".

		La variable serv_addr es una estructura de tipo struct sockaddr_in. Esta estructura tiene 
		cuatro campos. El primer campo es short sin_family, que contiene un código para la familia 
		de direcciones. Siempre debe establecerse en la constante simbólica AF_INET.

		El segundo campo de serv_addr es sin_port, que contiene el número de puerto. Sin embargo, 
		en lugar de simplemente copiar el número de puerto a este campo, es necesario convertirlo 
		a orden de bytes de red utilizando la función htons () que convierte un número de puerto en 
		orden de bytes de host a un número de puerto en orden de bytes de red.
	
	*/	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(numPuerto);
	
	/*	
	
	*/
	if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR conectando");

    //Imprimiendo el reloj logico de la maquina local 
	cout << "Mi reloj logico: " << relojLogico << endl; 

	//Se inicializa el buffer de intercambio de 256 bits de tamaño en cero.
	bzero(buffer,256);
    //Lectura del reloj logico del servidor de tiempo
	numCart = read(sockfd,buffer,255);
	if (numCart < 0) 
		error("ERROR leyendo de socket");

	//Imprimiendo el reloj logico del servidor de tiempo
    cout << "Tiempo del servidor (TD) iniciaondo el algoritmo de Berkeley!\n Este es el reloj lógico de TD: " << buffer << endl;
	
	// se crean dos string's para la manipulaccion de los buffers
	stringstream ss, ss1; // Se declaran ss y ssl como stringstream para poder manipularlas por medio del buffer
	
	ss << buffer; // Se inserta el contenido del buffer en ss
	string tmpstr1 = ss.str(); // Se crea una string temporal para guardar lo almacenado en ss
	
	// Se convierte en relog de timepo del servidor de caracter a entero
	int tmp = atoi(tmpstr1.c_str());	 // Se convierte lo almacenado en la string temporal a entero y se almacena en tmp 
	
    // Cálculo de la diferencia de tiempo de la máquina local a partir del tiempo del server restando el valor de la variable relojLogico y la variable tmp
    // Que contiene el rejoj del reloj logico del servidor
	int diff = relojLogico - tmp; 		
	cout << "Diferencia de tiempo respecto al reloj logico: "<< diff << endl;
	
	// Se limpia el buffer de mensaje
	bzero(buffer,256);


	//Se asigna el contenido de diff como string en ssl la variable strinstream previamente declarada
	ss1 << diff;
	// se asigna a una variable temporal
	string tmpstr2 = ss1.str();
	// Se copia su contenido al buffer
	strcpy(buffer,tmpstr2.c_str());

    // Enviar la diferencia horaria de esta máquina al servidor de tiempo
	numCart = write(sockfd,buffer,strlen(buffer));	
	if (numCart < 0) 
		error("ERROR escribiendo al socket");

	bzero(buffer,256);
    // Lectura del valor promedio final a ajustar en el reloj lógico de la máquina local
	numCart = read(sockfd,buffer,255);			
	printf("Ajuste del reloj = %s\n",buffer);

	int adj_clock = atoi(buffer);
	
	relojLogico = relojLogico + adj_clock;

	cout << "Mi reloj ajustado: " << relojLogico << endl;

    //Cierre el socket del cliente y finalice la conexión.
	close(sockfd);	
	return 0;
}