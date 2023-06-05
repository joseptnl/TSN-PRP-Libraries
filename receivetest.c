#include "receive.h"

int main (int argc, char *argv[]) {
	if (open_socket() < 0) return -1;

	pid_t pid;
	pid = fork();
	if (pid == 0) {
		// Child
		if (config_interface(0) < 0) return -1;
		configure_buffer (0, 10);
		log_init (0);
	} else {
		// Parent
		if (config_interface(1) < 0) return -1;
		configure_buffer (1, 10);
		log_init (1);
	}

	return 0;
}