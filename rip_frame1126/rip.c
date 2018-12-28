#include "rip.h"

TRtEntry *g_pstRouteEntry = NULL;
TRipPkt *ripReqPkt = NULL;		//rip请求包
TRipPkt *ripRecvPkt = NULL;
TRipPkt *ripUpdatePkt = NULL;
TRipPkt *ripResponsPkt = NULL;

//下面两个数组是一一对应的
char *pcLocalAddr[10]={};//存储本地接口ip地址
char *pcLocalName[10]={};//存储本地接口的接口名
struct in_addr pcLocalMask[10]={};
int sumOfInter = 0;

/*
**请求包是用来组播发送给当前的所有邻居节点的，请求获取他们的路由表信息。
**邻居节点收到之后将会发送respons包过来，包含它的路由表信息。
*/
void requestpkt_Encapsulate()
{
	//封装请求包  command =1,version =2,family =0,metric =16
	ripReqPkt = (struct RipPacket *) malloc(sizeof(struct RipPacket));
	memset(ripReqPkt, 0, sizeof(struct RipPacket));
	ripReqPkt->ucCommand = RIP_REQUEST;
	ripReqPkt->ucVersion = RIP_VERSION;
	ripReqPkt->usZero = 0;
	ripReqPkt->RipEntries[0].usFamily = 0;
	ripReqPkt->RipEntries[0].uiMetric = htonl(RIP_INFINITY);
}


/*****************************************************
*Func Name:    rippacket_Receive  
*Description:  接收rip报文
*Input:        
*	 
*Output: 
*
*Ret  ：
*
*******************************************************/
void rippacket_Receive()
{

	//接收ip设置
	struct sockaddr_in myaddr; 
	memset(&myaddr,0,sizeof(struct sockaddr_in));
	myaddr.sin_family = AF_INET; 
	myaddr.sin_addr.s_addr= htonl(INADDR_ANY); 	//htonl 将主机上的32位数转换为网络字节序
	myaddr.sin_port = htons(RIP_PORT); 

	//创建并绑定socket
	int iRecvfd = socket(AF_INET,SOCK_DGRAM,0);
	if(iRecvfd < 0){
		perror("socket create failed");
	}
	
	/*防止绑定地址冲突，仅供参考*/
	//设置地址重用
	int  iReUseddr = 1;
	if (setsockopt(iRecvfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&iReUseddr,sizeof(iReUseddr))<0)
	{
		perror("setsockopt failed, addr reuse\n");
		return ;
	}
	//设置端口重用
	int  iReUsePort = 1;
	if (setsockopt(iRecvfd,SOL_SOCKET ,SO_REUSEPORT,(const char*)&iReUsePort,sizeof(iReUsePort))<0)
	{
		perror("setsockopt failed, port reuse\n");
		return ;
	}

	if(bind(iRecvfd,(struct sockaddr *)&myaddr, sizeof(struct sockaddr_in))<0)
    {
        perror("bind failed\n");
		return;
    }

	//把本地地址加入到组播中 	
	for(int i = 0; i < sumOfInter; i++){
		struct ip_mreq mreq; 
		mreq.imr_multiaddr.s_addr = inet_addr(RIP_GROUP); 
		mreq.imr_interface.s_addr = inet_addr(pcLocalAddr[i]);
		if (setsockopt(iRecvfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) == -1)
        {
            perror ("join in multicast error\n");
			return;
        }
	}

	int recvLen = 0;
	char recvBuf[1500];
	while(1)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		size_t addrLen = sizeof(struct sockaddr_in);
		memset(&myaddr, 0, sizeof(struct sockaddr_in));
		//接收rip报文   存储接收源ip地址
		recvLen = (int)recvfrom(iRecvfd,&recvBuf,sizeof(recvBuf),0,(struct sockaddr *)&myaddr,(socklen_t *)&addrLen);
		if(recvLen < 0){
			perror("recv failed !\n");
			continue;
		}
		if(recvLen > RIP_MAX_PACKET){
			printf("oh my god, it is too large !\n");
			continue;
		}
		//判断command类型，request 或 response
		ripRecvPkt = (TRipPkt *)&recvBuf;
		printf("收到一个包 , command : %d , version : %d\n",ripRecvPkt->ucCommand,ripRecvPkt->ucVersion);
		if(ripRecvPkt->ucCommand == RIP_REQUEST){

		}else if(ripRecvPkt->ucCommand == RIP_RESPONSE){
			response_Handle(myaddr.sin_addr,recvLen);
		}
		
		//接收到的信息存储到全局变量里，方便request_Handle和response_Handle处理
		
	}

	close(iRecvfd);
	
}


