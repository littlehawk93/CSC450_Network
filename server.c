/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#include "packetlib.h"
#include "packetqueue.h"

#define NUM_SERVER_THREADS 50

/*
 * Encapsulates all of the arguments for the server program
 */
typedef struct {
	
	unsigned short rate, sequence_num, timeout, packet_length;
	
	struct sockaddr_in sin, ein;
	
} server_args;

/*
 * Encapsulates information on a single SWS frame index
 */
typedef struct {
	
	npp_packet *npp;
	int acked;
	unsigned long timeout;
	
} sws;

/*
 * Encapslates all information about a single SWS server thread
 */
typedef struct {
	
	unsigned int ip_ref;
	unsigned short port_ref;
	int sws_start, sws_end, sws_client, sws_total, pthread_index;
	sws *sws_frame;
	struct queue in_queue;
	pthread_mutex_t *in_queue_mutex;
	pthread_t *thread;
	
} thread_obj;

// GLOBAL VARIABLES

server_args args;

thread_obj *server_threads[NUM_SERVER_THREADS];

int sock;

pthread_mutex_t *thread_array_mutex;

// FUNCTION PROTOTYPES

int checkArgs(int argc, char **argv, server_args *args);

int findAppropriateServerThread(npp_packet *npp);

int createNewServerThread(npp_packet *npp);

void disposeThread(int index);

void *tMain(void *ptr);

int checkSWSWindowAcks(int index);

void deliverPacket(npp_packet *npp);

int sendPackets(sws *frame, int start, int end);

void printThreadObjectStatus(thread_obj *obj);

// END PROTOTYPES

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	int i;
	for(i=0;i<NUM_SERVER_THREADS;i++) {
		server_threads[i] = NULL;
	}
	if(!checkArgs(argc, argv, &args)) {
		fprintf(stderr,"Invalid Arguments\n");
		exit(1);
	}
	
	thread_array_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(thread_array_mutex, NULL);
	
	//sock = openUDPSocket(); Testing only
	sock = openRawSocket();
	
	if(bind(sock, (struct sockaddr*) &(args.sin), sizeof(args.sin))) {
		perror("Failed to bind socket");
		exit(errno);
	}
	
	while(1) {
		
		npp_packet *npp = receiveNPPPacket(sock, (struct sockaddr*) &args.sin);
		if(npp != NULL) {
			
			int index = findAppropriateServerThread(npp);
			
			if(index < 0) {
				int num = createNewServerThread(npp);
				if(num == 0) {
					fprintf(stderr, "Failed to create new server thread\n");
				}
			}
		}
	}
	
	return 0;
}

/*
 * Main thread for a single SWS server thread. Allows the SEND-ACK process to occur asynchronously
 * from all other client requests
 */
