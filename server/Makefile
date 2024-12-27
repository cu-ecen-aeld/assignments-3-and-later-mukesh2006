# MUKESH JHA

#CROSS_COMPILE=aarch64-none-linux-gnu-
CROSS_COMPILE=aarch64-poky-linux-
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Werror -Wall -g -v 
LDFLAGS=-pthread -lstdc++ -lrt 

SOURCES=aesdsocket.c
TARGET=aesdsocket
OBJS=aesdsocket.o
INCLUDE_DIR="." ## todo: ideally, it should be in the include folder 


 ## a. Is compiled by the “all” and “default” target of a Makefile in the “server” directory and supports cross compilation, placing the executable file in the “server” directory and named aesdsocket.
all: $(TARGET)

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET) *.map  *.elf

#aesdsocket: aesdsocket.c aesdsocket.h aesdsocket.o
#	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o aesdsocket aesdsocket.o
#	mv aesdsocket /usr/local/bin/aesdsocket  ## move the executable to /usr/bin/ location so that it can be exectued by deamon ## Permission issue