/*****************************************************
*Func Name:    rippacket_Send  
*Description:  向接收源发送响应报文
*Input:        
*	  1.stSourceIp    ：接收源的ip地址，用于发送目的ip设置
*Output: 
*
*Ret  ：
*
*******************************************************/
void rippacket_Send(struct in_addr stSourceIp,int number,int size)
{
	char *localAddr = pcLocalAddr[number];
	struct in_addr mask = pcLocalMask[number];
	bool judge = ((stSourceIp.s_addr & mask.s_addr) == (inet_addr(localAddr) & mask.s_addr));
	if(!judge){
		return;
	}
	struct sockaddr_in myaddr,peeraddr;
	//本地ip设置
	memset(&myaddr,0,sizeof(struct sockaddr_in));
	myaddr.sin_family = AF_INET; 
	myaddr.sin_addr.s_addr= inet_addr(localAddr); 	//htonl 将主机上的32位数转换为网络字节序
	myaddr.sin_port = htons(RIP_PORT); 
	

	//发送目的ip设置
	memset(&peeraddr,0,sizeof(struct sockaddr_in));
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_addr = stSourceIp;
	peeraddr.sin_port = htons(RIP_PORT);
	
	//创建并绑定socket
	int iSendfd = socket(AF_INET,SOCK_DGRAM,0);
	if(iSendfd < 0){
		perror("socket create failed");
	}

	/*防止绑定地址冲突，仅供参考*/
	//设置地址重用
	int  iReUseddr = 1;
	if (setsockopt(iSendfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&iReUseddr,sizeof(iReUseddr))<0)
	{
		perror("setsockopt\n");
		return ;
	}
	//设置端口重用
	int  iReUsePort = 1;
	if (setsockopt(iSendfd,SOL_SOCKET ,SO_REUSEPORT,(const char*)&iReUsePort,sizeof(iReUsePort))<0)
	{
		perror("setsockopt\n");
		return ;
	}


	//发送
	short len = (short)(4 + size * 20);
	if(sendto(iSendfd,ripResponsPkt,(size_t)len,0,
		(struct sockaddr *)&peeraddr,(socklen_t)sizeof(struct sockaddr_in)) < 0){
			perror("send failed !\n");
		}

	close(iSendfd);

	return;	
}

/*****************************************************
*Func Name:    rippacket_Multicast  
*Description:  组播请求报文
*Input:        
*	  1.pcLocalAddr   ：本地ip地址
*Output: 
*
*Ret  ：
*
*******************************************************/
void rippacket_Multicast(char *pcLocalAddr, TRipPkt *pkt, int size)
{

	struct sockaddr_in peeraddr,myaddr; 
	unsigned int socklen = sizeof(struct sockaddr_in);

	//目的ip设置 
	/* 设置对方的端口和IP信息 */
	memset(&peeraddr, 0, socklen);
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_port = htons(RIP_PORT);
	peeraddr.sin_addr.s_addr = inet_addr(RIP_GROUP); 

	//源ip设置 
	/* 设置发送方的端口和IP信息 */
	memset(&myaddr, 0, socklen);
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(RIP_PORT);
	myaddr.sin_addr.s_addr = inet_addr(pcLocalAddr); 

	//创建socket
	int iSendfd = socket(AF_INET, SOCK_DGRAM,0);
	if (iSendfd < 0) {
		printf("socket creating error\n");
		return ;
	}


	/*防止绑定地址冲突，仅供参考*/
	//设置地址重用
	int iReUseddr = 1;
	if (setsockopt(iSendfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&iReUseddr,sizeof(iReUseddr))<0)
	{
		perror("setsockopt:address reuse\n");
		return ;
	}
	//设置端口重用
	int  iReUsePort = 1;
	if (setsockopt(iSendfd,SOL_SOCKET ,SO_REUSEPORT,(const char*)&iReUsePort,sizeof(iReUsePort))<0)
	{
		perror("setsockopt:port reuse\n");
		return ;
	}
	//把本地地址加入到组播
	struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(RIP_GROUP);
    mreq.imr_interface.s_addr = inet_addr(pcLocalAddr);
    if (setsockopt(iSendfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) == -1)
    {
        perror ("join in multicast error\n");
		return;
    }	
	
	/*
	防止组播回环的参考代码	
	*/
	
	//0 禁止回环  1开启回环 
	int loop = 0;
	int err = setsockopt(iSendfd,IPPROTO_IP, IP_MULTICAST_LOOP,&loop, sizeof(loop));
	if(err < 0)
	{
		perror("setsockopt():IP_MULTICAST_LOOP");
		return;
	}

	if (bind(iSendfd, (struct sockaddr*)&myaddr, sizeof(myaddr))<0)
    {
        perror("bind error\n");
		return;
    }

	//发送
    short dataLength = (short)(4 + size * 20);
    if (sendto(iSendfd, pkt, (size_t)dataLength, 0, (struct sockaddr*)&peeraddr, sizeof(peeraddr))<0)
    {
        perror("send request error\n");
		close(iSendfd);
		return;
    }

	close(iSendfd);
}

