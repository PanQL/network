#include "checksum.h"
#include "lookuproute.h"
#include "arpfind.h"
#include "sendetherip.h"
#include "recvroute.h"
#include <pthread.h>
#include <net/if.h>

#define IP_HEADER_LEN sizeof(struct ip)
#define ETHER_HEADER_LEN sizeof(struct ether_header)


//接收路由信息的线程
void *thr_fn(void *arg)
{
	int st=0;
	struct selfroute *selfrt; 
	selfrt = (struct selfroute*)malloc(sizeof(struct selfroute));
	memset(selfrt,0,sizeof(struct selfroute));

	//get if.name
	struct if_nameindex *head, *ifni;
	ifni = if_nameindex();
  	head = ifni;
	char *ifname;


	bzero(&my_addr,sizeof(struct sockaddr_in));
	my_addr.sin_family=AF_INET; //设置为IP通信
	my_addr.sin_addr.s_addr=inet_addr("127.0.0.1");//服务器IP地址--允许连接到所有本地地址上
	my_addr.sin_port=htons(800); //服务器端口号

	/*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
	if((server_sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{  
		printf("socket error\n");
	}
    printf("socket is ok\n");

	/*将套接字绑定到服务器的网络地址上*/
	if(bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(my_addr))<0)
	{
		printf("bind error\n");
	}
    printf("bind is ok\n");

	/*监听client_sockfd=accept(server_sockfd,(struct sockaddr *)NULL,NULL)连接请求--监听队列长度为5*/
	if(listen(server_sockfd,5)<0)
	{
		printf("listen error\n");
	};
    printf("listen is ok\n");

	// add-24 del-25
	while(1)
	{
		st=static_route_get(selfrt);
		if(st == 1)
		{
			if(selfrt->cmdnum == 24)
			{
				printf("To insert a route !\n");
				while(ifni->if_index != 0) {
					if(ifni->if_index==selfrt->ifindex)
					{
						printf("if_name is %s\n",ifni->if_name);
						ifname = ifni->if_name;
						break;
					}
					ifni++;
				}

				{
					//插入到路由表里
					printf("Enter insert !!\n");
					printf("result of ne is %x .\n",selfrt->prefix.s_addr);
					insert_route(selfrt->prefix.s_addr,selfrt->prefixlen,selfrt->ifname,
								selfrt->ifindex,selfrt->nexthop.s_addr);
				}
			}
			else if(selfrt->cmdnum == 25)
			{
				//从路由表里删除路由
				printf("To delete a route !\n");
				int res = delete_route(selfrt->prefix,selfrt->prefixlen);
				if(res == 0)
					printf("fail to delete it.\n");
				else
					printf("success to delete it.\n");
			}else{
				printf("recive the other cmd num %d.\n",selfrt->cmdnum);
			}
			// printf("=============打印当前的路由表 \n");
			// struct route *entry = route_table->next;
			// while(entry != NULL){
			// 	print_route(entry);
			// 	entry = entry->next;
			// }
			// printf("=============打印完毕 \n");
		}
	}

}

void analyseIP(struct ip* ip_recvpkt){
	char IPdotdecSrc[20]; // 存放点分十进制IP地址
	struct in_addr src = (*ip_recvpkt).ip_src;
    inet_pton(AF_INET, IPdotdecSrc, (void *)&src);
    inet_ntop(AF_INET, (void *)&src, IPdotdecSrc, 16);// 反转换
	
	char IPdotdecDst[20]; // 存放点分十进制IP地址
	struct in_addr dst = (*ip_recvpkt).ip_dst;
    inet_pton(AF_INET, IPdotdecDst, (void *)&dst);
    inet_ntop(AF_INET, (void *)&dst, IPdotdecDst, 16);// 反转换
	printf("source address=%s,destination address=%s.\n",IPdotdecSrc,IPdotdecDst);
}

