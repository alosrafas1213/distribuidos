//Cliente para sincronización de relojes mediante el algoritmo de Cristian, usando directamente la interfaz de sockets.

#include <sys/socket.h>
#include <sys/types.h>
#include <cerrno>
#include <netdb.h>  			
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <sys/time.h>
#include <csignal>
#include <iostream>
#include <cstdlib>

#define TAMBUF 100

char nom_serv[80];				//nombre de la máquina servidora
char open1[] = "OPEN";
char sync1[] = "SYNC";
char term1[] = "TERM";		

char okey1[] = "OKEY";
char finn1[] = "FINN";	

char *comandos[] = {open1,sync1,term1};	//comandos de comunicación con el servidor
char *respuestas[] = {okey1,finn1};		//respuestas del servidor

struct timeval Dmin;

int compara_respuestas(char *s1,char *s2);
//Compara dos cadenas de exactamente 4 caracteres
int parser_respuestas (char *respuesta);
//Identifica el comando de que se trata
int calcular_desviacion_absoluta (long *sgs, long *usgs);
//Devuelve 1 sii el reloj local estuviese adelantado.
//        -1 sii el reloj local estuviese retrasado.
//         0 sii el reloj local estuviese ajustado.
int menorqueDmin(struct timeval *nuevaD);
//Devuelve 1 sii el tiempo D que se le pasa como argumento es menor que el que se tenía como mínimo hasta el momento.
//         0 e. o. c.
void sincronizar(int s);
void acabar(int s);