/*****************************************************
*Func Name:    request_Handle  
*Description:  响应request报文
*Input:        
*	  1.stSourceIp   ：接收源的ip地址
*Output: 
*
*Ret  ：
*
*******************************************************/
void request_Handle(struct in_addr stSourceIp)
{
	//处理request报文
	//遵循水平分裂算法
	//回送response报文，command置为RIP_RESPONSE
	printf("收到来自%s的请求包\n",inet_ntoa(stSourceIp));
	TRtEntry *now = g_pstRouteEntry;
    while(now != NULL)
    {
        ripResponsPkt->ucCommand = RIP_RESPONSE;
        ripResponsPkt->ucVersion = RIP_VERSION;
        ripResponsPkt->usZero = 0;
		int responseSize = 0;
        while(now != NULL && responseSize < RIP_MAX_ENTRY)
        {
            ripResponsPkt->RipEntries[responseSize].stAddr = now->stIpPrefix;
            ripResponsPkt->RipEntries[responseSize].stNexthop = now->stNexthop;
            ripResponsPkt->RipEntries[responseSize].uiMetric = htonl(now->uiMetric);
            ripResponsPkt->RipEntries[responseSize].stPrefixLen = now->uiPrefixLen;
            ripResponsPkt->RipEntries[responseSize].usFamily = htons(2);
            ripResponsPkt->RipEntries[responseSize].usTag = 0;
            responseSize ++;
            now = now->pstNext;
        }
        if(responseSize)
        {
            for(int i = 0; i < sumOfInter ; i ++)
            {
                rippacket_Send(stSourceIp, i, responseSize);
            }
        }
    }
	return;
}

