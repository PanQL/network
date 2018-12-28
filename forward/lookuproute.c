#include "lookuproute.h"

struct route *route_table; 

int insert_route(unsigned long  ip4prefix,unsigned int prefixlen,char *ifname,unsigned int ifindex,unsigned long  nexthopaddr)
{
	struct route *it = route_table;
    //before insert
    printf("插入路由之前\n");
    it = route_table->next;
    while(it != NULL){
        printf("%x \n",it->nexthop);
        print_route(it);
        it = it->next;
    }
    printf("===========\n");
    printf("%x , %d, %s \n", ntohl(ip4prefix), prefixlen, ifname);
	it = route_table;
    int find = 0;
    while(it->next != NULL){
        int i = 1;
        int res = 1;
        while(i < prefixlen){
            res = ( res << 1 );
            res += 1;
            i++;
        }
        res = res << (32 - prefixlen);
        if((ntohl(ip4prefix) & res) == (ntohl(it->next->ip4prefix.s_addr) & res)){
            find = 1;
            // break;
            print_route(it->next);
            it->next->ip4prefix.s_addr =  ip4prefix;//设置目的地址
            it->next->prefixlen = prefixlen; //设置前缀长度
            it->next->nexthop->ifname = strdup(ifname);     //接口名称
            it->next->nexthop->ifindex = ifindex;   // 接口索引号
            it->next->nexthop->nexthopaddr.s_addr = nexthopaddr;      //下一跳的终点地址
            print_route(it->next);
            break;
        }
        it = it->next;
    }
    if(find == 0){
        struct route *tail = (struct route*)malloc(sizeof(struct route));
        tail = (struct route*)malloc(sizeof(struct route));
        tail->next = NULL;
        tail->ip4prefix.s_addr =  ip4prefix;//设置目的地址
        tail->prefixlen = prefixlen; //设置前缀长度
        tail->nexthop = (struct nexthop*)malloc(sizeof(struct nexthop));    //初始化相关的下一跳信息
        tail->nexthop->ifname = strdup(ifname);     //接口名称
        tail->nexthop->ifindex = ifindex;   // 接口索引号
        tail->nexthop->nexthopaddr.s_addr = nexthopaddr;      //下一跳的终点地址
        tail->next = route_table->next;
        route_table->next = tail; 
        print_route(tail);
        //设置初始值
    }

    printf("插入路由成功\n");
    it = route_table->next;
    while(it != NULL){
        printf("%x \n",it->nexthop);
        // print_route(it);
        printf("prefix : %x \n",it->ip4prefix.s_addr);
        printf("prefixlen : %d \n",it->prefixlen);
        printf("next : %x \n",it->next);
        it = it->next;
    }
    return 0;
}

int lookup_route(struct in_addr dstaddr,struct nextaddr *nexthopinfo)
{
    struct route *ne = route_table->next;
    nexthopinfo->prefixl=0;
    int count = 0;
    while(ne != NULL){
        printf("正在查表\n");
        print_route(ne);

        //get the mask
        unsigned int x = 1;
        int i = 1;
        //小端序
        while(i<ne->prefixlen){
            x = (x<<1)|x;
            i++;
        }
        printf("掩码 %x .\n",x);
        printf("目的地址与上掩码： %x .\n",(dstaddr.s_addr & x));
        printf("当前路由表项的掩码部分 : %x .\n",(ne->ip4prefix.s_addr & x));
        if((dstaddr.s_addr & x) == (ne->ip4prefix.s_addr & x)){
            // printf("找到一个路由 !!! \n");
            if(ne->prefixlen > nexthopinfo->prefixl){
                nexthopinfo->ifname = strdup(ne->nexthop->ifname);
                nexthopinfo->ipv4addr = ne->nexthop->nexthopaddr;
                nexthopinfo->prefixl = ne->prefixlen;
                printf("找到的路由信息为: \n");
                print_route(ne);
                count=1;
                break;
            }
        }
        ne = ne->next;
    }
    return count;
}

int delete_route(struct in_addr dstaddr,unsigned int prefixlen)
{
	struct route *it = route_table;
    struct route *ne = it->next;
    int mark = 0;

    while(ne != NULL){
        printf("删除过程中查看路由表项 ： \n");
        print_route(ne);
        if(ne->ip4prefix.s_addr == dstaddr.s_addr && ne->prefixlen == prefixlen){
            mark = 1;
            printf("上面这个路由被删除了 \n");
            it->next = ne->next;
            ne = it->next;
        }else{
            it = ne;
            ne = ne->next;
        }
    }
    if(mark==1)return 1;
    else return 0;
}

int print_route(struct route *r){
	char IPdotdecDst[20]; // 存放点分十进制IP地址
    struct in_addr dst = r->ip4prefix;
    inet_pton(AF_INET, IPdotdecDst, (void *)&dst);
    inet_ntop(AF_INET, (void *)&dst, IPdotdecDst, 16);// 反转换
	printf("    静态路由地址 :%s .\n",IPdotdecDst);
    printf("    掩码长度 : %d, \n",r->prefixlen);
    printf("    接口名称 : %s . \n",r->nexthop->ifname);
    printf("    接口号 : %d . \n",r->nexthop->ifindex);

	char IPdotdecNext[20]; // 存放点分十进制IP地址
	struct in_addr next = r->nexthop->nexthopaddr;
    inet_pton(AF_INET, IPdotdecNext, (void *)&next);
    inet_ntop(AF_INET, (void *)&next, IPdotdecNext, 16);// 反转换
	printf("    下一跳地址 :%s.\n",IPdotdecNext);
    return 1;
}

