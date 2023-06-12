
#include "generics.h"

#include <limits.h>

int open_connection ();
int send_config_interface (char *if_name, int sockfd, unsigned char *src_mac, uint16_t eth_type);
int receive_config_interface (char *if_name, int sockfd, uint16_t eth_type);
int send_frame (int sockfd, char *frame, int frame_len);
