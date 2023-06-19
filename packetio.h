
#include "generics.h"

#include <limits.h>

int init_interface (char *if_name, uint16_t eth_type, unsigned char *src_mac);
int end_interface (int sockfd, char *if_name);
uint64_t send_frame (int sockfd, char *frame, uint16_t frame_len);
uint64_t receive_frame (int sockfd, char *rec_buffer, uint16_t frame_len);
