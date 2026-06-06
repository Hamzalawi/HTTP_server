#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080
#define THREAD_SIZE 4
#define QUEUE_SIZE 100
#define BUFFER_SIZE 1024

//defining the queue
int client_queue[QUEUE_SIZE];
int head = 0;
int tail = 0;
int count = 0;

//mutexes and conditions
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond_t = PTHREAD_COND_INITIALIZER;

// HTTP response
const char *HTTP_RESPONSE = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 62\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<html><body>"
    "<h1>Hello from the Thread Pool Server!</h1>"
    "</body></html>";

//error handling 
void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void queue_push(int socket){
    pthread_mutex_lock(&queue_mutex);
    if (count < QUEUE_SIZE){
        client_queue[tail] = socket;
        tail = (tail + 1 )%QUEUE_SIZE;
        count++;
        pthread_cond_signal(&queue_cond_t);
    }else {
        printf("queue is full ! Dropping connection. \n");
        close(socket);
    }
    pthread_mutex_unlock(&queue_mutex);
}

int dequeue(){

    pthread_mutex_lock(&queue_mutex);

    //waiting for the queue to have an element at least 
    while(count == 0){
        
        //unlock the mutex and put the thread to sleep until you recieve a signal
        //only then lock the mutex again and continue the program
        pthread_cond_wait(&queue_cond_t, &queue_mutex);
    }
    int client_socket = client_queue[head];
    head = (head + 1)% QUEUE_SIZE;
    count--;

    pthread_mutex_unlock(&queue_mutex);
    return client_socket;

}

void handle_connection(int client_socket){
    char buffer[BUFFER_SIZE] = {0};
    
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        char *first_line = strtok(buffer, "\n");
        if (first_line != NULL) {
            printf("Thread %lu handling request: %s\n", (unsigned long)pthread_self(), first_line);
        }
        write(client_socket, HTTP_RESPONSE, strlen(HTTP_RESPONSE));
    }
    
    close(client_socket);
}

void* thread_func(void* arg){
    while(1){


        //grab a task from the queue
        int client_socket = dequeue();

        //handle the task 
        handle_connection(client_socket);
    }

    return NULL;
}
int main(){
    
    int server_fd;         
    int new_socket; 
    struct sockaddr_in address;  
    int addrlen = sizeof(address);

    //initializing threads
    pthread_t thread_pool[THREAD_SIZE];
    for(int i = 0; i<THREAD_SIZE; i++ ){
        if(pthread_create(&thread_pool[i], NULL, thread_func, NULL) != 0)
            die("failed to create a thread");
    }

    //creating socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0); 

    if(server_fd < 0)
        die("failed to make a socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // in case the port has been previously in use to not get error
    
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;    

    //binding the  socket
    if(bind(server_fd, (struct sockaddr*) &address,addrlen))
        die("binding failed\n");

    if(listen(server_fd, 3))
        die("listeninng failed\n");
    
    printf("Thread Pool Server listening on port %d with %d workers...\n", PORT, THREAD_SIZE);

    //main loop
    while(1){
        new_socket = accept(server_fd, (struct sockaddr*) &address, (socklen_t *) &addrlen);
        //reading request 
        if(new_socket < 0)
            die("accept failed\n");
        
        queue_push(new_socket);
    }


    return 0;
}