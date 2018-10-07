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
  char* host;  
  int port;
} clienteConCopia;

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
  int lock;
  clienteConCopia* clientes[300];  
  int cantClientes;
} pagina;

int _fCloseThreads, _conexionServidor, _puerto, _puertoGlobal, _cantidadPaginas, _puertoCopiasGlobal = 2000;
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

void* clienteBorreCopia(char* host, int port){
  struct sockaddr_in clientePedir; //Declaración de la estructura con información para la conexión
  struct hostent *servidorPedir; //Declaración de la estructura con información del host
  servidorPedir = gethostbyname(host); //Asignacion
  if(servidorPedir == NULL)
  { //Comprobación 
    printf("Host erróneo\n");
    return NULL;
  }
  int conexionPedir;
  char bufferPedir[100];

  conexionPedir = socket(AF_INET, SOCK_STREAM, 0); //Asignación del socket       
  bzero((char *)&clientePedir, sizeof((char *)&clientePedir)); //Rellena toda la estructura de 0's
  //La función bzero() es como memset() pero inicializando a 0 todas la variables
  clientePedir.sin_family = AF_INET; //asignacion del protocolo
  clientePedir.sin_port = htons(port); //asignacion del puerto
  bcopy((char *)servidorPedir->h_addr, (char *)&clientePedir.sin_addr.s_addr, sizeof(servidorPedir->h_length));
             
  if(connect(conexionPedir,(struct sockaddr *)&clientePedir, sizeof(clientePedir)) < 0)
  { //conectando con el host
    printf("Error conectando con el host, puerto %d .\n", port);
    close(conexionPedir);
    return NULL;
  }
  printf("Conectado con %s:%d\n",inet_ntoa(clientePedir.sin_addr),htons(clientePedir.sin_port));
  //inet_ntoa(); está definida en <arpa/inet.h>

  //le envio al otro cliente 'leer' para que el em mande una copia (version) de la pagina que tiene que es a la que em estoy conectando
  send(conexionPedir, "borrar", 100, 0); //envio
  bzero(bufferPedir, 100);
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
        int paginaClienteInt = atoi(paginaCliente);
        int paginaClienteIndice = paginaClienteInt-1;
        printf("El cliente quiere %s la pagina #%d.\n", accion, paginaClienteInt);
        
        int trabajar = 1;
        while(trabajar){
          if(_paginasServer[paginaClienteIndice].lock){
             
          }else{
            trabajar = 0;
          }
        }
        _paginasServer[paginaClienteIndice].lock = 1;
       
        // Caso quiere leer
        if(strcmp(accion,"leer") == 0){
          //Envio data
          if(_paginasServer[paginaClienteIndice].puertoOwner == _puerto &&
             strcmp(_paginasServer[paginaClienteIndice].hostOwner, "127.0.0.1") == 0 ){
            printf("La pagina #%d no ha sido escrita por ningun cliente. Leerla en su version 0.\n", paginaClienteInt);
            bzero((char *)&buffer, sizeof(buffer));
            sprintf(buffer, "leer:0:0:%d", _puertoCopiasGlobal);
            send(data->id, buffer, 100, 0);

            clienteConCopia *args = malloc(sizeof *args);
            args->host = inet_ntoa(data->address);
            args->port = _puertoCopiasGlobal;
            _paginasServer[paginaClienteIndice].clientes[_paginasServer[paginaClienteIndice].cantClientes] = args;
            _paginasServer[paginaClienteIndice].cantClientes++; 

            _puertoCopiasGlobal += 1;
          } else {
            printf("La pagina #%d es de otro cliente.\n", paginaClienteInt);
            printf("El cliente va a pedir la pagina a otro cliente en el host: %s y puerto: %d\n",_paginasServer[paginaClienteIndice].hostOwner,
              _paginasServer[paginaClienteIndice].puertoOwner);
            bzero((char *)&buffer, sizeof(buffer));
            sprintf(buffer, "pedir:%s:%d:%d", _paginasServer[paginaClienteIndice].hostOwner,_paginasServer[paginaClienteIndice].puertoOwner, _puertoCopiasGlobal);
            _puertoCopiasGlobal += 1;
            send(data->id, buffer, 100, 0);
          }
        } else { // Caso quiere escribir
          if(_paginasServer[paginaClienteIndice].puertoOwner == _puerto &&
            strcmp(_paginasServer[paginaClienteIndice].hostOwner, "127.0.0.1") == 0 ){

            _puertoGlobal += 1;
            _paginasServer[paginaClienteIndice].hostOwner = inet_ntoa(data->address);
            _paginasServer[paginaClienteIndice].puertoOwner = _puertoGlobal;
            
            printf("La pagina #%d no ha sido escrita por ningun cliente. Entregar al cliente en su version 0.\n", paginaClienteInt);
            printf("Pagina #%d ha cambiando de dueno, host: %s utilizara puerto: %d para atender otros clientes.\n", paginaClienteInt,
              inet_ntoa(data->address), _puertoGlobal);

            bzero((char *)&buffer, sizeof(buffer));
            sprintf(buffer, "escribir:%s:%d:0000",inet_ntoa(data->address),_puertoGlobal);
            send(data->id, buffer, 100, 0);

          } else {
            _puertoGlobal += 1;

            for(int p = 0; p < _paginasServer[paginaClienteIndice].cantClientes; p++){
                  printf("El cliente %s:%d debe borrar su copia. \n", _paginasServer[paginaClienteIndice].clientes[p]->host,
                  _paginasServer[paginaClienteIndice].clientes[p]->port);
                
                clienteBorreCopia(
                  _paginasServer[paginaClienteIndice].clientes[p]->host,
                  _paginasServer[paginaClienteIndice].clientes[p]->port
                  );
            }
              memset(_paginasServer[paginaClienteIndice].clientes, 0, sizeof(_paginasServer[paginaClienteIndice].clientes));
              _paginasServer[paginaClienteIndice].cantClientes = 0;

            printf("La pagina #%d es de otro cliente, pedir en puerto: %d , atender en puerto: %d.\n", paginaClienteInt, _paginasServer[paginaClienteIndice].puertoOwner, _puertoGlobal);
            bzero((char *)&buffer, sizeof(buffer));
            sprintf(buffer, "pedir:%s:%d:%d", _paginasServer[paginaClienteIndice].hostOwner, _puertoGlobal,_paginasServer[paginaClienteIndice].puertoOwner);
            _paginasServer[paginaClienteIndice].hostOwner = inet_ntoa(data->address);
            _paginasServer[paginaClienteIndice].puertoOwner = _puertoGlobal;
            send(data->id, buffer, 100, 0);
          }
        }
        bzero((char *)&buffer, sizeof(buffer));
       _paginasServer[paginaClienteIndice].lock = 0;
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

char* clienteVersion(char* host, int port){
  struct sockaddr_in clientePedir; //Declaración de la estructura con información para la conexión
  struct hostent *servidorPedir; //Declaración de la estructura con información del host
  servidorPedir = gethostbyname(host); //Asignacion
  if(servidorPedir == NULL)
  { //Comprobación 
    printf("Host erróneo\n");
    return "error";
  }
  int conexionPedir;
  char bufferPedir[100];

  conexionPedir = socket(AF_INET, SOCK_STREAM, 0); //Asignación del socket       
  bzero((char *)&clientePedir, sizeof((char *)&clientePedir)); //Rellena toda la estructura de 0's
  //La función bzero() es como memset() pero inicializando a 0 todas la variables
  clientePedir.sin_family = AF_INET; //asignacion del protocolo
  clientePedir.sin_port = htons(port); //asignacion del puerto
  bcopy((char *)servidorPedir->h_addr, (char *)&clientePedir.sin_addr.s_addr, sizeof(servidorPedir->h_length));
  //bcopy(); copia los datos del primer elemendo en el segundo con el tamaño máximo del tercer argumento.
            
  //cliente.sin_addr = *((struct in_addr *)servidor->h_addr); //<--para empezar prefiero que se usen
  //inet_aton(argv[1],&cliente.sin_addr); //<--alguna de estas dos funciones
            
  if(connect(conexionPedir,(struct sockaddr *)&clientePedir, sizeof(clientePedir)) < 0)
  { //conectando con el host
    printf("Error conectando con el host, puerto %d .\n", port);
    close(conexionPedir);
    return "error";
  }
  //printf("Conectado con %s:%d\n",inet_ntoa(clientePedir.sin_addr),htons(clientePedir.sin_port));
  //inet_ntoa(); está definida en <arpa/inet.h>

  //le envio al otro cliente 'leer' para que el em mande una copia (version) de la pagina que tiene que es a la que em estoy conectando
  send(conexionPedir, "matese", 100, 0); //envio
  bzero(bufferPedir, 100);

  recv(conexionPedir, bufferPedir, 100, 0);
  close(conexionPedir);
  char *version = bufferPedir;
  return version;
}

void* finalizarPrograma(){
    printf("%s", (char*)"Presione 'E' para finalizar.\n");
    int ch;
    do{
        ch = getCharUsuario();
        if(ch=='E'){
            printf("%s", (char*)"Exit: terminando procesos.\n");
            _fCloseThreads = 0;

            //Pedirle a todos los clientes duenos que me dan le version y que mueran.
            for(int p = 0; p<_cantidadPaginas; p++){

              if(_paginasServer[p].puertoOwner == _puerto &&
                strcmp(_paginasServer[p].hostOwner, "127.0.0.1") == 0 ){
                printf("Pagina #%d en su version %d.\n", _paginasServer[p].id, _paginasServer[p].version);
              } else {
                char* version;
                version = clienteVersion(_paginasServer[p].hostOwner,_paginasServer[p].puertoOwner);
                printf("Pagina #%d en su version %d.\n", _paginasServer[p].id, atoi(version));
              }

            }


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
    _paginasServer[p].lock = 0;
  }

  /*printf("Se crearon %d paginas.\n", cantidadPaginas);
  for(int p = 0; p<cantidadPaginas; p++){
    printf("Pagina #%d en su version %d, actual dueño host: %s en puerto: %d \n", _paginasServer[p].id, 
      _paginasServer[p].version, _paginasServer[p].hostOwner, _paginasServer[p].puertoOwner);
  }*/

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