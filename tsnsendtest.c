
#include "tsn.h"

#include <unistd.h> /* For usleep */
#include <getopt.h>
#include <time.h>

#define SLEEP 		200000 /* Temps entre trames (microsegons) */
#define TOTALTIME 	60   /* Temps total de la prova en segons */
#define NFRAMES		1	/* Trames sense replicacio que s'envien */

#define N_IFS 2
#define IF_1 "eth1"
#define IF_2 "eth2"

#define MY_DEST_MAC0	0x01
#define MY_DEST_MAC1	0x02
#define MY_DEST_MAC2	0x03
#define MY_DEST_MAC3	0x04
#define MY_DEST_MAC4	0x05
#define MY_DEST_MAC5	0x06

int main (int argc, char *argv[]) {
	char *ifname[N_IFS];
	char ifsrcmac[6];
	char ifdstmac[6];
	char content[700];

	memset(content, 'x', 700);

	ifname[0] = (char *) calloc(IFNAMSIZ-1, sizeof(char)); 
	ifname[1] = (char *) calloc(IFNAMSIZ-1, sizeof(char)); 
	strcpy(ifname[0], IF_1);
	strcpy(ifname[1], IF_2);

	tsnInit();

	if (tsnConfig(ifname, 2)) return -1;

	ifdstmac[0] = MY_DEST_MAC0;
	ifdstmac[1] = MY_DEST_MAC1;
	ifdstmac[2] = MY_DEST_MAC2;
	ifdstmac[3] = MY_DEST_MAC3;
	ifdstmac[4] = MY_DEST_MAC4;
	ifdstmac[5] = MY_DEST_MAC5;

	for (int i = 0; i < NFRAMES; i++) {
		tsnSendFrame(1, 0x8000, ifdstmac, 5, content, 700);
		usleep(SLEEP);
	}

	tsnEnd();

	return 0;
}