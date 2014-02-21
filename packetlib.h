/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#ifndef __PACKETLIB
#define __PACKETLIB

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

/*
 * Structure for defining an IP Packet header
 */
typedef struct {
	
	unsigned char      ip_vhl;
	unsigned char      ip_tos;
	unsigned short int ip_len;
	unsigned short int ip_id;
	unsigned char      ip_flags;
	unsigned char	   ip_off;
	unsigned char      ip_ttl;
	unsigned char      ip_p;
	unsigned short int ip_sum;
	unsigned int       ip_source;
	unsigned int       ip_dest;
	
} ip_packet;

/*
 * Structure for defining a UDP Packet
 */
typedef struct {
	
	unsigned short int source_port;
	unsigned short int dest_port;
	unsigned short int size;
	unsigned short int checksum;
	
} udp_packet;

/*
 * Structure for defining a NPP (New Packet Payload) Packet
 */
typedef struct {
	
	unsigned char priority;
	unsigned int source_ip, dest_ip, payload_size;
	unsigned short source_port, dest_port;
	char * payload;
	
} npp_packet;

/*
 * Structure for defining a FTP (Assignment 2 Protocol) Packet
 */
typedef struct {
	
	char * payload;
	unsigned int payload_size;
	char packet_type;
	unsigned int sequence_num;
	
} ftp_packet;

/*
 * Get the current time in milliseconds
 * 
 * Returns:
 * 		- unsigned long : the time in milliseconds
 */
unsigned long getMillis();

/*
 * Creates a Raw Socket for sending / receiving raw bits
 * 
 * Returns:
 * 		- int : the file descriptor of the newly created socket
 */
int openRawSocket();

/*
 * Converts a FTP packet into a binary character stream to be written on a raw socket
 * 
 * Parameters:
 * 		- packet : A pointer to a valid FTP Packet Struct
 * 
 * Returns:
 * 		- char* : A binary character array of the FTP Packet
 */
char* ftpToString(ftp_packet *packet);

/*
 * Converts a NPP packet into a binary character stream to be written on a raw socket
 * 
 * Parameters:
 * 		- packet : A pointer to a valid NPP Packet Struct
 * 
 * Returns:
 * 		- char* : A binary character array of the NPP Packet
 */
char* nppToString(npp_packet *packet);

/*
 * Converts a raw character array into a NPP Packet Struct
 * 
 * Parameters:
 * 		- char* : A binary character array of the NPP Packet
 * 
 * Returns:
 * 		- npp_packet* : A pointer to a valid NPP Packet Struct
 */
npp_packet* stringToNpp(char *str);

/*
 * Converts a raw character array into a FTP Packet Struct
 * 
 * Parameters:
 * 		- char* : A binary character array of the FTP Packet
 * 
 * Returns:
 * 		- ftp_packet* : A pointer to a valid FTP Packet Struct
 */
ftp_packet* stringToFtp(char *str);

/*
 * Prints the header information and hex values of the entire IP Packet
 * 
 * Parameters:
 * 		- packet : A pointer to a valid IP Packet Struct
 */
void printIPPacket(ip_packet* packet);

/*
 * Prints the header information and hex values of the entire UDP Packet
 * 
 * Parameters:
 * 		- packet : A pointer to a valid UDP Packet Struct
 */
void printUDPPacket(udp_packet* packet);

/*
 * Prints the header information and hex values of the entire NPP Packet
 * 
 * Parameters:
 * 		- packet : A pointer to a valid NPP Packet Struct
 */
void printNPPPacket(npp_packet* packet);

/*
 * Prints the header information and hex values of the entire FTP Packet
 * 
 * Parameters:
 * 		- packet : A pointer to a valid FTP Packet Struct
 */
void printFTPPacket(ftp_packet* packet);

/*
 * Wraps a FTP Packet into the payload of a NPP Packet
 * 
 * Parameters:
 * 		- npp : A pointer to a valid NPP Packet Struct
 * 		- ftp : A pointer to a valid FTP Packet Struct
 */
void wrapFTPPacket(npp_packet* npp, ftp_packet* ftp);

/*
 * Extracts a FTP Packet from the payload of a NPP Packet
 * 
 * Parameters:
 * 		- npp : A pointer to a valid NPP Packet Struct
 * 
 * Returns:
 * 		- ftp_packet* : A pointer to a valid FTP Packet Struct
 */
ftp_packet* unwrapFTPPacket(npp_packet* npp);

/*
 * Get the check sum value for an entire packet payload
 * 
 * Parameters:
 * 		- buf : An array of words to checksum
 * 		- nwords : The numbers of words to check
 * 
 * Returns:
 * 		- unsigned short : A short (2 byte) check sum of the whole payload
 */
unsigned short checkSum(unsigned short *buf, int nwords);

/*
 * Get the IPV4 Address of a host from its hostname
 * 
 * Parameters:
 * 		- hostname : The name of the host whose IP address you want
 * 
 * Returns:
 * 		- char* : An character string representing the IPV4 address
 */
char* getIPV4AddressFromHost(char* hostname);

/*
 * Get the IPV4 Address of the local machine from its network interface 
 * 
 * Returns:
 * 		- char* : The inet address of the local machine from its network interface
 * 				  Or the loopback interface (127.0.0.1) if no other is found
 */ 
char* getLocalHostIPV4Address();

/*
 * Get the name of the local machine's network interface
 * 
 * Returns:
 * 		- char* : The name of the interface of the local machine 
 * 				  Or the loopback interface (lo) if no other is found
 */ 
char* getLocalHostInterface();

/*
 * Sends an NPP packet across the network by wrapping it in UDP and IP
 * and writing raw binary to a socket
 * 
 * Parameters:
 * 		- packet : A pointer to a valid NPP Packet Struct
 * 		- sock : The socket to write to
 * 		- addr : The socket address struct to use in sending
 * 
 * Returns:
 * 		- int : The total number of bytes written to the socket
 */
int sendNPPPacket(int sock, struct sockaddr *addr,npp_packet *packet);

/*
 * Receives a NPP packet from unwrapping a raw IP / UDP packet
 * 
 * Parameters:
 * 		- sock : The socket to read from
 * 		- addr : The socket address struct to use in sending
 * 
 * Returns:
 * 		- npp_packet : A pointer to the received npp packet
 */
npp_packet* receiveNPPPacket(int sock, struct sockaddr *addr);

/*
 * Place holder function for testing (doesn't write raw socket)
 */ 
int sendNPPPacket_Old(int sock, struct sockaddr *addr, npp_packet *packet);

/*
 * Place holder function for testing (doesn't read raw socket)
 */ 
npp_packet* receiveNPPPacket_Old(int sock, struct sockaddr *addr);

/*
 * Place holder function for testing (doesn't open raw socket)
 */ 
int openUDPSocket();

#endif
