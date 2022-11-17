/*
 *			C L I E N T C P
 *
 *	This is an example program that demonstrates the use of
 *	stream sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 */
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#define PUERTO 15667
#define TAM_BUFFER 10
#define BUFFERSIZE 1024
/*
 *			M A I N
 *
 *	This routine is the client which request service from the remote.
 *	It creates a connection, sends a number of
 *	requests, shuts down the connection in one direction to signal the
 *	server about the end of data, and then receives all of the responses.
 *	Status will be written to stdout.
 *
 *	The name of the system to which the requests will be sent is given
 *	as a parameter to the command.
 */
int main(argc, argv)
int argc;
char *argv[];
{
    int s;				/* connected socket descriptor */
   	struct addrinfo hints, *res;
    long timevar;			/* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
	int addrlen, i, j, errcode;
    /* This example uses TAM_BUFFER byte messages. */
	char buf[BUFFERSIZE];

	char * line = NULL;
    size_t len = 0;
    ssize_t read;
	char * checker = NULL;
	int flagData = 0;

	if (argc != 4) {
		fprintf(stderr, "Usage:  %s <remote host>\n", argv[0]);
		exit(1);
	}

	/* Create the socket. */
	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Set up the peer address to which we will connect. */
	servaddr_in.sin_family = AF_INET;
	
	/* Get the host information for the hostname that the
	 * user passed in. */
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
 	 /* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
    if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
        }
    else {
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	    }
    freeaddrinfo(res);

    /* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

		/* Try to connect to the remote server at the address
		 * which was just built into peeraddr.
		 */
	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
		exit(1);
	}
		/* Since the connect call assigns a free address
		 * to the local end of this connection, let's use
		 * getsockname to see what it assigned.  Note that
		 * addrlen needs to be passed in as a pointer,
		 * because getsockname returns the actual length
		 * of the address.
		 */
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	 }

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("Connected to %s on port %u at %s",
			argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));


	 //Abrir el archivo
	FILE *fOrd = fopen(argv[3], "r");

	if (fOrd == -1){
			perror(argv[0]);
        	fprintf(stderr, "Error al abrir el archivo %s", argv[3]);
        	exit(1);
	}
	i = recv(s, buf, BUFFERSIZE, 0);
	if (i == -1) {
            perror(argv[0]);
			fprintf(stderr, "%s: error reading result\n", argv[0]);
			exit(1);
		}
	else{
		fprintf(stdout,"CLIENTETCP antes while - Recibo: %s\n",buf);
			while ((read = getline(&line, &len, fOrd)) != -1) {				
				/*
					Enviará las lineas del fichero de ordenes y esperará respuesta.
					Si se envía DATA, no espera respuestas hasta que envíe .
				*/
				if (send (s, line, BUFFERSIZE, 0) != BUFFERSIZE) {
        			perror(argv[0]);
        			fprintf(stderr, "%s: unable to send line on TCP\n", argv[0]);
        			exit(1);
        		}				
				// Si envío el . termino de enviar Data y puedo recibir respuesta
				checker = strstr(line,".");
				if(checker == line) flagData = 0;
				// Si estoy enviando data, no tengo que esperar a recibir nada hasta que envíe el .
				if(flagData != 0){
					continue;
				}

				i = recv(s, buf, BUFFERSIZE, 0);
	
				if ( i == -1) {
					perror("clientTCP");
					fprintf(stderr,"%s: recv error\n", "clientTCP");
					exit(1);
				}

				buf[i]='\0';
				
				fprintf(stdout,"CLIENTETCP - Recibo: %s\n",buf);			

				// Comienza el envío de DATA, espera a recibir la respuesta 354, y luego no espera recepcion hasta que 
				// envíe un punto
				checker = strstr(line, "DATA");
				if(checker == line) flagData = 1;
				
    		}
	}
		/* Now, shutdown the connection for further sends.
		 * This will cause the server to receive an end-of-file
		 * condition after it has received all the requests that
		 * have just been sent, indicating that we will not be
		 * sending any further requests.
		 */
	if (shutdown(s, 1) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
		exit(1);
	}

    /* Print message indicating completion of task. */
	time(&timevar);
	printf("All done at %s", (char *)ctime(&timevar));
}
