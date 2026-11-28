#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "user.h"
#include "vm.h"
#include "policy.h"
#include "block.h"
#include "ext4.h"
#include "elf.h"
#include "session.h"
struct pmm frames;
static unsigned mode,allocated,current;
static unsigned char*const stack=(void*)(uintptr_t)USER_STACK;
static unsigned char disk[8*1024*1024],file[65536],expected[65536],resource[SESSION_DATA_MAX];
uint64_t ticks(void){return 0;}
void timer_irq(void){}
void console_write(const char*s,size_t n){(void)s;(void)n;}
void console_number(uint64_t n){(void)n;}
uint64_t vm_kernel_root(void){return 0;}
int vm_create_data(struct space*s,const void*code,size_t n,const void*data,size_t files,size_t mem){(void)data;(void)files;(void)mem;assert(!allocated&&code&&n);allocated=1;*s=(struct space){.root=1};return 1;}
void vm_destroy(struct space*s){assert(allocated&&!current&&s->root==1);allocated=0;}
void vm_activate(uint64_t root){current=(unsigned)root;}
static int call(struct trap_frame*f,uint64_t nr,uint64_t a,uint64_t b){f->vector=128;f->rax=nr;f->rdi=a;f->rsi=b;return trap_dispatch(f);}
void user_enter(uint64_t ip,uint64_t sp){assert(current==1);struct trap_frame f={.rip=ip,.rsp=sp,.cs=0x2b};memset(stack,2,GUI_FRAME_BYTES);
 if(mode>=5){
  f.rdx=0;
  if(mode==6){assert(!call(&f,SWP_SYS_DATA_READ,USER_STACK,1)&&(int64_t)f.rax==-1);assert(call(&f,SWP_SYS_EXIT,13,0));return;}
  assert(!call(&f,SWP_SYS_DATA_READ,USER_CODE,1)&&(int64_t)f.rax==-14);
  assert(!call(&f,SWP_SYS_DATA_READ,USER_STACK,257)&&(int64_t)f.rax==-14);
  f.rdx=SESSION_DATA_MAX-128;assert(!call(&f,SWP_SYS_DATA_READ,USER_STACK,256)&&f.rax==128);
  assert(!memcmp(stack,resource+SESSION_DATA_MAX-128,128));
  f.rdx=SESSION_DATA_MAX;assert(!call(&f,SWP_SYS_DATA_READ,USER_STACK,1)&&f.rax==0);
  f.rdx=UINT64_MAX;assert(!call(&f,SWP_SYS_DATA_READ,USER_STACK,1)&&f.rax==0);
  assert(call(&f,SWP_SYS_EXIT,0,0));return;
 }
 if(mode==0){assert(!call(&f,SWP_SYS_GUI_PUBLISH,USER_STACK,GUI_FRAME_BYTES)&&(int64_t)f.rax==-1);assert(call(&f,SWP_SYS_EXIT,13,0));return;}
 assert(!call(&f,SWP_SYS_GUI_PUBLISH,USER_STACK,GUI_FRAME_BYTES-1)&&(int64_t)f.rax==-22);assert(!call(&f,SWP_SYS_GUI_PUBLISH,USER_TOP-1,GUI_FRAME_BYTES)&&(int64_t)f.rax==-14);assert(!call(&f,SWP_SYS_GUI_PUBLISH,0,GUI_FRAME_BYTES)&&(int64_t)f.rax==-14);
 if(mode==2){stack[GUI_FRAME_BYTES-1]=6;assert(!call(&f,SWP_SYS_GUI_PUBLISH,USER_STACK,GUI_FRAME_BYTES)&&(int64_t)f.rax==-22);assert(call(&f,SWP_SYS_EXIT,0,0));return;}
 assert(!call(&f,SWP_SYS_GUI_PUBLISH,USER_STACK,GUI_FRAME_BYTES)&&f.rax==GUI_FRAME_BYTES);memset(stack,3,GUI_FRAME_BYTES);
 if(mode==3){assert(call(&f,SWP_SYS_EXIT,7,0));return;}if(mode==4){f.vector=14;assert(trap_dispatch(&f));return;}assert(call(&f,SWP_SYS_EXIT,0,0));}
static int read_disk(void*c,uint64_t lba,unsigned count,void*out){(void)c;if(lba>=sizeof disk/512||count>sizeof disk/512-lba)return 0;memcpy(out,disk+lba*512,count*512);return 1;}
int main(void){assert(mmap(stack,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==stack);unsigned char code=0xc3,out[GUI_FRAME_BYTES+2];struct user_image im={.code=&code,.size=1,.grants=CAP_GUI};assert(user_run_native(&im,0,0)==-1);im.grants=0;mode=0;assert(user_run_native(&im,0,0)==13);im.grants=CAP_GUI;
 for(unsigned i=0;i<2000;i++){mode=1+i%4;memset(out,0xa5,sizeof out);int r=user_run_gui(&im,out+1,GUI_FRAME_BYTES);assert(r==(mode==1?0:mode==2?-1:mode==3?7:0x10e));assert(out[0]==0xa5&&out[GUI_FRAME_BYTES+1]==0xa5&&!allocated&&!current);for(unsigned j=1;j<=GUI_FRAME_BYTES;j++)assert(out[j]==(mode==1?2:0));}
 for(unsigned i=0;i<sizeof resource;i++)resource[i]=(unsigned char)(i*31+7);
 mode=5;im.grants=CAP_DATA;assert(user_run_native(&im,resource,sizeof resource)==0&&!allocated&&!current);
 mode=6;im.grants=0;assert(user_run_native(&im,0,0)==13&&!allocated&&!current);
 puts("PASS: dispatcher 64 KiB resource boundary, user pointer and capability rejection, EOF and overflow");
 FILE*f=fopen("build/vault.ext4","rb");assert(f&&fread(disk,1,sizeof disk,f)==sizeof disk);fclose(f);struct blockdev dev={0,sizeof disk/512,read_disk};struct ext4 fs;struct fs_inode ino;struct elf_app a;assert(ext4_mount(&fs,&dev)&&ext4_lookup(&fs,"/bin/gui-demo",&ino)&&ino.size<sizeof file&&ext4_read(&fs,&ino,0,file,ino.size)&&elf_parse(file,ino.size,&a));f=fopen("build/gui-demo.elf","rb");assert(f&&fread(expected,1,ino.size,f)==ino.size&&fgetc(f)==EOF&&!memcmp(file,expected,ino.size));fclose(f);munmap(stack,4096);puts("PASS: 2000 native GUI dispatcher simulations, capability/range/palette rejection, commit only on clean exit, no stale frame, EXT4 ELF read-back (mock CPU, UBSan)");}
