#include "ethframes.h"

/**
 * Creates an ethernet frame with its source and dest. mac
 * addresses.
 * 
 * Returns the resultant frame.
*/
char *ethernet_frame (char *dst_mac, char *src_mac, uint16_t max_size) {
    char *frame = (char *) calloc(max_size, 1);

    for (int i = 0; i < MAC_ADDR_SIZE; i++) {
        *(frame + i) = (uint8_t) *(dst_mac + i);
        *(frame + i + MAC_ADDR_SIZE) = (uint8_t) *(src_mac + i);
    }

    return frame;
}

/**
 * Adds the ethernet type to an ethernet frame.
 * 
 * Returns the next empty position.
*/
uint16_t add_type (char *frame, uint16_t offset, uint16_t eth_type) {
    frame[offset++] = eth_type >> 8;
	frame[offset++] = eth_type & 0xff;

    return offset;
}

/**
 * Adds the vlan tag to an ethernet frame.
 * 
 * Returns the next empty position.
*/
uint16_t add_vlan_tag (char *frame, uint16_t offset, uint8_t pcp, uint8_t dei, uint16_t vlan) {
    frame[offset++] = 0x81;
	frame[offset++] = 0x00;
    frame[offset] = pcp << 5;
	frame[offset] |= ((dei & 0x01) << 4);
    frame[offset++] |= vlan >> 8;
    frame[offset++] = (vlan & 0xff);

    return offset;
}

/**
 * Adds the FRER tag or R-tag to an ethernet frame.
 * 
 * Returns the next empty position.
*/
uint16_t add_r_tag (char *frame, uint16_t offset, uint16_t seq_num) {
    frame[offset++] = 0xf1;
	frame[offset++] = 0xc1;
	frame[offset++] = 0x00;

	frame[offset++] = 0x00;
    frame[offset++] = seq_num >> 8;
	frame[offset++] = seq_num & 0xff;

    return offset;
}