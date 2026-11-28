#include "pci.h"
#include <assert.h>
#include <stdio.h>
struct fixture{unsigned reads,found,stop;};
static uint32_t read(void*ctx,uint8_t b,uint8_t d,uint8_t f,uint8_t r){struct fixture*x=ctx;x->reads++;if(b==0&&d==1&&(f==0||f==3)){if(r==0)return 0x12348086;if(r==12)return f?0:0x00800000;if(r==8)return 0x02000000;}if(b==255&&d==31&&f==0){if(r==0)return 0x567810ec;if(r==12)return 0;if(r==8)return 0x01060100;}return 0xffffffffu;}
static int visit(void*ctx,const struct pci_function*p){struct fixture*x=ctx;assert(p->vendor==(x->found==2?0x10ec:0x8086));assert(p->function==(x->found==1?3:0));assert(p->class_code==(x->found==2?1:2));x->found++;return !x->stop;}
int main(void){for(unsigned i=0;i<1000;i++){struct fixture x={0};assert(pci_scan(read,&x,visit,&x)==3&&x.found==3&&x.reads<8300);struct fixture y={.stop=1};assert(pci_scan(read,&y,visit,&y)==1&&y.found==1&&y.reads<10);}puts("PASS: PCI enumeration 1000 full and 1000 early-stop runs (mock config space)");}
