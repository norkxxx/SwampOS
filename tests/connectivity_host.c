#include "transport.h"
#include "net.h"
#include "swampwall.h"
#include "vring.h"
#include "vpn_policy.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static unsigned seed=0x5357414d;static unsigned rnd(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void b16(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
static void tcp_fixture(uint8_t*p){memset(p,0,40);p[0]=0x45;p[3]=40;p[8]=64;p[9]=6;p[12]=192;p[14]=2;p[15]=1;p[16]=192;p[18]=2;p[19]=2;p[20]=1;p[21]=1;p[23]=80;p[32]=0x50;p[33]=2;b16(p+10,net_checksum(p,20));uint8_t pseudo[32]={0};memcpy(pseudo,p+12,8);pseudo[9]=6;pseudo[11]=20;memcpy(pseudo+12,p+20,20);b16(p+36,net_checksum(pseudo,sizeof pseudo));}
static _Alignas(4096) uint8_t ring[VQ_BYTES];
static void complete(struct vq*q,uint32_t id,uint32_t len){uint16_t*u=(void*)(ring+q->used_off);struct vq_used*e=(void*)(ring+q->used_off+4);e[u[1]&(q->n-1)]=(struct vq_used){id,len};u[1]++;}
int main(void){assert(transport_selftest()&&wall_selftest());uint8_t tcp[40];tcp_fixture(tcp);struct tcp_view t;assert(tcp4_parse(tcp,40,&t)&&t.src==257&&t.dst==80&&t.flags==2&&!t.payload);tcp[39]^=1;assert(!tcp4_parse(tcp,40,&t));tcp_fixture(tcp);
 uint8_t dns[]={0x12,0x34,0x81,0x80,0,1,0,1,0,0,0,0,1,'a',0,0,1,0,1,0xc0,12,0,1,0,1,0,0,0,1,0,4,192,0,2,2};struct dns_view d;assert(dns_parse(dns,sizeof dns,&d)&&d.answers==1);dns[20]=19;assert(!dns_parse(dns,sizeof dns,&d));dns[20]=12;
 uint8_t dhcp[256]={0};dhcp[0]=2;dhcp[1]=1;dhcp[2]=6;dhcp[16]=192;dhcp[18]=2;dhcp[19]=2;memcpy(dhcp+236,"\x63\x82\x53\x63",4);memcpy(dhcp+240,"\x35\x01\x05\x36\x04\xc0\x00\x02\x01\xff",10);struct dhcp_view h;assert(dhcp_parse(dhcp,sizeof dhcp,&h)&&h.type==5&&h.server[3]==1);dhcp[249]=52;assert(!dhcp_parse(dhcp,sizeof dhcp,&h));dhcp[249]=255;
 uint8_t v6[48]={0x60};v6[5]=8;v6[6]=17;v6[7]=64;uint8_t proto;size_t len;assert(ipv6_parse(v6,sizeof v6,&proto,&len)&&len==8);v6[6]=44;assert(!ipv6_parse(v6,sizeof v6,&proto,&len));v6[6]=17;
 for(unsigned i=0;i<50000;i++){uint8_t b[256];unsigned k=i%4;const uint8_t*s=k==0?tcp:k==1?dns:k==2?dhcp:v6;size_t n=k==0?sizeof tcp:k==1?sizeof dns:k==2?sizeof dhcp:sizeof v6;memcpy(b,s,n);b[rnd()%n]^=(uint8_t)(1+rnd()%255);size_t l=(i%3)?n:rnd()%(n+1);if(k==0)(void)tcp4_parse(b,l,&t);else if(k==1)(void)dns_parse(b,l,&d);else if(k==2)(void)dhcp_parse(b,l,&h);else (void)ipv6_parse(b,l,&proto,&len);}puts("PASS: TCP checksum/header, DNS compression, DHCP TLVs, IPv6 subset; 50000 mutations");
 struct wall w={0};for(unsigned i=1;i<=WALL_MAX;i++){struct wall_rule r={i,{192,0,2,2},17,53};assert(wall_grant(&w,&r)&&wall_grant(&w,&r));}struct wall_rule r={17,{192,0,2,2},17,53};assert(!wall_grant(&w,&r));assert(wall_revoke(&w,3)&&wall_grant(&w,&r));for(unsigned i=0;i<50000;i++){uint32_t a=rnd()%20;assert(wall_allow(&w,a,r.ip,17,53)==(a>=1&&a<=17&&a!=3));assert(!wall_allow(&w,a,r.ip,6,53));}puts("PASS: exact network policy, capacity, revocation, 50000 identity checks");
 assert(vpn_selftest());for(unsigned a=0;a<2;a++)for(unsigned b=0;b<2;b++)for(unsigned m=0;m<4;m++)for(unsigned g=0;g<2;g++){struct vpn_state v={(uint8_t)a,(uint8_t)b};enum vpn_path want=!g?VPN_DROP:m==VPN_REQUIRED&&a?VPN_TUNNEL:m==VPN_DIRECT&&!b?VPN_BYPASS:VPN_DROP;assert(vpn_decide(&v,(enum vpn_mode)m,g)==want);}puts("PASS: VPN route-decision model, 32 combinations, no fallback on tunnel failure");
 struct vq q;struct vq_used large;
 assert(vq_init(&q,ring,1024) && q.used_off==20480 && vq_submit(&q,1023));
 complete(&q,1023,60);assert(vq_pop(&q,&large)==1 && large.id==1023);
 assert(!vq_init(&q,ring,2048));
 assert(!vq_init(&q,ring,3)&&!vq_init(&q,ring+1,8)&&vq_init(&q,ring,256));assert(q.used_off==8192);struct vq_used u;assert(!vq_pop(&q,&u));for(unsigned i=0;i<150000;i++){uint16_t id=i%q.n;assert(vq_submit(&q,id)&&!vq_submit(&q,id));complete(&q,id,60);assert(vq_pop(&q,&u)==1&&u.id==id&&u.len==60&&!vq_pop(&q,&u));}complete(&q,0,0);assert(vq_pop(&q,&u)==-1);assert(vq_init(&q,ring,8));complete(&q,99,0);assert(vq_pop(&q,&u)==-1);assert(vq_init(&q,ring,8));for(unsigned i=0;i<9;i++)complete(&q,0,0);assert(vq_pop(&q,&u)==-1);puts("PASS: 150000 virtqueue cycles including 16-bit wrap; duplicate/invalid completions and overrun rejection");
}
