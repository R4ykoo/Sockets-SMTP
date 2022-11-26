/*
** Fichero: servidor.c
** Autores:
** Daniel Lázaro Rubio DNI 70915667C
** Germán Francés Tostado DNI 70940996A
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#define PUERTO 15667
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	516	/* maximum size of packets to be received */
#define TAM_BUFFER 10
#define MAXHOST 128

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */

int validEmail(char str[]); // Funcion para comprobar que los emails son válidos
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
void finalizar(){ FIN = 1; }
FILE *fPet;
int main(argc, argv)
int argc;
char *argv[];
{

    int s_TCP, s_UDP, s_UDP_hijo;		/* connected socket descriptor */
    int ls_TCP;				/* listen socket descriptor */
    
    int cc;				    /* contains the number of bytes read */
     
    struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
    
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in clientaddr_in;	/* for peer socket address */
	int addrlen;
	
    fd_set readmask;
    int numfds,s_mayor;
    
    char buffer[BUFFERSIZE];	/* buffer for packets to be read into */
    
    struct sigaction vec;

		/* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

		/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
		/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	
	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	   }
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	    }

		/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
	//Archivo .log
	fPet = fopen("peticiones.log","a");// no sobreescribe, escribe al final en caso de existir el archivo
	setpgrp();

	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:     /* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		
		
			fclose(stdin);
			fclose(stderr);
		
		

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror(" sigaction(SIGCHLD)");
            fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
            exit(1);
            }
            
		    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
        vec.sa_handler = (void *) finalizar;
        vec.sa_flags = 0;
        if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGTERM)");
            fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            exit(1);
            }
        
		while (!FIN) {
            /* Meter en el conjunto de sockets los sockets UDP y TCP */
            FD_ZERO(&readmask);
            FD_SET(ls_TCP, &readmask);
            FD_SET(s_UDP, &readmask);
            /* 
            Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
            el conjunto de sockets (readmask)
            */ 
    	    if (ls_TCP > s_UDP) s_mayor=ls_TCP;
    		else s_mayor=s_UDP;

            if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
                if (errno == EINTR) {
                    FIN=1;
		            close (ls_TCP);
		            close (s_UDP);
                    perror("\nFinalizando el servidor. Se�al recibida en elect\n "); 
                }
            }
           else { 

                /* Comprobamos si el socket seleccionado es el socket TCP */
                if (FD_ISSET(ls_TCP, &readmask)) {
                    /* Note that addrlen is passed as a pointer
                     * so that the accept call can return the
                     * size of the returned address.
                     */
    				/* This call will block until a new
    				 * connection arrives.  Then, it will
    				 * return the address of the connecting
    				 * peer, and a new socket descriptor, s,
    				 * for that connection.
    				 */
    			s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen); //socket tcp del que se encargará el hijo a partir de  la llamada serverTCP()
    			if (s_TCP == -1) exit(1);
    			switch (fork()) {
        			case -1:	/* Can't fork, just exit. */
        				exit(1);
        			case 0:		/* Child process comes here. */
                    			close(ls_TCP); /* Close the listen socket inherited from the daemon. */
        				serverTCP(s_TCP, clientaddr_in);
        				exit(0);
        			default:	/* Daemon process comes here. */
        					/* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
        				close(s_TCP);
        			}
             } /* De TCP*/
          /* Comprobamos si el socket seleccionado es el socket UDP */
          if (FD_ISSET(s_UDP, &readmask)) {
                /* This call will block until a new
                * request arrives.  Then, it will
                * return the address of the client,
                * and a buffer containing its request.
                * BUFFERSIZE - 1 bytes are read so that
                * room is left at the end of the buffer
                * for a null character.
                */
                cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
                   (struct sockaddr *)&clientaddr_in, &addrlen);
                if ( cc == -1) {
                    perror(argv[0]);
                    printf("%s: recvfrom error\n", argv[0]);
                    exit (1);
                    }
                /* Make sure the message received is
                * null terminated.
                */
                buffer[cc]='\0';
                //Como aquí no se crea automático un socket al recibir (como en TCP) lo creamos para que posteriormente se encargue el hijo y liberar el principal -> s_UDP_hijo
                myaddr_in.sin_family = AF_INET;
			    myaddr_in.sin_addr.s_addr = INADDR_ANY;
                myaddr_in.sin_port = 0; 
				s_UDP_hijo = socket (AF_INET, SOCK_DGRAM, 0);

                if(s_UDP_hijo == -1){
                    perror(argv[0]);
                    printf("%s: No pudo crearse socket UDP cliente\n", argv[0]);
                    exit (1);
                }
				
                //Lo enlazamos a la dirección correspondiente como hicimos con el principal
                if (bind(s_UDP_hijo, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		            perror(argv[0]);
		            printf("%s: unable to bind address UDP_hijo\n", argv[0]);
		            exit(1);
	            }
                //como con el socketTCP, hacemos que el nuevo socket sea manejado por el hijo (codigo copiado de bloque TCP de arriba)
                switch (fork()) {
        			case -1:	/* Can't fork, just exit. */
        				exit(1);
        			case 0:		/* Child process comes here. */
                        //no necesitamos cerrar el socketUDP principal como en el caso de TCP, por lo que solo llamamos a la funcion			
        				serverUDP (s_UDP_hijo, buffer, clientaddr_in);
        				exit(0);
        			default:	/* Daemon process comes here. */
        					/* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
        				close(s_UDP_hijo);
        			}
                
                }
          }
		}   /* Fin del bucle infinito de atenci�n a clientes */
        /* Cerramos los sockets UDP y TCP */
        close(ls_TCP);
        close(s_UDP);
		fclose(fPet);
        printf("\nFin de programa servidor!\n");
        
	default:		/* Parent process comes here. */
		exit(0);
	}

}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;		/* keeps count of number of requests */
	char buf[BUFFERSIZE];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		/* remote host's name string */

	int len, len1, status,status2;
    struct hostent *hp;		/* pointer to host info for remote host */
    long timevar;			/* contains time returned by time() */
    
    struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */



	/*
	Cogemos los mismos mensajes que hemos utilizado en el serverUDP
	*/
    char ack[BUFFERSIZE]="220 Servicio de transferencia simple de correo preparado\r\n";
	char ok[BUFFERSIZE] = "250 OK\r\n";
	char dataRes[BUFFERSIZE] = "354 Comenzando con el texto del correo, finalice con .\r\n";
	char quitRes[BUFFERSIZE] = "221 Cerrando el servicio\r\n";
	char errRes[BUFFERSIZE] = "500 Error de sintaxis\r\n";
	int flagFlujo = 1;		
	int flagReceptor = 0;		
	int clientPort;
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
     status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
     if(status){
           	/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
			 /* inet_ntop para interoperatividad con IPv6 */
            if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");	


             }
			 
    /* Log a startup message. */
    time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	clientPort = ntohs(clientaddr_in.sin_port);
	fprintf(fPet,"Startup from %s on IP %s and port %u with protocol TCP at %s \n", hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort, (char *) ctime(&timevar));

	//Le notificamos al cliente que el servidorTCP ya está operativo
	if (send(s,ack, BUFFERSIZE, 0) != BUFFERSIZE){
		perror("serverTCP: No se ha podido enviar el mensaje de servicio preparado");
		printf("%s: send 220 error\n", "serverTCP");
		return;
	} 
	fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ack);

		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
					sizeof(linger)) == -1) {
		errout(hostname);
	}

		/* Go into a loop, receiving requests from the remote
		 * client.  After the client has sent the last request,
		 * it will do a shutdown for sending, which will cause
		 * an end-of-file condition to appear on this end of the
		 * connection.  After all of the client's requests have
		 * been received, the next recv call will return zero
		 * bytes, signalling an end-of-file condition.  This is
		 * how the server will know that no more requests will
		 * follow, and the loop will be exited.
		 */
	while (len = recv(s, buf, BUFFERSIZE, 0)) {
		if (len == -1) errout(hostname); /* error from recv */
			/* The reason this while loop exists is that there
			 * is a remote possibility of the above recv returning
			 * less than TAM_BUFFER bytes.  This is because a recv returns
			 * as soon as there is some data, and will not wait for
			 * all of the requested data to arrive.  Since TAM_BUFFER bytes
			 * is relatively small compared to the allowed TCP
			 * packet sizes, a partial receive is unlikely.  If
			 * this example had used 2048 bytes requests instead,
			 * a partial receive would be far more likely.
			 * This loop will keep receiving until all TAM_BUFFER bytes
			 * have been received, thus guaranteeing that the
			 * next recv at the top of the loop will start at
			 * the begining of the next request.
			 */
		while (len < BUFFERSIZE) {
			len1 = recv(s, &buf[len], BUFFERSIZE-len, 0);
			if (len1 == -1) errout(hostname);
			len += len1;
		}
			/* Increment the request count. */
		reqcnt++;
			/* This sleep simulates the processing of the
			 * request that a real server might do.
			 */
		sleep(1);
			/* Send a response back to the client. */
		buf[len]='\0';
		//Tratamos la orden recibida y su respuesta igual que con serverUDP
		fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- RECIBO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,buf);
		

		char *checker = NULL;		
		//Mientras lea data
		if(flagFlujo == 4){
			// Está leyendo data, solo va a parar cuando lea un . solo. Revisar strcmp
			checker = strstr(buf, ".");
			if( checker == buf) {
				// Fin de envío de datos	
				if(send (s, ok, BUFFERSIZE, 0) != BUFFERSIZE){
					perror("serverTCP: No se ha podido enviar el mensaje de OK en FINTXT");
					printf("%s: send FINTXT 250 error\n", "serverTCP");
					return;
				} 
					fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ok);
					flagFlujo = 2;
					flagReceptor = 0;
				continue;
				
			}	
			continue;
		}
		checker = strstr(buf, "HELO");	
		if(checker == buf && flagFlujo == 1){
			// Comienza por HELO
			if(send (s, ok, BUFFERSIZE, 0) != BUFFERSIZE){
					perror("serverTCP: No se ha podido enviar el mensaje de OK en HELO");
					printf("%s: send HELO 250 error\n", "serverTCP");
					return;
				} 
			fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ok);
			flagFlujo=2;
			continue;
		}

		checker = strstr(buf, "MAIL FROM:");
		if(checker == buf && flagFlujo == 2){
			// Comienza por MAIL FROM:
			// Comprobamos que el email sea válido.

			if(validEmail(checker) != 0){ // El email es valido, mandamos ok
				if(send (s, ok, BUFFERSIZE, 0) != BUFFERSIZE){
						perror("serverTCP: No se ha podido enviar el mensaje de OK en MAIL FROM");
						printf("%s: send MAIL 250 error\n", "serverTCP");
						return;
					} 
				fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ok);
				flagFlujo = 3;
				continue;
			}
			// Al no ser válido, continuará hacia abajo hasta que mande el error 500 error de sintaxis
		}

		checker = strstr(buf, "RCPT TO:");
		if(checker == buf && flagFlujo == 3){
			// Comienza por RCPT TO:
			// Comprobamos que el email sea válido.

			if(validEmail(checker) != 0){ // El email es valido, mandamos ok
				if(send (s, ok, BUFFERSIZE, 0) != BUFFERSIZE){
						perror("serverTCP: No se ha podido enviar el mensaje de OK en RCPT TO");
						printf("%s: send RCPT 250 error\n", "serverTCP");
						return;
					}
				fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ok);
				flagReceptor = 1;
				continue;
			}
			// Al no ser válido, continuará hacia abajo hasta que mande el error 500 error de sintaxis
		}

		checker = strstr(buf, "DATA");
		if(checker == buf && flagFlujo == 3 && flagReceptor != 0){
			// Comienza por DATA
			// Activar el flag de que está leyendo DATA
			flagFlujo=4;
			if(send (s, dataRes, BUFFERSIZE, 0) != BUFFERSIZE){
					perror("serverTCP: No se ha podido enviar el mensaje de 354 en DATA");
					printf("%s: send DATA 354 error\n", "serverTCP");
					return;
				} 		
			fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,dataRes);
			continue;
		}
		
		checker = strstr(buf, "QUIT");
		if(checker == buf){
			// Comienza por QUIT
			if(send (s, quitRes, BUFFERSIZE, 0) != BUFFERSIZE){
					perror("serverTCP: No se ha podido enviar el mensaje de 221 en QUIT");
					printf("%s: send QUIT 221 error\n", "serverTCP");
					return;
				} 
			fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,quitRes);
			// activamos flag para que pare de recibir mensajes y se cierre el servicio.
			
			break;
		}
		
		/*
		 Si aun no ha salido, del bucle es que no comienza por ninguna de las ordenes
		 por lo que será un error de sintaxis. 
		*/

		if(send (s, errRes, BUFFERSIZE, 0) != BUFFERSIZE){
					perror("serverTCP: No se ha podido enviar el mensaje de 500 en ERRSYN");
					printf("%s: send ERRSYN 500 error\n", "serverTCP");
					return;
				} 
			fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo TCP- ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,errRes);
			continue;
	}

		/* The loop has terminated, because there are no
		 * more requests to be serviced.  As mentioned above,
		 * this close will block until all of the sent replies
		 * have been received by the remote host.  The reason
		 * for lingering on the close is so that the server will
		 * have a better idea of when the remote has picked up
		 * all of the data.  This will allow the start and finish
		 * times printed in the log file to reflect more accurately
		 * the length of time this connection was used.
		 */
	close(s);

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */

	fprintf(fPet,"Completed %s  IP:%s port %u, on TCP protocol, at %s\n",
		hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort, (char *) ctime(&timevar));
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	fprintf(stderr,"Connection with %s aborted on error\n", hostname);
	exit(1);     
}


