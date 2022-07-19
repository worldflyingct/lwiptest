#include "fcntl.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "netif/etharp.h"
#include "sys/types.h"
#include "tapdriver.h"
#include <pthread.h>
#include <string.h>

int tapfd;
struct netif netif;
struct tcp_pcb *tcp_client;

static err_t low_tap_output(struct netif *netif, struct pbuf *p)
{
    char buff[1005];
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
    unsigned char buff[1005];
    while (1)
    {
        ssize_t len = read(tapfd, buff, 1005);
        if (len < 0)
        {
            continue;
        }
        struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
        if (p == NULL)
        {
            continue;
        }
        pbuf_take(p, buff, len);
        netif.input(p, &netif); // p在netif_input内已经释放了，无需再调用pbuf_free(p)
    }
    return NULL;
}

void *checktimeout()
{
    while (1)
    {
        usleep(100000);
        sys_check_timeouts();
    }
}

static err_t eth_init(struct netif *netif)
{
    netif->name[0] = 'w';
    netif->name[1] = 'f';
    netif->output = etharp_output;
    netif->linkoutput = low_tap_output;
    netif->mtu = 1005;
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

char ishead = 1;
/* TCP客户端接收到数据后的数据处理回调函数 */
static err_t TCPClientCallback(void *arg, struct tcp_pcb *tcp_client, struct pbuf *tcp_recv_pbuf, err_t err)
{
    struct pbuf *tcp_send_pbuf;
    char echoString[] = "This is the server content echo:";

    if (tcp_recv_pbuf != NULL)
    {

        char buff[1018];
        int len = 0;
        for (struct pbuf *q = tcp_recv_pbuf; q != NULL; q = q->next)
        {
            memcpy(buff + len, q->payload, q->len);
            len += q->len;
        }
        printf("len:%d, in %s, at %d\n", len, __FILE__, __LINE__);
        if (len < 4)
        {
            printf("http data error\n");
            // pbuf_free(tcp_recv_pbuf);
            tcp_close(tcp_client);
            return;
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
                // pbuf_free(tcp_recv_pbuf);
                tcp_close(tcp_client);
                return;
            }
        }
        int fd = open("abc.mp4", O_WRONLY | O_CREAT | O_APPEND, 0777);
        write(fd, ota_data, ota_data_len);
        close(fd);
        /* 更新接收窗口 */
        tcp_recved(tcp_client, len);
        pbuf_free(tcp_recv_pbuf);
    }
    else if (err == ERR_OK)
    {
        printf("in %s, at %d\n", __FILE__, __LINE__);
        tcp_close(tcp_client);
        return ERR_OK;
    }
    else
    {
        printf("err:%d, in %s, at %d\n", err, __FILE__, __LINE__);
    }

    return ERR_OK;
}

static err_t TCPClientConnected(void *arg, struct tcp_pcb *tcp_client, err_t err)
{
    printf("in %s, at %d\n", __FILE__, __LINE__);
    char clientString[] = "GET /852369.mp4 HTTP/1.1\r\n"
                          "Host: 192.168.37.1\r\n"
                          "Connection: keep-alive\r\n\r\n";
    tcp_recv(tcp_client, TCPClientCallback); // 配置接收回调函数
    tcp_write(tcp_client, clientString, strlen(clientString), TCP_WRITE_FLAG_COPY);
    // tcp_output(tcp_client);
    return ERR_OK;
}

/* TCP客户端连接服务器错误回调函数 */
static void TCPClientConnectError(void *arg, err_t err)
{
    printf("connect error: %d\n", err);
}

void main(void)
{
    tapfd = tap_alloc();
    pthread_t th;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&th, &attr, low_tap_input, NULL);
    pthread_attr_destroy(&attr);

    ip4_addr_t addr;
    ip4_addr_t netmask;
    ip4_addr_t gw;
    IP4_ADDR(&addr, 192, 168, 37, 84);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 37, 1);
    lwip_init();
    netif_add(&netif, &addr, &netmask, &gw, &netif, eth_init, netif_input);
    netif_set_default(&netif);
    netif_set_up(&netif);
    netif_set_link_up(&netif);
    ip4_addr_t dns;
    IP4_ADDR(&dns, 114, 114, 114, 114);
    dns_setserver(0, &dns);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&th, &attr, checktimeout, NULL);
    pthread_attr_destroy(&attr);
    sleep(5);
    unlink("abc.mp4");
    // tcp发送
    tcp_client = tcp_new();
    tcp_client->flags |= TF_NODELAY;
    tcp_bind(tcp_client, IP_ADDR_ANY, 0);
    ip_addr_t remoteip;
    IP4_ADDR(&remoteip, 192, 168, 37, 1);
    printf("in %s, at %d\n", __FILE__, __LINE__);
    tcp_err(tcp_client, TCPClientConnectError); // 连接错误处理回调
    printf("in %s, at %d\n", __FILE__, __LINE__);
    int err = tcp_connect(tcp_client, &remoteip, 80, TCPClientConnected); // 连接完成处理回调
    printf("err:%d, in %s, at %d\n", err, __FILE__, __LINE__);
    pause();
}
