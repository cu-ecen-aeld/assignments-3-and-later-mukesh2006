#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdbool.h>

// Assignment 6 part 1 
#include "queue.h"
#include <pthread.h>
#include <time.h>

//## MACROS
#define ERROR -1
#define NO_ERROR 0
#define RECEIVE_PACKET_SIZE 1024
#define PORT "9000"
#define RECEIVE_DATA_FILE "/var/tmp/aesdsocketdata"
#define LISTEN_BACKLOG 30
#define BUFFER_SIZE 2000

// Structure 
/** Assignment 6 part 1 */
/**
 *  d. Use the singly linked list APIs discussed in the video (or your own implementation if you prefer) to manage threads.
 *   --> This structure shall be passed to the thread upon a successful new connection, and freed upon completion of the thread.  
 */
/** The data type for the node */
typedef struct connectionHandler_t {
    pthread_t        thread_id;
    pthread_mutex_t *write_sync_mutex;
    int              client_fd; 
    char             client_address[INET6_ADDRSTRLEN];
    bool             is_thread_complete;
    SLIST_ENTRY(connectionHandler_t)  connectionHandler_next;
} connectionHandler_t;


/** Function Prototype */
void* connection_handler_thread_fxn(void* parameter);
void print_time_thread_fxn(union sigval sv);
void setup_print_time_thread(int seconds);


