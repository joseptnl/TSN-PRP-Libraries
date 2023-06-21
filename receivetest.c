#include "log.h"

#define IF1 "enp3s0f0"
#define IF2 "enp3s0f1"

int main (int argc, char *argv[]) {

	set_log_type(1);
	if (config_interface(IF1) < 0) return -1;
	configure_buffer (10);
	log_init ();
	/*
	pid_t pid;
	pid = fork();
	if (pid == 0) {
		// Child
		if (config_interface(IF1) < 0) return -1;
		configure_buffer (10);
		log_init ();
	} else {
		// Parent
		if (config_interface(IF2) < 0) return -1;
		configure_buffer (10);
		log_init ();
	}
	*/
	return 0;
}