int main()	
{
	char skbuf[1500];
	char data[1480];
	int recvfd,datalen;//,数据长度
	int recvlen;		
	struct ip *ip_recvpkt;
	pthread_t tid;
	ip_recvpkt = (struct ip*)malloc(sizeof(struct ip));

	//创建raw socket套接字
	if((recvfd=socket(AF_PACKET,SOCK_RAW,htons(ETH_P_IP)))==-1)	
	{
		printf("recvfd() error\n");
		return -1;
	}	
	
	//路由表初始化
	route_table=(struct route*)malloc(sizeof(struct route));

	if(route_table==NULL)
	{
			printf("malloc error!!\n");
			return -1;
	}
	memset(route_table,0,sizeof(struct route));
	route_table->next=NULL;


	{
		
	//调用添加函数insert_route往路由表里添加直连路由
	//insert_route(inet_addr("192.168.6.0"),24,"eth3",3,inet_addr("192.168.3.2"));
	
	}

	//创建线程去接收路由信息
	int pd;
	pd = pthread_create(&tid,NULL,thr_fn,NULL);


	while(1)
	{
		//接收ip数据包模块socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)
		//接收到的数据包先存在skbuf上
		//长度为recvlen
		recvlen=recv(recvfd,skbuf,sizeof(skbuf),0);
		if(recvlen>0)
		{
		
			//get the ip package, which is behind the ETHER_HEADER
			ip_recvpkt = (struct ip *)(skbuf+ETHER_HEADER_LEN);


			//192.168.1.10是测试服务器的IP，现在测试服务器IP是192.168.1.10到192.168.1.80.
			//使用不同的测试服务器要进行修改对应的IP。然后再编译。
			//192.168.6.2是测试时候ping的目的地址。与静态路由相对应。
			//检查是否是测试服务器发往对应目的地址的包
			if(ip_recvpkt->ip_dst.s_addr == inet_addr("127.0.0.1")){
				continue;
			}

				//分析打印ip数据包的源和目的ip地址
				analyseIP(ip_recvpkt);

				int s;
				memset(data,0,1480);			
				for(s=0;s<1480;s++)
				{
					data[s]=skbuf[s+34];
				}

				 
				// 校验计算模块
				struct _iphdr *iphead;
				int c=0;

				// iphead=(struct _iphdr *)malloc(sizeof(struct _iphdr));
				iphead = (struct _iphdr *)&skbuf[14];
				printf("#####ident is %d \n",iphead->ident);
					
				{
					//调用校验函数check_sum，成功返回1
					c = check_sum(ip_recvpkt, ip_recvpkt->ip_hl, ip_recvpkt->ip_sum);
				}

				if(c ==1)
				{
					printf("checksum is ok!!\n");
				}else
				{
					printf("checksum is error !!\n");
					return -1;
				}

				{
					//调用计算校验和函数count_check_sum，返回新的校验和 
					iphead->checksum = 0;
					iphead->ttl = iphead->ttl - 1;
					iphead->checksum = count_check_sum(ip_recvpkt);
				} 


				//查找路由表，获取下一跳ip地址和出接口模块
				struct nextaddr *nexthopinfo;
				nexthopinfo = (struct nextaddr *)malloc(sizeof(struct nextaddr));
				memset(nexthopinfo,0,sizeof(struct nextaddr));
				{
					//调用查找路由函数lookup_route，获取下一跳ip地址和出接口
					struct in_addr dstAddr = ip_recvpkt->ip_dst;
					int findRoute = lookup_route(dstAddr,nexthopinfo);
					if(findRoute==1){
						printf("成功找到对应表项\n");
					}
					else{
						printf("没有找到可用转发表项\n");
						continue;
					}
				}

					
				//arp find
				struct arpmac *srcmac;
				srcmac = (struct arpmac*)malloc(sizeof(struct arpmac));
				memset(srcmac,0,sizeof(struct arpmac));
				{
					//调用arpGet获取下一跳的mac地址		
					char n[20] = {'\0'};
					srcmac->mac = (char *)malloc(sizeof(char)*6);
					if(nexthopinfo->ipv4addr.s_addr == inet_addr("0.0.0.0")){
						printf("ifname is %s \n", nexthopinfo->ifname);
						if(arpGet(srcmac,nexthopinfo->ifname,ip_recvpkt->ip_dst)<0){
							continue;
						}
						printf("ifname is %s \n", nexthopinfo->ifname);
					}else{
						if(arpGet(srcmac,nexthopinfo->ifname,nexthopinfo->ipv4addr)<0){
							continue;
						}
					}
				}

				//send ether icmp
				{
				//调用ip_transmit函数   填充数据包，通过原始套接字从查表得到的出接口(比如网卡2)将数据包发送出去
				//将获取到的下一跳接口信息存储到存储接口信息的结构体ifreq里，通过ioctl获取出接口的mac地址作为数据包的源mac地址
				//封装数据包：
				//<1>.根据获取到的信息填充以太网数据包头，以太网包头主要需要源mac地址、目的mac地址、以太网类型eth_header->ether_type = htons(ETHERTYPE_IP);
				//<2>.再填充ip数据包头，对其进行校验处理；
				//<3>.然后再填充接收到的ip数据包剩余数据部分，然后通过raw socket发送出去
					struct ifreq ifr;
					
					int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

					char next_ifname[16] = {'\0'};
					unsigned char *next_if_mac = (char *)malloc(sizeof(char)*16);
					if_indextoname(srcmac->index,next_ifname);
					printf("下一跳的接口名称是： %s \n",next_ifname);
					strncpy(ifr.ifr_name,next_ifname,IF_NAMESIZE);

					if(ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0){
						// next_if_mac = ifr.ifr_hwaddr.sa_data;
						memcpy(next_if_mac,ifr.ifr_hwaddr.sa_data,6);
						printf("下一跳接口的mac地址是: %02x:%02x:%02x:%02x:%02x:%02x \n",
									next_if_mac[0],next_if_mac[1],next_if_mac[2],
									next_if_mac[3],next_if_mac[4],next_if_mac[5]);
					}else{
						printf("获取下一跳接口mac地址失败了\n");
					}
					close(sockfd);


					//修改以太网头的部分
					memcpy(&skbuf[0],srcmac,6);
					memcpy(&skbuf[6],next_if_mac,6);
					// eheader->ether_type = htons(ETHERTYPE_IP);

					printf("检查:下一跳目的地址的mac地址是: %02x:%02x:%02x:%02x:%02x:%02x \n,检查:下一跳接口的mac地址是: %02x:%02x:%02x:%02x:%02x:%02x \n",
									(unsigned char)skbuf[0],(unsigned char)skbuf[1],(unsigned char)skbuf[2],
									(unsigned char)skbuf[3],(unsigned char)skbuf[4],(unsigned char)skbuf[5],
									(unsigned char)skbuf[6],(unsigned char)skbuf[7],(unsigned char)skbuf[8],
									(unsigned char)skbuf[9],(unsigned char)skbuf[10],(unsigned char)skbuf[11]);


					//发包
					struct sockaddr_ll sadr_ll;
					sadr_ll.sll_family = PF_PACKET;
					sadr_ll.sll_ifindex = srcmac->index;
					sadr_ll.sll_halen = ETH_ALEN;

					memcpy(sadr_ll.sll_addr, next_if_mac, ETH_ALEN);

					int result = sendto(recvfd, skbuf, recvlen, 
							0,(const struct sockaddr *)&sadr_ll,sizeof(struct sockaddr_ll));
					if(result < 0){
						printf("send error ! %d \n",result);
					}else{
						printf("send succed ! %d \n",result);
					}
				}
		}
	}

	close(recvfd);	
	return 0;
}

