#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <termios.h>

#define BUFSIZE 1000
#define PARAMS_NUM 5

typedef struct {
    int puerto;
    int indice;
} paginaData;

typedef struct {  
  char* host;  
  int port;
} clienteConCopia;

typedef struct {  
  int id;  
  int version;  
  int copia;  
  int dueno;  
  clienteConCopia* clientes[300];  
  int cantClientes;
} paginaTrabajo;

paginaTrabajo _paginasTrabajo[100];

int _morirProceso = 1;

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

float calcularMedia(float valor){
  double random;
  double result;
  srand(time(NULL)); 
  random = (double)rand()/RAND_MAX*1.0;
  result = -1 * valor * log(1-random);
  /*
    Si da menor a 1 crashea el servidor
  */
  if(result < 2){
    result = 2.4534;
  }
  return result;
}

int getPagina(int min, int max){
  srand(time(NULL));
  return min + rand() % (max+1 - min);
}

int getLeer(int propabilidad){
  int random = 0 + rand() % (100+1 - 0);
  srand(time(NULL));
  if(random <= propabilidad){
    return 1;
  }
  return 0;
}

void* clienteBorreCopia(char* host, int port){
  struct sockaddr_in clientePedir; //Declaración de la estructura con información para la conexión
  struct hostent *servidorPedir; //Declaración de la estructura con información del host
  servidorPedir = gethostbyname(host); //Asignacion
  if(servidorPedir == NULL)
  { //Comprobación 
    printf("Host erróneo.\n");
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
  //bcopy(); copia los datos del primer elemendo en el segundo con el tamaño máximo del tercer argumento.
            
  //cliente.sin_addr = *((struct in_addr *)servidor->h_addr); //<--para empezar prefiero que se usen
  //inet_aton(argv[1],&cliente.sin_addr); //<--alguna de estas dos funciones
            
  if(connect(conexionPedir,(struct sockaddr *)&clientePedir, sizeof(clientePedir)) < 0)
  { //conectando con el host
    printf("Error conectando con el host, puerto %d .\n", port);
    close(conexionPedir);
    return NULL;
  }
  printf("Conectado con %s:%d .\n",inet_ntoa(clientePedir.sin_addr),htons(clientePedir.sin_port));
  //inet_ntoa(); está definida en <arpa/inet.h>

  //le envio al otro cliente 'leer' para que el em mande una copia (version) de la pagina que tiene que es a la que em estoy conectando
  send(conexionPedir, "borrar", 100, 0); //envio
  bzero(bufferPedir, 100);
}

void* serDueno(void* dataPagina) {
  paginaData *data = dataPagina;
  int puertoAtender = data->puerto;
  socklen_t longc; 
  struct sockaddr_in servidor, cliente;
  char buffer[100]; 
  int conexionServidor, conexion_cliente, fCloseThreads = 1;

  conexionServidor = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *)&servidor, sizeof(servidor));
  servidor.sin_family = AF_INET;
  servidor.sin_port = htons(puertoAtender);
  servidor.sin_addr.s_addr = INADDR_ANY; 
  // Conectar puerto con servidor
  if(bind(conexionServidor, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
  { 
    printf("Error creando servidor de dueno en puerto %d .\n", puertoAtender);
    close(conexionServidor);
    return 0;
  }
  // Escuchar
  listen(conexionServidor, 3);
 
  longc = sizeof(cliente);
  while(fCloseThreads) {
      conexion_cliente = accept(conexionServidor, (struct sockaddr *)&cliente, &longc); //Espera una conexion
      // Error
      if(conexion_cliente<0)
      {
        printf("\nError al conectar con cliente.\n");
      }
      char buffer[100];
  
      // Recibo data
      if(recv(conexion_cliente, buffer, 100, 0) < 0)
      { 
        printf("Conectado con %s:%d.\n", inet_ntoa(cliente.sin_addr),htons(cliente.sin_port));
        printf("Error al recibir los datos.\n");
        close(conexionServidor);
        return NULL;
      } else {
        printf("Conectado con %s:%d.\n", inet_ntoa(cliente.sin_addr),htons(cliente.sin_port));  
        char *saveptr, *accion, *paginaCliente, *puertoMatar;
        accion = strtok_r(buffer, ":", &saveptr);
        puertoMatar = strtok_r(NULL, ":", &saveptr);
        if(strcmp(accion,"leer") == 0){
          //meter ese cliente a la lista de los clientes que tienen esta copia          
          clienteConCopia *args = malloc(sizeof *args);
          args->host = inet_ntoa(cliente.sin_addr);
          args->port = atoi(puertoMatar);

          _paginasTrabajo[data->indice].clientes[_paginasTrabajo[data->indice].cantClientes] = args;   
          _paginasTrabajo[data->indice].cantClientes++; 
          bzero((char *)&buffer, sizeof(buffer));
          sprintf(buffer, "version:%d", _paginasTrabajo[data->indice].version);
          send(conexion_cliente, buffer, 100, 0);
        } else {
          if(strcmp(accion,"escribir") == 0){   
              for(int p = 0; p < _paginasTrabajo[data->indice].cantClientes; p++){
                  printf("El cliente %s:%d debe borrar su copia. \n", _paginasTrabajo[data->indice].clientes[p]->host,
                  _paginasTrabajo[data->indice].clientes[p]->port);
                
                clienteBorreCopia(
                  _paginasTrabajo[data->indice].clientes[p]->host,
                  _paginasTrabajo[data->indice].clientes[p]->port
                  );
              }
              memset(_paginasTrabajo[data->indice].clientes, 0, sizeof(_paginasTrabajo[data->indice].clientes));
              _paginasTrabajo[data->indice].cantClientes = 0;

              //cederle al cliente que pidio la pagina la version para que el cliente la escriba y le sume una posicion a la version
              bzero((char *)&buffer, sizeof(buffer));
              sprintf(buffer, "version:%d", _paginasTrabajo[data->indice].version);
              send(conexion_cliente, buffer, 100, 0);
              //borrar mi pagina
              _paginasTrabajo[data->indice].version = 0;
              _paginasTrabajo[data->indice].dueno = 0;
              _paginasTrabajo[data->indice].copia = 0;
              fCloseThreads = 0;
          } else{
            //Debo morir
            bzero((char *)&buffer, sizeof(buffer));
            sprintf(buffer, "%d", _paginasTrabajo[data->indice].version);
            send(conexion_cliente, buffer, 100, 0);
            fCloseThreads = 0;
            _morirProceso = 0;
          }
        }
      }
  }
  close(conexionServidor);
  return NULL;
}

void* borrarCopia(void* dataPagina) {
  paginaData *data = dataPagina;
  int puertoAtender = data->puerto;
  socklen_t longc; 
  struct sockaddr_in servidor, cliente;
  char buffer[100]; 
  int conexionServidor, conexion_cliente;

  conexionServidor = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *)&servidor, sizeof(servidor));
  servidor.sin_family = AF_INET;
  servidor.sin_port = htons(puertoAtender);
  servidor.sin_addr.s_addr = INADDR_ANY; 
  // Conectar puerto con servidor
  if(bind(conexionServidor, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
  { 
    printf("Error creando servidor de copia.\n");
    close(conexionServidor);
    return 0;
  }
  // Escuchar
  listen(conexionServidor, 3);
  
  longc = sizeof(cliente);
  conexion_cliente = accept(conexionServidor, (struct sockaddr *)&cliente, &longc); //Espera una conexion
  // Error
  if(conexion_cliente<0)
  {
      printf("\nError al conectar con cliente.\n");
  }
  
  // Recibo data
  if(recv(conexion_cliente, buffer, 100, 0) < 0)
    { 
      printf("Conectado con %s:%d.\n", inet_ntoa(cliente.sin_addr),htons(cliente.sin_port));
      printf("Error al recibir los datos.\n");
      close(conexionServidor);
      return NULL;
    } else {
      printf("Conectado con %s:%d.\n", inet_ntoa(cliente.sin_addr),htons(cliente.sin_port));  
      _paginasTrabajo[data->indice].version = 0;
      _paginasTrabajo[data->indice].dueno = 0;
      _paginasTrabajo[data->indice].copia = 0;
    }
  close(conexionServidor);
  return NULL;
}

int main(int argc, char* argv[]){
  // Declarar variables
  char bufferParams[BUFSIZE], buffer[100], ipServidor[BUFSIZE], 
       puertoConfig[BUFSIZE], mediaConfig[BUFSIZE], paginasMin[BUFSIZE], paginasMax[BUFSIZE],
       lecturaConfig[BUFSIZE];
  int puerto, conexion, pagina, cantidadPaginas, paginaMin, paginaMax,
      paginaIndice;
  int leer;
  char *saveptr, *paramName, *paramValue, *pagMin, *pagMax;
  struct sockaddr_in cliente; 
  struct hostent *servidor; 
  FILE* file_handle;
  float mediaExponencial;

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
    if (strcmp(paramName, "IPServidor") == 0) {
      strcpy(ipServidor, paramValue);
    } else if (strcmp(paramName, "Puerto") == 0) {
      strcpy(puertoConfig, paramValue);
    } else if (strcmp(paramName, "MediaExponencial") == 0) {
      strcpy(mediaConfig, paramValue);
    } else if (strcmp(paramName, "RangoPaginas") == 0) {
      pagMin = strtok_r(paramValue, "-", &saveptr);
      pagMax = strtok_r(NULL, "-", &saveptr);
      strcpy(paginasMin, pagMin);
      strcpy(paginasMax, pagMax);
    }  else if (strcmp(paramName, "PropabilidadLectura") == 0) {
      strcpy(lecturaConfig, paramValue);
    } 
  }
  fclose(file_handle);

  // Configurar servidor en el host proporcionado
  servidor = gethostbyname(trimPalabra(ipServidor)); 
  if(servidor == NULL) { 
    printf("Host erróneo, ip: %s.\n", trimPalabra(ipServidor));
    return 0;
  }

  // Configurar cliente en el puerto y servidor configurados
  puerto=(atoi(trimPalabra(puertoConfig))); 
  bzero((char *)&cliente, sizeof((char *)&cliente)); 
  cliente.sin_family = AF_INET; 
  cliente.sin_port = htons(puerto); 
  bcopy((char *)servidor->h_addr, (char *)&cliente.sin_addr.s_addr, sizeof(servidor->h_length));

  // Calcular tiempo de espera
  mediaExponencial = calcularMedia(atof(mediaConfig));

  // Inicializar array para trabajar paginas (copias o propias)
  paginaMin = atoi(paginasMin);
  paginaMax = atoi(trimPalabra(paginasMax));
  cantidadPaginas = paginaMax - paginaMin + 1;
  int i = 0;
  for(int p = paginaMin; p<=paginaMax; p++){
    _paginasTrabajo[i].id = p;
    _paginasTrabajo[i].version = 0;
    _paginasTrabajo[i].copia = 0;
    _paginasTrabajo[i].dueno = 0;
    i++;
  }

  printf("Voy a esperar %lfs para solicitar cada pagina.\n", mediaExponencial);
  // Ciclo de solicitudes
  while(_morirProceso){
    printf("\n");
    sleep(mediaExponencial);

    pagina = getPagina(paginaMin,paginaMax);
    paginaIndice = pagina-paginaMin;
    leer = getLeer(atoi(lecturaConfig));
    if(leer){
      printf("Intentando leer pagina #%d.\n",pagina);
      
      // Caso quiero leer, soy dueno.
      if(_paginasTrabajo[paginaIndice].dueno){
        printf("Leyendo mi pagina #%d en su version %d.\n", _paginasTrabajo[paginaIndice].id, _paginasTrabajo[paginaIndice].version);
      } else if(_paginasTrabajo[paginaIndice].copia){ // Caso quiero leer, no soy dueno pero tengo una copia.
        printf("Leyendo pagina #%d (copia) en su version %d.\n", _paginasTrabajo[paginaIndice].id, _paginasTrabajo[paginaIndice].version);
      } else { // Caso quiero leer no soy dueno ni tengo copia.

          // Con el cliente configurado trato de conectar con el servidor
          conexion = socket(AF_INET, SOCK_STREAM, 0); 
          if(connect(conexion,(struct sockaddr *)&cliente, sizeof(cliente)) < 0)
          {
            printf("Error conectando con el host.\n");
            close(conexion);
            return 0;
          }

          //Al conectarse es posible enviar y recibir mensajes
          printf("Conectado con el servidor.\n");

          // Envio data
          sprintf(buffer, "leer:%d", _paginasTrabajo[paginaIndice].id);
          send(conexion, buffer, 100, 0); 
          bzero(buffer, 100);

          // Espero data
          recv(conexion, buffer, 100, 0);
          char *saveptrRead, *accion, *puertoObtenido, *hostObtenido, *puertoMatarCopia;
          accion = strtok_r(buffer, ":", &saveptrRead);
          hostObtenido = strtok_r(NULL, ":", &saveptrRead);
          puertoObtenido = strtok_r(NULL, ":", &saveptrRead);
          puertoMatarCopia = strtok_r(NULL, ":", &saveptrRead);

          if(strcmp(accion,"leer") == 0) {
            printf("Adquiriendo copia y leyendo pagina #%d en su version %d.\n", _paginasTrabajo[paginaIndice].id, _paginasTrabajo[paginaIndice].version);
            _paginasTrabajo[paginaIndice].copia = 1;
          } else {
            printf("Debo pedirsela a otro cliente %s.\n", accion);
            if(strcmp(accion,"pedir") == 0){  
              printf("Debo conectarme con %s:%s para pedirle pagina a leer.\n",hostObtenido,puertoObtenido);
              //crear un mini cliente que se conecte con el otro lciente para pedir la copia de la pag
              //https://es.wikibooks.org/wiki/Programaci%C3%B3n_en_C/Sockets
              

              //******** 
              //Mini cliente para que se comunique con otro cliente.
              struct sockaddr_in clientePedir; //Declaración de la estructura con información para la conexión
              struct hostent *servidorPedir; //Declaración de la estructura con información del host
              servidorPedir = gethostbyname(hostObtenido); //Asignacion
              if(servidorPedir == NULL)
              { //Comprobación 
                printf("Host erróneo.\n");
                return 1;
              }
              int puertoPedir, conexionPedir;
              char bufferPedir[100];

              conexionPedir = socket(AF_INET, SOCK_STREAM, 0); //Asignación del socket
              puertoPedir=(atoi(puertoObtenido)); //conversion del argumento
              bzero((char *)&clientePedir, sizeof((char *)&clientePedir)); //Rellena toda la estructura de 0's
              //La función bzero() es como memset() pero inicializando a 0 todas la variables
              clientePedir.sin_family = AF_INET; //asignacion del protocolo
              clientePedir.sin_port = htons(puertoPedir); //asignacion del puerto
              bcopy((char *)servidorPedir->h_addr, (char *)&clientePedir.sin_addr.s_addr, sizeof(servidorPedir->h_length));
              //bcopy(); copia los datos del primer elemendo en el segundo con el tamaño máximo del tercer argumento.
              
              //cliente.sin_addr = *((struct in_addr *)servidor->h_addr); //<--para empezar prefiero que se usen
              //inet_aton(argv[1],&cliente.sin_addr); //<--alguna de estas dos funciones
              
              if(connect(conexionPedir,(struct sockaddr *)&clientePedir, sizeof(clientePedir)) < 0)
              { //conectando con el host
                printf("Error conectando con el host.\n");
                close(conexionPedir);
                return 1;
              }
              printf("Conectado con %s:%d\n",inet_ntoa(clientePedir.sin_addr),htons(clientePedir.sin_port));
              //inet_ntoa(); está definida en <arpa/inet.h>

              //le envio al otro cliente 'leer' para que el em mande una copia (version) de la pagina que tiene que es a la que em estoy conectando
              bzero((char *)&bufferPedir, sizeof(bufferPedir));
              sprintf(bufferPedir, "leer:%s",puertoMatarCopia);
              send(conexionPedir, bufferPedir, 100, 0); //envio
              bzero(bufferPedir, 100);

              //aqui recibo la version de la copia que pedi al cliente y la agrego al array de la paginas que tengo y pongo la bandera copia en1 y dueno en 0
              recv(conexionPedir, bufferPedir, 100, 0); //recepción
              char *accionSolicitud,*versionSolicitud, *saveptrReadPedir;
              accionSolicitud = strtok_r(bufferPedir, ":", &saveptrReadPedir);
              versionSolicitud = strtok_r(NULL, ":", &saveptrReadPedir);

              if(strcmp(accionSolicitud,"version")==0){
                printf("Adquiriendo copia y leyendo pagina #%d en su version %d.\n", _paginasTrabajo[paginaIndice].id,atoi(versionSolicitud));
                _paginasTrabajo[paginaIndice].copia = 1;
                _paginasTrabajo[paginaIndice].dueno = 0;
                _paginasTrabajo[paginaIndice].version = atoi(versionSolicitud);
              }
            }
          }
          pthread_t threadBorrarCopia;
          paginaData *args = malloc(sizeof *args);
          args->puerto = atoi(puertoMatarCopia);
          args->indice = paginaIndice;
          pthread_create(&threadBorrarCopia, NULL, borrarCopia, args);

          bzero(buffer, 100);
          close(conexion);
        }    
    } else {
      printf("Intentando escribir pagina #%d .\n",pagina);
      // Caso quiero escribir, soy dueno.
      if(_paginasTrabajo[paginaIndice].dueno){
        printf("Escribiendo mi pagina #%d en su version %d, nueva version %d .\n", _paginasTrabajo[paginaIndice].id, 
          _paginasTrabajo[paginaIndice].version, _paginasTrabajo[paginaIndice].version+1);
        _paginasTrabajo[paginaIndice].version += 1;

        for(int p = 0; p < _paginasTrabajo[paginaIndice].cantClientes; p++){
          printf("El cliente %s:%d debe borrar su copia. \n", _paginasTrabajo[paginaIndice].clientes[p]->host,
            _paginasTrabajo[paginaIndice].clientes[p]->port);
          clienteBorreCopia(_paginasTrabajo[paginaIndice].clientes[p]->host,_paginasTrabajo[paginaIndice].clientes[p]->port);
        }
        memset(_paginasTrabajo[paginaIndice].clientes, 0, sizeof(_paginasTrabajo[paginaIndice].clientes));
        _paginasTrabajo[paginaIndice].cantClientes = 0;

      } else { // Caso quiero escribir pero no soy dueno. 
        // Con el cliente configurado trato de conectar con el servidor
        conexion = socket(AF_INET, SOCK_STREAM, 0); 
        if(connect(conexion,(struct sockaddr *)&cliente, sizeof(cliente)) < 0)
        {
          printf("Error conectando con el host.\n");
          close(conexion);
          return 0;
        }

        //Al conectarse es posible enviar y recibir mensajes
        printf("Conectado con el servidor.\n");

        // Envio data
        sprintf(buffer, "escribir:%d", _paginasTrabajo[paginaIndice].id);
        send(conexion, buffer, 100, 0); 
        bzero(buffer, 100);

        // Espero data
        recv(conexion, buffer, 100, 0);
        char *saveptrRead, *accion, *puertoAtender, *host, *puertoPedir;
        long int puertoAtenderInt, puertoPedirInt;
        accion = strtok_r(buffer, ":", &saveptrRead);
        host = strtok_r(NULL, ":", &saveptrRead);
        puertoAtender = strtok_r(NULL, ":", &saveptrRead);
        puertoPedir = strtok_r(NULL, ":", &saveptrRead);
        puertoAtenderInt = atoi(puertoAtender);
        puertoPedirInt = atoi(puertoPedir);
        pthread_t threadAtender;

        if(strcmp(accion,"escribir") == 0) {

          printf("Escribir en pagina #%d, mi puerto de atencion sera %ld .\n", _paginasTrabajo[paginaIndice].id, puertoAtenderInt);
          printf("Escribiendo en pagina #%d en su version: %d nueva version: %d .\n", _paginasTrabajo[paginaIndice].id,_paginasTrabajo[paginaIndice].version,
            _paginasTrabajo[paginaIndice].version+1);
          _paginasTrabajo[paginaIndice].version += 1;
          _paginasTrabajo[paginaIndice].dueno = 1;
          _paginasTrabajo[paginaIndice].copia = 1;

          paginaData *args = malloc(sizeof *args);
          args->puerto = puertoAtenderInt;
          args->indice = paginaIndice;
          pthread_create(&threadAtender, NULL, serDueno, args);
          //pthread_join(threadAtender,NULL);
  
        } else {

          if(strcmp(accion,"pedir")==0){

            //******** 
            //Mini cliente para que se comunique con otro cliente.
            struct sockaddr_in clientePedir; //Declaración de la estructura con información para la conexión
            struct hostent *servidorPedir; //Declaración de la estructura con información del host
            servidorPedir = gethostbyname(host); //Asignacion
            if(servidorPedir == NULL)
            { //Comprobación 
              printf("Host erróneo.\n");
              return 1;
            }
            int conexionPedir;
            char bufferPedir[100];

            conexionPedir = socket(AF_INET, SOCK_STREAM, 0); //Asignación del socket       
            bzero((char *)&clientePedir, sizeof((char *)&clientePedir)); //Rellena toda la estructura de 0's
            //La función bzero() es como memset() pero inicializando a 0 todas la variables
            clientePedir.sin_family = AF_INET; //asignacion del protocolo
            clientePedir.sin_port = htons(puertoPedirInt); //asignacion del puerto
            bcopy((char *)servidor->h_addr, (char *)&cliente.sin_addr.s_addr, sizeof(servidor->h_length));
            //bcopy(); copia los datos del primer elemendo en el segundo con el tamaño máximo del tercer argumento.
            
            //cliente.sin_addr = *((struct in_addr *)servidor->h_addr); //<--para empezar prefiero que se usen
            //inet_aton(argv[1],&cliente.sin_addr); //<--alguna de estas dos funciones
            
            if(connect(conexionPedir,(struct sockaddr *)&clientePedir, sizeof(clientePedir)) < 0)
            { //conectando con el host
              printf("Error conectando con el host, puerto %ld .\n", puertoAtenderInt);
              close(conexionPedir);
              return 1;
            }
            printf("Conectado con %s:%d\n",inet_ntoa(clientePedir.sin_addr),htons(clientePedir.sin_port));
            //inet_ntoa(); está definida en <arpa/inet.h>

            //le envio al otro cliente 'leer' para que el em mande una copia (version) de la pagina que tiene que es a la que em estoy conectando
            send(conexionPedir, "escribir:0", 100, 0); //envio
            bzero(bufferPedir, 100);

            //aqui recibo la version de la copia que pedi al cliente y la agrego al array de la paginas que tengo y pongo la bandera copia en1 y dueno en 0
            recv(conexionPedir, bufferPedir, 100, 0); //recepción
            char *accionSolicitud,*versionSolicitud, *saveptrReadPedir;
            accionSolicitud = strtok_r(bufferPedir, ":", &saveptrReadPedir);
            versionSolicitud = strtok_r(NULL, ":", &saveptrReadPedir);

            if(strcmp(accionSolicitud,"version")==0){
              printf("Escribiendo en pagina #%d en su version: %s nueva version: %d .\n", _paginasTrabajo[paginaIndice].id,versionSolicitud,atoi(versionSolicitud)+1);
              _paginasTrabajo[paginaIndice].copia = 1;
              _paginasTrabajo[paginaIndice].dueno = 1;
              _paginasTrabajo[paginaIndice].version = atoi(versionSolicitud) +1;
            }

            paginaData *args = malloc(sizeof *args);
            args->puerto = puertoAtenderInt;
            args->indice = paginaIndice;
            pthread_create(&threadAtender, NULL, serDueno, args);
    
          }

        }
        bzero(buffer, 100);

      } 
    }

  }
  return 0;
}