#ifndef __ARP__
#define __ARP__
#include <stdio.h>   
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <sys/ioctl.h>  
#include <net/if_arp.h>  
#include <string.h> 
#include <net/if.h>
#include <errno.h>

//给出了包含mac表的结构
struct arpmac
{
    unsigned char * mac;
    unsigned int index;
};


int arpGet(struct arpmac *srcmac,char *ifname, struct in_addr nexthopaddr);

#endif 
