#include <mqueue.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "linked_list.h"
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifndef REQUEST_H
  #define REQUEST_H
  #include "request.h"
#endif



////////////////////////////////////////////////////////////////////////////////
////////////////// V A R I A B L E S - G L O B A L E S  ////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Variables de control de capacidad del servidor
#define MAX_THREADS 	10
#define MAX_PETICIONES 	256

// Cola del servidor
mqd_t server_q;

// Lista enlzada para guardar tuplas
Linked_list list;

// Variables del buffer de peticiones
request buffer_peticiones[MAX_PETICIONES];
int n_elementos;
int pos = 0;

// MUTEX de condiciones de estado de la cola
pthread_mutex_t mutex;
pthread_cond_t no_lleno;
pthread_cond_t no_vacio;

// MUTEX de control de estado de los threads
pthread_mutex_t mfin;
int fin = false;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// S E R V I C I O S ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int init(){
  int result = init_list(&list);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


int exists(char *key){
  return item_exist(list, key);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int set_value(char *key , char *v1, int *v2, float *v3){
  int result;

  if (exists(key) == 0){ 
    printf("Error: la clave ya existe.");
    return -1;
  }

  // Obtenemos el valor de lo que contienen las direcciones v2, v3.
  result = set(&list, key, v1, *v2, *v3); 

  return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int get_value(char *key , char *v1, int *v2, float *v3){
  int result;

  if (exists(key) == -1){
    printf("Error: la clave no existe.");
    return -1;
  }

  // Mandamos los punteros tal y como vienen, se utilizaran las direcciones que 
  // contienen para guardar el resultado de la funcion
  result = get(&list, key, v1, v2, v3);
  if (result == -1){
    printf("Error: clave no encontrada.");
     // esta parte no llega a ejecutarse porque si no se encuentra se sale en la comprobacion de arriba
     // lo cual hace el código algo redundante.
    return -1;
  } else {
    return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int modify_value(char *key , char *v1, int *v2, float *v3){
  int result;

  if (exists(key) == -1){ // TODO Estoy pasando las variables adecuadamente?
    printf("Error: la clave no existe.");
    return -1;
  }

  result = mod(&list, key, v1, v2, v3);

  //TODO: posibles casos de fallo: los valores introducidos son incorrectos.
  /*if (result == -1){
    printf("")
    return -1;
  }*/
  return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int delete_key(char *key){
  int result;

  if (exists(key) == -1){ 
    printf("Error: la clave no existe.");
    return -1;
  }

  result = delete(&list, key);

  return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int num_items(){
  if (list_size == -1){
    printf("Error: la lista no ha sido inicializada.\n");
  } 
  
  return list_size;
}

////////////////////////////////////////////////////////////////////////////////
//////////////// F U N C I O N - P A R A - T H R E A D S ///////////////////////
////////////////////////////////////////////////////////////////////////////////

void servicio(void ){
  // request  local
  request req;
  // Cola del cliente
  mqd_t client_q;
  // Resultado del servicio
  int res;

	while(true){

		pthread_mutex_lock(&mutex);

    // Esperar a recibir peticiones
		while (n_elementos == 0) {
      // Evaluar estado de ejecución del servidor
      if (fin == true) {
				fprintf(stderr,"Finalizando servicio\n");
				pthread_mutex_unlock(&mutex);
				pthread_exit(0);
			}
			pthread_cond_wait(&no_vacio, &mutex);
		}

    // Recoger la petición del buffer
		req = buffer_peticiones[pos];
    // Cambia el puntero de buffer
		pos = (pos + 1) % MAX_PETICIONES;
		n_elementos--;
    // Señales pertinentes a los threads
		pthread_cond_signal(&no_lleno);
		pthread_mutex_unlock(&mutex);

		// Procesado de la request

		/* ejecutar la petición del cliente y preparar respuesta */
    switch(req -> op){
      case INIT:
        res = init();
        break;
      case SET:
        res = set_value(req -> key, req -> v1, req -> v2, req -> v3);
        break;
      case GET:
        res = get_value(req -> key, req -> v1, req -> v2, req -> v3);
        break;
      case MOD:
        res = modify_value(req -> key, req -> v1, req -> v2, req -> v3);
        break;
      case DEL:
        res = delete_key(req -> key);
        break;
      case EXIST:
        res = exists(req -> key);
        break;
      case ITEMS:
        res = num_items();
        break;
      default:
        res = -1;
        perror("Código de operación invalido");
        break;
    }



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////






///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




		// Respuesta a la cola del cliente
		client_q = mq_open(req -> q_name, O_WRONLY);
		if (client_q == -1)
			perror("No se puede abrir la cola del cliente");
		else {
			mq_send(client_q, (const char *) &res, sizeof(int), 0);
			mq_close(client_q);
		}
	}
 	pthread_exit(0);
}

////////////////////////////////////////////////////////////////////////////////
///////////////////// E J E C U C I O N - S E R V I D O R //////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(void) {
    // Cola de mensajes
	mqd_t server_q;
  struct mq_attr attr;
  attr.mq_maxmsg = 10;
	attr.mq_msgsize = sizeof(request);

  // Lista enlazada para tuplas
  int err = init_list(&list);
  if (err == -1){
    printf("Error creando la lista de tupals.");
    return -1;
  }

  // request  actual
  request req;

	// Threads
	pthread_attr_t t_attr;
	pthread_t thid[MAX_THREADS];
	int error;
	int pos = 0;

  // Inicialización de la cola del server
  server_q = mq_open("/server_q", O_CREAT|O_RDONLY, 0700, &attr);
	if (server_q == -1) {
		perror("Fallo al crear la cola del servidor");
		return -1;
	}

  // Inicialización de MUTEX de la cola y la pool
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&no_lleno, NULL);
	pthread_cond_init(&no_vacio, NULL);
	pthread_mutex_init(&mfin, NULL);

  // Creación de la pool de threads
	pthread_attr_init(&t_attr);
	for (int ii = 0; ii < MAX_THREADS; ii++){
		if (pthread_create(&thid[ii], NULL, (void *) servicio, NULL) !=0){
			perror("Error creando la pool de threads\n");
			return 0;
		}
  }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




  // Bucle de ejecución del servidor
	while (true) {
    // Recepción de un request
		sc = accept(sd, (struct sockaddr *)&client_addr,(socklen_t *)&size);
		if (sc == -1){
			perror("Error en la recepción de un request");
			break;
		}

    // Esperar a que se desocupe un lugar en el buffer
		pthread_mutex_lock(&mutex);
		while (n_elementos == MAX_PETICIONES){
      pthread_cond_wait(&no_lleno, &mutex);
    }
		read ( sc, (char *) &req, sizeof(char));
		
		buffer_peticiones[pos] = req; // Recibe la request  en el buffer de peticiones
		pos = (pos + 1) % MAX_PETICIONES; // Mueve el puntero de posición del buffer de peticiones al siguiente hueco libre
		n_elementos++;
		pthread_cond_signal(&no_vacio); // Avisa al resto de threads parados por el cond_wait
		pthread_mutex_unlock(&mutex); // Libera el mutex global
	}

  // Ejecución del servidor terminada
	pthread_mutex_lock(&mfin);
	fin = true;
	pthread_mutex_unlock(&mfin);

	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&no_vacio);
	pthread_mutex_unlock(&mutex);

  // Recogida de threads
	for (int i = 0; i < MAX_THREADS; i++){
		pthread_join(thid[i], NULL);
  }

  // Limpieza
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&no_lleno);
	pthread_cond_destroy(&no_vacio);
	pthread_mutex_destroy(&mfin);

	return 0;
}