#include "send.h"

/**
 * Opens the connection for sending packages
*/
int send_open_connection () {
	return open_socket();
}

/**
 * Binds the interface to the opened socket and obtains
 * its mac address.
 * 
 * 	- if_name: Pointer to char with the name of the if.
 * 	- sockfd: Integer representing the socket.
 * 	- src_mac: Pointer to unsigned char, must be initialized
 * 	with at least 6 bytes in memory to be able to contain the
 * 	mac address.
*/
int send_config_interface (char *if_name, int sockfd, unsigned char *src_mac) {
	int res = 0;

	if (sockfd < 0) {
		printf("Socked not opened\n");
		return -1;
	}

	// Bind socket to interface
	struct sockaddr_ll socket_address;
	memset(&socket_address, 0, sizeof(struct sockaddr_ll));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(DEF_ETHER_TYPE);
    socket_address.sll_ifindex = if_nametoindex(if_name);
	if ((res = bind_socket(sockfd, &socket_address)) < 0) return -1;

	// Get the MAC address of the interface to send on 
	struct ifreq if_mac;
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, if_name, IFNAMSIZ-1);
	if (control_socket(sockfd, SIOCGIFHWADDR, if_name, &if_mac) < 0) return -1;

	for (int i = 0; i < 6; i++) {
		*(src_mac + i) = if_mac.ifr_hwaddr.sa_data[i];
	}

	return res;
}

/**
 * Sends the frame writing it in the socket buffer.
 * 
 * 	- sockfd: Integer representing the socket.
 *  - frame: Pointer to char containing the frame to send.
 *  - frame_len: Integer with the length of the frame to
 * 	send.
*/
int send_frame (int sockfd, char *frame, int frame_len) {
	int res = 0;

	if ((res = write(sockfd, (void *) frame, (size_t) frame_len)) < 0) {
		printf("[Send failed] Due to: %s \n", strerror(errno));
	}

	return res;
}

