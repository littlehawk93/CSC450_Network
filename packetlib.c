/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#include "packetlib.h"

/*
 * Get the current time in milliseconds
 * 
 * Returns:
 * 		- unsigned long : the time in milliseconds
 */
unsigned long getMillis()
{
	struct timeval time;
	if(gettimeofday(&time, NULL)) {
		perror("Failed to get time");
		exit(errno);
	}
	unsigned long millis = time.tv_sec * 1000;
	millis += (time.tv_usec / 1000);
	return millis;
}

/*
 * Creates a Raw Socket for sending / receiving raw bits
 * 
 * Returns:
 * 		- int : the file descriptor of the newly created socket
 */
int openRawSocket()
{
	int one = 1;
	int *val = &one;
	int sock;
	if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
		perror("Unable to create initial socket");
		exit(errno);
	}
	if(setsockopt(sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
		perror("Unable to execute set sock option");
		exit(errno);
	}
	return sock;
}

/*
 * Converts a FTP packet into a binary character stream to be written on a raw socket
 * 
 * Parameters:
 * 		- packet : A pointer to a valid FTP Packet Struct
 * 
 * Returns:
 * 		- char* : A binary character array of the FTP Packet
 */
char* ftpToString(ftp_packet *packet)
{
	char* str = (char*) malloc(packet->payload_size + 9);
	str[0] = packet->packet_type;
	int i;
	for(i=1;i<5;i++) {
		str[i] = (char) ((packet->sequence_num >> (4 - i)  * 8) & 0xFF);
	}
	for(i=5;i<9;i++) {
		str[i] = (char) ((packet->payload_size >> (8 - i)  * 8) & 0xFF);
	}
	for(i=0;i<packet->payload_size;i++)
	{
		str[i + 9] = packet->payload[i];
	}
	return str;
}

/*
 * Converts a NPP packet into a binary character stream to be written on a raw socket
 * 
 * Parameters:
 * 		- packet : A pointer to a valid NPP Packet Struct
 * 
 * Returns:
 * 		- char* : A binary character array of the NPP Packet
 */
char* nppToString(npp_packet *packet)
{
	char* str = (char*) malloc(packet->payload_size);
	str[0] = packet->priority;
	int i;
	for(i=1;i<5;i++) {
		str[i] = (packet->source_ip >> (8 * (4 - i))) & 0xFF;
	}
	str[5] = (packet->source_port >> 8) & 0xFF;
	str[6] = packet->source_port & 0xFF;
	for(i=7;i<11;i++) {
		str[i] = (packet->dest_ip >> (8 * (10 - i))) & 0xFF;
	}
	str[11] = (packet->dest_port >> 8) & 0xFF;
	str[12] = (packet->dest_port & 0xFF);
	for(i=13;i<17;i++) {
		str[i] = (packet->payload_size >> (8 * (16 - i))) & 0xFF;
	}
	for(i=0;i<packet->payload_size - 17;i++) {
		str[17 + i] = packet->payload[i];
	}
	return str;
}

/*
 * Converts a raw character array into a NPP Packet Struct
 * 
 * Parameters:
 * 		- char* : A binary character array of the NPP Packet
 * 
 * Returns:
 * 		- npp_packet* : A pointer to a valid NPP Packet Struct
 */
npp_packet* stringToNpp(char *str)
{
	npp_packet *packet = (npp_packet*) malloc(sizeof(npp_packet));
	packet->priority = str[0];
	int i;
	packet->source_ip = 0;
	for(i=1;i<5;i++) {
		packet->source_ip <<= 8;
		packet->source_ip |= str[i] & 0xFF;
	}
	packet->source_port = str[5] & 0xFF;
	packet->source_port <<= 8;
	packet->source_port |= str[6] & 0xFF;
	packet->dest_ip = 0;
	for(i=7;i<11;i++) {
		packet->dest_ip <<= 8;
		packet->dest_ip |= str[i] & 0xFF;
	}
	packet->dest_port = str[11] & 0xFF;
	packet->dest_port <<= 8;
	packet->dest_port |= str[12] & 0xFF;
	packet->payload_size = 0;
	for(i=13;i<17;i++) {
		packet->payload_size <<= 8;
		packet->payload_size |= (str[i] & 0xFF);
	}
	packet->payload = (char*) malloc(packet->payload_size - 17);
	for(i=0;i<packet->payload_size - 17;i++) {
		packet->payload[i] = str[i+17];
	}
	return packet;
}

