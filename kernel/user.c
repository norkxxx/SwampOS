#include "user.h"
#include "vm.h"
#include "core.h"
#include "policy.h"
extern void console_write(const char *,size_t), console_number(uint64_t);
extern void gdt_load(const void *),user_enter(uint64_t,uint64_t);
extern char user_blob_start[],user_blob_end[],user_normal[],user_kernel_read[],user_code_write[],user_nx[],user_io[],user_guard[],user_spin[],user_deny[],user_fpu[];
static uint64_t gdt[8];
static struct __attribute__((packed)) tss64 {uint32_t zero;uint64_t rsp0,rsp1,rsp2,reserved;uint64_t ist[7];uint64_t reserved2;uint16_t reserved3,iomap;} tss;
_Static_assert(sizeof(struct tss64)==104,"TSS layout");
static _Alignas(16) unsigned char ring0_stack[16384];
static int active, result;
static uint64_t deadline, grants, fault_error;
static void write(const char *s){size_t n=0;while(s[n])n++;console_write(s,n);}
void user_init(void){
 gdt[0]=0;gdt[1]=0x00cf9a000000ffffull;gdt[2]=0x00cf92000000ffffull;gdt[3]=0x00af9a000000ffffull;gdt[4]=0x00cff2000000ffffull;gdt[5]=0x00affa000000ffffull;
 tss.rsp0=(uintptr_t)(ring0_stack+sizeof ring0_stack);tss.iomap=sizeof tss;
 uint64_t b=(uintptr_t)&tss,l=sizeof tss-1;gdt[6]=(l&0xffff)|((b&0xffffff)<<16)|(0x89ull<<40)|((l&0xf0000)<<32)|((b&0xff000000)<<32);gdt[7]=b>>32;
 struct {uint16_t limit;uint64_t base;} __attribute__((packed)) gd={sizeof gdt-1,(uintptr_t)gdt};gdt_load(&gd);
 // No FPU context switching yet: trap user x87/SIMD access (kernel uses GPR only).
 uint64_t cr0;__asm__ volatile("mov %%cr0,%0":"=r"(cr0));cr0=(cr0&~(1ull<<2))|(1ull<<3);__asm__ volatile("mov %0,%%cr0"::"r"(cr0):"memory");
}
int trap_dispatch(struct trap_frame *f){
 int from_user=(f->cs&3)==3;
 if(f->vector==32){timer_irq();if(from_user&&active&&ticks()>=deadline){result=0x200;active=0;return 1;}return 0;}
 if(from_user&&active){
  if(f->vector==128){int check=policy_check(grants,f->rax,f->rdi,f->rsi);if(check){f->rax=(uint64_t)(int64_t)check;return 0;}switch(f->rax){case SWP_SYS_EXIT:result=(int)(f->rdi&255);active=0;return 1;case SWP_SYS_WRITE:console_write((const char*)(uintptr_t)f->rdi,f->rsi);f->rax=f->rsi;return 0;case SWP_SYS_CAPS:f->rax=grants;return 0;default:break;}}
  if(f->vector<32&&f->vector!=2&&f->vector!=8&&f->vector!=18){result=0x100+(int)f->vector;fault_error=f->error;active=0;return 1;}
 }
 write("KERNEL_TRAP vector=");console_number(f->vector);write(" error=");console_number(f->error);write("\n");for(;;)__asm__ volatile("cli; hlt");
}
static int run(char *entry,uint64_t caps){struct space s;size_t n=(size_t)(user_blob_end-user_blob_start);if(!vm_create(&s,user_blob_start,n))return -1;uint64_t offset=(uint64_t)(entry-user_blob_start);if(offset>=n){vm_destroy(&s);return -1;}result=-1;fault_error=0;grants=caps&CAP_CONSOLE;active=1;deadline=ticks()+100;vm_activate(s.root);user_enter(USER_CODE+offset,USER_TOP);vm_activate(vm_kernel_root());active=0;grants=0;vm_destroy(&s);__asm__ volatile("sti");return result;}
int user_selftest(void){
 unsigned before=frames.free;
 struct {char *entry;int expected;uint64_t caps,mask,error;const char *name;} cases[]={
 {user_normal,0,CAP_CONSOLE,0,0,"syscalls"},{user_deny,0,0,0,0,"default deny"},
 {user_kernel_read,0x10e,0,0x17,0x05,"kernel access"},{user_code_write,0x10e,0,0x17,0x07,"code write"},
 {user_nx,0x10e,0,0x17,0x15,"NX stack"},{user_io,0x10d,0,0,0,"port I/O"},
 {user_guard,0x10e,0,0x17,0x04,"guard page"},{user_fpu,0x107,0,0,0,"FPU disabled"},
 {user_spin,0x200,0,0,0,"watchdog"} };
 int ok=1;for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){int got=run(cases[i].entry,cases[i].caps);int pass=got==cases[i].expected&&(fault_error&cases[i].mask)==cases[i].error;write("[user] ");write(cases[i].name);write(pass?" PASS\n":" FAIL\n");if(!pass)ok=0;}
 return ok&&before==frames.free;
}
