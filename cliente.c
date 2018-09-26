#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>

#define BUFSIZE 1000
#define PARAMS_NUM 5

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

int main(int argc, char* argv[]){
  // Declarar variables
  char bufferParams[BUFSIZE], buffer[100], ipServidor[BUFSIZE], puertoConfig[BUFSIZE];
  int puerto, conexion;
  char *saveptr, *paramName, *paramValue;
  struct sockaddr_in cliente; 
  struct hostent *servidor; 
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
    if (strcmp(paramName, "IPServidor") == 0) 
    {
      strcpy(ipServidor, paramValue);
    } 
    else if (strcmp(paramName, "Puerto") == 0) {
      strcpy(puertoConfig, paramValue);
    }
  }
  fclose(file_handle);

  // Configurar servidor en el host proporcionado
  servidor = gethostbyname(trimPalabra(ipServidor)); 
  if(servidor == NULL) { 
    printf("Host erróneo, ip: %s \n.", ipServidor);
    return 0;
  }

  // Configurar cliente en el puerto y servidor configurados
  puerto=(atoi(trimPalabra(puertoConfig))); 
  bzero((char *)&cliente, sizeof((char *)&cliente)); 
  cliente.sin_family = AF_INET; 
  cliente.sin_port = htons(puerto); 
  bcopy((char *)servidor->h_addr, (char *)&cliente.sin_addr.s_addr, sizeof(servidor->h_length));

  // Ciclon de solicitudes
  while(1){
    sleep(2);

    // Con el cliente configurado trato de conectar con el servidor
    conexion = socket(AF_INET, SOCK_STREAM, 0); 
    if(connect(conexion,(struct sockaddr *)&cliente, sizeof(cliente)) < 0)
    {
      printf("Error conectando con el host\n.");
      close(conexion);
      return 0;
    }

    //Al conectarse es posible enviar y recibir mensajes
    printf("Conectado con %s:%d\n",inet_ntoa(cliente.sin_addr),htons(cliente.sin_port));
    sprintf(buffer, "Hola servidor!");
    send(conexion, buffer, 100, 0); 
    bzero(buffer, 100);
    recv(conexion, buffer, 100, 0);
    printf("%s", buffer);
  }
  
return 0;
}
