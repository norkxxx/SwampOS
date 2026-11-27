#include "vm.h"
#include <assert.h>
#include <stdio.h>
#define P 1ull
#define W 2ull
#define U 4ull
#define NX (1ull<<63)
#define ADDR 0x000ffffffffff000ull
extern int vm_host_init(void);
extern uint64_t vm_host_entry(uint64_t,unsigned);
int main(void){
 assert(vm_host_init());uint64_t r=vm_kernel_root(),pdpt=vm_host_entry(r,0)&ADDR,pd=vm_host_entry(pdpt,0)&ADDR,pt=vm_host_entry(pd,0)&ADDR;
 assert(!vm_host_entry(pt,0));assert((vm_host_entry(pt,256)&(P|W|U|NX))==P);assert((vm_host_entry(pt,259)&(P|W|U|NX))==(P|NX));assert((vm_host_entry(pt,260)&(P|W|U|NX))==(P|W|NX));
 unsigned before=frames.free;for(unsigned i=0;i<1000;i++)assert(vm_selftest());assert(frames.free==before);
 unsigned char code=0xc3;struct space s;assert(vm_create(&s,&code,1));assert((vm_host_entry(s.root,0)&(P|W|U))==(P|W|U));assert(!(vm_host_entry(s.pdpt,0)&U));assert((vm_host_entry(s.pt,0)&(P|W|U|NX))==(P|U));assert(vm_host_entry(s.pt,1)==0);assert((vm_host_entry(s.pt,2)&(P|W|U|NX))==(P|W|U|NX));vm_destroy(&s);assert(frames.free==before);
 // Force partial allocation failure, verify cleanup of every allocated frame.
 static uint64_t taken[FRAME_COUNT];unsigned n=0;while(frames.free>5){taken[n]=pmm_take(&frames);assert(taken[n]);n++;}assert(!vm_create(&s,&code,1));assert(frames.free==5&&!s.root);for(unsigned i=0;i<n;i++)assert(pmm_release(&frames,taken[i]));assert(frames.free==before);
 puts("PASS: actual page-table construction, W^X/NX flags, supervisor branch, guard, 1000 address-space cycles, OOM rollback (host simulation, UBSan)");return 0;
}