/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in)
{
    struct in_addr reqaddr;	/* for requested host's address */
    struct hostent *hp;		/* pointer to host info for requested host */
    int nc, errcode,status;
	int flagFlujo = 1;
	int flagReceptor = 0;
	int clientPort;
    struct addrinfo hints, *res;
	char hostname[MAXHOST];
	long timevar;			/* contains time returned by time() */

	int addrlen;
   	addrlen = sizeof(struct sockaddr_in);
	/*
		GERMAN:
		Quizas es mejor que los mensajes sean structs con un elemento que sea el numero y otro que sea la descripcion
	*/
	char ack[BUFFERSIZE]="220 Servicio de transferencia simple de correo preparado\r\n";
	char ok[BUFFERSIZE] = "250 OK\r\n";
	char dataRes[BUFFERSIZE] = "354 Comenzando con el texto del correo, finalice con .\r\n";
	char quitRes[BUFFERSIZE] = "221 Cerrando el servicio\r\n";
	char errRes[BUFFERSIZE] = "500 Error de sintaxis\r\n";
	int flagQuit = 1;
	
	/*Al no tener UDP confirmación, debemos enviar en bucle desde el cliente la misma línea de fichero hasta que recibamos aquí la línea 
	y confirmemos que la hemos recibido para que pase a enviar la siguiente en bucle*/

/*
	GERMAN:
	Antes de entrar al bucle de req-res debería indicar que el servidor está preparado, mandando el mensaje 220
	Luego, entrará en bucle infinito hasta que se active una flag, que será cuando reciba QUIT ó algún error.
	En este bucle infinito lo primeroque hace es recibir, con la linea recibida saca la orden o error y hace lo necesario
*/
	 status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
     if(status){
           	/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
			 /* inet_ntop para interoperatividad con IPv6 */
            if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");	
             }
			 
    /* Log a startup message. */
    time (&timevar);
	clientPort = ntohs(clientaddr_in.sin_port);
	fprintf(fPet,"Startup from %s on IP %s and port %u with protocol UDP at %s \n", hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort, (char *) ctime(&timevar));


	nc = sendto (s, ack, strlen(ack), 0, (struct sockaddr *)&clientaddr_in, addrlen);

	if (nc == -1) {
		perror("serverUDP: No se ha podido enviar el mensaje de servicio preparado");
		printf("%s: sendto 220 error\n", "serverUDP");
		return;
	}

	fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ack);

	while(flagQuit != 0) {

		nc = recvfrom(s, buffer, BUFFERSIZE - 1, 0,
				(struct sockaddr *)&clientaddr_in, &addrlen);
	
		if ( nc == -1) {
			perror("serverUDP");
			printf("%s: recvfrom error\n", "serverUDP");
			return;
		}

		buffer[nc]='\0';

		/*
		 Ahora en buffer tenemos la req del cliente, tenemos que tratarla y hacer lo consecuente
		*/
		fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - RECIBO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,buffer);
		
			
		char *checker = NULL;

		if(flagFlujo == 4){
			// Está leyendo data, solo va a parar cuando lea un . solo

			/* 
			strcmp(buffer,".") No funciona aquí, no sé por qué, lo haré con el metodo de checker, 
			pero si en el cuerpo de Data alguna frase comienza con punto, se lo tragará como si eso es el final.
			*/
			checker = strstr(buffer, ".");	
			if(checker == buffer){
				// Fin de envío de datos
				nc = sendto (s, ok, strlen(ok), 0, (struct sockaddr *)&clientaddr_in, addrlen);

				if (nc == -1) {
					perror("serverUDP: No se ha podido enviar el mensaje de OK en FINTXT");
					printf("%s: sendto FINTXT 250 error\n", "serverUDP");
					return;
				}
				fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ok);
				flagFlujo = 2;
				flagReceptor = 0;
				continue;
			}
			// Sigue recibiendo datos ya que todavía no ha llegado el punto solitario
			continue;
		}

		checker = strstr(buffer, "HELO");	
		if(checker == buffer && flagFlujo == 1){
			// Comienza por HELO
			nc = sendto (s, ok, strlen(ok), 0, (struct sockaddr *)&clientaddr_in, addrlen);

			if (nc == -1) {
				perror("serverUDP: No se ha podido enviar el mensaje de OK en HELO");
				printf("%s: sendto HELO 250 error\n", "serverUDP");
				return;
			}
			fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ok);
			flagFlujo = 2;
			continue;
		}

		checker = strstr(buffer, "MAIL FROM:");
		if(checker == buffer && flagFlujo == 2){
			// Comienza por MAIL FROM:
			// Comprobamos que el email sea válido.

			if(validEmail(checker) != 0){ // El email es valido, mandamos ok
				nc = sendto (s, ok, strlen(ok), 0, (struct sockaddr *)&clientaddr_in, addrlen);

				if (nc == -1) {
					perror("serverUDP: No se ha podido enviar el mensaje de OK en MAIL");
					printf("%s: sendto MAIL 250 error\n", "serverUDP");
					return;
				}
				fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ok);
				flagFlujo = 3;
				continue;
			}
			// Al no ser válido, continuará hacia abajo hasta que mande el error 500 error de sintaxis
		}

		checker = strstr(buffer, "RCPT TO:");
		if(checker == buffer && flagFlujo == 3){
			// Comienza por RCPT TO:
			// Comprobamos que el email sea válido.

			if(validEmail(checker) != 0){ // El email es valido, mandamos ok
				nc = sendto (s, ok, strlen(ok), 0, (struct sockaddr *)&clientaddr_in, addrlen);

				if (nc == -1) {
					perror("serverUDP: No se ha podido enviar el mensaje de OK en RCPT");
					printf("%s: sendto RCPT 250 error\n", "serverUDP");
					return;
				}
				fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,ok);
				flagReceptor = 1;
				continue;
			}
			// Al no ser válido, continuará hacia abajo hasta que mande el error 500 error de sintaxis
		}

		checker = strstr(buffer, "DATA");
		if(checker == buffer && flagFlujo == 3 && flagReceptor == 1){
			// Comienza por DATA
			// Activar un flag de que está leyendo DATA
			flagFlujo = 4;
			nc = sendto (s, dataRes, strlen(dataRes), 0, (struct sockaddr *)&clientaddr_in, addrlen);

			if (nc == -1) {
				perror("serverUDP: No se ha podido enviar el mensaje de 354 en DATA");
				printf("%s: sendto DATA 354 error\n", "serverUDP");
				return;
			}
			fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,dataRes);
			continue;
		}
		
		checker = strstr(buffer, "QUIT");
		if(checker == buffer){
			// Comienza por QUIT
			nc = sendto (s, quitRes, strlen(quitRes), 0, (struct sockaddr *)&clientaddr_in, addrlen);

			if (nc == -1) {
				perror("serverUDP: No se ha podido enviar el mensaje de 221 en QUIT");
				printf("%s: sendto QUIT 221 error\n", "serverUDP");
				return;
			}
			fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,quitRes);
			// activamos flag para que pare de recibir mensajes y se cierre el servicio.
			flagQuit = 0;
			break;
		}
		
		/*
		 Si aun no ha salido, del bucle es que no comienza por ninguna de las ordenes
		 por lo que será un error de sintaxis. 
		*/

		nc = sendto (s, errRes, strlen(errRes), 0, (struct sockaddr *)&clientaddr_in, addrlen);

		if (nc == -1) {
				perror("serverUDP: No se ha podido enviar el mensaje de 500 en ERRSYN");
				printf("%s: sendto ERRSYN 500 error\n", "serverUDP");
				return;
			}
		fprintf(fPet,"SERVIDOR %s con IP:%s, puerto %u y protocolo UDP - ENVIO: %s\n",hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort,errRes);

	}
	fprintf(fPet,"Completed %s  IP:%s port %u, on UDP protocol, at %s\n",
		hostname,inet_ntoa(clientaddr_in.sin_addr),clientPort, (char *) ctime(&timevar));	 
 }


int validEmail(char str[]){
	int i = 0;
	fprintf(stdout,"%s\n",str);
	do{
		i++;
		if(str[i] == '\0') i=0;
		if(str[i] == '@'){
			if(str[i-1] != '\0' && str[i+1] != '\0'){
				return 1;
			}
		}
	}while(i != 0);
	return 0;
}