#include "tsn.h"

struct if_data
{
	int sockfd;
	char src_mac[6];
	char *frame;
	char *name;
};

static struct if_data *if_init_data;
static uint16_t seq_num;
static uint8_t n_ifs;
static sem_t seq_num_mtx;

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
    uint16_t *frame_size)
{
	if (payload_size < MIN_PAYLOAD_SIZ) {
        printf("[Error when crafting frame] Payload isn't big enough.\n");
        *frame_size = 0;
        return (char *) 0;
    }

	char *frame = ethernet_frame(dst_mac, src_mac, frer > 0 ? FRER_MAX_FRAME_SIZ : TSN_MAX_FRAME_SIZ);

	// Ethertype field 
	*frame_size = add_vlan_tag(frame, MAC_ADDR_SIZE * 2, priority,  dei, vlan);

	if (frer > 0) {
		if (payload_size > FRER_MAX_PAYLOAD_SIZ) payload_size = FRER_MAX_PAYLOAD_SIZ;

		*frame_size = add_r_tag(frame, *frame_size, seq_num);
	} else {
		if (payload_size > TSN_MAX_PAYLOAD_SIZ) payload_size = TSN_MAX_PAYLOAD_SIZ;
	}

	// Ethertype field 
	*frame_size = add_type(frame, *frame_size, eth_type);

	for (int i = 0; i < payload_size; i++) {
		frame[(*frame_size)++] = payload[i];
	}

	return frame;
}

void tsnInit () {
	seq_num = 0;
	sem_init(&seq_num_mtx, 0, 1);
}

uint8_t tsnConfig (char **if_name_list, uint8_t ifs_num) {
	n_ifs = ifs_num;
	if_init_data = (struct if_data *) calloc(ifs_num, sizeof(struct if_data));

	for (int i = 0; i < n_ifs; i++) {
		if((if_init_data[i].sockfd = init_interface(if_name_list[i], TSN_ETH_PROTOCOL, if_init_data[i].src_mac)) < 0) return INIT_IF_ERR;
		if_init_data[i].name = (char *) calloc(IFNAMSIZ-1, 1);
		strncpy(if_init_data[i].name, if_name_list[i], IFNAMSIZ-1);
	}

	return SUCCESS;
}

uint8_t tsnSendFrame (uint8_t frer, uint16_t eth_t, char *dst_mac, uint8_t priority, char *data, uint16_t data_size) {
	uint16_t frame_size;

	// Create frame
	/**
	 * NOTE:
	 * 
	 * The DEI and VLAN fields have been set statically because
	 * I don't need them to be configurable for my project purposes.
	*/
	for (int i = 0; i < n_ifs; i++) {
		if_init_data[i].frame = craft_tsn_frame(frer, eth_t, dst_mac, if_init_data[i].src_mac, priority, 0, 1, seq_num, data, data_size, &frame_size);
		if (frame_size == 0) return -1;
	}

	if (frer > 0) {
		sem_wait(&seq_num_mtx);
		seq_num += 1;
		sem_post(&seq_num_mtx);
	}

	for (int i = 0; i < n_ifs; i++) {
		if (send_frame(if_init_data[i].sockfd, if_init_data[i].frame, frame_size) < 0) return -1;
	}

	for (int i = 0; i < n_ifs; i++) {
		free(if_init_data[i].frame);
	}

	return SUCCESS;
}

uint8_t tsnEnd () {
	free(if_init_data);

	for (int i = 0; i < n_ifs; i++) {
		if (end_interface(if_init_data[i].sockfd, if_init_data[i].name) < 0) return -1;
	}

	return 0;
}