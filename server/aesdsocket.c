#include "aesdsocket.h"

// signal handler function declearation 
void signal_handler(int);

// global variables 
int client_fd;
int server_fd;
int received_data_file_fd; 

// main function 
int main(int argc, char* argv[])
{

  // registr for signal
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  // initialize required variables for the program
  int bind_return=NO_ERROR;
  int listen_return=NO_ERROR;
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
  int number_of_bytes_sent = 0;
  int number_of_bytes_read = 0;
  char read_buffer[RECEIVE_PACKET_SIZE];

 // getaddrinfo 
  if(0 != getaddrinfo(NULL, PORT, &hints, &servinfo)) 
  {
      printf("Error in function getaddrinfo\n");
      syslog(LOG_DEBUG, "Error in function getaddrinfo\n");
      return ERROR;
  }
  else
  {
  	  syslog(LOG_DEBUG, "success in function getaddrinfo\n");
      printf("success in function getaddrinfo\n");
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
  else
  {
      syslog(LOG_DEBUG, "success in function socket\n");
      printf("success in function socket\n");
   }
  
  if (ERROR == setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) 
  {
      syslog(LOG_PERROR, "error in function setsockopt\n");
      printf("error in function setsockopt\n");
      return ERROR;
  }
  else
  {
      syslog(LOG_DEBUG, "succss in function setsockopt\n");
      printf("succss in function setsockopt\n");
   } 

  bind_return = bind(server_fd, servinfo->ai_addr, servinfo->ai_addrlen);
  if(ERROR == bind_return)
  { 
      syslog(LOG_PERROR, "error in function bind\n");
      printf("error in function bind\n");
      close(server_fd);
      return ERROR;
  }
  else
  {
      syslog(LOG_DEBUG, "success in function bind\n");
      printf("success in function bind\n");
  } 
  
  // free the memory 
  freeaddrinfo(servinfo);
   
  // c. Listens for and accepts a connection 
  // listen to the address  
  listen_return = listen(server_fd, LISTEN_BACKLOG);
  if(ERROR == listen_return)
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

  // h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received
  printf("Entering While (true)\n");
  while(true)
  {  
    addr_size = sizeof(client_addr);
	// c. Listens for and accepts a connection  
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
    
    // error in client request acceptance
    if(ERROR==client_fd)
    {
        syslog(LOG_PERROR, "error in function accept\n");
        printf("error in function aceept\n");
        return ERROR;
    }
    else
    {
        syslog(LOG_DEBUG, "success in function accept\n");
        printf("success in function accept\n");
    } 
    
    // d. Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client. 
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip_address, sizeof (client_ip_address));
    syslog(LOG_DEBUG, "Accepted connection from %s", client_ip_address);
    printf("Accepted connection from %s", client_ip_address);

    // e. Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
    received_data_file_fd = open(RECEIVE_DATA_FILE, O_CREAT | O_APPEND | O_RDWR , 0644);    
    memset(read_buffer, 0, RECEIVE_PACKET_SIZE);
    while((number_of_bytes_read = recv(client_fd, read_buffer, RECEIVE_PACKET_SIZE, 0)) > 0)
    {      
      write(received_data_file_fd, read_buffer, number_of_bytes_read); 
      // Your implementation should use a newline to separate data packets received.  
      // In other words a packet is considered complete when a newline character is found in the input receive stream, 
      // and each newline should result in an append to the /var/tmp/aesdsocketdata file.
      if(strchr(read_buffer, '\n') != NULL) break;
    }

    
    // f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
    lseek(received_data_file_fd, 0, SEEK_SET);
    while((number_of_bytes_sent = read(received_data_file_fd, read_buffer, RECEIVE_PACKET_SIZE)) > 0)
    {
      send(client_fd, read_buffer, number_of_bytes_sent, 0); 
    }
    
    // close the file descriptors
    close(received_data_file_fd);
    close(client_fd);
    
    //g. Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
    syslog(LOG_DEBUG, "Closed connection from %s", client_ip_address);
    printf("Closed connection from %s", client_ip_address);
  }
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
    close(server_fd);
    close(client_fd);
    remove(RECEIVE_DATA_FILE);
    exit(1);
  }
}
