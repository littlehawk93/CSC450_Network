#
# Makefile for CSC 450 Project 3
# Written By: John Hawkins& Jacob Burt
# Date: 2/19/14
#

CC = gcc
D = -g

all: client server emulator

client: client.c packetlib.h packetlib.c btree.h btree.c 
	@$(CC) -pthread client.c packetlib.c btree.c -o client
	
server: server.c packetlib.h packetlib.c packetqueue.h packetqueue.c
	@$(CC) -pthread server.c packetlib.c packetqueue.c -lm -o server
	
emulator: emulator.c packetlib.h packetlib.c 
	@$(CC) -pthread emulator.c packetlib.c -o emulator
	
debug-client: client.c packetlib.c packetlib.h btree.h btree.c 
	@$(CC) $(D) -pthread client.c packetlib.c btree.c -o client
	
debug-server: server.c packetlib.c packetlib.h packetqueue.h packetqueue.c
	@$(CC) $(D) -pthread server.c packetqueue.c packetlib.c -lm -o server
	
debug-emulator: emulator.c packetlib.c packetlib.h 
	@$(CC) $(D) -pthread emulator.c packetlib.c -o emulator
	
clean:
	@rm -f client emulator server
