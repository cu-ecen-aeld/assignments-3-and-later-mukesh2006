#include "aesdsocket.h"

// signal handler function declaration 
void signal_handler(int);

// global variables
int server_fd;
FILE *received_data_file_fd;
pthread_mutex_t write_sync_mutex; //Note: making mutex is not always a good idea, it's made it global because it is used for a simple Read-Write Mutex Scenario 


// Assignemnt 6 part 1 
// Made these variables global so that the signal_handler can access timerId, and head head of the linked list and do a graceful exit
SLIST_HEAD(head_t, connectionHandler_t) head; // Singly-linked List head
timer_t timerId; 

//todo: these variables can be local in respective functions esp: signal_handler and main, not require to be global.. but i'm lazy to do this todo!!!
connectionHandler_t *conn_hand_ptr;
connectionHandler_t *conn_hand_ptr_temp;

// main function 
int main(int argc, char* argv[])
{
  // registr for signal
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  received_data_file_fd=NULL;

  // initialize required variables for the program
  int optval=1;
  struct addrinfo *servinfo=NULL;
  //struct addrinfo *iterator=NULL;
  struct addrinfo hints;

  // initialize the hints 
  memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_UNSPEC;     
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_flags = AI_PASSIVE;     

  //struct sockaddr_storage client_addr;
  struct sockaddr_in client_addr;
  socklen_t addr_size;
  char client_ip_address[INET6_ADDRSTRLEN];
  memset(client_ip_address, 0, sizeof (client_ip_address));

  // Assignemnt 6 part 1 
  pthread_t conn_handler_thread;
  // Initialize the single linked list
  SLIST_INIT(&head);                
  
  // initialize the mutex: 
    if(NO_ERROR!=pthread_mutex_init(&write_sync_mutex, NULL))
    {
        printf("Error in function pthread_mutex_init\n");
        syslog(LOG_DEBUG, "Error in function pthread_mutex_init\n");
        return ERROR;    
    }

 // getaddrinfo 
  if(NO_ERROR!=getaddrinfo(NULL, PORT, &hints, &servinfo)) 
  {
      printf("Error in function getaddrinfo\n");
      syslog(LOG_DEBUG, "Error in function getaddrinfo\n");
      return ERROR;
  }
  
  //b. Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
  // socket 
  server_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if(ERROR == server_fd)
  { 
      syslog(LOG_PERROR, "Error in function socket\n");
      printf("Error in function socket\n");
      return ERROR;
  }

  if (ERROR == setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) 
  {
      syslog(LOG_PERROR, "error in function setsockopt\n");
      printf("error in function setsockopt\n");
      return ERROR;
  }

  if(ERROR == bind(server_fd, servinfo->ai_addr, servinfo->ai_addrlen))
  { 
      syslog(LOG_PERROR, "error in function bind\n");
      printf("error in function bind: serverFd: %d,   servinfo->ai_addrlen: %d\n", server_fd,   servinfo->ai_addrlen);
      perror("\n ERROR: \n");
      close(server_fd);
      return ERROR;
  }

  // free the memory 
  freeaddrinfo(servinfo);
   
  // c. Listens for and accepts a connection 
  // listen to the address
  if(ERROR == listen(server_fd, LISTEN_BACKLOG))
  {  
      syslog(LOG_PERROR, "error in function listen\n");
      printf("error in function listen\n");
      return ERROR;
  }
  
  
  // 5. Modify your program to support a -d argument which runs the aesdsocket application as a daemon. 
  // When in daemon mode the program should fork after ensuring it can bind to port 9000.
  if( (2 == argc) && (0 == strcmp(argv[1], "-d")) )
  {
  	syslog(LOG_DEBUG, "Starting the application aesdsocket as a daemon\n");
    printf("Starting the application aesdsocket as a daemon\n");
    daemon(0, 0);
  }


  // Timer init
  setup_print_time_thread(10);  

  // h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received
  syslog(LOG_DEBUG,"Entering While (true) Now\n");
  printf("Entering While (true) Now\n");
    
  while(true)
  {
    
     /* Assignment 6 part 1: Implementing the threading */
    // 1. Modify your socket based program to accept multiple simultaneous connections, with each connection spawning a new thread to handle the connection.
    conn_hand_ptr = (connectionHandler_t *)malloc(sizeof(connectionHandler_t));
    if (NULL == conn_hand_ptr)
    {
      syslog(LOG_PERROR, "failure in function malloc\n");
      continue; // still continue, maybe next time the malloc will work 
    }

    addr_size = sizeof(client_addr);
	  // c. Listens for and accepts a connection  
    conn_hand_ptr->client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
    
    // error in client request acceptance
    if(ERROR==conn_hand_ptr->client_fd)
    {
        syslog(LOG_PERROR, "error in function accept\n");
        printf("error in function aceept\n");
        return ERROR;
    }

    // initalize the thread variables 
    conn_hand_ptr->write_sync_mutex    = &write_sync_mutex;
    conn_hand_ptr->is_thread_complete  = false;

    // d. Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client. 
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip_address, sizeof (client_ip_address));
    syslog(LOG_DEBUG, "Accepted connection from %s\n", client_ip_address);
    printf("Accepted connection from %s\n", client_ip_address);
    strncpy(conn_hand_ptr->client_address, client_ip_address, INET6_ADDRSTRLEN);
   
    // create the thread to handle read/write from the client
    if (NO_ERROR != pthread_create(&conn_handler_thread, NULL, connection_handler_thread_fxn, conn_hand_ptr)) 
    {
      syslog(LOG_DEBUG, "Failure in pthread_create\n");
      printf("Failure in pthread_create\n");
      close(conn_hand_ptr->client_fd);
      syslog(LOG_INFO, "Closed connection from %s\n", conn_hand_ptr->client_address);
      printf("Closed connection from %s\n", conn_hand_ptr->client_address); 
      free(conn_hand_ptr);
      conn_hand_ptr = NULL;
      continue; // Continue the loop in hope that next client can be served 
    }
    // store the thread_id in th thread 
    conn_hand_ptr->thread_id = conn_handler_thread;
    SLIST_INSERT_HEAD(&head, conn_hand_ptr, connectionHandler_next);
    conn_hand_ptr = NULL;
    SLIST_FOREACH_SAFE(conn_hand_ptr, &head, connectionHandler_next, conn_hand_ptr_temp) 
    {
      if (conn_hand_ptr->is_thread_complete)
      {
        close(conn_hand_ptr->client_fd);
        syslog(LOG_INFO, "Closed connection from %s\n", conn_hand_ptr->client_address);
        printf("Closed connection from %s\n", conn_hand_ptr->client_address);
        // join the thread so that when it finishes it exists gracefully 
        if(NO_ERROR != pthread_join(conn_hand_ptr->thread_id, NULL))
        {
          syslog(LOG_DEBUG, "Failure in pthread_join\n");
          printf("Failure in pthread_join\n");
        }

        SLIST_REMOVE(&head, conn_hand_ptr, connectionHandler_t, connectionHandler_next);   // Deletion of the node
        free(conn_hand_ptr);
      }
    }
  }

  // cleanup 
  fclose(received_data_file_fd);
  timer_delete(timerId);

  return NO_ERROR;
}


