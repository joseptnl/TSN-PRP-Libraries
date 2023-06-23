/**
 * Test to send PRP frames.
*/

#include "prp.h"

#include <unistd.h> /* For usleep */
#include <getopt.h>
#include <time.h>

#define SLEEP 		200000 /* Temps entre trames (microsegons) */
#define TOTALTIME 	60   /* Temps total de la prova en segons */
#define NFRAMES		50	/* Trames sense replicacio que s'envien */
#define PAYLOAD_SZ	700
#define PRIORITY	5

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
	int 
		payload_sz = PAYLOAD_SZ, 
		interval = SLEEP, 
		n_frames = NFRAMES,
		priority = PRIORITY;
		
	char *ifname[N_IFS];

	ssize_t read;
	char *line;
	size_t len;
	FILE *file;
	int counter = 0;
	
	char c;
	while ((c = getopt(argc, argv, "f:s:d:i:n:p:h")) != -1)
	{
		switch (c){
			case 's':
				payload_sz = atoi(optarg);
				printf("Payload size: %d B\n", payload_sz);
				break;
			case 'd':
				interval = atoi(optarg);
				printf("Interval for Tx frames: %d\n", interval);
				break;
			case 'i':
				file = fopen((const char *) optarg, "r");
				while ((read = getline(&line, &len, file)) != -1 && counter < N_IFS) {
					ifname[counter] = (char *) calloc(IFNAMSIZ-1, sizeof(char));
					for (int i = 0; i < IFNAMSIZ-1; i++) {
						if (line[i] != '\n') 
							ifname[counter][i] = line[i];
						else
							ifname[counter][i] = 0;
					}
					printf("Interface %d name: %s\n", counter, ifname[counter]);
					counter += 1;
				}
				fclose(file);
				break;
			case 'n':
				n_frames= atoi(optarg);
				printf("Frame number: %d\n", n_frames);
				break;
			case 'p':
				priority= atoi(optarg);
				printf("Piority: %d\n", priority);
				break;
			case 'h':
				printf("usage: %s -f [include FRER tag or not] -s [size of payload in Bytes] -d [delay in ms] -i [interfaces defition file (there must be 2 ifs)] -n [num of frames] -p [priority]\n", argv[0]);
				break;
		}
	}

	char ifsrcmac[6];
	char ifdstmac[6];
	char content[payload_sz];

	memset(content, 'x', payload_sz);

	prpInit();

	if (prpConfig(ifname)) return -1;

	ifdstmac[0] = MY_DEST_MAC0;
	ifdstmac[1] = MY_DEST_MAC1;
	ifdstmac[2] = MY_DEST_MAC2;
	ifdstmac[3] = MY_DEST_MAC3;
	ifdstmac[4] = MY_DEST_MAC4;
	ifdstmac[5] = MY_DEST_MAC5;

	for (int i = 0; i < NFRAMES; i++) {
		prpSendFrame(0x8000, ifdstmac, content, payload_sz);
		usleep(SLEEP);
	}

	prpEnd();

	return 0;
}