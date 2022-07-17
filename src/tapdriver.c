#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
// 包入tun相关的头部
#include <linux/if_tun.h>
#include <net/if.h>
// 包入网络相关的头部
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

int tap_alloc()
{
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0)
    {
        printf("open tun node fail, in %s, at %d\n", __FILE__, __LINE__);
        return -1;
    }
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strcpy(ifr.ifr_name, "lwiptest");
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0)
    {
        printf("ioctl tun node fail, in %s, at %d\n", __FILE__, __LINE__);
        close(fd);
        return -2;
    }
    printf("create tap device success, in %s, at %d\n", __FILE__, __LINE__);
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ioctl(socket_fd, SIOCGIFFLAGS, (void *)&ifr) < 0)
    {
        printf("ioctl SIOCGIFFLAGS fail, in %s, at %d\n", __FILE__, __LINE__);
        close(socket_fd);
        close(fd);
        return -3;
    }
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(socket_fd, SIOCSIFFLAGS, &ifr) < 0)
    {
        printf("up tap device fail, in %s, at %d\n", __FILE__, __LINE__);
        close(socket_fd);
        close(fd);
        return -4;
    }
    printf("up tap device success, in %s, at %d\n", __FILE__, __LINE__);
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    unsigned char ip[] = {192, 168, 37, 1};
    memcpy(&sin.sin_addr, ip, 4);
    memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    if (ioctl(socket_fd, SIOCSIFADDR, &ifr) < 0)
    {
        printf("set ip addr for tap device fail, in %s, at %d\n", __FILE__, __LINE__);
        close(socket_fd);
        close(fd);
        return -5;
    }
    printf("set ip addr for tap device to %d.%d.%d.%d success, in %s, at %d\n", ip[0], ip[1], ip[2], ip[3], __FILE__,
           __LINE__);
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    unsigned char mask[] = {255, 255, 255, 0};
    memcpy(&sin.sin_addr, mask, 4);
    memcpy(&ifr.ifr_netmask, &sin, sizeof(struct sockaddr));
    if (ioctl(socket_fd, SIOCSIFNETMASK, &ifr) < 0)
    {
        printf("set netmask for tap device fail, in %s, at %d\n", __FILE__, __LINE__);
        close(socket_fd);
        close(fd);
        return -6;
    }
    printf("set netmask for tap device to %d.%d.%d.%d success, in %s, at %d\n", mask[0], mask[1], mask[2], mask[3],
           __FILE__, __LINE__);
    close(socket_fd);
    return fd;
}
