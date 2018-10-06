#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <termios.h>

#define BUFSIZE 1000
#define PARAMS_NUM 2

typedef struct {
    int id;
    struct in_addr address;
    int port;
} clienteData;

typedef struct {
  int id;
  int version;
  char* hostOwner;
  int puertoOwner;
} pagina;

int _fCloseThreads, _conexionServidor, _puerto, _puertoGlobal, _cantidadPaginas;
pagina _paginasServer[100];

int getCharUsuario (void){
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
  char *saveptr, *accion, *paginaCliente;
  
  // Recibo data
  if(recv(data->id, buffer, 100, 0) < 0)
  { 
    printf("Conectado con %s:%d.\n", inet_ntoa(data->address),htons(data->port));
    printf("Error al recibir los datos.\n");
    close(_conexionServidor);
    return NULL;
  } else {
      if(strcmp(buffer,"")){
        printf("Conectado con %s:%d.\n", inet_ntoa(data->address),htons(data->port));
        
        //Proceso data
        accion = strtok_r(buffer, ":", &saveptr);
        paginaCliente = strtok_r(NULL, ":", &saveptr);
        printf("El cliente quiere %s la pagina #%d.\n", accion, atoi(paginaCliente));
        
        // Caso quiere leer
        if(strcmp(accion,"leer") == 0){
          //Envio data
          if(_paginasServer[atoi(paginaCliente)-1].puertoOwner == _puerto &&
             strcmp(_paginasServer[atoi(paginaCliente)-1].hostOwner, "127.0.0.1") == 0 ){
            printf("La pagina #%d no ha sido escrita por ningun cliente. Leerla en su version 0.\n", atoi(paginaCliente));
            send(data->id, "leer", 100, 0);
          }else{
            printf("La pagina #%d es de otro cliente.\n", atoi(paginaCliente));
            printf("\t El cliente va a pedir la pagina a otro cliente con %s:%d\n",_paginasServer[atoi(paginaCliente)-1].hostOwner,_paginasServer[atoi(paginaCliente)-1].puertoOwner);
            sprintf(buffer, "pedir:%s:%d", _paginasServer[atoi(paginaCliente)-1].hostOwner,_paginasServer[atoi(paginaCliente)-1].puertoOwner);
            send(data->id, buffer, 100, 0);
          }

        } else { // Caso quiere escribir
          if(_paginasServer[atoi(paginaCliente)-1].puertoOwner == _puerto &&
            strcmp(_paginasServer[atoi(paginaCliente)-1].hostOwner, "127.0.0.1") == 0 ){

            _paginasServer[atoi(paginaCliente)-1].hostOwner = inet_ntoa(data->address);
            _paginasServer[atoi(paginaCliente)-1].puertoOwner = _puertoGlobal++;
            printf("La pagina #%d no ha sido escrita por ningun cliente. Entregar al cliente en su version 0.\n", atoi(paginaCliente));
            printf("Pagina #%d ha cambiando de dueno, host: %s utilizara puerto: %d para atender otros clientes.\n", atoi(paginaCliente),
              inet_ntoa(data->address), _puertoGlobal);
            bzero((char *)&buffer, sizeof(buffer));
            sprintf(buffer, "escribir:%s:%d",inet_ntoa(data->address),_puertoGlobal);
            send(data->id, buffer, 100, 0);
          }else{
            printf("La pagina #%d es de otro cliente.\n", atoi(paginaCliente));
            sprintf(buffer, "pedir:%s:%d", _paginasServer[atoi(paginaCliente)-1].hostOwner,_paginasServer[atoi(paginaCliente)-1].puertoOwner);
            send(data->id, buffer, 100, 0);

          }
        }
        bzero((char *)&buffer, sizeof(buffer));
      }
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
        printf("\nEsperando solicitud de algun cliente.\n");
        conexion_cliente = accept(_conexionServidor, (struct sockaddr *)&cliente, &longc); //Espera una conexion
        // Error
        if(conexion_cliente<0)
        {
          printf("\nError al conectar con cliente.\n");
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

void* finalizarPrograma(){
    printf("%s", (char*)"Presione 'E' para finalizar.\n");
    int ch;
    do{
        ch = getCharUsuario();
        if(ch=='E'){
            printf("%s", (char*)"Exit: terminando procesos.\n");
            _fCloseThreads = 0;
            /*
            Crear un cliente "tonto" para matar al servidor 
            en caso de que nadie haga un request
            */
            struct sockaddr_in cliente;
            struct hostent *servidor; 
            int conexion;

            // Configurar servidor en el host local
            servidor = gethostbyname("localhost"); 
            if(servidor == NULL) { 
              return 0;
            }

            // Configurar cliente "tonto" en el puerto y servidor configurados
            bzero((char *)&cliente, sizeof((char *)&cliente)); 
            cliente.sin_family = AF_INET; 
            cliente.sin_port = htons(_puerto); 
            bcopy((char *)servidor->h_addr, (char *)&cliente.sin_addr.s_addr, sizeof(servidor->h_length));

            //Trato de conectar para matarlo
            conexion = socket(AF_INET, SOCK_STREAM, 0); 
            connect(conexion,(struct sockaddr *)&cliente, sizeof(cliente));
            close(conexion);
            return 0;
        }
    } while(1);
}

char* trimPalabra(char *string){
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
  socklen_t longc; 
  struct sockaddr_in servidor, cliente;
  char buffer[100]; 
  pthread_t thread_arr[2];
  int iter_id[2], cantidadPaginas;
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
  _puerto = atoi(trimPalabra(puertoConfig));
  _puertoGlobal = _puerto;
  _conexionServidor = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *)&servidor, sizeof(servidor)); 
  servidor.sin_family = AF_INET;
  servidor.sin_port = htons(_puerto);
  servidor.sin_addr.s_addr = INADDR_ANY;
  cantidadPaginas = atoi(cantidadPaginasConfig);
  _cantidadPaginas = cantidadPaginas;

  // Inicializar paginas
  for(int p = 0; p<cantidadPaginas; p++){
    _paginasServer[p].id = (p+1);
    _paginasServer[p].version = 0;
    _paginasServer[p].hostOwner = "127.0.0.1";
    _paginasServer[p].puertoOwner = _puerto;
  }

  printf("Se crearon %d paginas.\n", cantidadPaginas);
  for(int p = 0; p<cantidadPaginas; p++){
    printf("Pagina #%d en su version %d, actual dueño host: %s en puerto: %d \n", _paginasServer[p].id, 
      _paginasServer[p].version, _paginasServer[p].hostOwner, _paginasServer[p].puertoOwner);
  }

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
        pthread_create(&thread_arr[iter], NULL, finalizarPrograma, &iter_id[iter]);
    } else{
        // Hilo para finalizar programa
        pthread_create(&thread_arr[iter], NULL, conectarCliente, &iter_id[iter]);
    }
  }

  for (int iter=0;iter<2;iter++) {
    pthread_join(thread_arr[iter],NULL);
  }

  close(_conexionServidor);
  return 0;
}