/*
 * Converts a raw character array into a FTP Packet Struct
 * 
 * Parameters:
 * 		- char* : A binary character array of the FTP Packet
 * 
 * Returns:
 * 		- ftp_packet* : A pointer to a valid FTP Packet Struct
 */
ftp_packet* stringToFtp(char *str)
{
	ftp_packet *packet = (ftp_packet*) malloc(sizeof(ftp_packet));
	packet->packet_type = str[0];
	packet->sequence_num = 0;
	int i;
	for(i=1;i<5;i++) {
		packet->sequence_num <<= 8;
		packet->sequence_num |= str[i] & 0xFF;
	}
	packet->payload_size = 0;
	for(i=5;i<9;i++) {
		packet->payload_size <<= 8;
		packet->payload_size |= str[i] & 0xFF;
	}
	packet->payload = (char*) malloc(packet->payload_size);
	for(i=0;i<packet->payload_size;i++) {
		packet->payload[i] = str[i+9];
	}
	return packet;
}

/*
 * Prints the header information and hex values of the entire IP Packet
 * 
 * Parameters:
 * 		- packet : A pointer to a valid IP Packet Struct
 */
void printIPPacket(ip_packet *packet)
{
	printf("IP Version: %d\n", (packet->ip_vhl >> 4) & 0x0F);
	printf("IP Header Len: %d\n", packet->ip_vhl & 0x0F);
	printf("IP Length: %d\n", packet->ip_len);
	printf("IP ID: %d\n", packet->ip_id);
	printf("IP Flags: %x\n", packet->ip_flags);
	printf("IP Fragment Offset: %d\n", packet->ip_off);
	printf("IP TTL: %d\n", packet->ip_ttl);
	printf("IP Protocol: %d\n", packet->ip_p);
	printf("IP Checksum: %x\n", packet->ip_sum);
	printf("IP Source IP: %x\n", packet->ip_source);
	printf("IP Dest IP: %x\n\n", packet->ip_dest);
	printf("\n\n");
}

/*
 * Prints the header information and hex values of the entire UDP Packet
 * 
 * Parameters:
 * 		- packet : A pointer to a valid UDP Packet Struct
 */
void printUDPPacket(udp_packet *packet)
{
	printf("UDP Source Port: %d\n", packet->source_port);
	printf("UDP Destination Port: %d\n", packet->dest_port);
	printf("UDP Length: %d\n", packet->size);
	printf("UDP Checksum: %d\n\n", packet->checksum);
	printf("\n\n");
}

/*
 * Prints the header information and hex values of the entire NPP Packet
 * 
 * Parameters:
 * 		- packet : A pointer to a valid NPP Packet Struct
 */
void printNPPPacket(npp_packet *packet)
{
	printf("NPP Priority: %d\n", packet->priority);
	printf("NPP Source IP: %x\n", packet->source_ip);
	printf("NPP Source Port: %d\n", packet->source_port);
	printf("NPP Destination IP: %x\n", packet->dest_ip);
	printf("NPP Destination Port: %d\n", packet->dest_port);
	printf("NPP Packet Size: %d\n\n", packet->payload_size);
	
	char *str = nppToString(packet);
	int i;
	for(i=0;i<packet->payload_size;i++) {
		if(i % 10 == 0) {
			printf("\n");
		}
		printf("%02x ", str[i] & 0xFF);
	}
	free(str);
	printf("\n\n");
}

/*
 * Prints the header information and hex values of the entire FTP Packet
 * 
 * Parameters:
 * 		- packet : A pointer to a valid FTP Packet Struct
 */
