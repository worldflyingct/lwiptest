#ifndef LWIP_HDR_LWIPOPTS_H
#define LWIP_HDR_LWIPOPTS_H

#define LWIP_DEBUG 1
#define TCPIP_DEBUG 1

#define MEM_ALIGNMENT 4

#define NO_SYS 0
#define SYS_LIGHTWEIGHT_PROT 0

#define LWIP_PROVIDE_ERRNO 1

#define LWIP_NETCONN 0
#define LWIP_SOCKET 1

#define LWIP_ETHERNET 1
#define LWIP_ARP 1

#define LWIP_DHCP 1
#define LWIP_AUTOIP 0

#define LWIP_IGMP 1
#define LWIP_DNS 1

#define MEM_SIZE 4096
#define TCP_SND_QUEUELEN 6

#define TCP_SNDLOWAT 1002
#define PBUF_POOL_SIZE 5
#endif
