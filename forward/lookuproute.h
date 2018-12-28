#ifndef __FIND__
#define __FIND__
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include <arpa/inet.h>


struct route
{
    struct route *next; //下一个路由表项
    struct in_addr ip4prefix;   //目的地址
	unsigned int prefixlen;     //掩码长度
    struct nexthop *nexthop;    //下一跳
};

struct nexthop  //下一跳
{
   struct nexthop *next;    //下一个下一跳
   char *ifname;    //接口名
   unsigned int ifindex;  //zlw ifindex2ifname()获取出接口  接口索引号 
   struct in_addr nexthopaddr;// Nexthop address,下一跳的ip地址
};

struct nextaddr
{
   char *ifname;    //接口名
   struct in_addr ipv4addr; //ip地址
   unsigned int prefixl;
};

extern struct route *route_table; 
int insert_route(unsigned long  ip4prefix,unsigned int prefixlen,char *ifname,unsigned int ifindex,unsigned long  nexthopaddr);
int lookup_route(struct in_addr dstaddr,struct nextaddr *nexthopinfo);
int delete_route(struct in_addr dstaddr,unsigned int prefixlen);
int print_route(struct route *r);
#endif
