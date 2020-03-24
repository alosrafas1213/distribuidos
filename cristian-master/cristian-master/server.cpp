/* Servidor de tiempos (modo concurrente).
 * Sincronización de relojes, siguiendo el algoritmo centralizado de Cristian. */

// Ficheros necesarios para el manejo de sockets, direcciones, etc.

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <csignal>
#include <cerrno>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>

using namespace std;

#define TAMBUF 100

// Comandos y respuestas definidos en el protocolo.

char *comandos[] =
{
	"OPEN",  // Solicitud de inicio de sesión.
	"SYNC",	// Iniciar proceso de sincronización.
	"TERM"	// Cierre de sesión.
};

int compara_comandos(char *s1, char *s2);
//Compara dos cadenas de exactamente 4 caracteres, identificando el comando.
int parser_comandos (char *comando);
//Identifica el comando de que se trata.
void limpiar_buffer (char *buffer, int inicio, int fin);
void traer (int s,struct sockaddr_in *info);

/***************************************************************************************************/

int main()
{
	int sock_escucha,sock_dialog;		// sockets de conexión y diálogo.
	
	/*
		dir_serv,dir_cli - es una estructura que contiene una dirección de internet. 
					Esta estructura se define en <netinet / in.h>. 
					La variable dir_serv contendrá la dirección del servidor.
					La variable dir_cliente contendra la direcion del cliente.
	*/
	struct sockaddr_in dir_serv,dir_cli;
	// buffer para lectura/escritura en el socket.
	char buf[TAMBUF];	
	/*	puerto - el número de puerto en el que el servidor acepta conexiones.
		lenght - el tamaño en bytes de la estructura señalada por dir_cli
	*/		
	int puerto;
	socklen_t length;

	system("clear");

	write(1,"ServidorCristian> conectar",25);

	//Obtenemos el puerto de conexion
	printf("\n\nPuerto (a partir de %d): ",IPPORT_RESERVED+1);
	scanf("%d",&puerto);

	while (!(puerto > IPPORT_RESERVED))
	{
		printf("\n\nEl puerto %d esta reservado. Intenta otra vez.",puerto);
		printf("\n\nPuerto (a partir de %d): ",IPPORT_RESERVED+1);
		scanf("%d",&puerto);
	}

	// Creamos el socket de escucha ...

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
	
		La llamada al sistema de socket devuelve una entrada en la tabla de descriptores de 
		archivo (es decir, un entero pequeño). Si la llamada del socket falla, devuelve -1. 
		En este caso, el programa muestra un mensaje de error y sale.
	*/

	if ((sock_escucha=socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		perror("\nApertura de socket");
		exit(1);
	}

	/*	
		La variable serv_addr es una estructura de tipo struct sockaddr_in. Esta estructura tiene 
		cuatro campos. El primer campo es short sin_family, que contiene un código para la familia 
		de direcciones. Siempre debe establecerse en la constante simbólica AF_INET.

		El segundo campo de serv_addr es sin_port, que contiene el número de puerto. Sin embargo, 
		en lugar de simplemente copiar el número de puerto a este campo, es necesario convertirlo 
		a orden de bytes de red utilizando la función htons () que convierte un número de puerto en 
		orden de bytes de host a un número de puerto en orden de bytes de red.
	
	*/	

	dir_serv.sin_family=AF_INET;
	dir_serv.sin_port=htons(puerto);
	dir_serv.sin_addr.s_addr=htonl(INADDR_ANY);	// Cualquiera puede conectarse al servidor.

	/*	Después de la creación del socket, la función bind() vincula el socket a la dirección 
		y al número de puerto especificados en addr (estructura de datos personalizada). 
		En el código, vinculamos el servidor al localhost, por lo tanto, usamos INADDR_ANY 
		para especificar la dirección IP.

		La funcion regresa 
		0 en caso de éxito.
		-1 en caso de error y establece perror para indicar el error.
	
	*/
	if (bind(sock_escucha,(struct sockaddr *)&dir_serv,sizeof(dir_serv)) < 0)
	{
		perror("\nBind");
		exit(1);
	}

	// Ponemos el socket a la escucha ...
	/*	Pone el socket del servidor en modo pasivo, donde espera a que el cliente se acerque al 
		servidor para establecer una conexión. La cartera de pedidos define la longitud máxima a 
		la que puede crecer la cola de conexiones pendientes para sock_escucha. Si llega una solicitud 
		de conexión cuando la cola está llena, el cliente puede recibir un error con una indicación 
		de ECONNREFUSED.

		La funcion regresa 
		0 en caso de éxito.
		-1 en caso de error y establece perror para indicar el error.
	*/
	if (listen(sock_escucha,7) < 0)
	{
		perror("\nListen");
		exit(1);
	}

	signal(SIGCHLD,SIG_IGN);	// evitar hijos zombies.

	printf("\n\nAPLICACION: Sincronizacion de relojes siguiendo el algoritmo centralizado\n");
	printf("            de Cristian.\n\n");
	printf("Servidor de tiempos, a la espera de peticiones ...\n");

	do
	{
		// Bloqueo a la espera de petición de conexión.
		/*	Extrae la primera solicitud de conexión en la cola de conexiones pendientes para el 
			socket de escucha, sock_escucha, crea un nuevo socket conectado y devuelve un nuevo 
			descriptor de archivo que hace referencia a ese socket. En este punto, se establece 
			una conexión entre el cliente y el servidor, y están listos para transferir datos.
			
			El segundo argumento es un puntero de referencia a la dirección del cliente 
			en el otro extremo de la conexión, y el tercer argumento es el tamaño de esta estructura.

		*/

		sock_dialog=accept(sock_escucha,(struct sockaddr *)&dir_cli,&length);

		//Hubo un error al aceptar en socket
		if (sock_dialog==-1)
		{
			// Tratamiento especial para interrupciones de hijos muertos.

			if (errno==EINTR) continue;
			perror("\nAccept");
			exit(1);
		}

		else
			switch(fork())
			{
				case -1:
					//Ocurrio un error creando un Hijo
					perror("\nCreando un hijo");
					break;

				case 0:
					//Se realiza la comunicacion con el cliente
					close(sock_escucha);
					traer(sock_dialog,&dir_cli);
					close(sock_dialog);
					exit(0);

				default:
					// Se cierra el socket 
					close(sock_dialog);
					break;
			}
	} while (1);

	exit(0);
}

/***************************************************************************************************/

int compara_comandos(char *s1, char *s2)
//Compara dos cadenas de exactamente 4 caracteres, identificando el comando.

{
	int i;
	int result=1;

	for (i=0; i<4; i++)
		if (s1[i] != s2 [i])
			result=0;

	return (result);
}

int parser_comandos (char *comando)
// Identifica el comando que se va realizar.
{
	if ((compara_comandos(comando, comandos[0])) == 1) return (0); // OPEN
	if ((compara_comandos(comando, comandos[1])) == 1) return (1); // SYNC
	if ((compara_comandos(comando, comandos[2])) == 1) return (2); // TERM

	return (-1);
}

void limpiar_buffer (char *buffer, int inicio, int fin)
/*	Limpia el buffer con el se esta trabajando.
	recibe un apuntador al buffer de tipo char
	Los indices de inicio y fin de la seccion que se va a limpiar como enteros
*/
{
	int i;

	for (i=inicio; i<=fin; i++) buffer[i]=' ';
}

void traer (int s,struct sockaddr_in *info)
//
{
	char *respuesta[] =
	{
	"OKEY",	// Puede llevar acompañado un parámetro indicando la hora (t(mt)) del servidor de de tiempos, o no llevar nada.
	"FINN"	// Puede ir acompañado de un string explicativo de porqué se ha producido la negativa.
	};

	// variables auxiliares.
	int k,lon,lon2;
	//	Buffer
	char buf[TAMBUF];

	/* 	horaservidor - Estructura que contendra los segundos y los microsegundos dentro de sus componentes de tipo long.
		(tv_sec,tv_usec)
		*tmp - Estructura que contiene una fecha y hora del calendario desglosadas en sus componentes de tipo int.
		(seg (0-61), min(0-59), horas(0-23), dia del mes(0-31), mes desde enero (0-11), año desde 1900)
	*/

	struct timeval horaservidor;
	struct tm *tmp;

	// Espera el envío del comando OPEN.
	k=read(s,buf,TAMBUF);

	//Error al abrir el buffer
	if (k<0)
	{
		perror("\nRecepcion Comando OPEN");
		return;
	}

	buf[k]='\0';

	//Se verifica el comando ingresado
	if ((compara_comandos(comandos[0],buf)) == 0)
	{
		perror("\nComando desconocido");
		sprintf(buf,"%s: Comando OPEN esperado",respuesta[1]);
		write(s,buf,strlen(buf));
	}

	// Todo correcto. Devolvemos OKEY y esperamos a que nos manden un comando, que desde este momento
	// puede ser: SYNC o TERM

	sprintf(buf,"%s: Sesion abierta",respuesta[0]);
	write(s,buf,strlen(buf));
	printf("\nIP=%ld       Conectado\n",ntohl(info->sin_addr.s_addr));

	// Entramos en el bucle de recepción de comandos.

	do
	{
		k=read(s,buf,TAMBUF);

		//Error al recibir el comando
		if (k<0)
		{
			perror("\nRecepción de un comando");
			return;
		}

		buf[k]='\0';

		switch(parser_comandos(buf))
		{
			case 1:	// SYNC

				/* calcular hora del servidor */

				gettimeofday(&horaservidor,NULL);

				// el servidor le envia como respuesta:
				//	OK +
				//	segundos 	(el valor comienza en &buf[10]) +
				//	microsegundos 	(el valor comienza en &buf[35])

				//Ingresamos el valor de los segundos del servidor al buffer
				sprintf(buf,"%s: seg:%ld\0",respuesta[0],horaservidor.tv_sec);
				//Obtenemos la longitud del buffer en lon
				lon=strlen(buf);
				//Limpiamos el buffer
				limpiar_buffer(buf,lon+1,29);
				//Ingresamos el valor de los microsegundos del servidor al buffer
				sprintf(&buf[30],"useg:%ld\0",horaservidor.tv_usec);
				//Obtenemos la longitud del buffer en lon2
				lon2=strlen(&buf[30]);
				//Escribimos en el socket el buffer el tamaño en bits que corresponde (es el tercer parametro)
				write(s,buf,30+lon2);

				//Se optine de manera temporal la hora local del servidor
				tmp=(struct tm*)localtime(&(horaservidor.tv_sec));
				printf("\nIP=%ld       Sincronizando ...\n",ntohl(info->sin_addr.s_addr));
				printf("                    t(mt): %d hrs. %d mns. %d sgs. %ld usgs.\n",tmp->tm_hour,tmp->tm_min,tmp->tm_sec,horaservidor.tv_usec);
				break;

			case 2:	// TERM - Se finaliza la conexion

				sprintf(buf,"%s: Cerrando sesion ...",respuesta[0]);
				write(s,buf,strlen(buf));
				close(s);
				printf("\nIP=%ld       Desconectado\n",ntohl(info->sin_addr.s_addr));
				return;

			case 0:	// OPEN 

				sprintf(buf,"%s: Error de protocolo en el cliente",respuesta[1]);
				write(s,buf,strlen(buf));
				close(s);
				return;

			default:

				sprintf(buf,"%s: Comando desconocido",respuesta[1]);
				write(s,buf,strlen(buf));
				close(s);
				return;
		}
	} while (1);
}

