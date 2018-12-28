#include "recvroute.h"

int static_route_get(struct selfroute *selfrt)
{
	/*等待客户端连接请求到达*/
	int client_sockfd =accept(server_sockfd,(struct sockaddr *)NULL,NULL);
	if(client_sockfd<0)
	{
		printf("accept error\n");
		return 0;
	}
	printf("get a client \n");

	int len=recv(client_sockfd,selfrt,sizeof(struct selfroute),0);

	if(len > 0){
		if_indextoname(selfrt->ifindex,selfrt->ifname);

		char IPdotdecDst[20]; // 存放点分十进制IP地址
		struct in_addr dst = selfrt->prefix;
		inet_pton(AF_INET, IPdotdecDst, (void *)&dst);
		inet_ntop(AF_INET, (void *)&dst, IPdotdecDst, 16);// 反转换

		char IPdotdecNext[20]; // 存放点分十进制IP地址
		struct in_addr next = selfrt->nexthop;
		inet_pton(AF_INET, IPdotdecNext, (void *)&next);
		inet_ntop(AF_INET, (void *)&next, IPdotdecNext, 16);// 反转换

		printf("	prefix : %s . \n", IPdotdecDst);
		printf("	prefixlen : %d . \n", selfrt->prefixlen);
		printf("	ifindex : %d . \n", selfrt->ifindex);
		printf("    nexthop : %s.\n",IPdotdecNext);
		printf("	cmd : %d . \n", selfrt->cmdnum);
		printf("	ifname : %s . \n", selfrt->ifname);
		close(client_sockfd);
		return 1;

	}else{
		printf("we recive nothing\n");
		close(client_sockfd);
		return 0;
	}
}
