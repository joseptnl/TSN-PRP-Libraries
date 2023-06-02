
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
int config_interfaces ();
int configure_buffer (int if_index, int max_n_of_frames);
void log_init (int if_index);
void* writer (void* if_i);
void* receiver (void* if_i);
void* timeout_manager (void* if_i);