/*****************************************************
*Func Name:    response_Handle  
*Description:  响应response报文
*Input:        
*	  1.stSourceIp   ：接收源的ip地址
*Output: 
*
*Ret  ：
*
*******************************************************/
void response_Handle(struct in_addr stSourceIp, int len)
{
	//处理response报文
	//计算报文中的rip表项个数
	int counter = (len - 4) / 20;
	printf("从 %16s 收到一个response包，其中rip表项个数为 : %d \n",inet_ntoa(stSourceIp),counter);
	int needUpdate = 0;

	for(counter; counter > 0; counter--){
		//初始化
		int find = 0;
		TRtEntry *rip = g_pstRouteEntry;

		struct in_addr dst = ripRecvPkt->RipEntries[counter - 1].stAddr;
		struct in_addr mask = ripRecvPkt->RipEntries[counter - 1].stPrefixLen;
		struct in_addr next = ripRecvPkt->RipEntries[counter - 1].stNexthop;
		unsigned int distance = ntohl(ripRecvPkt->RipEntries[counter - 1].uiMetric) + 1;

		//打印这一个路由表项的信息
		printf("	目的地址=%16s ", inet_ntoa(dst));
        printf("下一跳=%16s ", inet_ntoa(next));
        printf("掩码=%16s 距离=%2d ", inet_ntoa(mask), distance);

		if(distance >= RIP_INFINITY){	//忽略距离超过15跳的目的地址
			printf("忽略\n");
			continue;
		}
		printf("接受\n");

		while(rip->pstNext != NULL){	//遍历当前rip表项,查找可以更新的表项 

			if(isNeighbour(rip->pstNext->stIpPrefix,dst,mask)){
				//设定标记，找到了这一地址
				find = 1;
				
				if(!rip->pstNext->live){	//如果这一表项当前不是活跃的,激活并重新赋值
					rip->pstNext->live = true;
					rip->pstNext->time = 0;
					rip->pstNext->stNexthop = stSourceIp;
					rip->pstNext->uiMetric = distance;
					for(int i = 0; i < sumOfInter; i++){
						struct in_addr interface;
						interface.s_addr = inet_addr(pcLocalAddr[i]);
						if(isNeighbour(stSourceIp,interface,mask)){
							rip->pstNext->pcIfname = pcLocalName[i];
							break;
						}
					}
					route_SendForward(24, rip->pstNext);
					break;
				}
				//否则，是活跃的，那么
				if(distance < rip->pstNext->uiMetric){	//如果当前拿到的response中，度量值小于已知的度量值
					rip->pstNext->stNexthop = stSourceIp;
					rip->pstNext->uiMetric = distance;
					rip->pstNext->time = 0;
					for(int i = 0; i < sumOfInter; i++){
						struct in_addr interface;
						interface.s_addr = inet_addr(pcLocalAddr[i]);
						if(isNeighbour(stSourceIp,interface,mask)){
							rip->pstNext->pcIfname = pcLocalName[i];
							break;
						}
					}
					route_SendForward(24, rip->pstNext);
				}
				break;
			}
			printf("hello \n");
			rip = rip->pstNext;

		}
		printf("world \n");

		if(find == 0){	//找不到可以更新的表项,新建一个rip表项，插入
			rip->pstNext = (TRtEntry *)malloc(sizeof(TRtEntry));
			rip->pstNext->pstNext = NULL;
			rip->pstNext->stIpPrefix.s_addr = dst.s_addr & mask.s_addr;
			rip->pstNext->stNexthop = stSourceIp;
			rip->pstNext->uiMetric = distance;
			rip->pstNext->uiPrefixLen.s_addr = ripRecvPkt->RipEntries[counter - 1].stPrefixLen.s_addr;
			rip->pstNext->live = true;
			rip->pstNext->time = 0;
			for(int i = 0; i < sumOfInter; i++){
				struct in_addr interface;
				interface.s_addr = inet_addr(pcLocalAddr[i]);
				if(isNeighbour(stSourceIp,interface,mask)){
					rip->pstNext->pcIfname = pcLocalName[i];
					break;
				}
			}
			route_SendForward(24, rip->pstNext);
		}
	}


	TRtEntry *item = g_pstRouteEntry;
	while(item->pstNext != NULL){
		if(item->pstNext->time > 6 || item->pstNext->uiMetric >= RIP_INFINITY){
			deleteRip(item);
		}
		item = item->pstNext;
	}

	//打印当前的路由表项
	TRtEntry *entry = g_pstRouteEntry->pstNext;
	printf("打印当前的rip路由表项 ： \n");
	while(entry != NULL){
		printf("	目的地址=%16s ", inet_ntoa(entry->stIpPrefix));
        printf("下一跳=%16s ", inet_ntoa(entry->stNexthop));
        printf("掩码=%16s 距离=%2d, 是否有效=%s\n", inet_ntoa(entry->uiPrefixLen), entry->uiMetric, entry->live?"living":"dead");
		entry = entry->pstNext;
	}
	return;
}

void deleteRip(TRtEntry *father){
	father->pstNext->live = false;
}

bool isNeighbour(struct in_addr a, struct in_addr b, struct in_addr mask){
	return ((a.s_addr & mask.s_addr) == (b.s_addr & mask.s_addr));
}

