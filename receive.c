/*
 *  Author: Josep Antoni Naranjo Llompart
 */

#include "receive.h"


static int configured = -1;
static unsigned int received_frames = 0;
static int sockopt;
static int writer_stop_condition = 1;
static ssize_t buff_size;
static socklen_t addr_size = sizeof(struct sockaddr);
static struct timespec start;
static int sockopt;
static int writer_stop_condition = 1;

static const int n_ifs = N_IFS;
static const unsigned int nFrames = BUFFER_FRAMES;
static const int frame_sz = MAX_BUF_SIZ;

static const unsigned int buff_ftime_offset = sizeof(struct timespec);
static const unsigned int buff_fsize_offset = sizeof(ssize_t);
static const unsigned int buff_fstart_offset = buff_fsize_offset + buff_ftime_offset;
static const unsigned int buff_fcell_size = buff_fstart_offset + frame_sz;

static sem_t mutex[n_ifs], writer_sem[n_ifs];
static char if_names[n_ifs][IFNAMSIZ];
static int sockfd[n_ifs];
static char *frames_buffer[n_ifs];
static ssize_t last[n_ifs];
static struct ifreq if_req[n_ifs];
static struct sockaddr_ll addr[n_ifs];

static pthread_t threadId[3];

int open_socket () {
	sockfd[0] = socket(PF_PACKET, SOCK_RAW, htons(DEF_ETHER_TYPE));
	sockfd[1] = socket(PF_PACKET, SOCK_RAW, htons(DEF_ETHER_TYPE));
	if (!sockfd[0] || !sockfd[1]) {
		printf("[Open socket failed] Due to: %s \n", strerror(errno));
		return -1;
	}
	return 0;
}

static int bind_socket (int socket, struct sockaddr_ll *addr) {
	int res = 0;
	if ((res = bind(socket, (struct sockaddr *) addr, sizeof(struct sockaddr_ll))) < 0) {
        printf("[Set bind failed] Due to: %s\n", strerror(errno));
		close(socket);
		exit(EXIT_FAILURE);
    }
	return res;
}

int config_interfaces () {
	if (sockfd[0] < 0) {
		printf("Socked not opened\n");
		return -1;
	} 

	strcpy(if_names[0], IF_1);
	strcpy(if_names[1], IF_2); 

	memset(&addr[0], 0, sizeof(struct sockaddr_ll));
	memset(&addr[1], 0, sizeof(struct sockaddr_ll));

	addr[0].sll_family = AF_PACKET;
    addr[0].sll_protocol = htons(DEF_ETHER_TYPE);
    addr[0].sll_ifindex = if_nametoindex(if_names[0]);
	addr[1].sll_family = AF_PACKET;
    addr[1].sll_protocol = htons(DEF_ETHER_TYPE);
    addr[1].sll_ifindex = if_nametoindex(if_names[1]);

	// Bind the interfaces with the opened sockets
	if (bind_socket(sockfd[0], &addr[0]) < 0) return -1;
	if (bind_socket(sockfd[1], &addr[1]) < 0) return -1;

	configured = 0;

	return configured;
}

int configure_buffer (int if_index, int max_n_of_frames) {
	if (max_n_of_frames < BUFFER_FRAMES) nFrames = max_n_of_frames;

	// Frame buffer topology => frame_size | arriving_time | frame
	buff_size = nFrames * buff_fcell_size;
	frames_buffer[if_index] = (char *) calloc(buff_size, sizeof(char)); // Allocate space for the frame buffer	
	last[if_index] = 0;

	return 0;
}

void* receiver (void* if_i) {
	int if_index = *((int*) if_i);
	ssize_t *numbytes;
	struct timespec now;
	int f;

	clock_gettime(CLOCK_MONOTONIC, &start);

	while (1) {
		/* Save frame in buffer */
		f = (received_frames++ % nFrames);
		last[if_index] = f * buff_fcell_size;

		// Set the pointer to the next received frame bytesize place in buffer
		numbytes = (ssize_t *) &frames_buffer[if_index][last[if_index]];

		// Receive frame and storing its size into the value addresed by the pointer in the buffer
		*numbytes = recvfrom(sockfd[if_index], &frames_buffer[if_index][last[if_index] + buff_fstart_offset], frame_sz, 0, (struct sockaddr *) &addr[if_index], (socklen_t *) &addr_size);

		// Save time in buffer, just in front of the frame, and move pointer
		clock_gettime(CLOCK_MONOTONIC, &now);
		struct timespec *diff = (struct timespec *) &frames_buffer[if_index][last[if_index] + buff_ftime_offset];
		diff->tv_sec = now.tv_sec - start.tv_nsec;

		printf("[RECEIVER] I got frame n = %d. \n", received_frames);

		sem_post(&mutex[if_index]);
	}
}

static void process_prp_frame (int if_index, int nframe, struct timespec *frame_arrival_time, uint16_t *rct_seq_num) {
	ssize_t *fcell_ptr = &frames_buffer[if_index][last] buff_fcell_size * (nframe % nFrames);
}

void* writer (void* if_i) {
	int if_index = *((int*) if_i);
	int count = 0;
	FILE *file; // Log file

	// Open an erased existing file or create a new one
	file = fopen(&if_names[if_index], "w"); // Takes the if name

	while(1) {
		sem_wait(&mutex[if_index]);

		// Check if the execution must stop
		sem_wait(&writer_sem[if_index]);
		if (writer_stop_condition < 0) {
			writer_stop_condition  = 1;
			break;
		}
		sem_post(&writer_sem[if_index]);

		/* Write the frame log */
		struct timespec *frame_arrival_time;
		uint16_t *rct_seq_num;
		process_prp_frame(if_index, count++, frame_arrival_time, rct_seq_num);
		fprintf(file, "");

		printf("[WRITER] I writed a frame n = %d. \n", count);
	}
	// Close the previously opened file
	fclose(file);
}

void* timeout_manager (void* if_i) {
	int if_index = *((int*) if_i);
	sleep(MAX_WAIT_TIME);
	pthread_cancel(threadId[1]);
	sem_wait(&writer_sem[if_index]);
	writer_stop_condition = -1;
	sem_post(&writer_sem[if_index]);
	sem_post(&mutex[if_index]);
}

void log_init (int if_index) {
	sem_init(&mutex[if_index], 1, 0);
	sem_init(&writer_sem[if_index], 0, 1);

	pthread_create(&threadId[0], NULL, timeout_manager, &if_index);
	pthread_create(&threadId[1], NULL, receiver, &if_index);
	pthread_create(&threadId[2], NULL, writer, &if_index);

	pthread_join(threadId[0], NULL);
	pthread_join(threadId[1], NULL);
	pthread_join(threadId[2], NULL);
	
	close(sockfd[0]);
	close(sockfd[1]);
}