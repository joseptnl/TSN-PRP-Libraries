#include "packetio.h"

/**
 * Intilizes the interface binding it to a socket an obtaining
 * its mac address. Because the if is fullduplex the configuration
 * is done for both sending and receiving purposes. Is worth notice
 * to say that the only difference is to activate the PROMISCUOS
 * mode.
 * 
 * 	- if_name: Pointer to char with the name of the if.
 * 	- src_mac: Pointer to unsigned char, must be initialized
 * 	with at least 6 bytes in memory to be able to contain the
 * 	mac address.
 * 	- eth_type: integer of 2 bytes representing the used ethernet
 * 	protocol.
 * 
 * Returns socket identifier if success or -1 if failure.
*/
int init_interface (char *if_name, uint16_t eth_type, unsigned char *src_mac) {
	int socket = 0;

	if ((socket = open_socket()) < 0) return -1;

	struct sockaddr_ll socket_address;
	memset(&socket_address, 0, sizeof(struct sockaddr_ll));
	socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(eth_type);
    socket_address.sll_ifindex = if_nametoindex(if_name);
	// Bind the interfaces with the opened sockets
	if (bind_socket(socket, &socket_address) < 0) return -1;
	
	// Set promiscuos mode
	struct ifreq ifr;
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);
	if(control_socket(socket, SIOCGIFFLAGS, &ifr) < 0) return -1;
	ifr.ifr_flags |= IFF_PROMISC;
	if(control_socket(socket, SIOCSIFFLAGS, &ifr) < 0) return -1;

	// Set reusable socket
	uint8_t true = 1;
	setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(uint8_t));

	if(src_mac != NULL) {
	  	// Get the MAC address
		if (control_socket(socket, SIOCGIFHWADDR, &ifr) < 0) return -1;

		for (int i = 0; i < 6; i++) {
			*(src_mac + i) = ifr.ifr_hwaddr.sa_data[i];
		}
	}

	return socket;
}

/**
 * Terminates with the opened interface deleting its promiscuous
 * mode and closing the socket.
 * 
 * 	- sockfd: Integer representing the socket.
 * 
 * Returns 0 if success or -1 if failure.
 */
int end_interface (int sockfd, char *if_name) {
	if (sockfd < 0) return -1;

	// Close socket
	close(sockfd);

	return 0;
}

/**
 * Sends the frame writing it in the socket buffer.
 * 
 * 	- sockfd: Integer representing the socket.
 *  - frame: Pointer to char containing the frame to send.
 *  - frame_len: Integer with the length of the frame to
 * 	send.
 * 
 * Returns number of bytes sended if success or -1 if failure.
*/
uint64_t send_frame (int sockfd, char *frame, uint16_t frame_len) {
	uint64_t res = 0;

	if ((res = write(sockfd, (void *) frame, (size_t) frame_len)) < 0) {
		printf("[Send failed] Due to: %s \n", strerror(errno));
	}

	return res;
}

/**
 * Receives the frame with a passive blocking call. That means,
 * this function will just return when a frame is received or
 * and error ocurred.
 * 
 * 	- sockfd: Integer representing the socket.
 *  - rec_buffer: Pointer to char that will act as a received
 * 	frame storage.
 *  - frame_len: Integer with the length of the frame to
 * 	receive.
 * 
 * Returns number of bytes received if success or -1 if failure.
*/
uint64_t receive_frame (int sockfd, char *rec_buffer, uint16_t frame_len) {
	uint64_t numbytes = 0;

	if ((numbytes = recv(sockfd, rec_buffer, frame_len, 0)) < 0) {
		printf("[Receive failed] Due to: %s \n", strerror(errno));
	}

	return numbytes;
}