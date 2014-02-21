/*
 * CSC 450 Programming Assignment 3
 * 
 * Written By: John Hawkins & Jacob Burt
 * Date: 2/19/14
 */
#include "packetlib.h"
#include <time.h>

/*
 * Encapsulates all of the arguments for the emulator program
 */
typedef struct {
	
	struct sockaddr_in sin;
	unsigned short queue_size;
	char * fname;
	
} emu_args;

/*
 * Struct for a single queued packet
 */
typedef struct {
	
	npp_packet *npp;
	unsigned long timestamp;
	
} queued_packet;

// GLOBAL VARIABLES

emu_args args;
int q1_index, q2_index, q3_index, sock;
queued_packet *q1, *q2, *q3;
pthread_mutex_t *m1, *m2, *m3;
FILE * log_fd;

// FUNCTION PROTOTYPES

void *tMain(void *ptr);

void dequeuePacket(queued_packet *q, pthread_mutex_t *m, int *index);

void enqueuePacket(queued_packet *q, pthread_mutex_t *m, int *index, npp_packet *npp);

int checkArgs(int argc, char **argv, emu_args *args);

void logFile(char * msg, npp_packet* npp);

void deliverPacket(npp_packet *npp);

// END PROTOTYPES

int main(int argc, char **argv)
{
	srand(time(NULL));
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	if(!checkArgs(argc, argv, &args)) {
		fprintf(stderr,"Invalid Arguments\n");
		exit(1);
	}
	
	log_fd = fopen(args.fname, "w");
	
	printf("Emulator Launched");
	
	q1_index = q2_index = q3_index = 0;
	q1 = (queued_packet*) malloc(args.queue_size * sizeof(queued_packet));
	q2 = (queued_packet*) malloc(args.queue_size * sizeof(queued_packet));
	q3 = (queued_packet*) malloc(args.queue_size * sizeof(queued_packet));
	
	//sock = openUDPSocket(); Testing only
	sock = openRawSocket();
	
	if(bind(sock,(struct sockaddr*) &(args.sin), sizeof(args.sin)) < 0) {
		perror("Failed to bind to UDP socket");
		exit(errno);
	}
	
	m1 = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m1,NULL);
	m2 = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m2,NULL);
	m3 = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m3,NULL);
	
	pthread_t mThread;
	if(pthread_create(&mThread, NULL, tMain, (void*) NULL)) {
		perror("Unable to create thread");
		exit(errno);
	}
	while(1) {
		unsigned long time = getMillis();
		
		while(q1_index > 0 && q1[0].timestamp < time) {
			if(q1[0].timestamp == 0) {
				unsigned long delay = (10 * (rand() % 110)) + 100;
				q1[0].timestamp = time + delay;
			}else if(q1[0].timestamp <= time) {
				dequeuePacket(q1, m1, &q1_index);
			}
		}
		while(q2_index > 0 && q2[0].timestamp < time) {
			if(q2[0].timestamp == 0) {
				unsigned long delay = (10 * (rand() % 110)) + 100;
				q2[0].timestamp = time + delay;
			}else if(q2[0].timestamp <= time) {
				dequeuePacket(q2, m1, &q2_index);
			}
		}
		while(q3_index > 0 && q3[0].timestamp < time) {
			if(q3[0].timestamp == 0) {
				unsigned long delay = (10 * (rand() % 110)) + 100;
				q3[0].timestamp = time + delay;
			}else if(q3[0].timestamp <= time) {
				dequeuePacket(q3, m1, &q3_index);
			}
		}
	}
	pthread_cancel(mThread);
	fclose(log_fd);
	return 0;
}

/*
 * Separate thread for handling packets asynchronously
 */ 
void *tMain(void *ptr)
{
	while(1) {
		npp_packet *npp = receiveNPPPacket(sock, (struct sockaddr*) &(args.sin));
		if(npp == NULL) {
			continue;
		}
		logFile("New Packet Received", npp);
		switch(npp->priority) {
		
		case 0x01:
			enqueuePacket(q1, m1, &q1_index, npp);
			break;
			
		case 0x02:
			enqueuePacket(q2, m2, &q2_index, npp);
			break;
			
		case 0x03:
			enqueuePacket(q3, m3, &q3_index, npp);
			break;
		}
	}
	exit(0);
}

/*
 * Thread Safe. If delay is complete, dequeues a packet and then calculates the probably of sending it
 */ 
void dequeuePacket(queued_packet *q, pthread_mutex_t *m, int *index)
{
	while(pthread_mutex_lock(m));
	
	npp_packet *npp = q[0].npp;
	
	int i;
	for(i=0;i<args.queue_size - 1;i++) {
		q[i] = q[i+1];
	}
	index[0]--;
	
	if((rand() % 10) > 1) {
		deliverPacket(npp);
	}else {
		logFile("Dropped Packet - Random Drop", npp);
	}
	while(pthread_mutex_unlock(m));
}

/*
 * Thread Safe. Calculates delay and enqueues a packet
 */ 
void enqueuePacket(queued_packet *q, pthread_mutex_t *m, int *index, npp_packet *npp)
{
	while(pthread_mutex_lock(m));
	
	if(index[0] < args.queue_size) {
		q[index[0]].npp = npp;
		q[index[0]].timestamp = 0;
		index[0]++;
		logFile("Packet added to queue", npp);
	}else {
		logFile("Dropped Packet - Queue is full, unable to add packet", npp);
	}
	
	while(pthread_mutex_unlock(m));
}

/*
 * Performs any neccesssary preparation and then sends the packet off to the destination
 */
void deliverPacket(npp_packet *npp)
{
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = npp->dest_port;
	sin.sin_addr.s_addr = npp->dest_ip;
	sendNPPPacket(sock,(struct sockaddr*) &sin, npp);
	logFile("Packet successfully passed on", npp);
}

/*
 * Records an event to the log file
 */
void logFile(char *msg, npp_packet* npp)
{
	printf("%lu - %s\n", getMillis(), msg);
	fprintf(log_fd, "(Queue: %d) %lu - %s Info: [%x | %x | %x | %x]\n",npp->priority, getMillis(), msg, npp->source_ip, npp->source_port, npp->dest_ip, npp->dest_port);
	fflush(log_fd);
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
int checkArgs(int argc, char **argv, emu_args *args)
{
	unsigned char has_port = 0, has_queue_size = 0, has_fname = 0;
	if(argc < 7) {
		return 0;
	}
	
	memset(&(args->sin.sin_addr), 0, sizeof(args->sin.sin_addr));
	args->sin.sin_family = AF_INET;
	args->sin.sin_addr.s_addr = htonl(INADDR_ANY);
	
	int i;
	for(i=1;i<argc;i++) {
		if(strcmp("-q", argv[i]) == 0 && i + 1 < argc) {
			args->queue_size = (unsigned short) atoi(argv[i+1]);
			has_queue_size = 1;
		}else if(strcmp("-p", argv[i]) == 0 && i + 1 < argc) {
			args->sin.sin_port = htons(atoi(argv[i+1]));
			has_port = 1;
		}else if(strcmp("-l", argv[i]) == 0 && i + 1 < argc) {
			args->fname = argv[i+1];
			has_fname = 1;
		}
	}
	return has_fname * has_port * has_queue_size;
}
