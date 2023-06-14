#include "prp.h"

struct if_data
{
	int sockfd;
	char src_mac[6];
};

static struct if_data if_init_data[N_IFS];
static uint16_t seq_num = 0;
static sem_t seq_num_mtx;

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

void prpInit () {
	sem_init(&seq_num_mtx, 0, 1);
}

uint8_t prpConfig (char **if_name_list) {
	for (int i = 0; i < N_IFS; i++) {
		if(if_init_data[i].sockfd = init_interface(if_name_list[i], ETH_P_ALL, if_init_data[i].src_mac) < 0) return INIT_SENDING_IF_ERR;
	}

	if (check_macs(if_init_data[0].src_mac, if_init_data[1].src_mac) < 0) return DIFF_MACS_ERR;

	return SUCCESS;
}

uint8_t prpSendFrame (uint16_t eth_t, char *dst_mac, char *data, uint16_t data_size) {
	uint16_t rct_ptr, frame_size;

	// Create frame
	char *frame = craft_prp_frame(eth_t, if_init_data[0].src_mac, dst_mac, data, data_size, &rct_ptr, &frame_size);

	if (frame_size == 0) return -1;

	update_rct_seq (seq_num, frame, rct_ptr);

	sem_wait(&seq_num_mtx);
	seq_num += 1;
	sem_post(&seq_num_mtx);

	set_rct_lan(0xA0, frame, rct_ptr);
	if (send_frame(if_init_data[0].sockfd, frame, frame_size) < 0) return -1;
	set_rct_lan(0xB0, frame, rct_ptr);
	if (send_frame(if_init_data[1].sockfd, frame, frame_size) < 0) return -1;

	free(frame);

	return SUCCESS;
}

uint8_t prpEnd () {
	if (end_interface(if_init_data[0].sockfd) < 0) return -1;
	if (end_interface(if_init_data[1].sockfd) < 0) return -1;

	return 0;
}