#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "user.h"
#include "vm.h"
#include "policy.h"
#include "process.h"
#include "audit.h"
/* Simulate the privileged entry/CR3 operations only. The actual trap_dispatch,
 * group lifecycle, capability checks, queues and scratch paths execute unchanged. */
struct pmm frames;
static unsigned current, used[PROC_MAX], limit=PROC_MAX, mode;
static uint64_t timer;
static unsigned char stacks[PROC_MAX][4096];
static unsigned char *const userstack=(void*)(uintptr_t)USER_STACK;
uint64_t ticks(void){return timer;}
void timer_irq(void){timer++;}
void console_write(const char*s,size_t n){(void)s;(void)n;}
void console_number(uint64_t n){(void)n;}
uint64_t vm_kernel_root(void){return 0;}
int vm_create_data(struct space*s,const void*code,size_t n,const void*data,size_t files,size_t mem){assert(!data&&!files&&!mem);assert(code&&n);for(unsigned i=0;i<limit;i++)if(!used[i]){used[i]=1;memset(stacks[i],0,4096);*s=(struct space){.root=i+1};frames.free--;return 1;}return 0;}
void vm_destroy(struct space*s){assert(s->root&&used[s->root-1]&&current!=s->root);used[s->root-1]=0;frames.free++;*s=(struct space){0};}
void vm_activate(uint64_t root){if(current)memcpy(stacks[current-1],userstack,4096);current=(unsigned)root;if(current){assert(used[current-1]);memcpy(userstack,stacks[current-1],4096);}}
static int call(struct trap_frame*f,uint64_t nr,uint64_t a,uint64_t b,uint64_t c){f->vector=128;f->rax=nr;f->rdi=a;f->rsi=b;f->rdx=c;return trap_dispatch(f);}
static int tick(struct trap_frame*f){f->vector=32;return trap_dispatch(f);}
void user_enter(uint64_t ip,uint64_t sp){struct trap_frame f={.rip=ip,.rsp=sp,.cs=0x2b,.ss=0x23,.rflags=0x202};assert(current==1);
 if(mode==1){for(unsigned j=0;j<2*PROC_BUDGET;j++){int done=tick(&f);assert(done==(j+1==2*PROC_BUDGET));}return;}
 if(mode==2){f.vector=14;assert(!trap_dispatch(&f)&&current==2);assert(call(&f,SWP_SYS_EXIT,7,0,0));return;}
 if(mode==3){assert(!call(&f,SWP_SYS_SEND,USER_STACK,1,1)&&(int64_t)f.rax==-1);assert(!call(&f,SWP_SYS_SCRATCH_WRITE,USER_STACK,1,0)&&(int64_t)f.rax==-1);assert(!call(&f,SWP_SYS_EXIT,0,0,0)&&current==2);assert(call(&f,SWP_SYS_EXIT,0,0,0));return;}
 assert(!call(&f,SWP_SYS_ROLE,0,0,0)&&f.rax==0);f.r15=0x1234;f.rip+=32;userstack[0]='A';assert(!call(&f,SWP_SYS_SCRATCH_WRITE,USER_STACK,1,0)&&f.rax==1);assert(!call(&f,SWP_SYS_SEND,USER_STACK,1,1)&&f.rax==1);assert(!tick(&f)&&current==1);assert(!tick(&f)&&current==2&&f.r15==0&&f.rip==ip&&userstack[0]==0);
 assert(!call(&f,SWP_SYS_ROLE,0,0,0)&&f.rax==1);assert(!call(&f,SWP_SYS_SCRATCH_READ,USER_STACK,1,0)&&f.rax==0);assert(!call(&f,SWP_SYS_RECV,USER_CODE,1,0)&&(int64_t)f.rax==-14);assert(!call(&f,SWP_SYS_RECV,USER_STACK,1,0)&&f.rax==1&&userstack[0]=='A');userstack[0]='B';assert(!call(&f,SWP_SYS_SCRATCH_WRITE,USER_STACK,1,0)&&f.rax==1);assert(!call(&f,SWP_SYS_SEND,USER_STACK,1,0)&&f.rax==1);assert(!call(&f,SWP_SYS_YIELD,0,0,0)&&current==1&&f.r15==0x1234&&f.rip==ip+32&&userstack[0]=='A');assert(!call(&f,SWP_SYS_RECV,USER_STACK,1,0)&&f.rax==1&&userstack[0]=='B');assert(!call(&f,SWP_SYS_SCRATCH_READ,USER_STACK,1,0)&&f.rax==1&&userstack[0]=='A');assert(!call(&f,SWP_SYS_EXIT,0,0,0)&&current==2&&userstack[0]=='B'&&f.rax==0);assert(call(&f,SWP_SYS_EXIT,0,0,0));
}
int main(void){assert(mmap(userstack,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==userstack);frames.free=PROC_MAX;int out[2];unsigned char c=0xc3;
 for(unsigned i=0;i<1000;i++){mode=i%4;assert(user_run_group(&c,1,0,2,mode==3?0:CAP_IPC|CAP_SCRATCH,out));assert(!current&&frames.free==PROC_MAX);if(mode==1)assert(out[0]==PROC_TIMEOUT&&out[1]==PROC_TIMEOUT);else if(mode==2)assert(out[0]==0x10e&&out[1]==7);else assert(!out[0]&&!out[1]);}
 limit=1;assert(!user_run_group(&c,1,0,2,CAP_IPC,out)&&frames.free==PROC_MAX);assert(!user_run_group(&c,1,0,2,CAP_DATA,out));munmap(userstack,4096);puts("PASS: 1000 actual dispatcher simulations, register/stack switching, capability denial, IPC, independent scratch, fault/watchdog exit, partial allocation cleanup (mock CPU/CR3, UBSan)");return 0;}