void *tMain(void *ptr)
{
	int index = *((int*) ptr);
	int set_end_packet = 0;
	thread_obj *t = server_threads[index];
	
	npp_packet *end_packet = (npp_packet*) malloc(sizeof(npp_packet));

	int numt = 0; // Transmission count
	int numrt = 0; // Re-transmission count

	while(t->sws_total >= t->sws_end) {
		
		while(pthread_mutex_lock(t->in_queue_mutex));
		
		npp_packet *npp = (npp_packet*) dequeue(&t->in_queue);
		
		while(pthread_mutex_unlock(t->in_queue_mutex));
		
		if(npp != NULL) {
			
			if(!set_end_packet) {
				end_packet->source_ip = npp->dest_ip;
				end_packet->source_port = npp->dest_port;
				end_packet->priority = npp->priority;
				set_end_packet = 1;
			}
			
			unsigned long time = getMillis();
			ftp_packet *mftp = unwrapFTPPacket(npp);
			
			if(mftp->packet_type == 'A') {
				
				printf("Receiving ACK for packet [%d]\n", mftp->sequence_num);
				
				int i;
				for(i=t->sws_start;i<t->sws_end;i++) {
					ftp_packet *ftp = unwrapFTPPacket(t->sws_frame[i].npp);
					if(ftp->sequence_num == mftp->sequence_num) {
						t->sws_frame[i].acked = 1;
					}
				}
				
				int remainder = checkSWSWindowAcks(t->pthread_index);
				
				if(!remainder) {
					numt += t->sws_end - t->sws_start;
					t->sws_start = t->sws_end;
					if(t->sws_end == t->sws_total) {
						t->sws_end++;
					}else {
						t->sws_end = (t->sws_end + t->sws_client > t->sws_total) ? t->sws_total : t->sws_end + t->sws_client;
					}
				}
			}
			free(mftp);
			free(npp);
		}
		
		if(t->sws_end <= t->sws_total) {
			numrt += sendPackets(t->sws_frame, t->sws_start, t->sws_end);
		}
	}
	
	printf("Sending END packet\n");
	
	ftp_packet end_ftp;
	end_ftp.packet_type = 'E';
	end_ftp.payload = (char*) malloc(1);
	end_ftp.payload_size = 0;
	end_ftp.sequence_num = 0;
	
	end_packet->dest_ip = t->ip_ref;
	end_packet->dest_port = t->port_ref;
	
	wrapFTPPacket(end_packet, &end_ftp);
	
	// Send 3 packets to help ensure delivery of at least one
	deliverPacket(end_packet);
	deliverPacket(end_packet);
	deliverPacket(end_packet);
	deliverPacket(end_packet);
	deliverPacket(end_packet);
	
	printf("Disposing of finished server thread\n");
	printf("Server Thread Stats:\n");
	printf("\tNumber of Transmissions: %d\n", numt);
	printf("\tNumber of Re-Transmissions: %d\n", numrt - numt);
	double loss = (double) (numrt - numt) / (double) numrt; 
	printf("\tPercentage Packet Loss: %d%%\n", (int) ceil(loss * 100));
	
	disposeThread(t->pthread_index);
}

/*
 * Checks the current SWS frame for a server thread to see if all packets have been ACK'ed
 * 
 * Parameters:
 * 		- index : the index in the server thread array of the server thread to check
 * 
 * Returns:
 * 		- int : Number of packets that haven't been ACK'ed yet
 */
int checkSWSWindowAcks(int index)
{
	int sum = 0, i;
	for(i=server_threads[index]->sws_start;i<server_threads[index]->sws_end;i++) {
		if(!server_threads[index]->sws_frame[i].acked) {
			sum += 1;
		}
	}
	return sum;
}

/*
 * Sends all packets in a SWS frame that have timed out and are not already ACK'ed
 * 
 * Parameters:
 * 		- frame : the SWS frame
 * 		- start : the starting index of the frame
 * 		- end : the ending index of the frame
 * 
 * Returns:
 * 		- int : the number of packets sent
 */
int sendPackets(sws *frame, int start, int end)
{
	int i;
	int sum = 0;
	unsigned long time = getMillis();
	for(i=start;i<end;i++) {
		if(frame[i].acked == 0 && time > frame[i].timeout) {
			deliverPacket(frame[i].npp);
			frame[i].timeout = time + args.timeout;
			sum++;
			usleep(1000000 / args.rate);
		}
	}
	return sum;
}

/*
 * Performs any neccesssary preparation and then sends the packet off to the emulator
 */
void deliverPacket(npp_packet *npp)
{
	ftp_packet *ftp = unwrapFTPPacket(npp);
	printf("Sending Packet [%d] back to client...\n",ftp->sequence_num);
	free(ftp);
	sendNPPPacket(sock,(struct sockaddr*) &args.ein, npp);
}

/*
 * Thread safe. Find the server thread handling requests from the sender of this packet and enqueues it
 * into that thread's incoming packet queue
 * 
 * Returns:
 * 		- int : The index in the server thread array that corresponds to the packet or -1 if none found
 */