void signal_handler(int signal_number)
{
  printf("Signal Received: %d \n", signal_number);
 // i. Gracefully exits when SIGINT or SIGTERM is received, completing any open connection operations, closing any open sockets, and deleting the file /var/tmp/aesdsocketdata.
  if((SIGINT==signal_number) || (SIGTERM==signal_number))
  {
    printf("caught signal exiting\n");
    // Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received.
    syslog(LOG_DEBUG, "Caught signal, exiting");  
    shutdown(server_fd, SHUT_RDWR); 
    remove(RECEIVE_DATA_FILE);
    fclose(received_data_file_fd);
    timer_delete(timerId);
    pthread_mutex_destroy(&write_sync_mutex);

    SLIST_FOREACH_SAFE(conn_hand_ptr, &head, connectionHandler_next, conn_hand_ptr_temp) 
    {    
      close(conn_hand_ptr->client_fd);
      syslog(LOG_INFO, "signal_handler: Closed connection from %s\n", conn_hand_ptr->client_address);
      printf("signal_handler: Closed connection from %s\n", conn_hand_ptr->client_address);
      // join the thread so that when it finishes it exists gracefully 
      if(NO_ERROR != pthread_join(conn_hand_ptr->thread_id, NULL))
      {
        syslog(LOG_DEBUG, "signal_handler: Failure in pthread_join\n");
        printf("signal_handler: Failure in pthread_join\n");
      }

      SLIST_REMOVE(&head, conn_hand_ptr, connectionHandler_t, connectionHandler_next);   // Deletion. 
      free(conn_hand_ptr);
    }
  }
  exit(1);
}

