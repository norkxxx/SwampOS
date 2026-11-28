#include "handoff.h"
static void descriptor(struct efi_mem*d,const unsigned char*p){unsigned char*q=(void*)d;for(size_t i=0;i<sizeof *d;i++)q[i]=p[i];}
static int valid(const struct efi_mem*m){return m->pages&&!(m->physical%4096)&&m->pages<=(UINT64_MAX-m->physical)/4096;}
int handoff_map(struct swamp_bootinfo *b,const void *map,size_t bytes,size_t stride){
 if(!b||!map||stride<sizeof(struct efi_mem)||stride%8||!bytes||bytes%stride||bytes>65536)return 0;
 const unsigned char*p=map;b->count=0;b->phys_count=0;b->phys_complete=1;
 for(size_t i=0;i<bytes;i+=stride){struct efi_mem m;descriptor(&m,p+i);if(!valid(&m))return 0;
  /* Preserve every non-MMIO physical range for later MMIO-vs-RAM alias checks.
   * UEFI types 11/12 are MMIO/port-space; all other types are memory-backed. */
  if(m.type!=11&&m.type!=12&&b->phys_complete){
   uint64_t plo=m.physical,plen=m.pages*4096;
   if(b->phys_count&&b->phys_map[b->phys_count-1].base+b->phys_map[b->phys_count-1].length==plo){b->phys_map[b->phys_count-1].length+=plen;}
   else if(b->phys_count<64)b->phys_map[b->phys_count++]=(struct e820_entry){plo,plen,1,1};
   else {b->phys_count=0;b->phys_complete=0;}
  }
  if(m.type!=7||(m.attributes&(1ull<<63)))continue;
  uint64_t lo=m.physical,hi=lo+m.pages*4096;if(lo>=PHYS_LIMIT||hi<=SWAMP_UEFI_RESERVE)continue;
  if(lo<SWAMP_UEFI_RESERVE)lo=SWAMP_UEFI_RESERVE;
  if(hi>PHYS_LIMIT)hi=PHYS_LIMIT;
  /* A usable range must not overlap any other firmware descriptor. */
  for(size_t j=0;j<bytes;j+=stride){if(j==i)continue;struct efi_mem other;descriptor(&other,p+j);if(!valid(&other))return 0;uint64_t end=other.physical+other.pages*4096;if(lo<end&&other.physical<hi)return 0;}
  if(b->count&&b->map[b->count-1].base+b->map[b->count-1].length==lo){b->map[b->count-1].length+=hi-lo;continue;}
  if(b->count==64)return 0;
  b->map[b->count++]=(struct e820_entry){lo,hi-lo,1,1};
 }
 return b->count!=0;
}
