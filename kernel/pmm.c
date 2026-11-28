#include "pmm.h"
static int bit(const uint64_t *a,unsigned i){return (a[i/64]>>(i%64))&1;}
static void set(uint64_t *a,unsigned i){a[i/64]|=1ull<<(i%64);}
static void unset(uint64_t *a,unsigned i){a[i/64]&=~(1ull<<(i%64));}
int pmm_init(struct pmm *p,const struct e820_entry *m,size_t n,uint64_t reserved){
 if(!p||!m||!n||n>64||reserved>=PHYS_LIMIT)return 0;
 if(reserved<0x100000)reserved=0x100000;
 for(unsigned i=0;i<FRAME_COUNT/64;i++)p->eligible[i]=p->used[i]=p->pinned[i]=0;
 for(unsigned i=0;i<DMA_LEASE_MAX;i++)p->dma[i]=(struct dma_lease){0};
 p->next_token=0;
 p->total=p->free=0;p->hint=0;
 // Only whole pages of enabled usable ranges; all nonusable overlaps win.
 for(unsigned i=0;i<FRAME_COUNT;i++){
  uint64_t lo=(uint64_t)i*FRAME_SIZE,hi=lo+FRAME_SIZE;int usable=0,blocked=0;
  if(lo<reserved)continue;
  for(size_t j=0;j<n;j++){
   if(!(m[j].attr&1)||!m[j].length)continue;
   uint64_t a=m[j].base,b=a+m[j].length;if(b<a){blocked=1;break;}
   if(a>=hi||b<=lo)continue;
   if(m[j].type!=1)blocked=1;else if(a<=lo&&b>=hi)usable=1;
  }
  if(usable&&!blocked){set(p->eligible,i);p->total++;}
 }
 p->free=p->total;return p->total!=0;
}
uint64_t pmm_take(struct pmm *p){if(!p->free)return 0;for(unsigned j=0;j<FRAME_COUNT;j++){unsigned i=(p->hint+j)%FRAME_COUNT;if(bit(p->eligible,i)&&!bit(p->used,i)){set(p->used,i);p->free--;p->hint=(i+1)%FRAME_COUNT;return (uint64_t)i*FRAME_SIZE;}}return 0;}
int pmm_release(struct pmm *p,uint64_t a){if(!a||a>=PHYS_LIMIT||a%FRAME_SIZE)return 0;unsigned i=a/FRAME_SIZE;if(!bit(p->eligible,i)||!bit(p->used,i)||bit(p->pinned,i))return 0;unset(p->used,i);p->free++;p->hint=i;return 1;}

static int dma_slot(const struct pmm*p,const struct dma_lease*l){if(!p||!l||!l->base||!l->token||!l->pages||l->pages>DMA_LEASE_PAGES_MAX||l->base%FRAME_SIZE||l->base>=PHYS_LIMIT||l->pages>(PHYS_LIMIT-l->base)/FRAME_SIZE)return -1;for(unsigned i=0;i<DMA_LEASE_MAX;i++)if(p->dma[i].base==l->base&&p->dma[i].pages==l->pages&&p->dma[i].token==l->token){unsigned first=l->base/FRAME_SIZE;for(unsigned j=0;j<l->pages;j++)if(!bit(p->eligible,first+j)||!bit(p->used,first+j)||!bit(p->pinned,first+j))return -1;return (int)i;}return -1;}
int pmm_dma_valid(const struct pmm*p,const struct dma_lease*l){return dma_slot(p,l)>=0;}
int pmm_dma_take(struct pmm*p,unsigned pages,unsigned align,struct dma_lease*out){if(!p||!out||!pages||pages>DMA_LEASE_PAGES_MAX||pages>p->free||!align||align>256||(align&(align-1))||p->next_token==UINT64_MAX)return 0;unsigned slot=0;while(slot<DMA_LEASE_MAX&&p->dma[slot].token)slot++;if(slot==DMA_LEASE_MAX)return 0;
 for(unsigned first=align;first<=FRAME_COUNT-pages;first+=align){unsigned n=0;while(n<pages&&bit(p->eligible,first+n)&&!bit(p->used,first+n)&&!bit(p->pinned,first+n))n++;if(n!=pages)continue;for(unsigned j=0;j<pages;j++){set(p->used,first+j);set(p->pinned,first+j);}p->free-=pages;struct dma_lease l={(uint64_t)first*FRAME_SIZE,++p->next_token,pages};p->dma[slot]=l;*out=l;return 1;}return 0;}
int pmm_dma_release(struct pmm*p,const struct dma_lease*l,int quiesced){if(quiesced!=1)return 0;int slot=dma_slot(p,l);if(slot<0)return 0;unsigned first=l->base/FRAME_SIZE,n=l->pages;for(unsigned j=0;j<n;j++){unset(p->pinned,first+j);unset(p->used,first+j);}p->free+=n;p->hint=first;p->dma[slot]=(struct dma_lease){0};return 1;}
