#include "arpfind.h"

int arpGet(struct arpmac *srcmac,char *ifname, struct in_addr nexthopaddr)  
{  
    
    // struct arpreq arp_req;
    // struct sockaddr_in *sin;

    // sin = (struct sockaddr_in *)&(arp_req.arp_pa);

    // memset(&arp_req, 0, sizeof(arp_req));
    // sin->sin_family = AF_INET;
    // sin->sin_addr.s_addr = inet_addr(ipStr);
    // strncpy(arp_req.arp_dev, ifname, IF_NAMESIZE - 1);

    // int arp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    // int ret = ioctl(arp_fd, SIOCGARP, &arp_req);

    // if(ret < 0 || !(arp_req.arp_flags & ATF_COM)){
    //     fprintf(stderr, "获取ARP失败，由于 %s @%s : %s \n", ipStr, ifname, strerror(errno));
    //     close(arp_fd);
    //     return -1;
    // }

    // if(arp_req.arp_flags & ATF_COM){
    //     unsigned char *mac = (char *)malloc(sizeof(char)*6);
    //     memcpy(mac,arp_req.arp_ha.sa_data,6);
    //     printf("目标的mac地址是: %02x:%02x:%02x:%02x:%02x:%02x \n",
    //         mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    //     srcmac->mac = mac;
    //     srcmac->index = if_nametoindex(ifname);
    // }else{
    //     printf("未找到mac地址");
    // }

    // close(arp_fd);
    // return 0;  

    printf("in arpget..\n");
    printf("nexthop: %s ifname: %s\n", inet_ntoa(nexthopaddr), ifname);
    struct arpreq arp_req;
    struct sockaddr_in *sin;

    sin = (struct sockaddr_in *)&(arp_req.arp_pa);

    memset(&arp_req, 0, sizeof(arp_req));
    sin -> sin_family = AF_INET;
    sin -> sin_addr = nexthopaddr;

    strcpy(arp_req.arp_dev, ifname);

    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sfd < 0) {
        printf("Get ARP entry failed.\n");
    }
    printf("here1\n");
    int ret = 0;
    ret = ioctl(sfd, SIOCGARP, &arp_req);
    printf("ret=%d\n", ret);
    if (ret < 0 || !(arp_req.arp_flags & ATF_COM)) {
        printf("Get ARP entry failed : %s\n");
        close(sfd);
        return -1;
    }

    printf("here2\n");
    if (arp_req.arp_flags & ATF_COM) {
        srcmac->index = if_nametoindex(ifname);
        memcpy(srcmac, (unsigned char*)arp_req.arp_ha.sa_data, 6);
        // srcmac->mac = srcmac; 
        printf("dst mac addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
               srcmac[0], srcmac[1], srcmac[2], srcmac[3], srcmac[4], srcmac[5]);
    } else {
        printf("dst mac addr not in the ARP cache.\n");
        close(sfd);
        return -1;
    }
    close(sfd);
    return 0;
}  
                                                                                                        
                                                                                                          