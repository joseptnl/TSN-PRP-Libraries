/**
 * Author: Josep Antoni Naranjo Llompart
 * 
 * Auxiliar library to make a log with all the messages received
 * through a concrete interface. You can choose to process TSN or
 * PRP formated frames.
 * 
 * NOTE:
 * Made for my tfg purposes.
*/

#include "log.h"

#define MAX_BUF_SIZ 1524
#define BUFF_FSIZE_OFFSET sizeof(ssize_t)
#define BUFF_FTIME_OFFSET sizeof(struct timespec)
#define BUFF_FSTART_OFFSET BUFF_FSIZE_OFFSET + BUFF_FTIME_OFFSET
#define BUFF_FCELL_OFFSET BUFF_FSTART_OFFSET + MAX_BUF_SIZ

#define VLAN_TAG_OFFSET BUFF_FSTART_OFFSET + 12
#define R_TAG_OFFSET BUFF_FSTART_OFFSET + 12

static int configured = -1;
static int writer_stop_condition = 1;
static ssize_t buff_size;
static int addr_size = sizeof(struct sockaddr);

static unsigned int n_frames = BUFFER_FRAMES;
static const unsigned int buff_fsize_offset = BUFF_FSIZE_OFFSET;
static const unsigned int buff_fstart_offset = BUFF_FSTART_OFFSET;
static const unsigned int buff_fcell_offset = BUFF_FCELL_OFFSET;
static const unsigned int buff_ftime_offset = BUFF_FTIME_OFFSET;
static const unsigned int vlan_tag_offset = VLAN_TAG_OFFSET;
static const unsigned int r_tag_offset = R_TAG_OFFSET;

static sem_t mutex, writer_sem;
static char if_name[IFNAMSIZ];
static int sockfd;
static char *frames_buffer;

static pthread_t threadId[3];

/**
 * 0 = PRP frames log
 * 1 = TSN frames log
*/
static uint8_t log_type = 0;

int config_interface (char *if_nam) {
	strncpy(if_name, if_nam, IFNAMSIZ-1);

	if((sockfd = init_interface(if_name, ETH_P_ALL, NULL)) < 0) return -1;

	configured = 1;

	return 0;
}

int close_interface () {
	if (end_interface(sockfd, if_name) < 0) return -1; 
	return 0;
}

int configure_buffer (int max_n_of_frames) {
	if (configured < 0) {
		printf("Sockets haven't been configured.\n");
		return -1;
	} 

	if (max_n_of_frames > BUFFER_FRAMES) n_frames = max_n_of_frames;

	// Frame buffer topology => frame_size | arriving_time | frame
	buff_size = n_frames * buff_fcell_offset;
	frames_buffer = (char *) calloc(buff_size, sizeof(char)); // Allocate space for the frame buffer	

	return 0;
}

void set_log_type (uint8_t lt) {
	log_type = lt;
}

// Calculates the difference between timespecs (miliseconds)
static int64_t diff_timespec(const struct timespec after, const struct timespec before)
{
    return ((int64_t)after.tv_sec - (int64_t)before.tv_sec) * (int64_t) 1000000
         + ((int64_t)after.tv_nsec - (int64_t)before.tv_nsec) / 1000;
}

static int mac_filter (char *mac) {
    if(
		MAC_ADDR_1 == *(mac) &&
		MAC_ADDR_2 == *(mac + 1) &&
		MAC_ADDR_3 == *(mac + 2) &&
		MAC_ADDR_4 == *(mac + 3) &&
		MAC_ADDR_5 == *(mac + 4) &&
		MAC_ADDR_6 == *(mac + 5)) return 1;
		return 0;
}
 
static void* receiver () {
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
		numbytes = (ssize_t *) &frames_buffer[cell_start];
		diff = (int64_t *) &frames_buffer[cell_start + buff_fsize_offset];

		// Receive frame and storing its size into the value addresed by the pointer in the buffer
		*numbytes = receive_frame(sockfd, &frames_buffer[cell_start + buff_fstart_offset], MAX_BUF_SIZ);

		// Save time in buffer, just in front of the frame, and move pointer
		clock_gettime(CLOCK_REALTIME, &now);
		*diff = diff_timespec(now, start);
		
		// Filter frame
		if (mac_filter(&frames_buffer[cell_start + buff_fstart_offset]) > 0) {
			printf("[RECEIVER] %d: Size: %zd, Arrival time: %f.\n", count, *numbytes, ((double) *diff) / 1000);
			sem_post(&mutex);
		} else {
			count -= 1;
		}
	}
}

