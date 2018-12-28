#ifndef __MYRIP_H
#define __MYRIP_H
#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define RIP_VERSION    	2	//rip版本
#define RIP_REQUEST    	1	//代表rip请求报文
#define RIP_RESPONSE   	2	//代表rip反馈报文
#define RIP_INFINITY  	16	//代表无穷远的路由距离，单位为跳数
#define RIP_PORT		520	//rip的端口
#define RIP_PACKET_HEAD	4	//rip报头长度
#define RIP_MAX_PACKET  504
#define RIP_MAX_ENTRY   25
#define ROUTE_MAX_ENTRY 256
#define RIP_GROUP		"224.0.0.9"

#define RIP_CHECK_OK    1
#define RIP_CHECK_FAIL  0
#define AddRoute        24
#define DelRoute        25

//RIP报文表项的结构体
typedef struct RipEntry
{
	unsigned short usFamily;
	unsigned short usTag;
	struct in_addr stAddr;
	struct in_addr stPrefixLen;
	struct in_addr stNexthop;	//下一跳
	unsigned int uiMetric;
}TRipEntry;

//RIP报文的头部
typedef struct  RipPacket
{
	unsigned char ucCommand;
	unsigned char ucVersion;
	unsigned short usZero; 
	TRipEntry RipEntries[RIP_MAX_ENTRY];
}TRipPkt;

//自己构建的rip路由表
typedef struct RouteEntry
{
	struct RouteEntry *pstNext;	//指向下一个rip路由表项
	struct in_addr stIpPrefix; 	//目的ip地址
	struct in_addr  uiPrefixLen;	//掩码长度
	struct in_addr stNexthop;	//下一跳路由器的ip地址
	unsigned int   uiMetric;	//目的地址到当前路由器距离的度量值
	char  *pcIfname;	//本路由器中对应的接口名字
	unsigned int num;
	unsigned int time;
	bool live;
}TRtEntry;

//发送给转发引擎结构体
typedef struct SockRoute
{
	unsigned int uiPrefixLen;
	struct in_addr stIpPrefix;
	unsigned int uiIfindex;
	struct in_addr stNexthop;
	unsigned int uiCmd;		//插入 or 删除的命令编号
}TSockRoute;

struct selfroute 
{
	unsigned char selfprefixlen;
	struct in_addr selfprefix;
	unsigned int selfifindex;
	struct in_addr selfnexthop;
	unsigned int cmdnum;
	char ifname[10];
};


void route_SendForward(unsigned int uiCmd,TRtEntry *pstRtEntry);
void requestpkt_Encapsulate();
void rippacket_Receive();
void rippacket_Send(struct in_addr stSourceIp,int number,int size);
void rippacket_Multicast(char *pcLocalAddr, TRipPkt *pkt, int size);
void request_Handle(struct in_addr stSourceIp);
void response_Handle(struct in_addr stSourceIp, int len);
void rippacket_Update();
void routentry_Insert();
void localinterf_GetInfo();
void ripdaemon_Start();
void deleteRip(TRtEntry *father);
bool isNeighbour(struct in_addr a, struct in_addr b, struct in_addr mask);

#endif

