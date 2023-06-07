#include "prpsend.h"

void set_rct (char *frame, unsigned int ptr, unsigned int payload_size) {
    uint16_t siz = (uint16_t) payload_size;
	frame[ptr++] = 0x00;
    frame[ptr++] = 0x00;
	frame[ptr++] = (siz >> 8) & 0xff;
	frame[ptr++] = siz & 0xff;
}

void update_rct_seq (int seq_number, char *frame, int ptr) {
	frame[ptr++] = (seq_number >> 8) & 0xff;
	frame[ptr] = seq_number & 0xff;
}

void set_rct_lan (char lan_id, char *frame, int ptr) {
	ptr += 2;
    uint8_t id = (uint8_t) lan_id;
	frame[ptr] &= 0x0f;
	frame[ptr] |= lan_id;
}

int check_macs (char *ifmac1, char *ifmac2) {
    /* Check interfaces MAC address*/
	for (int i = 0; i < 6; i++) {
		if (ifmac1[i] != ifmac2[i]) {
			printf("The interfaces don't have the same mac addr., check config. \n");
			return -1;
		}
	}
    return 0;
}

char *craft_prp_frame (
    uint16_t eth_type,
	unsigned char *src_mac,
    unsigned char *dst_mac,
	char *payload, 
	unsigned int payload_size, 
	char lan,
    unsigned int *rct_ptr,
    unsigned int *frame_size
) {
    if (payload_size > MAX_PAYLOAD_SIZ - RCT_SIZE) payload_size = MAX_FRAME_SIZ - RCT_SIZE;
    if (payload_size < MIN_PAYLOAD_SIZ) {
        printf("[Error when crafting frame] Payload isn't big enough.\n");
        *frame_size = 0;
        return (char *) 0;
    }

	char *frame = (char *) calloc(MAX_FRAME_SIZ, sizeof(char)); // Allocate space for the frame buffer
	int tx_len = 0;

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
	set_rct_lan(lan, frame, tx_len);

    *frame_size = tx_len + RCT_SIZE;

	return frame;
}