/*****************************************************
*Func Name:    route_SendForward  
*Description:  响应response报文
*Input:        
*	  1.uiCmd        ：插入命令
*	  2.pstRtEntry   ：路由信息
*Output: 
*
*Ret  ：
*
*******************************************************/
void route_SendForward(unsigned int uiCmd,TRtEntry *pstRtEntry)
{
	//建立tcp短连接，发送插入、删除路由表项信息到转发引擎
	//从原来改良过的quagga中copy过来的代码如下：
	int sendfd;		//用于tcp短连接的套接字
	int sendlen=0;	//发送长度
	int tcpcount=0;	//次数计算
	struct sockaddr_in dst_addr;	//目标地址
	char buf[sizeof(struct selfroute)];

	memset(buf, 0, sizeof(buf));
	struct selfroute *selfrt;
	selfrt = (struct selfroute *)&buf;
	selfrt->selfprefixlen = 24;	//设置掩码长度,need do better
	selfrt->selfprefix	= pstRtEntry->stIpPrefix;		//目的ip地址
	selfrt->selfifindex	= if_nametoindex((const char *)pstRtEntry->pcIfname);		//本地端口号，unsigned int类型
	selfrt->selfnexthop	= pstRtEntry->stNexthop;		//下一跳ip地址
	selfrt->cmdnum        = uiCmd;		//命令编号

	//目标地址初始化
	bzero(&dst_addr, sizeof(struct sockaddr_in)); 
	dst_addr.sin_family = AF_INET; 
	dst_addr.sin_port   = htons(800);
	dst_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if ((sendfd = socket(AF_INET, SOCK_STREAM,0 )) == -1)
	{
		printf("self route sendfd socket error!!\n");
		return ;
	}

	while(tcpcount <6)
	{
		if(connect(sendfd,(struct sockaddr*)&dst_addr,sizeof(dst_addr))<0)
		{
			tcpcount++;
		}
		else {
			break;
		}
		usleep(10000);
		
	}
	if(tcpcount<6)
	{
	sendlen = send(sendfd, buf, sizeof(buf), 0); //struct sockaddr *)&dst_addr, sizeof(struct sockaddr_in));
	if (sendlen <= 0)
	{
		printf("self route sendto() error!!!\n");
		return ;
	}
	if(sendlen >0)
	{
		printf("send ok!!!");
	}
	usleep(50000);
	close(sendfd);
	}
	return;
}

void rippacket_Update()
{
	printf("进入update函数\n");
	//更新所有路由表项的时间,设置超时的rip表项为无用表项
	TRtEntry *e = g_pstRouteEntry->pstNext;
	while(e != NULL){
		if(e->live){
			e->time += 1;
			if(e->time > 5){
				e->live = false;
			}
		}
		e = e->pstNext;
	}

	//遍历rip路由表，封装更新报文
	//注意水平分裂算法    

	TRtEntry *entry = g_pstRouteEntry;
	int update_size = 0;
	
	memset(ripUpdatePkt, 0, sizeof(struct RipPacket));

	ripUpdatePkt->ucCommand = RIP_RESPONSE;
	ripUpdatePkt->ucVersion = RIP_VERSION;
	ripUpdatePkt->usZero = 0;

	for(int i = 0; i < sumOfInter ; i++){
		struct in_addr inter;
		inter.s_addr = inet_addr(pcLocalAddr[i]);
		printf("当前要发送的接口: %16s \n",inet_ntoa(inter));
		while(entry->pstNext != NULL){
			if(entry->pstNext->live){
				if(!isNeighbour(entry->pstNext->stNexthop,inter,pcLocalMask[i])){
					ripUpdatePkt->RipEntries[update_size].stAddr = entry->pstNext->stIpPrefix;
					ripUpdatePkt->RipEntries[update_size].stPrefixLen = entry->pstNext->uiPrefixLen;
					ripUpdatePkt->RipEntries[update_size].stNexthop = entry->pstNext->stNexthop;
					ripUpdatePkt->RipEntries[update_size].uiMetric = htonl(entry->pstNext->uiMetric);
					ripUpdatePkt->RipEntries[update_size].usFamily = htons(2);
					ripUpdatePkt->RipEntries[update_size].usTag = 0;
					update_size ++;
				}
			}
			entry = entry->pstNext;
		}
		if(update_size > 0){
			rippacket_Multicast(pcLocalAddr[i],ripUpdatePkt,update_size);
		}
	}
}

/*
	发送请求包
*/
void sendReq(){
	requestpkt_Encapsulate();
	for(int i = 0; i < sumOfInter ; i++){
		rippacket_Multicast(pcLocalAddr[i],ripReqPkt,1);
	}
}

void update(){
	while(1){
		sleep(30);
		rippacket_Update();
	}
}


