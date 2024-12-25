#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_SLEEP 128
#define MAX_MSG_LEN 1024
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

#define LOGIN 1 
#define LOGOUT 0
#define ERR_EXIT(s) perror(s), exit(errno);
const char *wellcome_banner =
        "\033[1;32m         ██████╗  ███████╗ ██╗ ███████╗                      \033[0m\n"
        "\033[1;32m        ██╔════╝  ██╔════╝ ██║ ██╔════╝                      \033[0m\n"
        "\033[1;32m        ██║       ███████╗ ██║ █████╗                     \033[0m\n"
        "\033[1;32m        ██║       ╚════██║ ██║ ██╔══╝                     \033[0m\n"
        "\033[1;32m        ╚██████╗  ███████║ ██║ ███████║                      \033[0m\n"
        "\033[1;32m         ╚═════╝  ╚══════╝ ╚═╝ ╚══════╝                      \033[0m\n"
        "\033[1;36m──────────────────────────────────────────────────────────────\033[0m\n"
        "\033[1;33m        Welcome to CSIE real-time online chat room!           \033[0m\n"
        "\033[1;36m──────────────────────────────────────────────────────────────\033[0m\n";

void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM || sig ==SIGHUP) {
        printf("\n%sCannot exit directly. Please type 'LOGOUT'.%s\n", RED, RESET);
    }
}
void disable_signal() {
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;  // Set to default action
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
}
void enable_signal() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;  // Set to default action
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
}

int connect_retry(int domain, int type, int protocol,
                  const struct sockaddr *addr, socklen_t alen) {
  int numsec, fd;

  for (numsec = 1; numsec <= MAX_SLEEP; numsec <<= 1) {
    if ((fd = socket(domain, type, protocol)) < 0) {
      return (-1);
    }
    if (connect(fd, addr, alen) == 0) {
      printf("Connect to server\n");
      return fd;
    }
    close(fd);
    if (numsec <= MAX_SLEEP / 2) {
      sleep(numsec);
    }
  }
  return (-1);
}

pthread_mutex_t terminal_mutex = PTHREAD_MUTEX_INITIALIZER;
char buffer[MAX_MSG_LEN];

void* recieve_msg(void* arg){
    int sock = *(int *)arg;
    char buffer2[BUFFER_SIZE];
    fprintf(stderr, "recieve_thread create\n");
    while (1) {
        memset(buffer2, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, buffer2, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Disconnected from server.\n");
            exit(0);
        }
        pthread_mutex_lock(&terminal_mutex);
        printf("\033[F\033[K");
        printf("%s\n", buffer2);
        printf("%s", buffer);
        pthread_mutex_unlock(&terminal_mutex);
    }
    return NULL;
}

void communicate_with_server(int client_socket) {
  char command[50], username[50], password[50];
  char message[MAX_MSG_LEN];
  int client_state = LOGOUT; 
  pthread_t recieve_thread;
  char msg;
  printf("%s", wellcome_banner);
  
  while (1) {
    memset(command, 0, 50);
    memset(buffer, 0, BUFFER_SIZE);
    msg = 0 ;
    if(recv(client_socket, buffer, BUFFER_SIZE, 0)==0){
      printf("Server disconnected\n");
    }
    printf("%s", buffer);
    if(strcmp(command, "CHAT") == 0) pthread_mutex_unlock(&terminal_mutex);
    memset(command, 0, 50);
    scanf("%s", command);

    if (strcmp(command, "REGISTER") == 0 || strcmp(command, "LOGIN") == 0) {
      printf("Username: ");
      scanf("%s", username);
      printf("Password: ");
      scanf("%s", password);

      snprintf(buffer, BUFFER_SIZE, "%s %s %s", command, username, password);
      send(client_socket, buffer, strlen(buffer), 0);
    } else if (strcmp(command, "EXIT") == 0) {
      snprintf(buffer, BUFFER_SIZE, "EXIT");
      send(client_socket, buffer, strlen(buffer), 0);
      //printf("Exit\n");
      break;
    } else if (strcmp(command, "LOGOUT")==0) {
      snprintf(buffer, BUFFER_SIZE, "LOGOUT");
      send(client_socket, buffer, strlen(buffer), 0);
      //printf("Logout\n");
    } else if(strcmp(command, "CHAT") == 0){
      pthread_mutex_lock(&terminal_mutex);
      memset(message, 0, MAX_MSG_LEN);
      printf("To: ");
      scanf("%s", username);
      printf("Message: ");
      fgets(message, MAX_MSG_LEN, stdin);
      //message[strcspn(message, "\n")] = 0; // Remove newline
      
      snprintf(buffer, MAX_MSG_LEN, "%s %s %s", command, username, message);
      send(client_socket, buffer, strlen(buffer), 0);


    } else{
      snprintf(buffer, BUFFER_SIZE, "UNKNOWN");
      send(client_socket, buffer, strlen(buffer), 0);
      //printf("Unknown command\n");
    }

    memset(buffer, 0, BUFFER_SIZE);
    if(recv(client_socket, buffer, BUFFER_SIZE, 0)==0){
      printf("Server disconnected\n");
    }
    if(strcmp(buffer, "Login successful\n") == 0){
      client_state = LOGIN;
      pthread_create(&recieve_thread, NULL, recieve_msg, (void*)&client_socket);
      pthread_detach(recieve_thread);
      send(client_socket, &msg, 1, 0);

    } else if (strcmp(buffer, "Logout successful\n") == 0){
      client_state = LOGOUT;
      pthread_cancel(recieve_thread);
    }
    msg = 1;
    send(client_socket, &msg, 1, 0);
    printf("Server: %s", buffer);

  }
}

int main() {
  int client_socket;
  struct sockaddr_in server_addr;
  setvbuf(stdout, NULL, _IONBF, 0);
  enable_signal();
  // client_socket = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  // connect(client_socket, (struct sockaddr*)&server_addr,
  // sizeof(server_addr)); printf("Connected to server\n");

  if ((client_socket = connect_retry(AF_INET, SOCK_STREAM, 0,
                                     (struct sockaddr *)&server_addr,
                                     sizeof(server_addr))) < 0)
    ERR_EXIT("conncetion fail\n");
  communicate_with_server(client_socket);

  close(client_socket);
  return 0;
}
