#include "vtd.h"
#include "vm.h"
#include "core.h"

enum { CAP=8, ECAP=16, GCMD=24, GSTS=28, RTADDR=32, CCMD=40 };
#define TE (1u<<31)
#define SRTP (1u<<30)
#define BUSY (1ull<<63)
#define R_W 3ull

/* Outside the device domain. Legacy root/context entries are 128 bits. */
static _Alignas(4096) uint64_t root[512], context[512], pdpt[512], pd[512], pt[512];
static unsigned iotlb, installed, claimed, root_index, context_index;
static uint64_t reg_base, drain;
static unsigned cache_line;
static void publish_tables(void){
 if(cache_line){
  uint64_t *pages[]={root,context,pdpt,pd,pt};
  for(unsigned i=0;i<5;i++)for(unsigned off=0;off<4096;off+=cache_line)
   __asm__ volatile("clflush (%0)"::"r"((uint8_t*)pages[i]+off):"memory");
 }
 __asm__ volatile("mfence":::"memory");
}
static volatile uint32_t *reg32(unsigned off){return (void*)(uintptr_t)(VM_IOMMU_VIRT+off);}
static volatile uint64_t *reg64(unsigned off){return (void*)(uintptr_t)(VM_IOMMU_VIRT+off);}
static int wait32(unsigned off,uint32_t mask,uint32_t want){
 uint64_t start=ticks();
 do{if((*reg32(off)&mask)==want)return 1;__asm__ volatile("pause");}while(ticks()-start<50);
 return 0;
}
static int invalidate(unsigned off,uint64_t request,unsigned result_shift){
 if(*reg64(off)&BUSY)return 0;
 *reg64(off)=BUSY|request;
 uint64_t start=ticks(),value;
 do{value=*reg64(off);if(!(value&BUSY))return ((value>>result_shift)&3u)==1;
    __asm__ volatile("pause");}while(ticks()-start<50);
 return 0;
}
static int flush(void){
 __atomic_thread_fence(__ATOMIC_SEQ_CST);
 return invalidate(CCMD,1ull<<61,59)&&invalidate(iotlb,(1ull<<60)|drain,57);
}
int vtd_available(uint64_t registers){
 if(!vm_map_iommu(registers))return 0;
 if(installed)return reg_base==registers&&!claimed&&(*reg32(GSTS)&TE)&&*reg64(RTADDR)==(uintptr_t)root;
 uint64_t cap=*reg64(CAP),ext=*reg64(ECAP);
 /* Support coherent walks or explicitly publish CPU cache lines, with 39-bit AW.
  * Refuse inherited translation, queue/interrupt remapping or root ownership. */
 iotlb=(unsigned)((ext>>8)&1023u)*16+8;
 if((*reg32(0)>>4&15)!=1||!(cap&(1u<<9))||iotlb<0x80||iotlb>4088||
    (cap&(1u<<4))||*reg32(GSTS)||*reg64(RTADDR))return 0;
 if(!(ext&1)){
  unsigned a=1,b,c,d;
  __asm__ volatile("cpuid":"+a"(a),"=b"(b),"=c"(c),"=d"(d));
  cache_line=((b>>8)&255)*8;
  if(!(d&(1u<<19))||!cache_line||cache_line>4096||(cache_line&(cache_line-1)))return 0;
 }
 drain=((cap>>54)&1u)<<48ull; /* DWD -> drain writes */
 drain|=((cap>>55)&1u)<<49ull; /* DRD -> drain reads */
 reg_base=registers;
 return 1;
}
int vtd_bind(unsigned bus,unsigned devfn,const struct dma_lease l[USB_DMA_WINDOWS],unsigned count){
 if(!reg_base||claimed||bus>255||devfn>255||!count||count>USB_DMA_WINDOWS)return 0;
 for(unsigned i=0;i<count;i++){
  if(!pmm_dma_valid(&frames,&l[i])||l[i].pages>16)return 0;
  for(unsigned j=0;j<i;j++)if(l[i].base==l[j].base)return 0;
 }
 root_index=bus*2;context_index=devfn*2;
 for(unsigned i=0;i<count;i++)for(unsigned j=0;j<l[i].pages;j++)
  pt[(USB_IOVA(i)>>12)+j]=(l[i].base+(uint64_t)j*4096)|R_W;
 pd[0]=(uintptr_t)pt|R_W;pdpt[0]=(uintptr_t)pd|R_W;
 context[context_index+1]=(1u<<8)|1u; /* Domain 1, three-level translation. */
 __atomic_thread_fence(__ATOMIC_RELEASE);
 context[context_index]=(uintptr_t)pdpt|1u;
 root[root_index]=(uintptr_t)context|1u;
 claimed=1;
 publish_tables();
 if(!installed){
  *reg64(RTADDR)=(uintptr_t)root;
  *reg32(GCMD)=SRTP;
  if(!wait32(GSTS,SRTP,SRTP)||*reg64(RTADDR)!=(uintptr_t)root)return 0;
 }
 if(!flush())return 0;
 if(!installed){*reg32(GCMD)=TE;if(!wait32(GSTS,TE,TE))return 0;installed=1;}
 return 1;
}
int vtd_revoke(void){
 if(!claimed||!installed||!(*reg32(GSTS)&TE))return 0;
 root[root_index]=0;context[context_index]=0;context[context_index+1]=0;
 publish_tables();
 if(!flush())return 0;
 for(unsigned n=0;n<512;n++)pt[n]=0;
 claimed=0;
 return 1;
}