void ripdaemon_Start()
{
	//创建更新线程，30s更新一次,向组播地址更新Update包
    pthread_t tid1;
    int ret1 = pthread_create((pthread_t *)&tid1, NULL, (void *)update, NULL);

	//封装请求报文，并组播
    pthread_t tid2;
    int ret2 = pthread_create((pthread_t *)&tid2, NULL, (void *)sendReq, NULL);
    
	//接收rip报文
	rippacket_Receive();

	return;
}


void routentry_Insert()
{
	//将本地接口表添加到rip路由表里
	// g_pstRouteEntry->pstNext = (TRtEntry *)malloc(sizeof(TRtEntry));
	TRtEntry* routeEntry = g_pstRouteEntry;
	g_pstRouteEntry->num = 0;
	printf("初始化，在rip表中插入本地接口的信息： \n");
	for(int i = 0; i < sumOfInter ; i++){

		routeEntry->pstNext = (TRtEntry *)malloc(sizeof(TRtEntry));
		routeEntry = routeEntry->pstNext;
		g_pstRouteEntry->num ++;

		routeEntry->pcIfname = pcLocalName[i];
		routeEntry->stIpPrefix.s_addr = inet_addr((const char *)pcLocalAddr[i]) & pcLocalMask[i].s_addr;
		routeEntry->stNexthop.s_addr = inet_addr("0.0.0.0");
		routeEntry->uiPrefixLen = pcLocalMask[i];
		routeEntry->uiMetric = 1;
		routeEntry->pstNext = NULL;
		routeEntry->live = true;
		routeEntry->time = 0;


		printf("	接口名称及ip地址: %s, %s \n",pcLocalName[i],pcLocalAddr[i]);

		route_SendForward(24,routeEntry);
	}

	routeEntry->pstNext = (TRtEntry *)malloc(sizeof(TRtEntry));
	routeEntry = routeEntry->pstNext;
	g_pstRouteEntry->num ++;

	routeEntry->stIpPrefix.s_addr = inet_addr("192.168.11.0");
	routeEntry->stNexthop.s_addr = inet_addr("0.0.0.0");
	routeEntry->uiPrefixLen.s_addr = inet_addr("255.255.255.0");
	routeEntry->uiMetric = 1;
	routeEntry->pstNext = NULL;
	routeEntry->live = true;
	routeEntry->time = 0;


	printf("	插入一条rip表项: 192.168.11.0/24, 0.0.0.0 \n");

	return ;
}

void check_interface(){

	struct ifaddrs *pstIpAddrStruct = NULL;
	struct ifaddrs *pstIpAddrStCur  = NULL;
	void *pAddrPtr=NULL;
	const char *pcLo = "127.0.0.1";
	
	getifaddrs(&pstIpAddrStruct); //linux系统函数
	pstIpAddrStCur = pstIpAddrStruct;
	
	int i = 0;
	while(pstIpAddrStruct != NULL)
	{
		if(pstIpAddrStruct->ifa_addr->sa_family==AF_INET)
		{
			pAddrPtr = &((struct sockaddr_in *)pstIpAddrStruct->ifa_addr)->sin_addr;
			char cAddrBuf[INET_ADDRSTRLEN];
			memset(&cAddrBuf,0,sizeof(INET_ADDRSTRLEN));
			inet_ntop(AF_INET, pAddrPtr, cAddrBuf, INET_ADDRSTRLEN);	//端口名转换为端口号
			if(strcmp((const char*)&cAddrBuf,pcLo) != 0)
			{
				//check in rip table
				//if new one, also add into pcLocalAddr table
				struct in_addr interface;
				interface.s_addr = inet_addr((const char*)&cAddrBuf);
				TRtEntry *entry = g_pstRouteEntry;
				int mark = 0;
				while(entry->pstNext != NULL){
					if(isNeighbour(entry->stIpPrefix, interface, entry->uiPrefixLen)){
						entry->live = true;
						entry->time = 0;
						mark = 1;
						break;
					}
					entry = entry->pstNext;
				}
				if(mark == 0){	//new one 
					pcLocalAddr[sumOfInter] = (char *)malloc(sizeof(INET_ADDRSTRLEN));
					pcLocalName[sumOfInter] = (char *)malloc(sizeof(IF_NAMESIZE));
					pcLocalMask[sumOfInter] = ((struct sockaddr_in *)(pstIpAddrStruct->ifa_netmask))->sin_addr;
					strcpy(pcLocalAddr[sumOfInter],(const char*)&cAddrBuf);	//获取对应的接口的ip
					strcpy(pcLocalName[sumOfInter],(const char*)pstIpAddrStruct->ifa_name);	//获取对应的接口的名字
					sumOfInter ++;

					TRtEntry *toInsert = (TRtEntry *)malloc(sizeof(TRtEntry));
					g_pstRouteEntry->num ++;

					toInsert->pcIfname = pcLocalName[i];
					toInsert->stIpPrefix.s_addr = inet_addr((const char *)pcLocalAddr[i]) & pcLocalMask[i].s_addr;
					toInsert->stNexthop.s_addr = inet_addr("0.0.0.0");
					toInsert->uiPrefixLen = pcLocalMask[i];
					toInsert->uiMetric = 1;
					toInsert->pstNext = NULL;
					toInsert->live = true;
					toInsert->time = 0;

					toInsert->pstNext = g_pstRouteEntry->pstNext;
					g_pstRouteEntry->pstNext = toInsert;
				}
			}	
		}
		pstIpAddrStruct = pstIpAddrStruct->ifa_next;
	}
	freeifaddrs(pstIpAddrStCur);//linux系统函数
	return ;
}

