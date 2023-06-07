
#include "generics.h"

#include <limits.h>

int send_open_connection ();
int send_config_interface (char *if_name, int sockfd, unsigned char *src_mac);
int send_frame (int sockfd, char *frame, int frame_len);
