#include "path.h"
static unsigned le16(const uint8_t*p){return p[0]|((unsigned)p[1]<<8);}
static int eq(const uint8_t*a,const uint8_t*b,size_t n){while(n--)if(*a++!=*b++)return 0;return 1;}
int parse_disk_path(const uint8_t*p,size_t cap,struct disk_path*out){size_t i=0,hd=0;int found=0;if(!p||!out||cap>4096)return 0;while(i+4<=cap){unsigned n=le16(p+i+2);if(n<4||n>cap-i)return 0;if(p[i]==0x7f){if(p[i+1]!=0xff||n!=4||!found||i!=hd+42)return 0;*out=(struct disk_path){hd,i};return 1;}if(p[i]==4&&p[i+1]==1){if(found||!i||n!=42||p[i+40]!=2||p[i+41]!=2)return 0;hd=i;found=1;}else if(found)return 0;i+=n;}return 0;}
int same_disk(const uint8_t*a,const struct disk_path*x,const uint8_t*b,const struct disk_path*y){return x->hd==y->hd&&eq(a,b,x->hd);}
/* Stable laboratory partition GUID, not a cryptographic identity. */
static const uint8_t login_id[16]={0x02,0x00,0x70,0x5a,0x4d,0x1b,0x07,0x47,0x91,0x10,0x53,0x57,0x41,0x4d,0x50,0x02};
int login_partition(const uint8_t*p,const struct disk_path*x){return eq(p+x->hd+24,login_id,16);}
size_t append_login_path(uint8_t*out,size_t cap,const uint8_t*p,const struct disk_path*x){static const char name[]="\\EFI\\SWAMP\\LOGIN.EFI";size_t n=4+2*sizeof(name),total=x->end+n+4;if(total>cap||total>4096)return 0;for(size_t i=0;i<x->end;i++)out[i]=p[i];uint8_t*q=out+x->end;q[0]=4;q[1]=4;q[2]=(uint8_t)n;q[3]=(uint8_t)(n>>8);for(size_t i=0;i<sizeof(name);i++){q[4+2*i]=(uint8_t)name[i];q[5+2*i]=0;}q[n]=0x7f;q[n+1]=0xff;q[n+2]=4;q[n+3]=0;return total;}
