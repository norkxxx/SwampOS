#include "transport.h"
#include "net.h"
static uint16_t b16(const uint8_t*p){return (uint16_t)((uint16_t)p[0]<<8|p[1]);}
static uint32_t b32(const uint8_t*p){return (uint32_t)b16(p)<<16|b16(p+2);}
static uint32_t add(uint32_t s,const uint8_t*p,size_t n){while(n>=2){s+=b16(p);p+=2;n-=2;}if(n)s+=(uint32_t)*p<<8;return s;}
static int valid(uint32_t s){while(s>>16)s=(s&65535)+(s>>16);return s==65535;}
int tcp4_parse(const uint8_t*p,size_t n,struct tcp_view*v){
 if(!p||!v||n<40||n>1500||p[0]!=0x45||p[9]!=6||!p[8]||b16(p+2)!=n||(b16(p+6)&0xbfff)||net_checksum(p,20))return 0;
 const uint8_t*t=p+20;size_t len=n-20,h=(t[12]>>4)*4u;
 if(h<20||h>len||(t[12]&15)||!b16(t)||!b16(t+2)||!valid(add(add(6+(uint32_t)len,p+12,8),t,len)))return 0;
 for(size_t i=20;i<h;){uint8_t k=t[i++];if(!k){while(i<h)if(t[i++])return 0;break;}if(k==1)continue;if(i==h)return 0;size_t l=t[i++];if(l<2||l-2>h-i)return 0;i+=l-2;}
 *v=(struct tcp_view){b16(t),b16(t+2),b16(t+14),b32(t+4),b32(t+8),t[13],len-h};return 1;
}
/* DNS compressed names: strictly backward pointers, bounded traversal and expanded length. */
static int name(const uint8_t*p,size_t n,size_t*off){size_t i=*off,end=0,expanded=0;unsigned steps=0;while(i<n&&++steps<=128){uint8_t c=p[i++];if(!c){*off=end?end:i;return 1;}if((c&0xc0)==0xc0){if(i>=n)return 0;size_t target=((size_t)(c&63)<<8)|p[i++];if(target<12||target>=i-2)return 0;if(!end)end=i;i=target;continue;}if(c>63||c>n-i||expanded+1+c>254)return 0;expanded+=1+c;i+=c;}return 0;}
int dns_parse(const uint8_t*p,size_t n,struct dns_view*v){if(!p||!v||n<12||n>4096)return 0;unsigned q=b16(p+4),a=b16(p+6),rr=a+b16(p+8)+b16(p+10);if(q>16||rr>128)return 0;size_t o=12;for(unsigned i=0;i<q;i++){if(!name(p,n,&o)||n-o<4)return 0;o+=4;}for(unsigned i=0;i<rr;i++){if(!name(p,n,&o)||n-o<10)return 0;size_t l=b16(p+o+8);o+=10;if(l>n-o)return 0;o+=l;}if(o!=n)return 0;*v=(struct dns_view){b16(p),b16(p+2),(uint16_t)q,(uint16_t)a};return 1;}
int dhcp_parse(const uint8_t*p,size_t n,struct dhcp_view*v){if(!p||!v||n<241||n>1472||p[0]!=2||p[1]!=1||p[2]!=6||b32(p+236)!=0x63825363)return 0;struct dhcp_view d={0};for(unsigned i=0;i<4;i++)d.address[i]=p[16+i];unsigned seen=0;for(size_t o=240;o<n;){uint8_t k=p[o++];if(!k)continue;if(k==255){if(!(seen&1))return 0;while(o<n)if(p[o++])return 0;*v=d;return 1;}if(o==n)return 0;size_t l=p[o++];if(l>n-o)return 0;if(k==53){if(l!=1||(seen&1)||p[o]<2||p[o]>6||p[o]==3||p[o]==4)return 0;d.type=p[o];seen|=1;}if(k==54){if(l!=4||(seen&2))return 0;for(unsigned i=0;i<4;i++)d.server[i]=p[o+i];seen|=2;}if(k==51){if(l!=4||(seen&4))return 0;d.lease=b32(p+o);seen|=4;}/* Option overload is deliberately unsupported. */if(k==52)return 0;o+=l;}return 0;}
int ipv6_parse(const uint8_t*p,size_t n,uint8_t*protocol,size_t*payload){if(!p||!protocol||!payload||n<40||n>1500||(p[0]>>4)!=6||!p[7]||b16(p+4)!=n-40)return 0;uint8_t t=p[6];size_t l=n-40;if((t==6&&l<20)||(t==17&&l<8)||(t==58&&l<4)||(t!=6&&t!=17&&t!=58))return 0;*protocol=t;*payload=l;return 1;}
int transport_selftest(void){uint8_t dns[]={0x12,0x34,1,0,0,1,0,0,0,0,0,0,1,'a',0,0,1,0,1};struct dns_view d;uint8_t ip6[48]={0x60};ip6[5]=8;ip6[6]=17;ip6[7]=64;uint8_t proto;size_t len;return dns_parse(dns,sizeof dns,&d)&&d.id==0x1234&&!dns_parse(dns,sizeof dns-1,&d)&&ipv6_parse(ip6,sizeof ip6,&proto,&len)&&proto==17&&len==8;}
