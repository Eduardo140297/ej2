#include <stdio.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
int main(int argc, char **argv) // en argv[1] == servidor 
{
	int sd;
	struct sockaddr_in server_addr;
	struct hostent *hp;
	int32_t a, b , res; char op;
	if (argc != 2){
		printf("Uso: cliente <direccion_servidor> \n");
		return(0);
	}
	sd = socket(AF_INET, SOCK_STREAM, 0);
	bzero((char *)&server_addr, sizeof(server_addr));
	hp = gethostbyname (argv[1]);
	memcpy (&(server_addr.sin_addr), hp->h_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(4200);
	// se establece la conexión
	connect(sd, (struct sockaddr *) &server_addr, 
	sizeof(server_addr));
	
	a = htonl(5);
	b = htonl(2);
	op = 0; // suma
	write(sd, (char *) &op, sizeof(char)); // envía la operacion
	write(sd, (char *) &a, sizeof(int32_t)); // envía a
	write(sd, (char *) &b, sizeof(int32_t)); // envía b
	read(sd, (char *) &res, sizeof(int32_t)); // recibe la respuesta
	printf("Resultado es %d \n", ntohl(res));
	close (sd);
	return(0);
} /* fin main */