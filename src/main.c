#include "fcntl.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "netif/etharp.h"
#include "sys/types.h"
#include "tapdriver.h"
#include <pthread.h>
#include <string.h>

int tapfd;
struct netif netif;

static err_t low_tap_output(struct netif *netif, struct pbuf *p)
{
    char buff[1518];
    int len = 0;
    for (struct pbuf *q = p; q != NULL; q = q->next)
    {
        memcpy(buff + len, q->payload, q->len);
        len += q->len;
    }
    write(tapfd, buff, len);
    return ERR_OK;
}

void *low_tap_input()
{
    unsigned char buff[1518];
    while (1)
    {
        ssize_t len = read(tapfd, buff, 1518); // 接收底层物理数据
        if (len < 0)
        {
            printf("in %s, at %d\n", __FILE__, __LINE__);
            continue;
        }
        struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
        if (p == NULL)
        {
            printf("in %s, at %d\n", __FILE__, __LINE__);
            continue;
        }
        pbuf_take(p, buff, len);
        printf("in %s, at %d\n", __FILE__, __LINE__);
        netif.input(p, &netif); // p在netif_input内已经释放了，无需再调用pbuf_free(p)
    }
    return NULL;
}

static err_t eth_init(struct netif *netif)
{
    netif->name[0] = 'w';
    netif->name[1] = 'f';
    netif->output = etharp_output;
    netif->linkoutput = low_tap_output;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0x23;
    netif->hwaddr[2] = 0xC1;
    netif->hwaddr[3] = 0xDE;
    netif->hwaddr[4] = 0xD0;
    netif->hwaddr[5] = 0x0D;
    netif->hwaddr_len = 6;

    return ERR_OK;
}

void main(void)
{
    tapfd = tap_alloc();
    printf("tapfd:%d, in %s, at %d\n", tapfd, __FILE__, __LINE__);
    pthread_t th;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&th, &attr, low_tap_input, NULL);
    pthread_attr_destroy(&attr);
/*
    ip4_addr_t addr;
    ip4_addr_t netmask;
    ip4_addr_t gw;
    IP4_ADDR(&addr, 192, 168, 37, 84);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 37, 1);
    tcpip_init(NULL, NULL);
    netif_add(&netif, &addr, &netmask, &gw, &netif, eth_init, tcpip_input);
    netif_set_default(&netif);
    netif_set_up(&netif);
    netif_set_link_up(&netif);
    ip4_addr_t dns;
    IP4_ADDR(&dns, 114, 114, 114, 114);
    dns_setserver(0, &dns);

    sleep(5);
    unlink("abc.mp4");
    // tcp发送
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(80);
    clientAddr.sin_addr.s_addr = inet_addr("192.168.37.1");
    if(connect(fd, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        printf("connect error");
        return 1;
    }
    char clientString[] = "GET /852369.mp4 HTTP/1.1\r\n"
                          "Host: 192.168.37.1\r\n"
                          "Connection: keep-alive\r\n\r\n";
    send(fd, clientString, strlen(clientString), 0);
    char buff[1024];
    char ishead = 1;
    while (1) {
        ssize_t len = recv(fd, buff, 1024, 0);
        if (len < 0) {
            break;
        }
        char *ota_data = buff;
        int ota_data_len = len;
        if (ishead)
        {
            int endstep = len - 4;
            for (int i = 0; i < endstep; i++)
            {
                if (buff[i] == '\r' && buff[i + 1] == '\n' && buff[i + 2] == '\r' && buff[i + 3] == '\n')
                {
                    i += 4;
                    ota_data = buff + i;
                    ota_data_len = len - i;
                    ishead = 0;
                    break;
                }
            }
            if (ishead)
            {
                printf("http data error\n");
                break;
            }
        }
        int fd2 = open("abc.mp4", O_WRONLY | O_CREAT | O_APPEND, 0777);
        write(fd2, ota_data, ota_data_len);
        close(fd2);
    }
    close(fd);
    */
    pause();
}
