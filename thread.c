#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int _fCloseThreads;

int getch (void)
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

void* serveClients()
{
    while(_fCloseThreads){
        sleep(1);
        printf("Serving clients: %d\n", _fCloseThreads);
    }
    printf("Process finished :) ! \n");
}

void* finishProgram()
{
    int ch;
    do{
        printf("%s", (char*)"Exit? \n");
        ch = getch();
        if(ch=='E'){
            printf("%s", (char*)"Yes \n");
            _fCloseThreads = 0;
            return 0;
        }
        printf("%s", (char*)"No \n");
    }while(1);
}

int main(void)
{
  pthread_t thread_arr[2];
  int iter_id[2];
  _fCloseThreads = 1;

  for (int iter=0; iter<2; iter++) {
    iter_id[iter] = iter;
    if(iter == 0){
        pthread_create(&thread_arr[iter], NULL, serveClients, &iter_id[iter]);
    } else{
        pthread_create(&thread_arr[iter], NULL, finishProgram, &iter_id[iter]);
    }
  }

  for (int iter=0;iter<2;iter++) {
    pthread_join(thread_arr[iter],NULL);
  }
  return 0;
}
