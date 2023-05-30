
#include "generics.h"

#include <limits.h>

#define PRIORITY 	5	/* Prioritat per defecte */

#define IF_1		"eth1"
#define IF_2		"eth2"

static int control_interface (unsigned long request, char *if_name, int sockfd, struct ifreq *if_req);
static void set_socket_addr (int index);
static void set_rct ();
static void update_rct_seq ();
static void update_rct_lan (uint8_t lan_id);
int open_socket ();
int config_interfaces ();
int craft_frame (uint16_t eth_type);
int set_payload (char *payload, int payload_size);
int delete_frame ();
int send_frame ();
