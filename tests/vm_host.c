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
 // Optional initialized RW/NX page with zero-filled BSS, independent roots and guard.
 unsigned char initial[3]={1,2,3};struct space t;
 for(unsigned j=0;j<1000;j++){assert(vm_create_data(&s,&code,1,initial,3,16)&&vm_create_data(&t,&code,1,initial,3,16));assert(s.data!=t.data&&vm_host_entry(s.pt,3)==0&&(vm_host_entry(s.pt,4)&(P|W|U|NX))==(P|W|U|NX));assert(vm_host_entry(s.data,0)==0x030201&&vm_host_entry(s.data,1)==0&&vm_host_entry(t.data,0)==0x030201);vm_destroy(&s);vm_destroy(&t);assert(frames.free==before);}
 assert(!vm_create_data(&s,&code,1,initial,4,3)&&!vm_create_data(&s,&code,1,0,1,1)&&!vm_create_data(&s,&code,1,0,0,4097));
 // Force partial allocation failure, verify cleanup of every allocated frame.
 static uint64_t taken[FRAME_COUNT];unsigned n=0;while(frames.free>5){taken[n]=pmm_take(&frames);assert(taken[n]);n++;}assert(!vm_create(&s,&code,1));assert(!vm_create_data(&s,&code,1,initial,3,16));assert(frames.free==5&&!s.root);for(unsigned i=0;i<n;i++)assert(pmm_release(&frames,taken[i]));assert(frames.free==before);
 assert(!vm_map_live_fb(0x100000,4096));assert(!vm_map_live_fb(0xe0000000,4097));
 n=0;while(frames.free>2){taken[n]=pmm_take(&frames);assert(taken[n]);n++;}assert(!vm_map_live_fb(0xe0000000,4096)&&frames.free==2&&!vm_host_entry(r,1));for(unsigned i=0;i<n;i++)assert(pmm_release(&frames,taken[i]));assert(frames.free==before);
 assert(vm_map_live_fb(0xe0000000,0x201000));uint64_t high=vm_host_entry(r,1),hp=vm_host_entry(high&ADDR,0),ht=vm_host_entry(hp&ADDR,0);assert(!(high&U)&&!(hp&U)&&!(ht&U));assert((vm_host_entry(ht&ADDR,0)&(P|W|U|NX|0x18))==(P|W|NX|0x18));assert((vm_host_entry(ht&ADDR,0)&ADDR)==0xe0000000);uint64_t ht2=vm_host_entry(hp&ADDR,1);assert((vm_host_entry(ht2&ADDR,0)&ADDR)==0xe0200000&&!vm_host_entry(ht2&ADDR,1));assert(vm_create(&s,&code,1)&&vm_host_entry(s.root,1)==high);vm_destroy(&s);assert(!vm_map_live_fb(0xf0000000,4096));puts("PASS: framebuffer high mapping, NX/supervisor, user-root sharing, OOM rollback");
 puts("PASS: actual page-table construction, W^X/NX flags, supervisor branch, guard, 1000 legacy + 1000 dual-data-space cycles, BSS and OOM rollback (host simulation, UBSan)");return 0;
}
