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

//## MACROS
#define ERROR -1
#define NO_ERROR 0
#define RECEIVE_PACKET_SIZE 1024
#define PORT "9000"
#define RECEIVE_DATA_FILE "/var/tmp/aesdsocketdata"
#define LISTEN_BACKLOG 30