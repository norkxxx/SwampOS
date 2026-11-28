#include "net.h"
static uint16_t be16(const uint8_t*p){return (uint16_t)((uint16_t)p[0]<<8|p[1]);}
static void put16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static int same(const uint8_t*a,const uint8_t*b,size_t n){while(n--)if(*a++!=*b++)return 0;return 1;}
static void copy(uint8_t*a,const uint8_t*b,size_t n){while(n--)*a++=*b++;}
static uint32_t sum(uint32_t s,const uint8_t*p,size_t n){while(n>=2){s+=be16(p);s=(s&65535)+(s>>16);p+=2;n-=2;}if(n)s+=(uint32_t)*p<<8;return s;}
static uint16_t fold(uint32_t s){while(s>>16)s=(s&65535)+(s>>16);return (uint16_t)~s;}
uint16_t net_checksum(const void*p,size_t n){return fold(sum(0,p,n));}
static uint16_t udp_sum(const uint8_t*ip,const uint8_t*p,size_t n){return fold(sum(sum(17+(uint32_t)n,ip+12,8),p,n));}
static int source_ok(const uint8_t*ip){return ip[0]&&ip[0]<224&&ip[0]!=127&&!(ip[0]==169&&ip[1]==254);}
static int mac_ok(const uint8_t*m){static const uint8_t zero[6]={0};return !(m[0]&1)&&!same(m,zero,6);}
size_t net_receive(const struct net_endpoint*ep,const uint8_t*in,size_t n,uint8_t*out,size_t cap,struct net_stats*st){
 if(!ep||!in||!out||!st||in==out)return 0;
 st->received++;static const uint8_t broadcast[6]={255,255,255,255,255,255};
 if(n<14||n>NET_MAX_FRAME||!mac_ok(in+6)||same(in+6,ep->mac,6))goto reject;
 uint16_t type=be16(in+12);size_t written=0;
 if(type==0x806){const uint8_t*p=in+14;if(n<42||cap<42||(!same(in,ep->mac,6)&&!same(in,broadcast,6))||be16(p)!=1||be16(p+2)!=0x800||p[4]!=6||p[5]!=4||be16(p+6)!=1||!same(p+8,in+6,6)||!source_ok(p+14)||!same(p+24,ep->ip,4))goto reject;
  copy(out,in,42);copy(out,in+6,6);copy(out+6,ep->mac,6);uint8_t*q=out+14;put16(q+6,2);copy(q+8,ep->mac,6);copy(q+14,ep->ip,4);copy(q+18,p+8,6);copy(q+24,p+14,4);st->arp++;written=42;
 }else if(type==0x800){const uint8_t*ip=in+14;if(n<34||!same(in,ep->mac,6)||ip[0]!=0x45||!source_ok(ip+12)||!same(ip+16,ep->ip,4)||!ip[8]||net_checksum(ip,20))goto reject;
  size_t len=be16(ip+2);if(len<28||len>n-14||cap<len+14||(be16(ip+6)&0xbfff))goto reject;
  const uint8_t*p=ip+20;size_t payload=len-20;
  if(ip[9]==1){if(p[0]!=8||p[1]||net_checksum(p,payload))goto reject;}
  else if(ip[9]==17){if(!be16(p)||be16(p+2)!=7||be16(p+4)!=payload||!be16(p+6)||udp_sum(ip,p,payload))goto reject;}
  else goto reject;
  copy(out,in,len+14);copy(out,in+6,6);copy(out+6,ep->mac,6);uint8_t*q=out+14;copy(q+12,ep->ip,4);copy(q+16,ip+12,4);q[8]=64;put16(q+10,0);put16(q+10,net_checksum(q,20));uint8_t*t=q+20;
  if(ip[9]==1){t[0]=0;put16(t+2,0);put16(t+2,net_checksum(t,payload));st->icmp++;}
  else {put16(t,7);put16(t+2,be16(p));put16(t+6,0);uint16_t c=udp_sum(q,t,payload);put16(t+6,c?c:65535);st->udp++;}written=len+14;
 }else goto reject;
 st->replies++;return written;
 reject:st->rejected++;return 0;
}
int net_selftest(void){static const struct net_endpoint ep={{2,0,0,0,0,1},{192,0,2,1}};uint8_t in[64]={0},out[64];struct net_stats s={0};
 for(unsigned i=0;i<6;i++){in[i]=255;}in[6]=2;in[11]=2;put16(in+12,0x806);uint8_t*p=in+14;put16(p,1);put16(p+2,0x800);p[4]=6;p[5]=4;put16(p+6,1);copy(p+8,in+6,6);p[14]=192;p[16]=2;p[17]=2;copy(p+24,ep.ip,4);
 if(net_receive(&ep,in,42,out,sizeof out,&s)!=42||be16(out+20)!=2||!same(out+6,ep.mac,6))return 0;
 for(unsigned i=0;i<64;i++){in[i]=0;}copy(in,ep.mac,6);in[6]=2;in[11]=2;put16(in+12,0x800);p=in+14;p[0]=0x45;put16(p+2,29);p[8]=64;p[9]=1;p[12]=192;p[14]=2;p[15]=2;copy(p+16,ep.ip,4);put16(p+10,net_checksum(p,20));p[20]=8;p[28]=42;put16(p+22,net_checksum(p+20,9));
 if(net_receive(&ep,in,43,out,sizeof out,&s)!=43||out[34]!=0||net_checksum(out+14,20)||net_checksum(out+34,9))return 0;
 p[9]=17;put16(p+10,0);put16(p+10,net_checksum(p,20));put16(p+20,40000);put16(p+22,7);put16(p+24,9);put16(p+26,0);uint16_t c=udp_sum(p,p+20,9);put16(p+26,c?c:65535);
 if(net_receive(&ep,in,43,out,sizeof out,&s)!=43||be16(out+34)!=7||be16(out+36)!=40000||udp_sum(out+14,out+34,9))return 0;
 if(net_receive(&ep,in,42,out,sizeof out,&s)||net_receive(&ep,in,43,out,42,&s))return 0;
 p[28]^=1;if(net_receive(&ep,in,43,out,sizeof out,&s))return 0;
 return s.received==6&&s.rejected==3&&s.arp==1&&s.icmp==1&&s.udp==1&&s.replies==3;
}
#ifndef NET_HOST
extern void console_write(const char *,size_t);
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void say(const char*s){size_t n=0;while(s[n])n++;console_write(s,n);}
int net_command(const char*s){if(eq(s,"nettest")){say(net_selftest()?"NET_TEST_OK\n":"NET_TEST_FAIL\n");return 1;}if(eq(s,"net")){say("Ethernet / ARP / IPv4 / ICMP / UDP. NIC only in explicit netlab; no sockets or Internet.\n");return 1;}return 0;}
#endif
