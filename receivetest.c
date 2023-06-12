#include "prpreceive.h"

int main (int argc, char *argv[]) {

	pid_t pid;
	pid = fork();
	if (pid == 0) {
		// Child
		if (open_connection() < 0) return -1;
		if (config_interface(0) < 0) return -1;
		configure_buffer (10);
		log_init ();
	} else {
		// Parent
		if (open_connection() < 0) return -1;
		if (config_interface(1) < 0) return -1;
		configure_buffer (10);
		log_init ();
	}

	return 0;
}