int findAppropriateServerThread(npp_packet *npp)
{
	while(pthread_mutex_lock(thread_array_mutex));
	int i;
	for(i=0;i<NUM_SERVER_THREADS;i++) {
		if(server_threads[i] != NULL) {
			if(server_threads[i]->ip_ref == npp->source_ip) {
				
				while(pthread_mutex_lock(server_threads[i]->in_queue_mutex));
				
				enqueue(&(server_threads[i]->in_queue), (void *) npp);
				
				while(pthread_mutex_unlock(server_threads[i]->in_queue_mutex));
				while(pthread_mutex_unlock(thread_array_mutex));
				return i;
			}
		}
	}
	while(pthread_mutex_unlock(thread_array_mutex));
	return -1;
}

/*
 * Prints general information regarding a particular server thread
 * 
 * Parameters:
 * 		- obj : The thread object whose information is to be printed
 */
void printThreadObjectStatus(thread_obj *obj)
{
	if(obj == NULL) {
		printf("Thread pointer is null\n");
		return;
	}
	printf("IP Reference ID: %x\n", obj->ip_ref);
	printf("Thread Array Index: %d\n", obj->pthread_index);
	printf("Total SWS Size: %d\n", obj->sws_total);
	printf("SWS Frame: %d -> %d\n", obj->sws_start, obj->sws_end);
	printf("Incoming Queue Size: %d\n", obj->in_queue.size);
	printf("SWS Frame Data:\n");
	int i;
	for(i=obj->sws_start;i<obj->sws_end;i++) {
		printf("[%d] %d | %lu (NPP: size: %d priority: %d)\n", i, obj->sws_frame[i].acked, obj->sws_frame[i].timeout, obj->sws_frame[i].npp->payload_size, obj->sws_frame[i].npp->priority);
	}
}

/*
 * Thread safe. Attempts to create a new server SWS thread given an unclaimed NPP Packet
 * 
 * Parameters:
 * 		- npp : the initial client request packet for the thread
 * 
 * Returns:
 * 		- int : non zero on success, 0 on failure
 */
