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

#define BUFSIZE 1000
#define PARAMS_NUM 5

typedef struct {
  int id;
  int version;
  bool copia;
  bool dueno;
} paginaTrabajo;

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
  paginaTrabajo paginas[cantidadPaginas];
  int i = 0;
  for(int p = paginaMin; p<=paginaMax; p++){
    paginas[i].id = p;
    paginas[i].version = 0;
    paginas[i].copia = false;
    paginas[i].dueno = false;
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
      if(paginas[paginaIndice].dueno){
        printf("Leyendo mi pagina #%d en su version %d.\n", paginas[paginaIndice].id, paginas[paginaIndice].version);
      } else if(paginas[paginaIndice].copia){ // Caso quiero leer, no soy dueno pero tengo una copia.
        printf("Leyendo pagina #%d (copia) en su version %d.\n", paginas[paginaIndice].id, paginas[paginaIndice].version);
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
          sprintf(buffer, "leer:%d", paginas[paginaIndice].id);
          send(conexion, buffer, 100, 0); 
          bzero(buffer, 100);

          // Espero data
          recv(conexion, buffer, 100, 0);
          if(strcmp(buffer,"leer") == 0){
            printf("Adquiriendo copia y leyendo pagina #%d en su version %d.\n", paginas[paginaIndice].id, paginas[paginaIndice].version);
            paginas[paginaIndice].copia = true;
          } else{
            printf("Debo pedirsela a otro cliente.\n");
          }
          bzero(buffer, 100);

          close(conexion);
        }    
    }else{
      printf("Intentando escribir pagina #%d.\n",pagina);
      // Caso quiero escribir, soy dueno.
      if(paginas[paginaIndice].dueno){
        printf("Escribiendo mi pagina #%d en su version %d, nueva version %d.\n", paginas[paginaIndice].id, 
          paginas[paginaIndice].version, paginas[paginaIndice].version+1);
        paginas[paginaIndice].version += 1;
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
        sprintf(buffer, "escribir:%d", paginas[paginaIndice].id);
        send(conexion, buffer, 100, 0); 
        bzero(buffer, 100);

        // Espero data
        recv(conexion, buffer, 100, 0);
        char *saveptrRead, *accion, *puerto, *host;
        accion = strtok_r(buffer, ":", &saveptrRead);
        puerto = strtok_r(NULL, ":", &saveptrRead);
        host = strtok_r(NULL, ":", &saveptrRead);

        if(strcmp(accion,"escribir") == 0){
          printf("Escribir en pagina #%d, mi puerto de atencion sera %s.\n", paginas[paginaIndice].id,puerto);
          //Abrir thread para escuchar en caso de que otro cliente quiera una copia y asi darle la version.
          printf("Escribiendo en pagina #%d en su version: %d nueva version: %d.\n", paginas[paginaIndice].id,paginas[paginaIndice].version,
            paginas[paginaIndice].version+1);
          paginas[paginaIndice].version += 1;
          paginas[paginaIndice].dueno = true;
          paginas[paginaIndice].copia = true;
        }
        bzero(buffer, 100);

      } 
    }

  }
  return 0;
}