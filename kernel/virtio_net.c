#include "virtio_net.h"
#include "vring.h"
#include "net.h"
#include "pci.h"
#include "core.h"
/* Transitional legacy virtio PCI, trusted emulator only. No IOMMU; no hotplug/SMP.
 * Explicit finite lab session, no application or Internet path. */
#define HDR 10u
#define BUF 1536u
static _Alignas(4096) uint8_t rx_ring[VQ_BYTES],tx_ring[VQ_BYTES];
static _Alignas(16) uint8_t rx_buf[VQ_MAX][BUF],tx_buf[BUF];
static struct vq rx,tx;
static struct pci_function dev;
static uint16_t io,oldcmd;
static int found,active,busy,reset_failed;
static struct net_endpoint ep;
static struct net_stats stats;
extern void console_write(const char *,size_t);
extern void console_number(uint64_t);
static void say(const char*s){size_t n=0;while(s[n])n++;console_write(s,n);}
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static uint8_t in8(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static uint16_t in16(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static uint32_t in32(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static void out8(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p):"memory");}
static void out16(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p):"memory");}
static void out32(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p):"memory");}
static void cmd(uint16_t v){uint64_t f;__asm__ volatile("pushfq; pop %0; cli":"=r"(f)::"memory");out32(0xcf8,0x80000004u|((uint32_t)dev.bus<<16)|((uint32_t)dev.device<<11)|((uint32_t)dev.function<<8));out16(0xcfc,v);if(f&(1ull<<9))__asm__ volatile("sti":::"memory");}
static int visit(void*c,const struct pci_function*p){(void)c;if(p->vendor==0x1af4&&p->product==0x1000&&p->class_code==2){uint32_t b=pci_config_read(0,p->bus,p->device,p->function,0x10);if((b&1)&&(b&~3u)>=0x100&&(b&~3u)<=0xffc0){dev=*p;io=(uint16_t)(b&~3u);found=1;return 0;}}return 1;}
static void stop(void){if(!active)return;out8(io+18,0);for(unsigned i=0;i<100000&&in8(io+18);i++)__asm__ volatile("pause");if(in8(io+18))reset_failed=1;cmd(oldcmd&~4u);active=0;busy=0;}
static int setup(struct vq*q,uint16_t i,uint8_t*b){out16(io+14,i);uint16_t n=in16(io+12);if(in32(io+8)||!vq_init(q,b,n))return 0;out32(io+8,(uint32_t)((uintptr_t)b>>12));return in32(io+8)==(uint32_t)((uintptr_t)b>>12);}
static int emulator(void){uint32_t a=1,b,c,d;__asm__ volatile("cpuid":"+a"(a),"=b"(b),"=c"(c),"=d"(d));if(!(c&(1u<<31)))return 0;a=0x40000000;__asm__ volatile("cpuid":"+a"(a),"=b"(b),"=c"(c),"=d"(d));return (b==0x54474354&&c==0x43544743&&d==0x47435447)||(b==0x4b4d564b&&c==0x564b4d56&&d==0x0000004d);}
static int start(void){if(reset_failed||!emulator())return 0;found=0;pci_scan(pci_config_read,0,visit,0);if(!found)return 0;oldcmd=(uint16_t)pci_config_read(0,dev.bus,dev.device,dev.function,4);/* Do not take over a device already bus-mastering. */if(oldcmd&4)return 0;cmd((oldcmd|1|0x400)&~4u);active=1;out8(io+18,0);if(in8(io+18))goto fail;out8(io+18,1);out8(io+18,3);if(!(in32(io)&(1u<<5)))goto fail;out32(io+4,1u<<5);/* MAC only, no mergeable RX, offload, event idx or MSI-X. */
 for(unsigned i=0;i<6;i++){ep.mac[i]=in8(io+20+i);}
 unsigned nz=0;for(unsigned i=0;i<6;i++)nz|=ep.mac[i];if(!nz||(ep.mac[0]&1))goto fail;
 ep.ip[0]=192;ep.ip[1]=0;ep.ip[2]=2;ep.ip[3]=1;
 if(!setup(&rx,0,rx_ring)||!setup(&tx,1,tx_ring))goto fail;
 for(uint16_t i=0;i<rx.n;i++){vq_desc_set(&rx,i,(uintptr_t)rx_buf[i],BUF,2);if(!vq_submit(&rx,i))goto fail;}stats=(struct net_stats){0};busy=0;
 cmd(oldcmd|1|4|0x400);out8(io+18,7);if(in8(io+18)!=7)goto fail;out16(io+16,0);return 1;
 fail:stop();return 0;}
static int poll(void){struct vq_used u;int rc=vq_pop(&tx,&u);if(rc<0||(rc&&(!busy||u.id||u.len)))return 0;if(rc)busy=0;rc=vq_pop(&rx,&u);if(rc<0)return 0;if(!rc)return 1;if(u.len>BUF||u.len<HDR)return 0;uint8_t*b=rx_buf[u.id];/* With no offloads, header flags and GSO type must be zero. */
 if(!busy&&!b[0]&&!b[1]){size_t n=net_receive(&ep,b+HDR,u.len-HDR,tx_buf+HDR,BUF-HDR,&stats);if(n){for(unsigned i=0;i<HDR;i++)tx_buf[i]=0;while(n<60)tx_buf[HDR+n++]=0;vq_desc_set(&tx,0,(uintptr_t)tx_buf,(uint32_t)(n+HDR),0);if(!vq_submit(&tx,0))return 0;busy=1;out16(io+16,1);}}
 if(!vq_submit(&rx,(uint16_t)u.id)){return 0;}out16(io+16,0);return 1;}
int netlab_command(const char*s){if(!eq(s,"netlab"))return 0;if(!start()){say("NETLAB_UNAVAILABLE\n");return 1;}say("NETLAB_READY 192.0.2.1 (10s trusted VM only)\n");uint64_t t=ticks();int ok=1;for(unsigned long i=0;i<50000000ul&&ticks()-t<1000;i++){if(!poll()){ok=0;break;}__asm__ volatile("pause");}stop();say(ok?"NETLAB_STOP replies=":"NETLAB_DEVICE_ERROR replies=");console_number(stats.replies);say("\n");return 1;}
