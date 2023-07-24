
#include "packetio.h"
#include "ethframes.h"

#include <pthread.h>
#include <sys/sem.h>
#include <semaphore.h>

#define SUCCESS 0
#define OPEN_CONNEC_ERR -1
#define INIT_IF_ERR -2
#define DIFF_MACS_ERR -3

#define TSN_MAX_FRAME_SIZ 1518   // Frame size (without CRC)
#define FRER_MAX_FRAME_SIZ 1524   // Frame size (without CRC)
#define TSN_MAX_PAYLOAD_SIZ 1500 // Payload size
#define FRER_MAX_PAYLOAD_SIZ 1500 // Payload size
#define MIN_PAYLOAD_SIZ 46

#define TSN_ETH_PROTOCOL 0x8100


static char *craft_tsn_frame(
	uint8_t frer,
    uint16_t eth_type,
	unsigned char *dst_mac,
	unsigned char *src_mac,
    uint8_t priority,
	uint8_t dei,
	uint16_t vlan,
	uint16_t seq_num,
	char *payload, 
	uint16_t payload_size,
    uint16_t *frame_size);

void tsnInit ();
uint8_t tsnConfig (char **if_name_list, uint8_t ifs_num);
uint8_t tsnSendFrame (uint8_t frer, uint16_t eth_t, char *dst_mac, uint8_t priority, char *data, uint16_t data_size);
uint8_t tsnEnd ();