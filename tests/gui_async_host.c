#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "user.h"
#include "vm.h"
#include "policy.h"
#include "audit.h"
struct pmm frames;
static unsigned allocated,current,mode,slices;
static uint64_t clock_ticks;
static unsigned char *const stack=(void*)(uintptr_t)USER_STACK;
uint64_t ticks(void){return clock_ticks;}
void timer_irq(void){clock_ticks++;}
void console_write(const char*s,size_t n){(void)s;(void)n;}
void console_number(uint64_t n){(void)n;}
uint64_t vm_kernel_root(void){return 0;}
int vm_create_data(struct space*s,const void*code,size_t n,const void*data,size_t files,size_t mem){(void)data;(void)files;(void)mem;assert(!allocated&&code&&n);allocated=1;*s=(struct space){.root=1};return 1;}
void vm_destroy(struct space*s){assert(allocated&&!current&&s->root==1);allocated=0;memset(s,0,sizeof *s);}
void vm_activate(uint64_t root){current=(unsigned)root;}
void user_enter(uint64_t ip,uint64_t sp){(void)ip;(void)sp;assert(0);}
static int call(struct trap_frame*f,uint64_t nr,uint64_t a,uint64_t b){f->vector=128;f->rax=nr;f->rdi=a;f->rsi=b;return trap_dispatch(f);}
void user_resume(const struct trap_frame*saved){assert(current==1&&saved->cs==0x2b&&saved->ss==0x23&&saved->rflags==0x202);struct trap_frame f=*saved;
 if(!slices){assert(f.rip==USER_CODE&&f.rsp==USER_TOP&&!f.r15);}else{assert(f.r15==0xdeadbeef&&f.rip==USER_CODE+slices&&f.rbx==42);}
 slices++;f.rip=USER_CODE+slices;f.r15=0xdeadbeef;f.rbx=42;
 assert(!call(&f,SWP_SYS_WRITE,USER_STACK,1)&&(int64_t)f.rax==-1);
 assert(!call(&f,SWP_SYS_GUI_PUBLISH,USER_TOP-1,GUI_FRAME_BYTES)&&(int64_t)f.rax==-14);
 memset(stack,6,GUI_FRAME_BYTES);assert(!call(&f,SWP_SYS_GUI_PUBLISH,USER_STACK,GUI_FRAME_BYTES)&&(int64_t)f.rax==-22);
 if(mode==2){f.vector=14;assert(trap_dispatch(&f));return;}
 if(mode==3){f.vector=32;assert(!trap_dispatch(&f));assert(trap_dispatch(&f));return;}
 memset(stack,slices%6,GUI_FRAME_BYTES);assert(!call(&f,SWP_SYS_GUI_PUBLISH,USER_STACK,GUI_FRAME_BYTES)&&f.rax==GUI_FRAME_BYTES);
 memset(stack,0xff,GUI_FRAME_BYTES); /* kernel owns a copy */
 if(mode==1){assert(call(&f,SWP_SYS_EXIT,7,0));return;}
 if(slices==3){assert(call(&f,SWP_SYS_EXIT,0,0));return;}
 assert(call(&f,SWP_SYS_YIELD,0,0));
}
int main(void){assert(mmap(stack,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==stack);
 unsigned char code=0xc3,buf[GUI_FRAME_BYTES+2];struct user_image im={.code=&code,.size=1,.grants=CAP_GUI};int pub;
 im.grants=CAP_GUI|CAP_CONSOLE;assert(!user_gui_start(&im));im.grants=CAP_GUI;
 for(unsigned i=0;i<2000;i++){mode=i%5;slices=0;memset(buf,0xa5,sizeof buf);assert(user_gui_start(&im)&&user_gui_active()&&!user_gui_start(&im));assert(user_run_native(&im,0,0)==-1);int group_result;assert(!user_run_images(&im,1,&group_result));
  if(mode==4){assert(user_gui_step(buf+1,GUI_FRAME_BYTES,&pub)==0&&pub);user_gui_cancel();}
  else if(mode==3){for(unsigned j=0;j<99;j++){assert(user_gui_step(buf+1,GUI_FRAME_BYTES,&pub)==0&&!pub&&allocated&&!current);}assert(user_gui_step(buf+1,GUI_FRAME_BYTES,&pub)<0&&!pub);}
  else if(mode==0){for(unsigned j=1;j<=3;j++){assert(user_gui_step(buf+1,GUI_FRAME_BYTES,&pub)==(j==3)&&pub);for(unsigned k=1;k<=GUI_FRAME_BYTES;k++)assert(buf[k]==j);}}
  else {assert(user_gui_step(buf+1,GUI_FRAME_BYTES,&pub)<0&&!pub);for(unsigned k=1;k<=GUI_FRAME_BYTES;k++)assert(buf[k]==0xa5);}
  assert(!allocated&&!current&&!user_gui_active()&&buf[0]==0xa5&&buf[GUI_FRAME_BYTES+1]==0xa5);user_gui_cancel();}
 munmap(stack,4096);puts("PASS: 2000 GUI sessions; register resume, cooperative yield, timer slices, 100-slice budget, denied syscalls, frame copy, cancellation, fault/exit cleanup (mock CPU)");}
