#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NAME_LEN 32

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];

void str_overwrite_stdout(){
  printf("\r%s","> " );
  fflush(stdout);
}

void str_trim_lf(char* arr, int lenght){
  for(int i=0; i<lenght; i++){
    if(arr[i] == '\n'){
      arr[i] = '\0';
      break;
    }
  }
}

void catch_ctrl_c_and_exit(){
	flag = 1;
}

int main(int argc, char **argv){
  if (argc != 2){
    printf("Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);

  signal(SIGINT, catch_ctrl_c_and_exit);

  printf("Enter your name: ");
  fgets(name, NAME_LEN, stdin);
  str_trim_lf(name, strlen(name));

  if(strlen(name) > NAME_LEN - 1 || strlen(name) < 2){
    printf("Enter name correctly\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_in server_addr;
  //Socket Settings
  sockfd = socket(AF_INET , SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);

  //Conect to the server
  int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if(err == -1){
    printf("ERROR: connect\n");
    return EXIT_FAILURE;
  }

  //Send the name
  send(sockfd, name, NAME_LEN, 0);
  printf("=== Bienvenido al CHATROOM====\n");

  return EXIT_SUCCESS;
}