int createNewServerThread(npp_packet *npp)
{	
	ftp_packet *ftp = stringToFtp(npp->payload);
	
	if(ftp->packet_type != 'C' || ftp->payload_size < 4) {
		return -1;
	}
	
	unsigned int client_sws_window_size = 0;
	int i;
	for(i=0;i<4;i++) {
		client_sws_window_size <<= 8;
		client_sws_window_size |= (ftp->payload[i] & 0xFF);
	}
	
	char fname[10];
	sprintf(fname, "%u.txt", ftp->sequence_num);
	
	FILE * file;
	if((file = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Unable to open %s\n", fname);
		return 0;
	}
	fseek(file, 0, SEEK_END);
	unsigned long fsize = ftell(file);
	
	printf("Request received for file: %s (%lu bytes)\n", fname, fsize);
	
	unsigned int sws_len = (fsize / args.packet_length) + 1;
	fseek(file, 0, SEEK_SET);
	while(pthread_mutex_lock(thread_array_mutex));
	
	int index = -1;
	for(i=0;i<NUM_SERVER_THREADS;i++) {
		if(server_threads[i] == NULL) {
			index = i;
			break;
		}
	}
	if(index < 0) {
		fclose(file);
		return 0;
	}
	server_threads[index] = (thread_obj*) malloc(sizeof(thread_obj));
	server_threads[index]->ip_ref = npp->source_ip;
	server_threads[index]->port_ref = npp->source_port;
	server_threads[index]->thread = (pthread_t*) malloc(sizeof(pthread_t));
	server_threads[index]->pthread_index = index;
	server_threads[index]->in_queue_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	server_threads[index]->sws_start = 0;
	server_threads[index]->sws_client = client_sws_window_size;
	server_threads[index]->sws_total = sws_len;
	server_threads[index]->sws_end = (client_sws_window_size > sws_len) ? sws_len : client_sws_window_size;
	initQueue(&(server_threads[index]->in_queue));
	pthread_mutex_init(server_threads[index]->in_queue_mutex, NULL);
	
	server_threads[index]->sws_frame = (sws*) malloc(sizeof(sws) * sws_len);
	int msequence_number = args.sequence_num;
	
	for(i=0;i<sws_len;i++) {
		
		char buf[args.packet_length];
		char *b = &buf[0];
		int bytes_read = 0;
		while(!feof(file) && bytes_read < args.packet_length) {
			bytes_read += fread(b, 1, args.packet_length - bytes_read, file);
			b = &buf[bytes_read];
		}
		
		ftp_packet nftp;
		nftp.payload = buf;
		nftp.payload_size = bytes_read;
		nftp.sequence_num = msequence_number;
		nftp.packet_type = 'S';
		
		npp_packet *nnpp = (npp_packet*) malloc(sizeof(npp_packet));
		nnpp->priority = npp->priority;
		nnpp->dest_ip = npp->source_ip;
		nnpp->source_ip = npp->dest_ip;
		nnpp->dest_port = npp->source_port;
		nnpp->source_port = npp->dest_port;
		
		wrapFTPPacket(nnpp, &nftp);
		
		server_threads[index]->sws_frame[i].acked = 0;
		server_threads[index]->sws_frame[i].npp = nnpp;
		server_threads[index]->sws_frame[i].timeout = 0;
		
		msequence_number += bytes_read;
	}
	
	if(pthread_create(server_threads[index]->thread, NULL, tMain, (void*) &index)) {
		perror("Unable to spawn new thread");
		exit(errno);
	}
	printf("New Server thread created\n");
	while(pthread_mutex_unlock(thread_array_mutex));
	return 1;
}

/*
 * Thread safe. Remove a finished thread from the server thread array
 * 
 * Parameters:
 * 		- index : the index in the array to dispose the thread from
 */
void disposeThread(int index)
{
	while(pthread_mutex_lock(thread_array_mutex));
	
	server_threads[index] = NULL;
	
	while(pthread_mutex_unlock(thread_array_mutex));
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
int checkArgs(int argc, char **argv, server_args *args)
{
	unsigned char has_server_port = 0, has_emu_port = 0, has_sequence_num = 0, has_rate = 0, has_emu_hostname = 0,
				  has_packet_length = 0, has_timeout = 0;
	if(argc < 13) {
		return 0;
	}
	
	memset(&(args->sin.sin_addr), 0, sizeof(args->sin.sin_addr));
	args->ein.sin_family = AF_INET;
	args->sin.sin_family = AF_INET;
	args->sin.sin_addr.s_addr = htonl(INADDR_ANY);
	
	int i;
	for(i=1;i<argc;i++) {
		if(strcmp("-r", argv[i]) == 0 && i + 1 < argc) {
			args->rate = (unsigned short) atoi(argv[i+1]);
			has_rate = 1;
		}else if(strcmp("-p", argv[i]) == 0 && i + 1 < argc) {
			args->sin.sin_port = htons(atoi(argv[i+1]));
			has_server_port = 1;
		}else if(strcmp("-e", argv[i]) == 0 && i + 1 < argc) {
			args->ein.sin_addr.s_addr = inet_addr(getIPV4AddressFromHost(argv[i+1]));
			has_emu_hostname = 1;
		}else if(strcmp("-h", argv[i]) == 0 && i + 1 < argc) {
			args->ein.sin_port = htons(atoi(argv[i+1]));
			has_emu_port = 1;
		}else if(strcmp("-q", argv[i]) == 0 && i + 1 < argc) {
			args->sequence_num = (unsigned short) atoi(argv[i+1]);
			has_sequence_num = 1;
		}else if(strcmp("-t", argv[i]) == 0 && i + 1 < argc) {
			args->timeout = (unsigned short) atoi(argv[i+1]);
			has_timeout = 1;
		}else if(strcmp("-l", argv[i]) == 0 && i + 1 < argc) {
			args->packet_length = (unsigned short) atoi(argv[i+1]);
			if(args->packet_length > 50000) {
				args->packet_length = 50000;
			}
			has_packet_length = 1;
		}
	}
	if(has_sequence_num == 0) {
		args->sequence_num = 1;
		has_sequence_num = 1;
	}
	return has_server_port * has_emu_port * has_sequence_num * has_rate * has_emu_hostname * has_packet_length * has_timeout;
}
