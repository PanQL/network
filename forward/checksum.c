#include "checksum.h"

int check_sum(unsigned short *iphd,int len,unsigned short checksum)
{
    printf("旧的校验和为 : %x \n",checksum);
    printf("ip头的长度为 : %d\n",len);
    int csum = 0;
    unsigned short ans = 0;
    int length = len*4;
    while(length > 1){
        csum += *iphd++;
        length -= 2;
    }
    if(length > 0){
        csum += *iphd;
    }
    csum = (csum>>16) + (csum&0xffff);
    csum += (csum>>16);
    ans = ~(csum);
    printf("校验过程中算出校验和 : %x \n",ans);
    if(ans == 0){
        return 1;
    }else{
        return -1;
    }
}

unsigned short count_check_sum(unsigned short *iphd)
{
    int csum = 0;
    unsigned short ans = 0;
    int length = 20;
    while(length > 1){
        csum += *iphd++;
        length -= 2;
    }
    if(length > 0){
        csum += *iphd;
    }
    csum = (csum>>16) + (csum&0xffff);
    while((csum>>16)){
        csum += (csum>>16);
    }
    ans = ~(csum);
    printf("新的校验和 : %x \n",ans);
    return ans;
}
