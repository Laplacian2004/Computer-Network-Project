#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_USERS 1024
#define THREAD_POOL_SIZE 10
#define MAX_WORD_LEN 50
#define MAX_MSG_LEN 1024
#define LOGOUT 0 
#define LOGIN 1

// User structure to store username, password, and login status
typedef struct {
    char username[MAX_WORD_LEN];
    char password[MAX_WORD_LEN];
} User;

//Client table entry structure
typedef struct {
  int socket;
  char* username;
  bool is_active;
} Client;

// Task structure for thread pool
typedef struct {
    Client* client;
    bool is_active;
} Task;



User users[MAX_USERS]; // Array to hold user data
int user_count = 0; // Current count of registered users
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread safety

Task task_queue[THREAD_POOL_SIZE]; // Fixed-size task queue
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for task queue
pthread_cond_t task_cond = PTHREAD_COND_INITIALIZER;   // Condition variable for task signaling

Client clients[MAX_USERS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

char *enter_cmd_1 = "Enter command (REGISTER/LOGIN/EXIT): ";
char *enter_cmd_2 = "Enter command (CHAT/LOGOUT): ";
char *relay_0 = "Recipent not found\n";
char *relay_1 = "Message successfully send\n";
char *relay_2 = "Recipent is currently offline\n";
char *relay_3 = "Recipent cannot be sender\n";
// Function to save user data to a file
void save_users_to_file() {
    FILE *file = fopen("users.txt", "w");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s %s\n", users[i].username, users[i].password);
    }
    fclose(file);
}

// Function to load user data from a file
void load_users_from_file() {
    FILE *file = fopen("users.txt", "r");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    char username[MAX_WORD_LEN], password[MAX_WORD_LEN];
    while (fscanf(file, "%s %s", username, password) == 2) {
        strcpy(users[user_count].username, username);
        strcpy(users[user_count].password, password);
        user_count++;
    }
    fclose(file);
}

// Function to register a new user
int register_user(const char *username, const char *password) {
    pthread_mutex_lock(&user_mutex);

    for(int i=0;i<user_count;++i){
      if(strcmp(username, users[i].username)==0){
        pthread_mutex_unlock(&user_mutex);
        return 2;
      }
    }

    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    user_count++;
    save_users_to_file();
    pthread_mutex_unlock(&user_mutex);
    return 0;
}

// Function to authenticate a user during login
int authenticate_user(const char *username, const char *password) {
    int ret = 0;
    pthread_mutex_lock(&user_mutex);
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0) {
            ret = 1; // Authentication successful
            break;
        }
    }
    pthread_mutex_unlock(&user_mutex);
    return ret;
}

void put_name_to_client(Client* client, const char* username){
      pthread_mutex_lock(&clients_mutex);
      client->username = (char *) malloc(MAX_WORD_LEN*sizeof(char));
      strcpy(client->username, username);
      pthread_mutex_unlock(&clients_mutex);
}
// Function to log out a user
void logout_user(Client* client) {
    pthread_mutex_lock(&clients_mutex);
    if(client == NULL){
      fprintf(stderr, "logout NULL user\n");
    }else{
      free(client->username);
      client->username = NULL;
    }
    pthread_mutex_unlock(&clients_mutex);
}

void exit_client(Client* client){
    pthread_mutex_lock(&clients_mutex);
    client->is_active = false;
    client->socket = -1;
    client->username = NULL;
    pthread_mutex_unlock(&clients_mutex);
}