void checkInterf(){
	while(1){
		sleep(5);
		check_interface();
	}
}

//这个函数已经实现好了，获取本地接口的ip地址和接口名称
void localinterf_GetInfo()
{
	struct ifaddrs *pstIpAddrStruct = NULL;
	struct ifaddrs *pstIpAddrStCur  = NULL;
	void *pAddrPtr=NULL;
	const char *pcLo = "127.0.0.1";
	
	getifaddrs(&pstIpAddrStruct); //linux系统函数
	pstIpAddrStCur = pstIpAddrStruct;
	
	int i = 0;
	while(pstIpAddrStruct != NULL)
	{
		if(pstIpAddrStruct->ifa_addr->sa_family==AF_INET)
		{
			pAddrPtr = &((struct sockaddr_in *)pstIpAddrStruct->ifa_addr)->sin_addr;
			char cAddrBuf[INET_ADDRSTRLEN];
			memset(&cAddrBuf,0,sizeof(INET_ADDRSTRLEN));
			inet_ntop(AF_INET, pAddrPtr, cAddrBuf, INET_ADDRSTRLEN);	//端口名转换为端口号
			if(strcmp((const char*)&cAddrBuf,pcLo) != 0)
			{
				pcLocalAddr[i] = (char *)malloc(sizeof(INET_ADDRSTRLEN));
				pcLocalName[i] = (char *)malloc(sizeof(IF_NAMESIZE));
				pcLocalMask[i] = ((struct sockaddr_in *)(pstIpAddrStruct->ifa_netmask))->sin_addr;
				strcpy(pcLocalAddr[i],(const char*)&cAddrBuf);	//获取对应的接口的ip
				strcpy(pcLocalName[i],(const char*)pstIpAddrStruct->ifa_name);	//获取对应的接口的名字
				i++;
				sumOfInter ++;
			}	
		}
		pstIpAddrStruct = pstIpAddrStruct->ifa_next;
	}
	freeifaddrs(pstIpAddrStCur);//linux系统函数
	return ;
}

int main(int argc,char* argv[])
{
	g_pstRouteEntry = (TRtEntry *)malloc(sizeof(TRtEntry));
	if(g_pstRouteEntry == NULL)
	{
		perror("g_pstRouteEntry malloc error !\n");
		return -1;
	}
	ripUpdatePkt = (TRipPkt *)malloc(sizeof(TRipPkt));
	if(ripUpdatePkt == NULL){
		perror("update package malloc error !\n");
		return -1;
	}
	localinterf_GetInfo();	//获取本地接口的名称和网址
	routentry_Insert();		//插入静态路由信息
	ripdaemon_Start();
	return 0;
}

/*
localhost.localdomain# con t
localhost.localdomain(config)# router rip
localhost.localdomain(config-router)# network 192.168.1.1/24 
localhost.localdomain(config-router)# redistribute  static
localhost.localdomain(config-router)# end
localhost.localdomain# sho ip rip  
*/