/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#include "packetlib.h"
#include "btree.h"

/*
 * Encapsulates all of the arguments for the client program
 */
typedef struct {
	
	unsigned short window_size, file_option;
	struct sockaddr_in sin, ein, din;
	
} client_args;

// GLOBAL VARIABLES

client_args args;
struct ordered_btree btree;
int sock, response_received;

// FUNCTION PROTOTYPES

int checkArgs(int argc, char **argv, client_args *args);

void sendAck(unsigned int  seq_num);

void *tMain(void *ptr);

// END PROTOTYPES

int main(int argc, char **argv)
{
	srand(time(NULL));
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	initBtree(&btree);
	if(!checkArgs(argc, argv, &args)) {
		fprintf(stderr,"Invalid Arguments\n");
		exit(1);
	}
	
	response_received = 0;
	
	npp_packet *send_npp = (npp_packet*) malloc(sizeof(npp_packet));
	send_npp->dest_ip = (unsigned int) args.din.sin_addr.s_addr;
	send_npp->dest_port = (unsigned short) args.din.sin_port;
	send_npp->priority = (rand() % 3) + 1; // Give a random priority from 1 - 3
	send_npp->source_ip = (unsigned int) inet_addr(getLocalHostIPV4Address());
	send_npp->source_port = (unsigned short) args.sin.sin_port;
	
	ftp_packet ftp;
	ftp.packet_type = 'C';
	ftp.sequence_num = args.file_option;
	ftp.payload_size = 4;
	ftp.payload = (char*) malloc(4);
	
	int i;
	for(i=0;i<4;i++) {
		ftp.payload[i] = (args.window_size >> (8 * (3 - i))) & 0xFF;
	}
	
	wrapFTPPacket(send_npp, &ftp);
	
	//sock = openUDPSocket(); Testing only
	sock = openRawSocket();
	
	if(bind(sock, (struct sockaddr*) &(args.sin), sizeof(args.sin))) {
		perror("Unable to bind socket");
		exit(errno);
	}
	
	pthread_t requestThread;
	
	if(pthread_create(&requestThread, NULL, tMain, (void*) send_npp)) {
		perror("Unable to spawn new thread");
		exit(errno);
	}
	
	FILE * output;
	
	char fname[16];
	sprintf(fname, "%d-out.txt",args.file_option);
	
	if((output = fopen(fname, "w")) < 0) {
		perror("Could not open file");
		exit(errno);
	}
	
	npp_packet *npp;
	
	unsigned int smallest_ack = 0;
	
	while(1) {
		
		npp = receiveNPPPacket(sock, (struct sockaddr*) &(args.sin));
		if(npp == NULL) {
			continue;
		}
		response_received = 1;
		ftp_packet *ftp = unwrapFTPPacket(npp);
		
		printf("Received packet [%d] type %c\n", ftp->sequence_num, ftp->packet_type);
		
		if(ftp->packet_type == 'E') {
			printf("End packet received\n");
			break;
		}else if(ftp->packet_type  == 'S') {
			
			sendAck(ftp->sequence_num);
			if(ftp->sequence_num >= smallest_ack) {
				if(!contains(&btree, ftp->sequence_num)) {
					add(&btree, ftp->sequence_num, ftp->payload, ftp->payload_size);
				}
				
				if(btree.size >= args.window_size) {
					smallest_ack = getMaxSequenceNumber(&btree);
					writeTree(&btree, output);
					clear(&btree);
				}
			}
		}
	}
	
	writeTree(&btree, output);
	fclose(output);
	return 0;
}

/*
 * Sends an ACK to the server for the provided packet
 * 
 * Parameters:
 * 		- seq_num : The sequence number to ACK
 */ 
void sendAck(unsigned int seq_num)
{
	npp_packet npp;
	npp.dest_ip = (unsigned int) args.din.sin_addr.s_addr;
	npp.dest_port = (unsigned short) args.din.sin_port;
	npp.priority = 3;
	npp.source_ip = (unsigned int) inet_addr(getLocalHostIPV4Address());
	npp.source_port = (unsigned short) args.sin.sin_port;
	
	ftp_packet ftp;
	ftp.packet_type = 'A';
	ftp.payload = (char*) malloc(1);
	ftp.payload_size = 0;
	ftp.sequence_num = seq_num;
	
	wrapFTPPacket(&npp, &ftp);
	
	sendNPPPacket(sock, (struct sockaddr*) &(args.ein), &npp);
	printf("Sent ACK for packet [%d]\n", seq_num);
}

/*
 * Main thread for checking for a server response. If the server doesn't respond within the
 * timeout, the client resends the intial request packet
 */
void *tMain(void *ptr)
{
	npp_packet * send_npp = (npp_packet*) ptr;
	while(1) {
		if(response_received == 0) {
			printf("Sending initial request packet\n");
			sendNPPPacket(sock, (struct sockaddr*) &(args.ein), send_npp);
		}else {
			break;
		}
		usleep(12000000); // Sleep for 30 seconds and try another request
	}
	free(send_npp);
}

/*
 * Checks to make sure all valid arguments are provided and fills the included args struct
 *
 * Parameters:
 * 		- argc : Number of arguments
 * 		- argv : String array of arguments
 * 
 * Returns:
 * 		- int : 1 if all arguments valid and provided, 0 otherwise
 */
int checkArgs(int argc, char **argv, client_args *args)
{
	unsigned char has_window_size = 0, has_server_port = 0, has_emu_port = 0, 
				  has_file_option = 0, has_server_hostname = 0, has_emu_hostname = 0;
				  
	memset(&(args->sin.sin_addr), 0, sizeof(args->sin.sin_addr));
	memset(&(args->ein.sin_addr), 0, sizeof(args->ein.sin_addr));
	memset(&(args->din.sin_addr), 0, sizeof(args->din.sin_addr));
	args->sin.sin_family = AF_INET;
	args->ein.sin_family = AF_INET;
	args->din.sin_family = AF_INET;
	
	args->sin.sin_addr.s_addr = htonl(INADDR_ANY);
	args->sin.sin_port = 9991;
				  
	if(argc < 13) {
		return 0;
	}
	int i;
	for(i=1;i<argc;i++) {
		if(strcmp("-s", argv[i]) == 0 && i + 1 < argc) {
			args->din.sin_addr.s_addr = inet_addr(getIPV4AddressFromHost(argv[i+1]));
			has_server_hostname = 1;
		}else if(strcmp("-g", argv[i]) == 0 && i + 1 < argc) {
			args->din.sin_port = htons(atoi(argv[i+1]));
			has_server_port = 1;
		}else if(strcmp("-e", argv[i]) == 0 && i + 1 < argc) {
			args->ein.sin_addr.s_addr = inet_addr(getIPV4AddressFromHost(argv[i+1]));
			has_emu_hostname = 1;
		}else if(strcmp("-h", argv[i]) == 0 && i + 1 < argc) {
			args->ein.sin_port = htons(atoi(argv[i+1]));
			has_emu_port = 1;
		}else if(strcmp("-o", argv[i]) == 0 && i + 1 < argc) {
			args->file_option = (unsigned short) atoi(argv[i+1]);
			has_file_option = 1;
		}else if(strcmp("-w", argv[i]) == 0 && i + 1 < argc) {
			args->window_size = (unsigned short) atoi(argv[i+1]);
			has_window_size = 1;
		}
	}
	return has_file_option * has_server_port * has_emu_port * has_emu_hostname * has_server_hostname * has_window_size;
}