void* connection_handler_thread_fxn(void* thread_parameter)
{
  connectionHandler_t *thread_func_args = (connectionHandler_t *)thread_parameter;

  #if USE_AESD_CHAR_DEVICE
    int cmd_length = strlen(aesd_ioctl_cmd);
  #endif

  if(NO_ERROR == pthread_mutex_lock(thread_func_args->write_sync_mutex))
  {
    char read_buffer[RECEIVE_PACKET_SIZE];
    memset(read_buffer, 0, RECEIVE_PACKET_SIZE);
    int number_of_bytes_sent = 0;
    int number_of_bytes_read = 0;

    printf("\nconnection_handler_thread_fxn: number_of_bytes_read = recv(thread_func_args->client_fd, read_buffer, RECEIVE_PACKET_SIZE, 0)) > 0  \n");
    while((number_of_bytes_read = recv(thread_func_args->client_fd, read_buffer, RECEIVE_PACKET_SIZE, 0)) > 0)
    { 
      
#if (USE_AESD_CHAR_DEVICE == 1)
    // Check if the received data matches the AESD IOCTL command
    if (strncmp(read_buffer, aesd_ioctl_cmd, cmd_length) == 0)
    {
        struct aesd_seekto aesd_seekto_data;

        // Parse the received command to extract seek parameters
        int command_count = sscanf(read_buffer, "AESDCHAR_IOCSEEKTO:%d,%d", &aesd_seekto_data.write_cmd,
                                    &aesd_seekto_data.write_cmd_offset);
        printf("\n connection_handler_thread_fxn AESDCHAR_IOCSEEKTO: aesd_seekto_data.write_cmd : %d  aesd_seekto_data.write_cmd_offset: %d command_count: %d \n", aesd_seekto_data.write_cmd, aesd_seekto_data.write_cmd_offset, command_count);

        syslog(LOG_INFO,"\n connection_handler_thread_fxn AESDCHAR_IOCSEEKTO: aesd_seekto_data.write_cmd : %d  aesd_seekto_data.write_cmd_offset: %d command_count: %d \n", aesd_seekto_data.write_cmd, aesd_seekto_data.write_cmd_offset, command_count);

        if (command_count != 2)
        {
            syslog(LOG_ERR, "connection_handler_thread_fxn: Failed to parse IOCTL command: %s", strerror(errno));
        }
        else
        {
            // Executing the IOCTL command to seek to the specified position
            received_data_file_fd = fopen(RECEIVE_DATA_FILE, "w+"); 
            if (ioctl(fileno(received_data_file_fd), AESDCHAR_IOCSEEKTO, &aesd_seekto_data) != 0)
            {
         
                syslog(LOG_ERR, "connection_handler_thread_fxn: Failed to execute IOCTL command: %s", strerror(errno));
            }
            else
            {
              printf(" connection_handler_thread_fxn fialed ioctl ");
            }
            //fclose(received_data_file_fd);
        }

        // After handling the IOCTL command, proceeding to read data
        goto read;
    }

#endif

      printf("\nconnection_handler_thread_fxn: received_data_file_fd = fopen(RECEIVE_DATA_FILE)  \n");
      received_data_file_fd = fopen(RECEIVE_DATA_FILE, "w+"); 

      //write(received_data_file_fd, read_buffer, number_of_bytes_read); 
      fwrite(read_buffer, sizeof(char), number_of_bytes_read, received_data_file_fd);
      fclose(received_data_file_fd); // MJ closing will reset the seek 
      // Your implementation should use a newline to separate data packets received.  
      // In other words a packet is considered complete when a newline character is found in the input receive stream, 
      // and each newline should result in an append to the /var/tmp/aesdsocketdata file.
      if(strchr(read_buffer, '\n') != NULL) 
      {
        break;
      }
    }
    // f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
    received_data_file_fd = fopen(RECEIVE_DATA_FILE, "w+"); 
    lseek(fileno(received_data_file_fd), 0, SEEK_SET);

read: 
    while((number_of_bytes_sent = read(fileno(received_data_file_fd), read_buffer, RECEIVE_PACKET_SIZE)) > 0)
    {
      send(thread_func_args->client_fd, read_buffer, number_of_bytes_sent, 0); 
    }
    thread_func_args->is_thread_complete = true;
    pthread_mutex_unlock(thread_func_args->write_sync_mutex);
  }
  fclose(received_data_file_fd);
  return thread_parameter;
}

