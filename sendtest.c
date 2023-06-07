
#include "send.h"
#include "prpsend.h"

#include <unistd.h> /* For usleep */
#include <getopt.h>
#include <time.h>

#define SLEEP 		200000 /* Temps entre trames (microsegons) */
#define TOTALTIME 	60   /* Temps total de la prova en segons */
#define NFRAMES		50	/* Trames sense replicacio que s'envien */

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
	int socket[N_IFS];
	char ifname[N_IFS][IFNAMSIZ-1];
	char ifsrcmac[N_IFS][6];
	char ifdstmac[N_IFS][6];
	int seq_num[N_IFS];
	char *frame[N_IFS];
	unsigned int rct_ptr[N_IFS];
	unsigned int frame_size[N_IFS];
	char content[700];

	memset(content, 'x', 700);

	strcpy(ifname[0], IF_1);
	strcpy(ifname[1], IF_2);

	seq_num[0] = 0;
	seq_num[1] = 0;

	for (int i = 0; i < N_IFS; i++) {
		if ((socket[i] = send_open_connection()) < 0) return -1;
	}

	for (int i = 0; i < N_IFS; i++) {
		if (send_config_interface(ifname[i], socket[i], ifsrcmac[i]) < 0) return -1;
	}

	for (int i = 0; i < N_IFS; i++) {
		ifdstmac[i][0] = MY_DEST_MAC0;
		ifdstmac[i][1] = MY_DEST_MAC1;
		ifdstmac[i][2] = MY_DEST_MAC2;
		ifdstmac[i][3] = MY_DEST_MAC3;
		ifdstmac[i][4] = MY_DEST_MAC4;
		ifdstmac[i][5] = MY_DEST_MAC5;
	}

	char lan = 0;
	for (int i = 0; i < N_IFS; i++) {
		if (i == 0) lan = 0xa0;
		else lan = 0xb0;
		frame[i] = craft_prp_frame(0x8000, ifsrcmac[i], ifdstmac[i], content, 700, lan, &rct_ptr[i], &frame_size[i]);
		if (frame_size < 0) return -1;
	}

	pid_t pid = fork();
	if (pid == 0) {
		for (int i = 0; i < NFRAMES; i++) {
			send_frame(socket[0], frame[0], frame_size[0]);
			update_rct_seq(seq_num[0]++, frame[0], rct_ptr[0]);
			usleep(SLEEP);
		}
	} else {
		for (int i = 0; i < NFRAMES; i++) {
			send_frame(socket[1], frame[1], frame_size[1]);
			update_rct_seq(seq_num[1]++, frame[1], rct_ptr[1]);
			usleep(SLEEP);
		}
	}

	return 0;
}