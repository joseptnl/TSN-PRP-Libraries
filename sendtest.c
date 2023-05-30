
#include "send.h"

#include <unistd.h> /* For usleep */
#include <getopt.h>
#include <time.h>

#define SLEEP 		200000 /* Temps entre trames (microsegons) */
#define TOTALTIME 	60   /* Temps total de la prova en segons */
#define NFRAMES		50	/* Trames sense replicacio que s'envien */

int main (int argc, char *argv[]) {
	char content[700];
	memset(content, 'x', 700);

	if (open_socket() < 0) return -1;

	if (config_interfaces() < 0) return -1;

	craft_frame(0x8000);

	if(set_payload(content, 700) < 0) return -1;

	for (int j=1; j<=NFRAMES; j++) {
		send_frame();
		usleep(SLEEP);
	}

	return 0;
}