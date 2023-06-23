#define _GNU_SOURCE
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#define MAC_ADDR_SIZE 6

char *ethernet_frame (char *dst_mac, char *src_mac, uint16_t max_size);
uint16_t add_type (char *frame, uint16_t offset, uint16_t eth_type);
uint16_t add_vlan_tag (char *frame, uint16_t offset, uint8_t pcp, uint8_t dei, uint16_t vlan);
uint16_t add_r_tag (char *frame, uint16_t offset, uint16_t seq_num);