
#include "packetio.h"

#include <linux/ip.h>
#include <linux/udp.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/sem.h>

#define BUFFER_FRAMES 10
#define MAX_WAIT_TIME 60 // Seconds

int open_connection ();
int config_interface (char *if_name);
int configure_buffer (int max_n_of_frames);
void log_init ();

static void* writer ();
static void* receiver ();
static void* timeout_manager ();
static int64_t diff_timespec(const struct timespec after, const struct timespec before);
static void process_prp_frame (int nframe, 
		ssize_t *frame_size,
		int64_t *frame_arrival_time, 
		uint32_t *rct_seq_num,
		char *lan_id);