int relay_send(const char* sender, const char* recipient, const char* msg){
    int flag = 0;
    if(strcmp(sender, recipient) == 0){
      return 3;
    }
    pthread_mutex_lock(&user_mutex);
    for (int i = 0 ;i < user_count; i++){
      if(strcmp(users[i].username, recipient) == 0){
        flag = 1;
      }
    }
    pthread_mutex_unlock(&user_mutex);
    
    if(flag == 0){
      return 0;
    }
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_USERS; i++){
      if((clients[i].is_active == true )&&(strcmp(recipient, clients[i].username) == 0)){
          flag =1;
          char relay_message[BUFFER_SIZE];
          snprintf(relay_message, BUFFER_SIZE, "%s: %s", sender, msg);
          send(clients[i].socket, relay_message, strlen(relay_message), 0);
      }
    }
    pthread_mutex_unlock(&clients_mutex);

    if(flag) return 1;
    else return 2;
}
// Handle client requests
void handle_client(Client* client, int* client_state) {
    int client_socket = client->socket;
    char buffer[BUFFER_SIZE];
  
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if((*client_state) == LOGOUT) {
          send(client_socket, enter_cmd_1, strlen(enter_cmd_1), 0);
        } else {
          send(client_socket, enter_cmd_2, strlen(enter_cmd_2), 0);
        }
        if(recv(client_socket, buffer, BUFFER_SIZE, 0)==0){
          break;
        }
        fprintf(stderr, "Server read: %s\n", buffer);
        // Split command and arguments
        char *command = strtok(buffer, " ");
        char *username;
        char *password;
        char recipient[MAX_WORD_LEN];
        char message[MAX_MSG_LEN];
        char msg = 0;
        int ret;

        if (strcmp(command, "REGISTER") == 0 && (*client_state)== LOGOUT) {
            fprintf(stderr, "User register\n");
            username = strtok(NULL, " ");
            password = strtok(NULL, " ");
            if ((ret = register_user(username, password)) == 0)
                send(client_socket, "Registration successful\n", 24, 0);
            else if (ret==2)
                send(client_socket, "Username has been used\n", 23, 0);
            else 
                send(client_socket, "Registration failed\n", 20, 0);
        } else if (strcmp(command, "LOGIN") == 0 && (*client_state) == LOGOUT) {
            fprintf(stderr, "User login\n");
            username = strtok(NULL, " ");
            password = strtok(NULL, " ");
            if (authenticate_user(username, password)) {
                (*client_state) = LOGIN;
                send(client_socket, "Login successful\n", 17, 0);
                recv(client_socket, &msg, 1, 0); // receive OK of client receive_thread creation
                put_name_to_client(client, username);
            }
            else {
                send(client_socket, "Invalid credentials\n", 20, 0);
            }
        } else if (strcmp(command, "CHAT") == 0 && (*client_state) == LOGIN) {
            fprintf(stderr, "User chatting\n");
            sscanf(buffer+5, "%s %[^\n]", recipient, message);
            ret = relay_send(client->username, recipient, message);
            if (ret == 0){
              send(client_socket, relay_0, strlen(relay_0), 0);
            } else if (ret == 1){
              send(client_socket, relay_1, strlen(relay_1), 0);
            } else if (ret == 2){
              send(client_socket, relay_2, strlen(relay_2), 0);
            } else if (ret == 3){
              send(client_socket, relay_3, strlen(relay_3), 0);
            }
        } else if (strcmp(command, "LOGOUT") == 0 && (*client_state) ==LOGIN) {
            fprintf(stderr, "User logout\n");
            logout_user(client);
            (*client_state) = LOGOUT;
            send(client_socket, "Logout successful\n", 18, 0);
        } else if (strcmp(command, "EXIT")==0 && (*client_state) == LOGOUT) {
            fprintf(stderr, "User exit\n");
            send(client_socket, "Exit successful\n", 16, 0);
            break;
        } else {
            send(client_socket, "Unknown command\n", 16, 0);
        }
        
        if(recv(client_socket, &msg, 1, 0)==0) break;
    }
    close(client_socket);
    exit_client(client);
}

// Worker thread function for thread pool
void *worker_thread(void *arg) {
    printf("thread %d is on\n", (int)arg);
    while (1) {
        pthread_mutex_lock(&task_mutex);

        while (task_queue[0].is_active == false) {
            pthread_cond_wait(&task_cond, &task_mutex);
        }
        printf("thread %d get connection\n", (int)arg);
        // Retrieve the task from the front of the queue
        Client* working_client = task_queue[0].client;
        int client_state;
        task_queue[0].is_active = false;

        // Shift tasks in the queue
        for (int i = 0; i < THREAD_POOL_SIZE - 1; i++) {
            task_queue[i] = task_queue[i + 1];
        }
        task_queue[THREAD_POOL_SIZE-1].is_active = false;
        pthread_mutex_unlock(&task_mutex);

        // Handle the client
        handle_client(working_client, &client_state);
    }
    return NULL;
}

// Function to add a task to the task queue
void add_task(Client* client) {
    pthread_mutex_lock(&task_mutex);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (task_queue[i].is_active == false) {
            task_queue[i].client = client;
            task_queue[i].is_active = true;
            pthread_cond_signal(&task_cond); // Signal a worker thread
            break;
        }
    }

    pthread_mutex_unlock(&task_mutex);
}

int init_server(int type, struct sockaddr* add, socklen_t a_len, int q_len);

int main() {
    load_users_from_file(); // Load existing user data from file
    setvbuf(stdout, NULL, _IONBF, 0); 
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    // Initialize task queue
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        task_queue[i].is_active = false;
    }
    // Initialize client socket table
    for (int i = 0; i < MAX_USERS; i++){
        clients[i].is_active = false;
        clients[i].socket = -1;
        clients[i].username = NULL;
    }
    // Create a thread pool
    pthread_t thread_pool[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, worker_thread, (void*)i);
    }
    
    // Create a socket for the server
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    server_socket = init_server(SOCK_STREAM, (struct sockaddr*)&server_addr, sizeof(server_addr), 5);
    printf("Server is running on port %d\n", PORT);

    while (1) {
        // Accept a client connection
        printf("new round\n");
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        printf("accepts conn\n");
        int flag = 0;

        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        for(int i = 0; i < MAX_USERS ;i++){
          if(clients[i].is_active == false){
            clients[i].socket = client_socket;
            clients[i].is_active = true;
            flag = 1;
            add_task(&clients[i]);
            printf("Client connected\n");
            break;
          }
        }
        pthread_mutex_unlock(&clients_mutex);

        if(flag == 0){
          printf("Server busy\n");
          close(client_socket);
        }
    }

    close(server_socket); // Close the server socket
    return 0;
}

int init_server(int type, struct sockaddr* addr, socklen_t a_len, int qlen){
    int fd, err;
    int reuse =1;

    if((fd=socket(addr->sa_family, type ,0))<0){
      return (-1);
    }
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))<0){
      goto errout;
    }
    if(bind(fd, addr, a_len)<0){
      goto errout;
    }
    if(type==SOCK_STREAM||type==SOCK_SEQPACKET){
      if(listen(fd, qlen)<0)
        goto errout;
    }
    return fd;
errout:
  err=errno;
  close(fd);
  errno=err;
  return (-1);
}


