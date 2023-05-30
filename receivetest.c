#include "receive.h"

int main (int argc, char *argv[]) {
	if (open_socket() < 0) return -1;

	if (config_interfaces() < 0) return -1;

	configure_buffer (0, 10);

    log_init (0);

	return 0;
}