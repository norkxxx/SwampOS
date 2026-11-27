#include "pmm.h"
static int bit(const uint64_t *a,unsigned i){return (a[i/64]>>(i%64))&1;}
static void set(uint64_t *a,unsigned i){a[i/64]|=1ull<<(i%64);}
static void unset(uint64_t *a,unsigned i){a[i/64]&=~(1ull<<(i%64));}
int pmm_init(struct pmm *p,const struct e820_entry *m,size_t n,uint64_t reserved){
 if(!p||!m||!n||n>64||reserved>=PHYS_LIMIT)return 0;
 if(reserved<0x100000)reserved=0x100000;
 for(unsigned i=0;i<FRAME_COUNT/64;i++)p->eligible[i]=p->used[i]=0;
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
int pmm_release(struct pmm *p,uint64_t a){if(!a||a>=PHYS_LIMIT||a%FRAME_SIZE)return 0;unsigned i=a/FRAME_SIZE;if(!bit(p->eligible,i)||!bit(p->used,i))return 0;unset(p->used,i);p->free++;p->hint=i;return 1;}
