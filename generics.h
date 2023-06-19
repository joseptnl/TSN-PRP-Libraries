
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

/* Multicast address */
#define MY_DEST_MAC0	0x01 
#define MY_DEST_MAC1	0x02
#define MY_DEST_MAC2	0x03
#define MY_DEST_MAC3	0x04
#define MY_DEST_MAC4	0x05
#define MY_DEST_MAC5	0x06

#define DEF_ETHER_TYPE	0x8000

#define BILLION 	1000000000L

int open_socket ();
int bind_socket (int socket, struct sockaddr_ll *addr);
int control_socket (int socket, unsigned long request, struct ifreq *if_req);