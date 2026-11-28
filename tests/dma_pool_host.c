#include "vm.h"
#include <assert.h>
#include <stdio.h>
extern int vm_host_init(void);
extern uint64_t vm_host_entry(uint64_t,unsigned);
int main(void){assert(vm_host_init());unsigned before=frames.free;struct dma_lease a,b,stale;assert(!vm_dma_alloc(0,16,&a)&&!vm_dma_alloc(65,16,&a)&&!vm_dma_alloc(1,3,&a));
 for(unsigned i=0;i<10000;i++){unsigned n=i%64+1;assert(vm_dma_alloc(n,16,&a)&&!(a.base&65535)&&a.pages==n);assert(vm_dma_alloc(2,1,&b)&&!(a.base<b.base+8192&&b.base<a.base+n*FRAME_SIZE));assert(pmm_dma_valid(&frames,&a)&&pmm_dma_valid(&frames,&b));for(unsigned j=0;j<n;j++)assert(!pmm_release(&frames,a.base+j*FRAME_SIZE));assert(!vm_dma_release(&a,0)&&pmm_dma_valid(&frames,&a));stale=a;stale.pages++;assert(!vm_dma_release(&stale,1));stale=a;stale.token^=1;assert(!vm_dma_release(&stale,1));stale=a;assert(vm_dma_release(&a,1)&&!a.base&&!pmm_dma_valid(&frames,&stale));assert(vm_dma_alloc(n,16,&a)&&!pmm_dma_valid(&frames,&stale));assert(vm_dma_release(&a,1)&&vm_dma_release(&b,1)&&frames.free==before);}
 struct dma_lease leases[DMA_LEASE_MAX];for(unsigned i=0;i<DMA_LEASE_MAX;i++)assert(vm_dma_alloc(1,1,leases+i));assert(!vm_dma_alloc(1,1,&a));for(unsigned i=0;i<DMA_LEASE_MAX;i++)assert(vm_dma_release(leases+i,1));assert(frames.free==before);
 /* Pinned ranges remain supervisor-only in user roots. */
 assert(vm_dma_alloc(2,16,&a));uint8_t code=0xc3;struct space s;assert(vm_create(&s,&code,1));assert(!(vm_host_entry(s.pdpt,0)&4));vm_destroy(&s);assert(vm_dma_release(&a,1)&&frames.free==before);
 /* Exhaust the allocator; release single scattered frames, no 2-page run. */
 static uint64_t taken[FRAME_COUNT];unsigned k=0;while(frames.free){taken[k]=pmm_take(&frames);assert(taken[k]);k++;}for(unsigned i=0;i<k;i++)if(((taken[i]/FRAME_SIZE)&1)==0)assert(pmm_release(&frames,taken[i]));assert(!vm_dma_alloc(2,1,&a));for(unsigned i=0;i<k;i++)if((taken[i]/FRAME_SIZE)&1)assert(pmm_release(&frames,taken[i]));assert(frames.free==before);
 puts("DMA_POOL_HOST_OK: 10000 lease cycles, alignment, pinning, stale-token rejection, halt gate, table limit, fragmentation and supervisor-only mapping");}