static void process_prp_frame (int nframe, 
		ssize_t *frame_size,
		int64_t *frame_arrival_time, 
		uint32_t *rct_seq_num,
		char *lan_id) {
	// Set pointer to the start of the buffer cell
	char *fcell_ptr = &frames_buffer[(nframe % n_frames) * buff_fcell_offset];
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

static void process_tsn_frame (int nframe,
		ssize_t *frame_size,
		int64_t *frame_arrival_time,
		uint8_t *is_frer,
		uint16_t *frer_seq_num) {
	// Set pointer to the start of the buffer cell
	char *fcell_ptr = &frames_buffer[(nframe % n_frames) * buff_fcell_offset];
	// Get frame size
	*frame_size = *((ssize_t *) fcell_ptr);
	// Get arrival time miliseconds
	*frame_arrival_time = *((int64_t *)  (fcell_ptr + buff_fsize_offset));
	// Get the lan id content
	uint16_t frer_const = ntohs(*((uint16_t *) (fcell_ptr + r_tag_offset)));
	if (frer_const == 0xf1c1) {
		*is_frer = 1;
		*frer_seq_num = *((uint16_t *) (fcell_ptr + r_tag_offset + 4));
	} else {
		*is_frer = 0;
		*frer_seq_num = 0;
	}
}


static void* writer () {
	char lan;
	FILE *file; // Log file
	int seq_num = 0;
	int count = 0;

	ssize_t frame_size;
	int64_t frame_arrival_time;
	uint32_t rct_seq_num;
	uint8_t is_frer;
	uint16_t frer_seq_num;

	char lan_id;

	// Open an erased existing file or create a new one
	file = fopen((const char *)&if_name, "w"); // Takes the if name

	while(1) {
		sem_wait(&mutex);

		// Check if the execution must stop
		sem_wait(&writer_sem);
		if (writer_stop_condition < 0) {
			writer_stop_condition  = 1;
			break;
		}
		sem_post(&writer_sem);

		/* Write the frame log */
		
		// Destructure the frame to get rellevant data
		if (log_type == 0) {
			process_prp_frame(count++, &frame_size, &frame_arrival_time, &rct_seq_num, &lan_id);
			printf("[WRITER] Lan: %hhX, Size: %zd, Arrival time: %f, Seq Num: %hd.\n", lan_id, frame_size, ((double) frame_arrival_time) / 1000, ntohs(rct_seq_num));
			// Write into file
			fprintf(file, "%hhX, %zd, %f, %hd\n", lan_id, frame_size, ((double) frame_arrival_time) / 1000, ntohs(rct_seq_num));
		} else {
			process_tsn_frame(count++, &frame_size, &frame_arrival_time, &is_frer, &frer_seq_num);
			printf("[WRITER] Size: %zd, Arrival time: %f, Is FRER frame: %hd, Seq Num: %hd.\n", frame_size, ((double) frame_arrival_time) / 1000, is_frer, ntohs(frer_seq_num));
			// Write into file
			//show_frame(&frames_buffer[(count++ % n_frames) * buff_fcell_offset]);
			fprintf(file, "%zd, %f, %hd, %hd\n", frame_size, ((double) frame_arrival_time) / 1000, is_frer, ntohs(frer_seq_num));
		}
	}
	// Close the previously opened file
	fclose(file);
}

static void* timeout_manager () {
	sleep(MAX_WAIT_TIME);
	pthread_cancel(threadId[1]);
	sem_wait(&writer_sem);
	writer_stop_condition = -1;
	sem_post(&writer_sem);
	sem_post(&mutex);
}

void log_init () {
	sem_init(&mutex, 1, 0);
	sem_init(&writer_sem, 0, 1);

	pthread_create(&threadId[0], NULL, timeout_manager, NULL);
	pthread_create(&threadId[1], NULL, receiver, NULL);
	pthread_create(&threadId[2], NULL, writer, NULL);

	pthread_join(threadId[0], NULL);
	pthread_join(threadId[1], NULL);
	pthread_join(threadId[2], NULL);
}