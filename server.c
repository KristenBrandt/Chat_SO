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
#define ACTIVO 0
#define OCUPADO 1
#define INACTIVO 2


static _Atomic unsigned int cli_count = 0;
static int uid = 10;

//gcc -Wall -g3 -fsanitize=address -pthread serer.c -o server
//telnet localhost 4444

//Client Structure - Para poder diferenciar a los clientes
typedef struct{
  struct sockaddr_in address;
  int sockfd;
  int uid;
  char name[NAME_LEN];
  int status;
}client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

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
/* agregar clientes al queue */
void queue_add(client_t *cl){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(!clients[i]){
      clients[i] = cl;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

/* remover clientes del queue */
void queue_remove(int uid){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; i++){
    if(clients[i]){
      if(clients[i] -> uid == uid){
        clients[i] = NULL;
        break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}


void print_ip_addr(struct sockaddr_in addr) {
  printf("%d.%d.%d.%d",
  addr.sin_addr.s_addr & 0xff,
  (addr.sin_addr.s_addr & 0xff00) >> 8,
  (addr.sin_addr.s_addr & 0xff0000) >> 16,
  (addr.sin_addr.s_addr & 0xff000000) >> 24);

/* mandar mensaje a todos los clientes exepto el que lo manda */
}void send_message(char *s, int uid){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0;i<MAX_CLIENTS;i++){
      if(clients[i]){
        if (clients[i]->uid != uid){
          if(write(clients[i]-> sockfd,s,strlen(s))<0){
            printf("ERROR: write to descriptor failed\n");
            break;
          }
        }
      }
  }
  pthread_mutex_unlock(&clients_mutex);
}

/* obtener el estado del cliente*/
/* los nombres de los estados son asi por el protocolo */
string obtenerEstado(int state){
  if (state == 0){
    return "ACTIVO";
  } else if (state == 1){
    return "OCUPADO";
  }
  return "INACTIVO";
}

/* manejar todoas las comunicaciones con el cliente */
void *handle_client(void *arg){
  char buff_out[BUFFER_SZ];
  char name[NAME_LEN];
  int leave_flag = 0; //para ver si hay algun error y desconectar
  cli_count++;

  client_t *cli = (client_t*)arg;

  //nombre del clientes
  if(recv(cli -> sockfd, name, NAME_LEN, 0) <= 0 || strlen(name)< 2|| strlen(name)>= NAME_LEN-1){
    printf("Ingrese el nombre correctamente\n");
    leave_flag = 1;
  }
  strint what(name);
  Payload registrar_proto_payload;
  registrar_proto_payload.ParseFromString(name);
  
  /*revisar si el usuario ya existe */
  else if (get_client_index(registrar_proto_payload.sender() >= 0 )){
    printf("Este usuario ya existe, no se puede usar");
    leave_flag =1;
  }
  else{
    strcpy(cli -> name, registrar_proto_payload.sender().c_str());
    sprintf(buff_out, "%s se ha unido\n", cli->name);
    printf("%s",buff_out);
    send_message(buff_out, cli-> uid);
  }
  bzero(buff_out, BUFFER_SZ);
  int registro = get_client_index(register_payload.sender());
  printf("Usuario registrado exitosamente: %d\n", registro );
  while(1){
    if(leave_flag){
      break;
    }
    int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);

    if(receive > 0){
      if(strlen(buff_out)>0){
        send_message(buff_out, cli-> uid);
        str_trim_lf(buff_out, strlen(buff_out));
        printf("%s -> %s", buff_out, cli-> name);
      }
    } else if(receive ==0 || strcmp(buff_out, "exit")==0){
      sprintf(buff_out, "%s se salio\n", cli-> name);
      printf("%s",buff_out);
      send_message(buff_out, cli-> uid);
      leave_flag = 1;
    }else{
      printf("ERROR: -1\n");
      leave_flag = 1;
    }
    bzero(buff_out, BUFFER_SZ);
  }
  close(cli-> sockfd);
  queue_remove(cli-> uid);
  free(cli);
  cli_count--;
  pthread_detach(pthread_self());

  return NULL;
}

int main(int argc, char **argv){
  if (argc != 2){
    printf("Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }
  char *ip = "10.0.2.15";
  int port = atoi(argv[1]);

  int option = 1;
  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  //Socket Settings
  listenfd = socket(AF_INET , SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);


  //Signals - software generated interrupts
  signal(SIGPIPE, SIG_IGN);

  if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option))<0){
    perror("ERROR: setsockopt\n");
    return EXIT_FAILURE;
  }

  //Bind
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){
    perror("ERROR: bind\n");
    return EXIT_FAILURE;
  }

  //listen
  if(listen(listenfd,10)<0){
    perror("ERROR: listen\n");
    return EXIT_FAILURE;
  }

  printf("=== BIENVENIDO A EL CHATROOM DE BLOCK Y KRISTEN ===\n");

  while(1){
    socklen_t clilen = sizeof(cli_addr);
    connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

    //Check for MAX Client
    if((cli_count +1 ) == MAX_CLIENTS){
      printf("Maximo numero de clientes conectados. Conexion no valida\n" );
      print_ip_addr(cli_addr);
      close(connfd);
      continue;
    }
    //client Settings
    client_t *cli = (client_t *)malloc(sizeof(client_t));
    cli -> address = cli_addr;
    cli -> sockfd = connfd;
    cli -> uid = uid ++;
    cli -> status = ACTIVE;

    //add client to queue
    queue_add(cli);
    pthread_create(&tid, NULL, &handle_client, (void*)cli);

    //reduce CPU Usage
    sleep(1);
  }
  return EXIT_SUCCESS;
}
