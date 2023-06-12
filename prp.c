#include "prp.h"

struct if_sdata
{
	int sockfd;
	char src_mac[6];
	char *frame;
};

struct if_rdata
{
	int sockfd;
	char *frame;
};

static struct if_sdata if_send_data[N_IFS];
static struct if_rdata if_rec_data[N_IFS];
static pthread_t threadId[N_IFS];
static uint16_t rct_ptr, frame_size, seq_num = 0;

static void set_rct (char *frame, unsigned int ptr, unsigned int payload_size) {
    uint16_t siz = (uint16_t) payload_size;
	frame[ptr++] = 0x00;
    frame[ptr++] = 0x00;
	frame[ptr++] = (siz >> 8) & 0xff;
	frame[ptr++] = siz & 0xff;
}

static void update_rct_seq (int seq_number, char *frame, int ptr) {
	frame[ptr++] = (seq_number >> 8) & 0xff;
	frame[ptr] = seq_number & 0xff;
}

static void set_rct_lan (char lan_id, char *frame, int ptr) {
	ptr += 2;
    uint8_t id = (uint8_t) lan_id;
	frame[ptr] &= 0x0f;
	frame[ptr] |= lan_id;
}

static int check_macs (char *ifmac1, char *ifmac2) {
    /* Check interfaces MAC address*/
	for (int i = 0; i < 6; i++) {
		if (ifmac1[i] != ifmac2[i]) {
			printf("The interfaces don't have the same mac addr., check config. \n");
			return -1;
		}
	}
    return 0;
}

static char *craft_prp_frame (
    uint16_t eth_type,
	unsigned char *src_mac,
    unsigned char *dst_mac,
	char *payload, 
	uint16_t payload_size,
    uint16_t *rct_ptr,
    uint16_t *frame_size 
) {
    if (payload_size > MAX_PAYLOAD_SIZ - RCT_SIZE) payload_size = MAX_FRAME_SIZ - RCT_SIZE;
    if (payload_size < MIN_PAYLOAD_SIZ) {
        printf("[Error when crafting frame] Payload isn't big enough.\n");
        *frame_size = 0;
        return (char *) 0;
    }

	char *frame = (char *) calloc(MAX_FRAME_SIZ, sizeof(char)); // Allocate space for the frame buffer
	uint16_t tx_len = 0;

	struct ether_header *eh = (struct ether_header *) frame; // Eth struct header startint at the first of the buff.

	// Build the Ethernet header
	// Source mac addr */
	eh->ether_shost[0] = (uint8_t) *(src_mac);
	eh->ether_shost[1] = (uint8_t) *(src_mac + 1);
	eh->ether_shost[2] = (uint8_t) *(src_mac + 2);
	eh->ether_shost[3] = (uint8_t) *(src_mac + 3);
	eh->ether_shost[4] = (uint8_t) *(src_mac + 4);
	eh->ether_shost[5] = (uint8_t) *(src_mac + 5);

	// Destination mac addr
	eh->ether_dhost[0] = (uint8_t) *(dst_mac);
	eh->ether_dhost[1] = (uint8_t) *(dst_mac + 1);
	eh->ether_dhost[2] = (uint8_t) *(dst_mac + 2);
	eh->ether_dhost[3] = (uint8_t) *(dst_mac + 3);
	eh->ether_dhost[4] = (uint8_t) *(dst_mac + 4);
	eh->ether_dhost[5] = (uint8_t) *(dst_mac + 5);

	// Ethertype field 
	eh->ether_type = htons(eth_type);

	tx_len += sizeof(struct ether_header);

	for (int i = 0; i < payload_size; i++) {
		frame[tx_len++] = payload[i];
	}

    *rct_ptr = tx_len;

	set_rct(frame, tx_len, payload_size);

    *frame_size = tx_len + RCT_SIZE;

	return frame;
}

static void* if_send_frame (void *args) {
	uint8_t if_idx = *((uint8_t *)args);
	uint8_t lan = *((uint8_t *)args + 1);

	set_rct_lan(lan, if_send_data[if_idx].frame, rct_ptr);
	send_frame(if_send_data[if_idx].sockfd, if_send_data[if_idx].frame, frame_size);
}

uint8_t prpConfigSendingIfs (char **if_name_list) {
	printf("%s\n", if_name_list[0]);
	printf("%s\n", if_name_list[1]);
	for (int i = 0; i < N_IFS; i++) {
		if ((if_send_data[i].sockfd = open_connection()) < 0) return OPEN_CONNEC_ERR;
		if(send_config_interface(if_name_list[i], if_send_data[i].sockfd, if_send_data[i].src_mac, ETH_P_ALL) < 0) return SEND_CONF_IF_ERR;
	}

	if (check_macs(if_send_data[0].src_mac, if_send_data[1].src_mac) < 0) return DIFF_MACS_ERR;

	return SUCCESS;
}

uint8_t prpSendFrame (uint16_t eth_t, char *dst_mac, char *data, uint16_t data_size) {
	// Create frame
	if_send_data[0].frame = craft_prp_frame(eth_t, if_send_data[0].src_mac, dst_mac, data, data_size, &rct_ptr, &frame_size);
	update_rct_seq (seq_num++, if_send_data[0].frame, rct_ptr);
	if_send_data[1].frame = (char *) calloc(frame_size, sizeof(char));
	memcpy(if_send_data[1].frame, if_send_data[0].frame, frame_size);

	uint8_t args1[2];
	args1[0] = 0;
	args1[1] = 0xA0;
	uint8_t args2[2];
	args2[0] = 1;
	args2[1] = 0xB0;

	pthread_create(&threadId[0], NULL, &if_send_frame, args1);
	pthread_create(&threadId[1], NULL, &if_send_frame, args2);

	pthread_join(threadId[0], NULL);
	pthread_join(threadId[1], NULL);

	free(if_send_data[0].frame);
	free(if_send_data[1].frame);

	return SUCCESS;
}

uint8_t prpEnd () {
	close(if_send_data[0].sockfd);
	close(if_send_data[1].sockfd);
}