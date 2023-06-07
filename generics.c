#include "generics.h"

/**
 * Opens the socket.
 * 
 * Returns its index if success or -1 if failure.
*/
int open_socket () {
	int sockfd = socket(AF_PACKET, SOCK_RAW, htons(DEF_ETHER_TYPE));
	if (sockfd < 0) {
		printf("[Open socket failed] Due to: %s \n", strerror(errno));
		return -1;
	}
	return sockfd;
}

/**
 * Binds the socket to and address.
 * 
 * 	- socket: Integer representing the socket.
 *  - addr: Pointer to a sockaddr_ll struct representing
 * 	the socket address.
 * 
 * Returns >0 if success or -1 if failure.
*/
int bind_socket (int socket, struct sockaddr_ll *addr) {
	int res = 0; 	
	if ((res = bind(socket, (struct sockaddr *) addr, sizeof(*addr))) < 0) {
        printf("[Set bind failed] Due to: %s\n", strerror(errno));
		close(socket);
		exit(EXIT_FAILURE);
    }
	return res;
}

/**
 * Calls the ioctl system function to communicate with the io driver.
 * 
 * 	- socket: Integer representing the socket.
 * 	- request: Long that indicates the request to the driver.
 * 	- if_name: Name of the interface.
 * 	- if_req: If req returned as a result.
 * 
 * Returns >0 if success or -1 if failure.
*/
int control_socket (int socket, unsigned long request, char *if_name, struct ifreq *if_req) {
	int res = 0;
	if ((res = ioctl(socket, request, if_req)) < 0) {
	    printf("[Get interface data failed] Interface: %s, Request: %lu, Due to: %s \n", if_name, request, strerror(errno));
	}
	return res;
}
