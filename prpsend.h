
#include <stdint.h>
#include <netinet/if_ether.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define MAX_FRAME_SIZ 1514   // Frame size (without CRC)
#define MAX_PAYLOAD_SIZ 1500 // Payload size
#define MIN_PAYLOAD_SIZ 64
#define RCT_SIZE 4

char *craft_prp_frame(
    uint16_t eth_type,
    unsigned char *src_mac,
    unsigned char *dst_mac,
    char *payload,
    unsigned int payload_size,
    char lan,
    unsigned int *rct_ptr,
    unsigned int *frame_size);
int check_macs (char *ifmac1, char *ifmac2);
void set_rct_lan (char lan_id, char *frame, int ptr);
void update_rct_seq (int seq_number, char *frame, int ptr);
void set_rct (char *frame, unsigned int ptr, unsigned int payload_size);
