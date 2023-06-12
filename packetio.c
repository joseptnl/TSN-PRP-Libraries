#include "packetio.h"

/**
 * Opens the connection for sending packages
 * 
 * Returns socket identifier if success or -1 if failure.
*/
int open_connection () {
	return open_socket();
}

/**
 * For SENDING purposes.
 * Binds the interface to the opened socket and obtains
 * its mac address.
 * 
 * 	- if_name: Pointer to char with the name of the if.
 * 	- sockfd: Integer representing the socket.
 * 	- src_mac: Pointer to unsigned char, must be initialized
 * 	with at least 6 bytes in memory to be able to contain the
 * 	mac address.
 *  - eth_type: integer of 2 bytes representing the used ethernet
 * 	protocol.
 * 
 * Returns >0 if success or -1 if failure.
*/
int send_config_interface (char *if_name, int sockfd, unsigned char *src_mac, uint16_t eth_type) {
	int res = 0;

	if (sockfd < 0) {
		printf("Socked not opened\n");
		return -1;
	}

	// Bind socket to interface
	struct sockaddr_ll socket_address;
	memset(&socket_address, 0, sizeof(struct sockaddr_ll));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(eth_type);
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
 * 
 * Returns >0 if success or -1 if failure.
*/
int send_frame (int sockfd, char *frame, int frame_len) {
	int res = 0;

	if ((res = write(sockfd, (void *) frame, (size_t) frame_len)) < 0) {
		printf("[Send failed] Due to: %s \n", strerror(errno));
	}

	return res;
}

/**
 * For RECEIVING purposes.
 * Binds the interface to the opened socket and obtains
 * its mac address.
 * 
 * 	- if_name: Pointer to char with the name of the if.
 * 	- sockfd: Integer representing the socket.
 * 	- eth_type: integer of 2 bytes representing the used ethernet
 * 	protocol.
 * 
 * Returns >0 if success or -1 if failure.
*/
int receive_config_interface (char *if_name, int sockfd, uint16_t eth_type) {
	int res = 0;

	if (sockfd < 0) {
		printf("Socket not opened\n");
		return -1;
	} 

	struct sockaddr_ll socket_address;
	memset(&socket_address, 0, sizeof(struct sockaddr_ll));
	socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(eth_type);
    socket_address.sll_ifindex = if_nametoindex(if_name);

	// Set promiscuos mode
	struct ifreq promisc_req;
	strncpy(promisc_req.ifr_name, if_name, IFNAMSIZ-1);
	ioctl(sockfd, SIOCGIFFLAGS, &promisc_req);
	if(control_socket(sockfd, SIOCGIFFLAGS, if_name, &promisc_req) < 0) return -1;
	promisc_req.ifr_flags |= IFF_PROMISC;
	if(control_socket(sockfd, SIOCSIFFLAGS, if_name, &promisc_req) < 0) return -1;

	// Bind the interfaces with the opened sockets
	if ((res = bind_socket(sockfd, &socket_address)) < 0) return -1;

	return res;
}

