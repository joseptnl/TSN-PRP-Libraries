/*
 *  Author: Josep Antoni Naranjo Llompart
 */

#include "send.h"

/* Global variables */
static char *frame;
static int configured = -1;

static char if_names[N_IFS][IFNAMSIZ];
static int n_ifs = N_IFS;
static int frame_sz = MAX_BUF_SIZ;
static int priority = PRIORITY;

static int priority_ptr = -1;
static int payload_start = -1;
static int payload_end = -1;
static int real_size = -1;
static int frame_completed = -1;

static int sockfd[N_IFS];
static struct ifreq if_idx[N_IFS];
static struct ifreq if_mac[N_IFS];
static struct sockaddr_ll socket_address[N_IFS];

static uint16_t seq_number = 0;

static int control_interface (unsigned long request, char *if_name, int sockfd, struct ifreq *if_req) {
	int res = 0;
	memset(if_req, 0, sizeof(struct ifreq));
	strncpy(if_req->ifr_name, if_name, IFNAMSIZ-1);
	if ((res = ioctl(sockfd, request, if_req)) < 0) {
	    printf("[Get interface data failed] Interface: %s, Request: %lu, Due to: %s \n", if_name, request, strerror(errno));
	}
	return res;
}

static void set_socket_addr (int index) {
	/* Index of the network device */
	socket_address[index].sll_ifindex = if_idx[index].ifr_ifindex;

	/* Address length*/
	socket_address[index].sll_halen = ETH_ALEN;

	/* Destination MAC */
	socket_address[index].sll_addr[0] = MY_DEST_MAC0;
	socket_address[index].sll_addr[1] = MY_DEST_MAC1;
	socket_address[index].sll_addr[2] = MY_DEST_MAC2;
	socket_address[index].sll_addr[3] = MY_DEST_MAC3;
	socket_address[index].sll_addr[4] = MY_DEST_MAC4;
	socket_address[index].sll_addr[5] = MY_DEST_MAC5;
}

static void set_rct () {
	int ptr = payload_end;
	frame[ptr] = 0x0000;
	ptr += 2;
	uint16_t siz = (uint16_t) (payload_end - payload_start);
	frame[ptr++] = (siz >> 8) & 0xff;
	frame[ptr++] = siz & 0xff;
	real_size = ptr;
}

static void update_rct_seq () {
	seq_number += 1;
	int ptr = payload_end;
	frame[ptr++] = (seq_number >> 8) & 0xff;
	frame[ptr] = seq_number & 0xff;
}

static void update_rct_lan (uint8_t lan_id) {
	int ptr = payload_end;
	ptr += 2;
	frame[ptr] = frame[ptr] & 0x0f;
	frame[ptr] = frame[ptr] | lan_id;
}

int open_socket () {
	sockfd[0] = socket(AF_PACKET, SOCK_RAW, htons(DEF_ETHER_TYPE));
	sockfd[1] = socket(AF_PACKET, SOCK_RAW, htons(DEF_ETHER_TYPE));
	if (!sockfd[0] || !sockfd[1]) {
		printf("[Open socket failed] Due to: %s \n", strerror(errno));
		return -1;
	}
	return 0;
}

int config_interfaces () {
	if (sockfd[0] < 0) {
		printf("Socked not opened\n");
		return -1;
	} 

	strcpy(if_names[0], IF_1);
	strcpy(if_names[1], IF_2); 

	/* Get the interface index to send on */
	if (control_interface(SIOCGIFINDEX, if_names[0], sockfd[0], &if_idx[0]) < 0) return -1;	// Interface 0
	if (control_interface(SIOCGIFINDEX, if_names[1], sockfd[1], &if_idx[1]) < 0) return -1;	// Interface 1

	/* Get the MAC address of the interface to send on */
	if (control_interface(SIOCGIFHWADDR, if_names[0], sockfd[0], &if_mac[0]) < 0) return -1;	// Interface 0
	if (control_interface(SIOCGIFHWADDR, if_names[1], sockfd[1], &if_mac[1]) < 0) return -1;	// Interface 1

	/* Check interfaces MAC address*/
	for (int i = 0; i < 6; i++) {
		if (((uint8_t *)&if_mac[0].ifr_hwaddr.sa_data)[i] != ((uint8_t *)&if_mac[1].ifr_hwaddr.sa_data)[i]) {
			printf("The interfaces don't have the same mac addr., check config. \n");
			return -1;
		}
	}

	/* Set socket addresses */
	set_socket_addr(0);
	set_socket_addr(1);

	configured = 0;

	return configured;
}

int craft_frame (
	uint16_t eth_type
) {
	if (configured < 0) {
		printf("The interfaces haven't been configured \n");
		return -1;
	}

	if (payload_start >= 0) {
		printf("The frame is already built \n");
		return -1;
	}

	frame = (char *) calloc(frame_sz, sizeof(char)); // Allocate space for the frame buffer
	int tx_len = 0;

	struct ether_header *eh = (struct ether_header *) frame; // Eth struct header startint at the first of the buff.

	/* Build the Ethernet header */
	/* Source mac addr */
	eh->ether_shost[0] = ((uint8_t *)&if_mac[0].ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac[0].ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac[0].ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac[0].ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac[0].ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac[0].ifr_hwaddr.sa_data)[5];

	/* Destination mac addr */
	eh->ether_dhost[0] = MY_DEST_MAC0;
	eh->ether_dhost[1] = MY_DEST_MAC1;
	eh->ether_dhost[2] = MY_DEST_MAC2;
	eh->ether_dhost[3] = MY_DEST_MAC3;
	eh->ether_dhost[4] = MY_DEST_MAC4;
	eh->ether_dhost[5] = MY_DEST_MAC5;

	/* Ethertype field */
	if (eth_type != 0x8100 || eth_type != 0x8000) eth_type = DEF_ETHER_TYPE; 
	eh->ether_type = htons(eth_type);
	tx_len += sizeof(struct ether_header);

	/* Pointer to the the start of the payload */
	payload_start = tx_len;

	return 0;
}

int set_payload (char *payload, int payload_size) {
	if (payload_start < 0 || payload_size > MAX_PAYLOAD_SIZ - RCT_SIZE || payload_size < MIN_PAYLOAD_SIZ - RCT_SIZE) {
		printf("The frame hasn't been created or payload has not the correct size (Payload: %d) \n", payload_start);
		return -1;
	}

	payload_end = payload_start;

	for (int i = 0; i < payload_size; i++) {
		frame[payload_end++] = payload[i];
	}

	set_rct();

	frame_completed = 0;

	return 0;
}

int delete_frame () {
	if (payload_start < 0) {
		printf("The frame hasn't been created\n");
		return -1;
	}

	free(frame);

	priority_ptr = -1;
	payload_start = -1;
	payload_end = -1;
	frame_completed = -1;

	return 0;
}

int send_frame () {
	if (frame_completed < 0) {
		printf("The frame doesn't have payload");
		return -1;
	}
	int res1 = 0, res2 = 0;

	update_rct_seq();
	update_rct_lan(0xA0);
	res1 = sendto(sockfd[0], frame, real_size, 0, (struct sockaddr*) &socket_address[0], sizeof(struct sockaddr_ll));
	update_rct_lan(0xB0);
	res2 = sendto(sockfd[1], frame, real_size, 0, (struct sockaddr*) &socket_address[1], sizeof(struct sockaddr_ll));

	if (!res1 || !res2)
		printf("[Send failed] Due to: %s \n", strerror(errno));
	return res1;
}