void print_time_thread_fxn(union sigval sv) 
{    
  static char format[] = "timestamp: %F %T\n";
  char timestamp[64];
  memset(timestamp,0,64);
  struct tm * local_now;
  time_t now=time(NULL);
  local_now = localtime(&now);
  strftime(timestamp, sizeof(timestamp), format, local_now);

  #ifndef USE_AESD_CHAR_DEVICE
    if(NO_ERROR==pthread_mutex_lock(&write_sync_mutex))
    {  
      write(received_data_file_fd, timestamp, strlen(timestamp));
      if(NO_ERROR!=pthread_mutex_unlock(&write_sync_mutex))
      {
        syslog(LOG_DEBUG, "print_time_thread_fxn: Failure in pthread_mutex_unlock\n");
        printf("print_time_thread_fxn: Failure in pthread_mutex_unlock\n"); 
        exit (1);
      }
    }
    else
    {
      syslog(LOG_DEBUG, "print_time_thread_fxn: Failure in pthread_mutex_lock\n");
      printf("print_time_thread_fxn: Failure in pthread_mutex_lock\n"); 
      exit (1);
    }
  #endif 
}


void setup_print_time_thread(int seconds) 
{
  // Sets the timing
  struct itimerspec timerSpec;
  struct sigevent timerEvent;

  //set the values of the intervals 
  memset(&timerEvent, 0, sizeof(timerEvent));
  timerEvent.sigev_notify = SIGEV_THREAD;
  
  timerEvent.sigev_notify_function = print_time_thread_fxn;
  
  memset(&timerSpec, 0, sizeof(timerSpec));
  timerSpec.it_interval.tv_sec  = seconds;
  timerSpec.it_interval.tv_nsec = 0;
  timerSpec.it_value.tv_sec     = seconds;
  timerSpec.it_value.tv_nsec    = 0;

  // create the timer
  if(NO_ERROR!= timer_create(CLOCK_MONOTONIC, &timerEvent, &timerId))
  {
    syslog(LOG_DEBUG, "setup_print_time_thread: Failure in timer_create\n");
    printf("setup_print_time_thread: Failure in timer_create\n"); 
  }
  
  // set the time interval for the timer
  if(NO_ERROR!= timer_settime(timerId, TIMER_ABSTIME, &timerSpec, NULL))
  {
    syslog(LOG_DEBUG, "setup_print_time_thread: Failure in timer_create\n");
    printf("setup_print_time_thread: Failure in timer_create\n"); 
  }
}