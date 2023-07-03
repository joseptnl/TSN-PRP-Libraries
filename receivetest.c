/**
 * Test to receive frames using the log library.
*/

#include "log.h"
#include <getopt.h>

#define IF1 "enp3s0f0"
#define IF2 "enp3s0f1"

int main (int argc, char *argv[]) {

	char c;
	uint8_t type, time_log_sec = MAX_WAIT_TIME;
	while ((c = getopt(argc, argv, "p:t:")) != -1)
	{
		switch (c){
			case 'p':
				// 0 PRP frames and 1 TSN frames
				type = (uint8_t) atoi(optarg);
				type = type == 0 ? 0 : 1;
				printf("Type is: %d\n", type);
				break;
			case 't':
				// 0 PRP frames and 1 TSN frames
				time_log_sec = (uint8_t) atoi(optarg);
				printf("Time to log in seconds: %d\n", time_log_sec);
				break;
		}
	}

	set_log_type(type);
	set_elapsed_time(time_log_sec);
	pid_t pid;
	pid = fork();
	if (pid == 0) {
		// Child
		if (config_interface(IF1) < 0) return -1;
		configure_buffer (10);
		log_init ();
		close_interface();
	} else {
		// Parent
		if (config_interface(IF2) < 0) return -1;
		configure_buffer (10);
		log_init ();
		close_interface();
	}
	return 0;
}