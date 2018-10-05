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
#include <stdbool.h> 
#include <pthread.h>
#include <termios.h>

#define BUFSIZE 1000
#define PARAMS_NUM 5

typedef struct {
    int puerto;
    int indice;
} paginaData;

typedef struct {
  int id;
  int version;
  bool copia;
  bool dueno;
} paginaTrabajo;

typedef struct {
  int idPagina;
  char* host;
  int port;
} clientesConCopias;

paginaTrabajo _paginasTrabajo[100];
clientesConCopias _clientesConCopias[100];

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
  if(result < 1){
    result = 1.0;
  }
  return result;
}

int getPagina(int min, int max){
  srand(time(NULL));
  return min + rand() % (max+1 - min);
}

bool getLeer(int propabilidad){
  int random = 0 + rand() % (100+1 - 0);
  srand(time(NULL));
  if(random <= propabilidad){
    return true;
  }
  return false;
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
    printf("ERROR CREANDO MI SERVIDOR DE DUENO! (BORRAR)\n");
    close(conexionServidor);
    return 0;
  }
  // Escuchar
  listen(conexionServidor, 3);
  printf("LISTO PARA DAR COPIAS DE LA PAGINA %d EN EL PUERTO: %d.\n", _paginasTrabajo[data->indice].id, ntohs(servidor.sin_port));

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

        //if(leer){
          bzero((char *)&buffer, sizeof(buffer));
          sprintf(buffer, "%d", _paginasTrabajo[data->indice].version);
          send(conexion_cliente, buffer, 100, 0);
        //}
      }

      //fCloseThreads cuando el servidor me quite el dueno lo cambio a 0
  }
  return NULL;
}

int main(int argc, char* argv[]){
  // Declarar variables
  char bufferParams[BUFSIZE], buffer[100], ipServidor[BUFSIZE], 
       puertoConfig[BUFSIZE], mediaConfig[BUFSIZE], paginasMin[BUFSIZE], paginasMax[BUFSIZE],
       lecturaConfig[BUFSIZE];
  int puerto, conexion, pagina, cantidadPaginas, paginaMin, paginaMax,
      paginaIndice;
  bool leer;
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
    _paginasTrabajo[i].copia = false;
    _paginasTrabajo[i].dueno = false;
    i++;
  }

  printf("Voy a esperar %lfs para solicitar cada pagina.\n", mediaExponencial);
  // Ciclo de solicitudes
  while(true){
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
          if(strcmp(buffer,"leer") == 0){
            printf("Adquiriendo copia y leyendo pagina #%d en su version %d.\n", _paginasTrabajo[paginaIndice].id, _paginasTrabajo[paginaIndice].version);
            _paginasTrabajo[paginaIndice].copia = true;
            //Abrir thread para escuchar en caso de que deba matar esta copia.
          } else{
            printf("Debo pedirsela a otro cliente.\n");
            
            char *saveptrRead, *accion, *puerto, *host;
            printf("\t----> ***** BUFFER: %s *****\n",buffer);
            accion = strtok_r(buffer, ":", &saveptrRead);

            if(strcmp(accion,"pedir") == 0){
              puerto = strtok_r(NULL, ":", &saveptrRead);
              host = strtok_r(NULL, ":", &saveptrRead);
              printf("\t----> Debo conectarme con %s:%s para pedirle pagina a leer*****\n",puerto,host);
              //crear un mini cliente que se conecte con el otro lciente para pedir la copia de la pag
              //https://es.wikibooks.org/wiki/Programaci%C3%B3n_en_C/Sockets
              //********
              struct sockaddr_in clientePedir; //Declaración de la estructura con información para la conexión
              struct hostent *servidorPedir; //Declaración de la estructura con información del host
              servidorPedir = gethostbyname(host); //Asignacion
              if(servidorPedir == NULL)
              { //Comprobación 
                printf("Host erróneo\n");
                return 1;
              }
              int puertoPedir, conexionPedir
              char bufferPedir[100];;
              //********
            }


          }
          bzero(buffer, 100);

          close(conexion);
        }    
    }else{
      printf("Intentando escribir pagina #%d.\n",pagina);
      // Caso quiero escribir, soy dueno.
      if(_paginasTrabajo[paginaIndice].dueno){
        printf("Escribiendo mi pagina #%d en su version %d, nueva version %d.\n", _paginasTrabajo[paginaIndice].id, 
          _paginasTrabajo[paginaIndice].version, _paginasTrabajo[paginaIndice].version+1);
        _paginasTrabajo[paginaIndice].version += 1;
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
        char *saveptrRead, *accion, *puertoAtender, *host;
        long int puertoAtenderInt;
        accion = strtok_r(buffer, ":", &saveptrRead);
        puertoAtender = strtok_r(NULL, ":", &saveptrRead);
        puertoAtenderInt = atoi(puertoAtender);
        host = strtok_r(NULL, ":", &saveptrRead);
        pthread_t threadAtender;

        if(strcmp(accion,"escribir") == 0) {

          printf("Escribir en pagina #%d, mi puerto de atencion sera %ld.\n", _paginasTrabajo[paginaIndice].id, puertoAtenderInt);
          //Abrir thread para escuchar en caso de que otro cliente quiera una copia y asi darle la version.
          printf("Escribiendo en pagina #%d en su version: %d nueva version: %d.\n", _paginasTrabajo[paginaIndice].id,_paginasTrabajo[paginaIndice].version,
            _paginasTrabajo[paginaIndice].version+1);
          _paginasTrabajo[paginaIndice].version += 1;
          _paginasTrabajo[paginaIndice].dueno = true;
          _paginasTrabajo[paginaIndice].copia = true;

          paginaData *args = malloc(sizeof *args);
          args->puerto = puertoAtenderInt;
          args->indice = paginaIndice;
          pthread_create(&threadAtender, NULL, serDueno, args);
          //pthread_join(threadAtender,NULL);
  
        }
        bzero(buffer, 100);

      } 
    }

  }
  return 0;
}