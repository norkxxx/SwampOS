#include "../drivers/usb/core/enumeration.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct model{unsigned step,fail,short_at,writes,changed;uint8_t dev[18],cfg[41];};
static const uint8_t dev[]={18,1,0,2,0,0,0,64,0x34,0x12,0x78,0x56,0,1,0,0,0,1};
static const uint8_t cfg[]={9,2,41,0,2,2,0,0x80,50,9,4,0,0,1,3,1,1,0,7,5,0x81,3,8,0,10,9,4,1,0,1,3,1,2,0,7,5,0x82,3,3,0,5};
static void init(struct model*m){memset(m,0,sizeof *m);memcpy(m->dev,dev,18);memcpy(m->cfg,cfg,41);}
static int xfer(void*p,const uint8_t s[8],uint8_t*d,size_t n,size_t*a){struct model*m=p;unsigned i=++m->step;assert(i<=6);assert(n==(size_t)(s[6]|s[7]<<8));if(!(s[0]&128))m->writes++;if(i==m->fail)return 0;const uint8_t*src=0;if(i==1){assert(s[0]==0x80&&s[1]==6&&s[3]==1&&n==18);src=m->dev;}else if(i==2||i==3){assert(s[0]==0x80&&s[1]==6&&s[3]==2);if(n>41)return 0;src=m->cfg;}else if(i==4){assert(s[0]==0&&s[1]==9&&!s[3]&&!n);}else{assert(s[0]==0x21&&s[1]==0xb&&!s[2]&&!s[3]&&!n);}if(src)memcpy(d,src,n);if(i==3&&m->changed)d[5]^=1;*a=n;if(i==m->short_at)*a=n?n-1:1;return 1;}
int main(void){assert(usb_enum_selftest());struct model m;uint8_t b[4096];struct usb_enumerated d;init(&m);assert(usb_enumerate_boot(xfer,&m,b,sizeof b,&d)==1&&m.step==6&&m.writes==3&&d.config.count==2&&d.config.configuration==2);for(unsigned i=1;i<=6;i++){init(&m);m.fail=i;assert(usb_enumerate_boot(xfer,&m,b,sizeof b,&d)==(i<=3?0:-1)&&!d.config.count&&m.step==i);init(&m);m.short_at=i;assert(usb_enumerate_boot(xfer,&m,b,sizeof b,&d)==(i<=3?0:-1)&&!d.config.count);}
 init(&m);m.changed=1;assert(!usb_enumerate_boot(xfer,&m,b,sizeof b,&d)&&!m.writes);init(&m);m.cfg[30]=8;assert(!usb_enumerate_boot(xfer,&m,b,sizeof b,&d)&&!m.writes);init(&m);m.cfg[36]=0x81;assert(!usb_enumerate_boot(xfer,&m,b,sizeof b,&d)&&!m.writes);init(&m);assert(!usb_enumerate_boot(xfer,&m,b,20,&d)&&!m.writes);
 unsigned seed=1;for(unsigned i=0;i<50000;i++){init(&m);seed=seed*1664525u+1013904223u;unsigned off=(seed>>16)%41;seed=seed*1664525u+1013904223u;m.cfg[off]^=(uint8_t)(seed>>24);int r=usb_enumerate_boot(xfer,&m,b,sizeof b,&d);if(r==1)assert(d.config.count==2&&m.writes==3);else assert(!d.config.count&&!m.writes);}puts("USB_ENUM_HOST_OK: 50000 descriptor mutations, exact control sequence, short transfers, fail-stop, header consistency and boot-only composite policy");}