int main()
{
	/* puerto - el número de puerto en el que el servidor acepta conexiones.
	
	
	*/
	int sock, k, puerto, i;
	/*	server - es una estructura que contiene una dirección de internet. 
					Esta estructura se define en <netinet / in.h>. 
					La variable server contendrá la dirección del servidor.
	
	*/
  	struct sockaddr_in server;
	
	/*
		La variable hp es un puntero a una estructura de tipo hostent. 
		Esta estructura se define en el archivo de encabezado netdb.h.
		*gethostbyname - es un puntero hacia la IP del host 
	*/
  	struct hostent *hp, *gethostbyname2();

	// buffer para lectura/escritura en el socket.
  	char buf[TAMBUF];
	// Seleccionar una opcion en el menu
	char opcion[50];

	system("clear");

	printf("APLICACION: Sincronizacion de relojes siguiendo el algoritmo centralizado\n");
	printf("            de Cristian.\n\n");

 	//pedir la ip del servidor.

  	printf("Introduce el ip del servidor\n ");
	scanf("%s", nom_serv);

	//Pedir el puerto por donde se va a realizar la comunicación 
	printf("\n\nIntroduce un puerto (a partir de %d): ",IPPORT_RESERVED+1);
	scanf("%d", &puerto);

	while (puerto <= IPPORT_RESERVED)
	{
		printf("\n\nEl puerto %d esta reservado. Intenta otra vez.\n",puerto);
		printf("\nPuerto (a partir de %d): ",IPPORT_RESERVED+1);
		scanf("%d", &puerto);
	}

  	//Abrimos el socket para establecer la conexión.

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

  	sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (sock <0)
	{
		perror("No se puede crear el socket");
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
  	server.sin_family = AF_INET;
  	server.sin_port = htons(puerto);

  	//Extraer del DNS la información del servidor.

  	hp = gethostbyname(nom_serv);
  	if (hp == NULL)
  	{
		perror("gethostbyname");
	  	close(sock);
	  	exit(1);
  	}

	// Copia en bytes asignadospor hp-> h_length desde la ubicación apuntada por hp-> h_addr directamente al bloque de memoria al que apunta &server.sin_addr.
  	memcpy(&server.sin_addr, hp->h_addr, hp->h_length);

	/* 	La llamada al sistema connect () conecta el socket mencionado por el descriptor de archivo 
		sock a la dirección especificada por server. La dirección y el puerto del servidor se 
		especifican en server.

		La funcion regresa 
		0 en caso de éxito.
		-1 en caso de error y establece perror para indicar el error.
	*/
  	if (connect(sock,(struct sockaddr*)&server, sizeof server) < 0)
	{
		perror("connect");
	  	close(sock);
	  	exit(1);
  	}

  	//Empieza el protocolo de aplicación con el envío de comando OPEN.

  	printf("\n\nConectando al puerto %d de la maquina, %s ...\n",puerto,nom_serv);
  	sprintf(buf,"%s", comandos[0]);
  	write(sock, buf, strlen(buf));

  	//Si estamos aquí, deberemos recibir el OKEY de confirmación.

  	if ((k == read(sock, buf, TAMBUF)) < 0)
  	{
		perror("read");
	  	close(sock);
	  	exit(1);
  	}

  	switch(parser_respuestas(buf))
  	{
  		case 0:	//correcto inicio de sesión
	  		printf("\nInicio de sesion establecido ...\n");
	  		break;

  		case 1:	//inicio de sesión fallido.
	  		printf("\nInicio de sesion fallido ...\n");
	  		close(sock);
	         	exit(1);

  		default://no sabemos qué ha pasado ...
	  		printf("\nError de protocolo ... Abandono.\n");
	  		close(sock);
	  		exit(1);
  	}

	//Inicializar Dmin con un valor lo suficientemente grande.

	Dmin.tv_sec=1000;	//por ejemplo. (1000 sgs.)
	Dmin.tv_usec=1000000;

  	while(1)
  	{
		printf("\n--------------------------------------");
		printf("\n             Elija la opcion que desee:");
	  	printf("\n              1.- Sincronizar el reloj");
	  	printf("\n             2.- Salir");
		printf("\n--------------------------------------\n\n");
	  	scanf("%s",opcion);

	  	switch(opcion[0])
	     	{
	       		case '1':	sincronizar(sock); break;
	       		case '2':	acabar(sock); break;
               		default:	printf("\nOpcion desconocida. Vuelva a intentarlo.\n"); break;
	     	}
  	}
}

int compara_respuestas(char *s1,char *s2)
//Compara dos cadenas de exactamente 4 caracteres
{
	int i;
	int result = 1;
	for (i=0; i<4; i++)
		if(s1[i] != s2[i]) result = 0;
	return(result);
}

int parser_respuestas (char *respuesta)
//Identifica el comando que se va realizar.
{
	if ((compara_respuestas(respuesta,respuestas[0])) == 1) return(0); //OKEY
	if ((compara_respuestas(respuesta,respuestas[1])) == 1) return(1); //FIN
	return(-1);
}

int calcular_desviacion_absoluta (long *sgs, long *usgs)
//Devuelve 1 si el reloj local estuviese adelantado.
//        -1 si el reloj local estuviese retrasado.
//         0 si el reloj local estuviese ajustado.
{
	if (*sgs > 0)
	{
		printf("adelantado.");
		if (*usgs < 0)
		{
			*usgs=(*usgs) + 1000000;
			*sgs--;
		}
		return(1);
	}
	else if (*sgs == 0)
	{
		if (*usgs > 0)
		{
			printf("adelantado.");
			return(1);
		}
		else if (*usgs == 0)
		{
			printf("ajustado.");
			return(0);
		}
		else
		{
			printf("retrasado.");
			*usgs=(*usgs) * (-1);
			return(-1);
		}
	}
	else
	{
		*sgs=(*sgs) * (-1);
		printf("retrasado.");
		if (*usgs > 0)
		{
			*usgs=(*usgs) * (-1) + 1000000;
			*sgs=(*sgs) - 1;
		}
		else if (*usgs < 0) *usgs=(*usgs) * (-1);
		return(-1);
	}
}

int menorqueDmin(struct timeval *nuevaD)
//Devuelve 1 sii el tiempo D que se le pasa como argumento es menor que el que se tenía como mínimo hasta el momento.
//         0 e. o. c.
{
	if (nuevaD->tv_sec < Dmin.tv_sec) return(1);
	else if (nuevaD->tv_sec > Dmin.tv_sec) return(0);
	else
	{
		if (nuevaD->tv_usec < Dmin.tv_usec) return(1);
		else return(0);
	}
}

void sincronizar(int s)
{
	//Variables para la manipulacion del reloj
  	char buf[TAMBUF];
	int k, adelantado;


	struct timeval t0, t1, D, desviacion, desviacionabsoluta, tmt, t, ajuste;
	struct tm *tmp, *tmp2, *tmp3;

	system("clear");

	//Anotamos la hora actual justo antes de la petición de la hora al servidor.

	gettimeofday(&t0,NULL);

	//Enviamos el comando SYNC para empezar la sincronización.

	sprintf(buf, "%s", comandos[1]);
	if (write(s, buf, strlen(buf)) < strlen(buf))
	{
		perror("Error al enviar el comando SYNC. Abandono..");
		close(s);
		exit(0);
	}

	//Recibimos la hora proporcionada por el servidor.

	if ((k = read(s, buf, TAMBUF)) <0)
	{
		perror("Leyendo la hora recibida por el servidor.");
		exit(1);
	}

	//Anotamos la hora actual, tras haber recibido la hora del servidor.

	gettimeofday(&t1,NULL);

	//Calculo de D. D=t1-t0

	D.tv_sec=t1.tv_sec-t0.tv_sec;
	D.tv_usec=t1.tv_usec-t0.tv_usec;

	//Se restan por un lado segundos y por otro lado microsegundos. La diferencia (t1-t0) puede no ser real.
	//t1 > t0

	if (D.tv_usec < 0)
	{
		D.tv_usec=D.tv_usec + 1000000;
		D.tv_sec--;
	}

	printf("Intento. D igual a, %ld sgs. y %ld usgs.\n",D.tv_sec,D.tv_usec);

	//Manipular la hora del servidor recibida.

	tmt.tv_sec=atol(&buf[10]);			//segundos y microsegundos de t(mt).
	tmt.tv_usec=atol(&buf[35]);
	tmp=(struct tm*)localtime(&(tmt.tv_sec));

	t.tv_sec=t1.tv_sec-(D.tv_sec/2);
	t.tv_usec=t1.tv_usec-(D.tv_usec/2);

	//Se restan por un lado segundos y por otro lado microsegundos. La diferencia puede no ser real.

	if (t.tv_usec < 0)
	{
		t.tv_usec=t.tv_usec + 1000000;
		t.tv_sec--;
	}

	tmp2=(struct tm*)localtime(&(t.tv_sec));

	printf("\n               t: %d hrs. %d mns. %d sgs. %ld usgs.",tmp2->tm_hour,tmp2->tm_min,tmp2->tm_sec,t.tv_usec);
	printf("\n           t(mt): %d hrs. %d mns. %d sgs. %ld usgs. (Servidor de tiempos)\n",tmp->tm_hour,tmp->tm_min,tmp->tm_sec,tmt.tv_usec);

	//Se calcula la diferencia de hora entre nuestro reloj y el del servidor, y lo ajustamos.

	//La desviación actual del reloj local vendrá dada por: desviacion=t-t(mt)

	printf("\nLuego, tendrias el reloj ");

	desviacionabsoluta.tv_sec=t.tv_sec-tmt.tv_sec;
	desviacionabsoluta.tv_usec=t.tv_usec-tmt.tv_usec;

	//Se resta por un lado segundos y por otro lado microsegundos.

	adelantado=calcular_desviacion_absoluta(&desviacionabsoluta.tv_sec,&desviacionabsoluta.tv_usec);

	//adelantado toma el valor:  1 si el reloj local está adelantado.
	//                          -1 si el reloj local está retrasado.
	//                           0 si el reloj local está ajustado.


	tmp3=(struct tm*)localtime(&(desviacionabsoluta.tv_sec));
	printf("\nDesviacion |t- t(mt)|: %d hrs. %d mns. %d sgs. %ld usgs.\n\n",tmp3->tm_hour-1,tmp3->tm_min,tmp3->tm_sec,desviacionabsoluta.tv_usec);

	printf("El valor de D obtenido en este intento,\n");

	if (menorqueDmin(&D) == 0)
	{
		printf("NO es MENOR que el minimo obtenido hasta el momento.\n");
		printf("Minimo D: %ld sgs. y %ld usgs.\n\n\nSe opta por NO ajustar. No hay garantias de una mejor precision.\n\n",Dmin.tv_sec,Dmin.tv_usec);
	}
	else
	{
		printf("SI es MENOR que el minimo obtenido hasta el momento.\n");
		Dmin.tv_sec=D.tv_sec;
		Dmin.tv_usec=D.tv_usec;
		printf("Nuevo minimo D: %ld sgs. y %ld usgs.\n\n\n",Dmin.tv_sec,Dmin.tv_usec);

		gettimeofday(&ajuste,NULL);

		if (adelantado==1) 		//restar diferencia
		{
			ajuste.tv_usec=ajuste.tv_usec-desviacionabsoluta.tv_usec;

			if (ajuste.tv_usec < 0)
			{
				ajuste.tv_usec=ajuste.tv_usec + 1000000;
				ajuste.tv_sec--;
			}
			ajuste.tv_sec=ajuste.tv_sec-desviacionabsoluta.tv_sec;

			if ((settimeofday(&ajuste,NULL)) <0) perror("Ajustando reloj ...");
			else
			{
				tmp3=(struct tm*)localtime(&(ajuste.tv_sec));
				printf("               Sincronizado reloj: %d hrs. %d mns. %d sgs. %ld usgs.\n\n",tmp3->tm_hour,tmp3->tm_min,tmp3->tm_sec,ajuste.tv_usec);
			}
		}
		else if (adelantado==-1)	//sumar diferencia
		{
			ajuste.tv_usec=ajuste.tv_usec+desviacionabsoluta.tv_usec;

			if (ajuste.tv_usec >= 1000000)
			{
				ajuste.tv_usec=ajuste.tv_usec - 1000000;
				ajuste.tv_sec++;
			}
			ajuste.tv_sec=ajuste.tv_sec+desviacionabsoluta.tv_sec;

			if ((settimeofday(&ajuste,NULL)) <0) perror("Ajustando reloj ...");
			else
			{
				tmp3=(struct tm*)localtime(&(ajuste.tv_sec));
				printf("               Sincronizado reloj: %d hrs. %d mns. %d sgs. %ld usgs.\n\n",tmp3->tm_hour,tmp3->tm_min,tmp3->tm_sec,ajuste.tv_usec);
			}
		}
		else				//se deja tal cual
		{
			tmp3=(struct tm*)localtime(&(ajuste.tv_sec));
			printf("               Sincronizado reloj: %d hrs. %d mns. %d sgs. %ld usgs.\n\n",tmp3->tm_hour,tmp3->tm_min,tmp3->tm_sec,ajuste.tv_usec);
		}
	}
}

void acabar(int s)
{
	char buf[TAMBUF];
	int k;

	//Enviamos el comando TERM

	sprintf(buf,"%s",comandos[2]);

	if (write(s, buf, strlen(buf)) < strlen(buf))
	{
		perror("Error al enviar el comando TERM, Abandono..");
		close(s);
		exit(0);
	}

	//Esperamos la recepción de la cadena OKEY.

	if ((k = read(s, buf, 4)) < 0)
	{
		perror("Leyendo respuesta al comando TERM");
		exit(1);
	}

	//Verificamos que hemos leído un OKEY.

	switch(parser_respuestas(buf))
	{
		case 0:	//Correcto, cerramos la conexión.
			close(s);
			exit(0);
		case 1:	//Algo anda mal.
			printf("Rechazado el comando TERM\n");
			close(s);
			return;
		default://No sabemos qué ha pasado ...
			close(s);
			exit(1);
	}
}
