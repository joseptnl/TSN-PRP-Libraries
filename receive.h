
#include "generics.h"

#include <linux/ip.h>
#include <linux/udp.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/sem.h>

#define IF_1		"enp3s0f0"
#define IF_2		"enp3s0f1"
#define BUFFER_FRAMES 10
#define MAX_WAIT_TIME 20 // Seconds

static int bind_socket (int socket, struct sockaddr_ll *addr);
int open_socket ();
int config_interface (int if_index);
int configure_buffer (int if_index, int max_n_of_frames);
void log_init (int if_index);
void* writer (void* if_i);
void* receiver (void* if_i);
void* timeout_manager (void* if_i);
int64_t diff_timespec(const struct timespec after, const struct timespec before);
static void process_prp_frame (int if_index, int nframe, 
		ssize_t *frame_size,
		int64_t *frame_arrival_time, 
		uint32_t *rct_seq_num,
		char *lan_id);