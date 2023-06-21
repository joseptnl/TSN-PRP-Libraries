
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

#define PRP_ETH_TYPE 0x8000
#define TSN_ETH_TYPE 0x8100

#define MAC_ADDR_1 0x01
#define MAC_ADDR_2 0x02
#define MAC_ADDR_3 0x03
#define MAC_ADDR_4 0x04
#define MAC_ADDR_5 0x05
#define MAC_ADDR_6 0x06

int config_interface (char *if_name);
int configure_buffer (int max_n_of_frames);
void log_init ();
void set_log_type (uint8_t lt);

static void* writer ();
static void* receiver ();
static void* timeout_manager ();
static int64_t diff_timespec(const struct timespec after, const struct timespec before);
static void process_prp_frame (int nframe, 
		ssize_t *frame_size,
		int64_t *frame_arrival_time, 
		uint32_t *rct_seq_num,
		char *lan_id);
static void process_tsn_frame (int nframe,
		ssize_t *frame_size,
		int64_t *frame_arrival_time,
		uint8_t *priority,
		uint8_t *is_frer,
		uint16_t *frer_seq_num);