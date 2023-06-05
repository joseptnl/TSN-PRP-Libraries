/*
 *  Author: Josep Antoni Naranjo Llompart
 */

#include "receive.h"

#define BUFF_FSIZE_OFFSET sizeof(ssize_t)
#define BUFF_FTIME_OFFSET sizeof(struct timespec)
#define BUFF_FSTART_OFFSET BUFF_FSIZE_OFFSET + BUFF_FTIME_OFFSET
#define BUFF_FCELL_OFFSET BUFF_FSTART_OFFSET + MAX_BUF_SIZ

static int configured = -1;
static int sockopt;
static int writer_stop_condition = 1;
static ssize_t buff_size;
static int addr_size = sizeof(struct sockaddr);
static struct timespec start;
static int sockopt;

static unsigned int n_frames = BUFFER_FRAMES;
static const unsigned int buff_fsize_offset = BUFF_FSIZE_OFFSET;
static const unsigned int buff_fstart_offset = BUFF_FSTART_OFFSET;
static const unsigned int buff_fcell_offset = BUFF_FCELL_OFFSET;
static const unsigned int buff_ftime_offset = BUFF_FTIME_OFFSET;

static sem_t mutex[N_IFS], writer_sem[N_IFS];
static char if_names[N_IFS][IFNAMSIZ];
static int sockfd[N_IFS];
static char *frames_buffer[N_IFS];
static struct sockaddr_ll addr[N_IFS];
static struct ifreq ifopt;

static pthread_t threadId[3];

int open_socket () {
	sockfd[0] = socket(AF_PACKET, SOCK_RAW, htons(DEF_ETHER_TYPE));
	sockfd[1] = socket(AF_PACKET, SOCK_RAW, htons(DEF_ETHER_TYPE));
	if (sockfd[0] < 0 || sockfd[1] < 0) {
		printf("[Open socket failed] Due to: %s \n", strerror(errno));
		return -1;
	}
	return 0;
}

static int bind_socket (int socket, struct sockaddr_ll *addr) {
	int res = 0; 	
	if ((res = bind(socket, (struct sockaddr *) addr, sizeof(*addr))) < 0) {
        printf("[Set bind failed] Due to: %s\n", strerror(errno));
		close(socket);
		exit(EXIT_FAILURE);
    }
	return res;
}

int config_interfaces () {
	if (sockfd[0] < 0 || sockfd[1] < 0) {
		printf("Socket not opened\n");
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

	strncpy(ifopt.ifr_name, if_names[0], IFNAMSIZ-1);
	ioctl(sockfd[0], SIOCGIFFLAGS, &ifopt);
	ifopt.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd[0], SIOCSIFFLAGS, &ifopt);

	strncpy(ifopt.ifr_name, if_names[1], IFNAMSIZ-1);
	ioctl(sockfd[1], SIOCGIFFLAGS, &ifopt);
	ifopt.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd[1], SIOCSIFFLAGS, &ifopt);

	// Bind the interfaces with the opened sockets
	if (bind_socket(sockfd[0], &addr[0]) < 0) return -1;
	if (bind_socket(sockfd[1], &addr[1]) < 0) return -1;

	configured = 0;

	return configured;
}

int configure_buffer (int if_index, int max_n_of_frames) {
	if (configured < 0) {
		printf("Sockets haven't been configured.\n");
		return -1;
	} 

	if (max_n_of_frames > BUFFER_FRAMES) n_frames = max_n_of_frames;

	// Frame buffer topology => frame_size | arriving_time | frame
	buff_size = n_frames * buff_fcell_offset;
	frames_buffer[if_index] = (char *) calloc(buff_size, sizeof(char)); // Allocate space for the frame buffer	

	return 0;
}

// Calculates the difference between timespecs (miliseconds)
int64_t diff_timespec(const struct timespec after, const struct timespec before)
{
    return ((int64_t)after.tv_sec - (int64_t)before.tv_sec) * (int64_t) 1000
         + ((int64_t)after.tv_nsec - (int64_t)before.tv_nsec) / 1000000;
}
 
void* receiver (void* if_i) {
	int if_index = *((int*) if_i);
	int count = 0;
	ssize_t *numbytes;
	struct timespec start, now;
	int64_t *diff;
	int f, cell_start;

	clock_gettime(CLOCK_REALTIME, &start);

	while (1) {
		/* Save frame in buffer */
		f = count++ % n_frames;
		cell_start = f * buff_fcell_offset;

		// Set the pointers to the next received frame bytesize and arrivaltime place in buffer
		numbytes = (ssize_t *) &frames_buffer[if_index][cell_start];
		diff = (int64_t *) &frames_buffer[if_index][cell_start + buff_fsize_offset];

		// Receive frame and storing its size into the value addresed by the pointer in the buffer
		*numbytes = recvfrom(sockfd[if_index], &frames_buffer[if_index][cell_start + buff_fstart_offset], MAX_BUF_SIZ, 0, (struct sockaddr *) &addr[if_index], (socklen_t *) &addr_size);

		// Save time in buffer, just in front of the frame, and move pointer
		clock_gettime(CLOCK_REALTIME, &now);
		*diff = diff_timespec(now, start);

		printf("[RECEIVER] %d: Size: %zd, Arrival time: %f.\n", count, *numbytes, ((double) *diff) / 1000);

		sem_post(&mutex[if_index]);
	}
}

static void process_prp_frame (int if_index, int nframe, 
		ssize_t *frame_size,
		int64_t *frame_arrival_time, 
		uint32_t *rct_seq_num,
		char *lan_id) {
	// Set pointer to the start of the buffer cell
	char *fcell_ptr = &frames_buffer[if_index][(nframe % n_frames) * buff_fcell_offset];
	// Get frame size
	*frame_size = *((ssize_t *) fcell_ptr);
	// Get arrival time miliseconds
	*frame_arrival_time = *((int64_t *)  (fcell_ptr + buff_fsize_offset));
	// Get the seq number
	*rct_seq_num = *((uint32_t *) (fcell_ptr + buff_fstart_offset + *frame_size - sizeof(uint32_t)));
	// Get the lan id content
	char lan_start = *((uint32_t *) (fcell_ptr + buff_fstart_offset + *frame_size - sizeof(uint16_t)));
	*lan_id = (lan_start & 0xf0) >> 4;
}

void* writer (void* if_i) {
	int if_index = *((int*) if_i);
	char lan;
	FILE *file; // Log file
	int seq_num = 0;
	int count = 0;

	ssize_t frame_size;
	int64_t frame_arrival_time;
	uint32_t rct_seq_num;
	char lan_id;

	// Open an erased existing file or create a new one
	file = fopen((const char *)&if_names[if_index], "w"); // Takes the if name

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
		
		// Destructure the prp frame to get rellevant data
		process_prp_frame(if_index, count++, &frame_size, &frame_arrival_time, &rct_seq_num, &lan_id);

		printf("[WRITER] Lan: %hhX, Size: %zd, Arrival time: %f, Seq Num: %hd.\n", lan_id, frame_size, ((double) frame_arrival_time) / 1000, ntohs(rct_seq_num));
		// Write into file
		fprintf(file, "%hhX, %zd, %f, %hd\n", lan_id, frame_size, ((double) frame_arrival_time) / 1000, ntohs(rct_seq_num));
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