void printFTPPacket(ftp_packet *packet)
{
	printf("FTP Packet Type: %c\n", packet->packet_type);
	printf("FTP Sequence Number: %d\n", packet->sequence_num);
	printf("FTP Payload Size: %d\n\n", packet->payload_size);
	
	char *str = ftpToString(packet);
	int i;
	for(i=0;i<packet->payload_size + 9;i++) {
		if(i % 10 == 0) {
			printf("\n");
		}
		printf("%02x ", str[i] & 0xFF);
	}
	printf("\n\n");
	free(str);
}

/*
 * Wraps a FTP Packet into the payload of a NPP Packet
 * 
 * Parameters:
 * 		- npp : A pointer to a valid NPP Packet Struct
 * 		- ftp : A pointer to a valid FTP Packet Struct
 */
void wrapFTPPacket(npp_packet* npp, ftp_packet* ftp)
{
	char* payload = ftpToString(ftp);
	npp->payload_size = ftp->payload_size + 26;
	npp->payload = payload;
}

/*
 * Extracts a FTP Packet from the payload of a NPP Packet
 * 
 * Parameters:
 * 		- npp : A pointer to a valid NPP Packet Struct
 * 
 * Returns:
 * 		- ftp_packet* : A pointer to a valid FTP Packet Struct
 */
ftp_packet* unwrapFTPPacket(npp_packet *npp)
{
	return stringToFtp(npp->payload);
}

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
unsigned short checkSum(unsigned short *buf, int nwords)
{       
		unsigned long sum;
		for(sum=0; nwords>0; nwords--)
				sum += *buf++;
		sum = (sum >> 16) + (sum &0xffff);
		sum += (sum >> 16);
		return (unsigned short)(~sum);
}

/*
 * Get the IPV4 Address of a host from its hostname
 * 
 * Parameters:
 * 		- hostname : The name of the host whose IP address you want
 * 
 * Returns:
 * 		- unsigned int : An integer (4 bytes) representing the IPV4 address in network byte order 
 */
char* getIPV4AddressFromHost(char* hostname)
{
	struct hostent *host;
	struct in_addr h_addr;
	if((host = gethostbyname(hostname)) == 0) {
		printf("Unable to resolve the host");
		exit(-1);
	}
	h_addr.s_addr = *((unsigned long*) host->h_addr_list[0]);
	return inet_ntoa(h_addr);
}

/*
 * Get the IPV4 Address of the local machine from its network interface 
 * 
 * Returns:
 * 		- unsigned int : The inet address of the local machine from its network interface
 * 						 Or the loopback interface (127.0.0.1) if no other is found
 */ 
