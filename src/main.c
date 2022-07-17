#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "tapdriver.h"
#include <pthread.h>
#include <string.h>

int tapfd;
struct netif netif;

static err_t low_tap_output(struct netif *netif, struct pbuf *p)
{
    /*
    char buff[1018];
    int boffset = 0;
    for (struct pbuf *q = p; q != NULL; q = q->next)
    {
        memcpy(buff + boffset, q->payload, q->len);
        boffset += q->len;
    }
    printf("len:%d\n", boffset);
    write(tapfd, buff, boffset);
    */
    for (struct pbuf *q = p; q != NULL; q = q->next)
    {
        write(tapfd, q->payload, q->len);
        printf("len:%d\n", q->len);
    }
    return ERR_OK;
}

void *low_tap_input()
{
    unsigned char buff[1018];
    while (1)
    {
        ssize_t len = read(tapfd, buff, 1018);
        if (len < 0)
        {
            continue;
        }
        struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
        if (p == NULL)
        {
            continue;
        }
        /*
        for (int i = 0; i < len; i++)
        {
            if (i % 16 == 0)
            {
                printf("\n");
            }
            printf("0x%02x ", buff[i]);
        }
        */
        printf("\n");
        pbuf_take(p, buff, len);
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
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP;
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
    pthread_t th;
    pthread_create(&th, NULL, low_tap_input, NULL);

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
    while (1)
    {
        usleep(10000);
        sys_check_timeouts();
    }
}
