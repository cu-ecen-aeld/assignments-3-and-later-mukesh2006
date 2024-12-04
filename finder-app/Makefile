# ============================================================================
# Makefile for assignment-2-mukesh2006
# ============================================================================
# Author:      Mukesh Jha
# Version:     1
# Description: assignment submission 2
#
# Usage:
#   make            - Build the project (default target)
#   make clean      - Remove all generated files
#   make writer     - Create executable writer from writer.c
#
# Variables:
#   CC              - Compiler used (default: gcc)
#   CFLAGS          - Compiler flags (default: -Wall -Wextra)
#   TARGET          - writer
#   SRCS            - writer.c
#   OBJS            - writer.o
# ============================================================================


CC = $(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -Wextra -O2

all: writer

writer: writer.o
	$(CC) $(CFLAGS) writer.o -o writer

writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c -o writer.o

clean:
	rm -rf *.o writer
