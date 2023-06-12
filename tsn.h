
#include "packetio.h"

#define SUCCESS 0
#define OPEN_CONNEC_ERR -1
#define SEND_CONF_IF_ERR -2
#define DIFF_MACS_ERR -3

#define MAX_FRAME_SIZ 1518   // Frame size (without CRC)
#define MAX_PAYLOAD_SIZ 1500 // Payload size
#define MIN_PAYLOAD_SIZ 64

static char *craft_tsn_frame(
    uint16_t eth_type,
	unsigned char *src_mac,
    unsigned char *dst_mac,
	char *payload, 
	uint16_t payload_size,
    uint16_t *rct_ptr,
    uint16_t *frame_size);
static int check_macs (char *ifmac1, char *ifmac2);
static void set_rct_lan (char lan_id, char *frame, int ptr);
static void update_rct_seq (int seq_number, char *frame, int ptr);
static void set_rct (char *frame, unsigned int ptr, unsigned int payload_size);
static void* if_send_frame (void *args);
uint8_t prpConfigSendingIfs (char **if_name_list);
uint8_t prpSendFrame (uint16_t eth_t, char *dst_mac, char *data, uint16_t data_size);
uint8_t prpSendingEnd ();
