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

char* chop(char *string)
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

  char bufferParams[BUFSIZE];
  FILE* file_handle;
  file_handle = fopen(argv[1], "r");

  char *saveptr;
  char *paramName, *paramValue;
  char ipServidor[BUFSIZE], puertoConfig[BUFSIZE];

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

  struct sockaddr_in cliente; 
  struct hostent *servidor; 
  servidor = gethostbyname(chop(ipServidor)); 
  if(servidor == NULL)
  { 
    printf("Host erróneo, ip:%s test", ipServidor);
    return 1;
  }
  int puerto, conexion;
  char buffer[100];
  puerto=(atoi(chop(puertoConfig))); 
  bzero((char *)&cliente, sizeof((char *)&cliente)); 

  cliente.sin_family = AF_INET; 
  cliente.sin_port = htons(puerto); 
  bcopy((char *)servidor->h_addr, (char *)&cliente.sin_addr.s_addr, sizeof(servidor->h_length));

  int r = rand();
  while(1){
    sleep(2);
    conexion = socket(AF_INET, SOCK_STREAM, 0); 
    if(connect(conexion,(struct sockaddr *)&cliente, sizeof(cliente)) < 0)
    {
      printf("Error conectando con el host\n");
      close(conexion);
      return 1;
    }
    printf("Conectado con %s:%d\n",inet_ntoa(cliente.sin_addr),htons(cliente.sin_port));
    sprintf(buffer, "Hola mi ID es: %d", r);
    send(conexion, buffer, 100, 0); 
    bzero(buffer, 100);
    recv(conexion, buffer, 100, 0);
    printf("%s", buffer);
  }
  
return 0;
}
