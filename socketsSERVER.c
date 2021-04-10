#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
	struct sockaddr_in server_addr, client_addr;
	socklen_t size;
	int sd, sc;
	int val;
	char op; int32_t a, b, res;
	if ((sd = socket(AF_INET, SOCK_STREAM, 0))<0){
		printf ("SERVER: Error en el socket");
		return (0);
	}
	val = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(4200);
	bind(sd, (const struct sockaddr *)&server_addr,
	sizeof(server_addr));
	listen(sd, SOMAXCONN);
	size = sizeof(client_addr);
	
	
	while (1){
		printf("esperando conexion\n");
		sc = accept(sd, (struct sockaddr *)&client_addr,(socklen_t *)&size);
		read ( sc, (char *) &op, sizeof(char)); // recibe la operació
		read ( sc, (char *) &a, sizeof(int32_t)); // recibe a
		read ( sc, (char *) &b, sizeof(int32_t)); // recibe b
		if (op == 0) // procesa la petición
		res = ntohl(a) + ntohl(b);
		else
		res = ntohl(a) – ntohl(b);
		res = htonl(res);
		write(sc, (char *)&res, sizeof(int32_t)); // envía el resultado 
		close(sc); // cierra la conexión (sc)
	}
	close (sd);
	return(0);
} /*fin main */