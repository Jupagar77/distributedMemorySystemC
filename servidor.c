#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>
#include <pthread.h>
#include <termios.h>

#define BUFSIZE 1000
#define PARAMS_NUM 2

int _fCloseThreads, _conexionServidor;

typedef struct {
    int id;
    struct in_addr address;
    int port;
} clienteData;

int getCharUsuario (void)
{
    int ch;
    struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return ch;
}

void* atenderCliente(void* clienteDataParam){
  clienteData *data = clienteDataParam;
  char buffer[100];

  printf("Conectando con %s:%d.\n", inet_ntoa(data->address),htons(data->port));
  if(recv(data->id, buffer, 100, 0) < 0)
  { 
    printf("Error al recibir los datos.\n");
    close(_conexionServidor);
    return NULL;
  } else {
    printf("%s\n", buffer);
    bzero((char *)&buffer, sizeof(buffer));
    send(data->id, "Recibido.\n", 13, 0);
  }
  free(data);
  return NULL;
}


void* conectarCliente(){
    struct sockaddr_in cliente;
    socklen_t longc; 
    int conexion_cliente;
    longc = sizeof(cliente);

    while(_fCloseThreads) {
        printf("Esperando solicitud de algun cliente.\n");
        conexion_cliente = accept(_conexionServidor, (struct sockaddr *)&cliente, &longc); //Espera una conexion
        // Error
        if(conexion_cliente<0)
        {
          printf("Error al conectar con cliente.\n");
        }

        pthread_t thread_cliente;
        clienteData *args = malloc(sizeof *args);
        args->id = conexion_cliente;
        args->address = cliente.sin_addr;
        args->port =  cliente.sin_port;
        if(pthread_create(&thread_cliente, NULL, atenderCliente, args)){
          free(args);
        }
    }
    return NULL;
}

void* finalizarPrograma()
{
    printf("%s", (char*)"Presion 'E' para finalizar.\n");
    int ch;
    do{
        ch = getCharUsuario();
        if(ch=='E'){
            printf("%s", (char*)"Saliendo...\n");
            _fCloseThreads = 0;
            return 0;
        }
    } while(1);
}

char* trimPalabra(char *string)
{
    int i, len;
    len = strlen(string);
    char *newstring;
    newstring = (char *)malloc(len-1);
    for(i = 0; i < strlen(string)-1; i++)
    {
        newstring[i] = string[i]; 
    }
    return newstring;
}


int main(int argc, char **argv){
  // Declarar variables
  int puerto; 
  socklen_t longc; 
  struct sockaddr_in servidor, cliente;
  char buffer[100]; 
  pthread_t thread_arr[2];
  int iter_id[2];
  char bufferParams[BUFSIZE], cantidadPaginasConfig[BUFSIZE], puertoConfig[BUFSIZE];
  char *saveptr, *paramName, *paramValue;
  FILE* file_handle;

  // Obtener parametros del archivo de configuracion
  file_handle = fopen(argv[1], "r");
  if(file_handle == NULL)
  {
    printf("Error al abrir el archivo de configuracion.\n");
    return 1;
  }

  while(fgets(bufferParams, BUFSIZE - 1, file_handle) != NULL) 
  {
    paramName = strtok_r(bufferParams, ":", &saveptr);
    paramValue = strtok_r(NULL, ":", &saveptr);
    if (strcmp(paramName, "Puerto") == 0) 
    {
      strcpy(puertoConfig, paramValue);
    } 
    else if (strcmp(paramName, "CantidadPaginas") == 0) {
      strcpy(cantidadPaginasConfig, paramValue);
    }
  }
  fclose(file_handle);
  
  // Inizializar variables
  puerto = atoi(trimPalabra(puertoConfig));
  _conexionServidor = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *)&servidor, sizeof(servidor)); 
  servidor.sin_family = AF_INET;
  servidor.sin_port = htons(puerto);
  servidor.sin_addr.s_addr = INADDR_ANY; 

  // Conectar puerto con servidor
  if(bind(_conexionServidor, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
  { 
    printf("Error al asociar el puerto a la conexion.\n");
    close(_conexionServidor);
    return 0;
  }

  // Escuchar
  listen(_conexionServidor, 3); 
  printf("A la escucha en el puerto %d.\n", ntohs(servidor.sin_port));
  
  _fCloseThreads = 1;
  for (int iter=0; iter<2; iter++) {
    iter_id[iter] = iter;
    if(iter == 0){
        // Hilo para atender solicitudes de clientes
        pthread_create(&thread_arr[iter], NULL, conectarCliente, &iter_id[iter]);
    } else{
        // Hilo para finalizar programa
        pthread_create(&thread_arr[iter], NULL, finalizarPrograma, &iter_id[iter]);
    }
  }

  for (int iter=0;iter<2;iter++) {
    pthread_join(thread_arr[iter],NULL);
  }

  close(_conexionServidor);
  return 0;
}