char* getLocalHostIPV4Address()
{
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	char *addr;
	if(getifaddrs(&ifap)) {
		perror("Failed to load inet addresses");
		exit(errno);
	}
	
	for(ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if(ifa->ifa_addr->sa_family == AF_INET) {
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			addr = inet_ntoa(sa->sin_addr);
			if(strcmp("127.0.0.1", addr)) {
				return addr;
			}
		}
	}
	return "127.0.0.1";
}

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
int sendNPPPacket(int sock, struct sockaddr *addr,npp_packet *packet)
{
	int total_len = packet->payload_size + 28;
	char buffer[total_len];
	buffer[0] = 0x45;
	buffer[1] = 16;
	buffer[2] = ((total_len >> 8) & 0xFF);
	buffer[3] = (total_len & 0xFF);
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 0;
	buffer[7] = 0;
	buffer[8] = 0xFF;
	buffer[9] = 17;
	buffer[10] = 0;
	buffer[11] = 0;
	buffer[12] = (packet->source_ip & 0xFF);
	buffer[13] = ((packet->source_ip >> 8) & 0xFF);
	buffer[14] = ((packet->source_ip >> 16) & 0xFF);
	buffer[15] = ((packet->source_ip >> 24) & 0xFF);
	unsigned int dest = (unsigned int) ((struct sockaddr_in*) addr)->sin_addr.s_addr;
	buffer[16] = (dest & 0xFF);
	buffer[17] = ((dest >> 8) & 0xFF);
	buffer[18] = ((dest >> 26) & 0xFF);
	buffer[19] = ((dest >> 24) & 0xFF);
	unsigned short src_p = packet->source_port;
	buffer[20] = ((src_p >> 8) & 0xFF);
	buffer[21] = (src_p & 0xFF);
	unsigned short dest_p = ((struct sockaddr_in*) addr)->sin_port;
	buffer[22] = (dest_p & 0xFF);
	buffer[23] = ((dest_p >> 8) & 0xFF);
	unsigned short udp_sz = packet->payload_size + 8;
	buffer[24] = ((udp_sz >> 8) & 0xFF);
	buffer[25] = (udp_sz & 0xFF);
	buffer[26] = 0;
	buffer[27] = 5;
	unsigned short ip_csum = checkSum((unsigned short *) buffer, 9);
	buffer[10] = ((ip_csum >> 8) & 0xFF);
	buffer[11] = (ip_csum & 0xFF);
	
	char * data = nppToString(packet);
	int i;
	for(i=0;i<packet->payload_size;i++) {
		buffer[28 + i] = data[i];
	}
	int temp = sendto(sock, buffer, total_len, 0, addr, sizeof(struct sockaddr));
	if(temp < 0) {
		perror("Unable to send on Raw socket");
		exit(errno);
	}
	return temp;
}

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
npp_packet* receiveNPPPacket(int sock, struct sockaddr *addr)
{
	char buffer[65535];
	int temp = recvfrom(sock, buffer, 65535, 0, NULL, NULL);
	if(temp < 0) {
		return NULL;
	}
	unsigned short self_port = htons(((struct sockaddr_in*) addr)->sin_port);
	unsigned short incoming_port = (((unsigned short) buffer[22] & 0xFF) << 8) | (buffer[23] & 0xFF);
	
	if(incoming_port != self_port) {
		return NULL;
	}
	
	npp_packet *npp = stringToNpp(&buffer[28]);
	return npp;
}

/*
 * Place holder function for testing (doesn't write raw socket)
 */ 
int sendNPPPacket_Old(int sock, struct sockaddr *addr, npp_packet *packet)
{
	char *str = nppToString(packet);
	char *s = &str[0];
	int bytes_sent = 0, total_bytes = packet->payload_size, temp;
	while(bytes_sent < total_bytes) {
		temp = sendto(sock, str, total_bytes - bytes_sent, 0, addr, sizeof(struct sockaddr));
		if(temp < 0) {
			perror("Unable to send on UDP socket");
			exit(errno);
		}
		bytes_sent += temp;
		s = &str[bytes_sent];
	}
	return bytes_sent;
}

/*
 * Place holder function for testing (doesn't read raw socket)
 */
npp_packet* receiveNPPPacket_Old(int sock, struct sockaddr *addr)
{
	int addrlen = sizeof(struct sockaddr);
	char header[17];
	int total = 17, temp;
	temp = recvfrom(sock, header, total, MSG_PEEK, addr, &addrlen);
	if(temp < 0) {
		perror("Unable to read from UDP socket");
		exit(errno);
	}
	unsigned int len = ((header[13] & 0xFF) << 24) | ((header[14] & 0xFF) << 16) | ((header[15] & 0xFF) << 8) | (header[16] & 0xFF);
	total = len + 17;
	char *buf = (char*) malloc(total);
	temp = recvfrom(sock, buf, total, 0, addr, &addrlen);
	if(temp < 0) {
		perror("Unable to read from UDP socket");
		exit(errno);
	}
	npp_packet *npp = stringToNpp(buf);
	return npp;
}

/*
 * Place holder function for testing (doesn't open raw socket)
 */ 
int openUDPSocket()
{
	int sock;
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Unable to create UDP socket");
		exit(errno);
	}
	return sock;
}
