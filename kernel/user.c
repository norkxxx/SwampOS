#include "user.h"
#include "vm.h"
#include "core.h"
#include "policy.h"
#include "audit.h"
#include "session.h"
#include "process.h"
#include "scratch.h"
extern void console_write(const char *,size_t), console_number(uint64_t);
extern void gdt_load(const void *),user_enter(uint64_t,uint64_t);
extern char user_blob_start[],user_blob_end[],user_normal[],user_kernel_read[],user_code_write[],user_nx[],user_io[],user_guard[],user_spin[],user_deny[],user_fpu[];
#ifndef USER_HOST
static uint64_t gdt[8];
static struct __attribute__((packed)) tss64 {uint32_t zero;uint64_t rsp0,rsp1,rsp2,reserved;uint64_t ist[7];uint64_t reserved2;uint16_t reserved3,iomap;} tss;
_Static_assert(sizeof(struct tss64)==104,"TSS layout");
static _Alignas(16) unsigned char ring0_stack[16384];
#endif
static int active, result, single_data;
static struct app_session session;
static uint64_t next_id=1;
static uint64_t started, grants, fault_error;
static struct scratch single_scratch;
static struct proc_group group;
static struct {struct space vm;struct trap_frame frame;struct scratch scratch;uint64_t grants;} jobs[PROC_MAX];
static unsigned batch_active;
/* One paused GUI client. Its address space and register frame survive desktop polls. */
static struct {struct space vm;struct trap_frame frame;uint64_t id,start;unsigned live,running,steps,valid;int result;unsigned char pixels[GUI_FRAME_BYTES];} gui_job;
extern void user_resume(const struct trap_frame *);
_Static_assert(offsetof(struct trap_frame,rip)==136 && offsetof(struct trap_frame,ss)==168 && sizeof(struct trap_frame)==176,"resume frame layout");
/* Only a single explicitly authorized capture session can publish a frame. */
static unsigned gui_capture,gui_valid;
static unsigned char gui_pending[GUI_FRAME_BYTES];
static void gui_clear(void){volatile unsigned char*p=gui_pending;for(unsigned i=0;i<GUI_FRAME_BYTES;i++)p[i]=0;gui_valid=0;}
static long gui_publish(const unsigned char*p){if(!gui_capture)return -1;for(unsigned i=0;i<GUI_FRAME_BYTES;i++)if(p[i]>5)return -22;for(unsigned i=0;i<GUI_FRAME_BYTES;i++)gui_pending[i]=p[i];gui_valid=1;return GUI_FRAME_BYTES;}

#ifdef USER_HOST
#define IRQ_OFF() ((void)0)
#define IRQ_ON() ((void)0)
#else
#define IRQ_OFF() __asm__ volatile("cli":::"memory")
#define IRQ_ON() __asm__ volatile("sti":::"memory")
#endif
static void clean_write(const char *p,size_t n){while(n--){unsigned char c=(unsigned char)*p++;char safe=(c>=32&&c<=126)||c=='\n'||c=='\t'?(char)c:'.';console_write(&safe,1);}}
/* All group transitions occur with interrupts disabled by the IDT interrupt gate. */
static int group_resume(struct trap_frame*f,unsigned old,int next){
 if(next<0){batch_active=0;return 1;}
 if((unsigned)next!=old){if(group.slots[old].state!=PROC_DONE)jobs[old].frame=*f;*f=jobs[next].frame;vm_activate(jobs[next].vm.root);}
 return 0;
}
static int group_end(struct trap_frame*f,int status,uint32_t kind){unsigned old=group.current;audit_add(&kernel_audit,kind,status,group.slots[old].id);scratch_clear(&jobs[old].scratch);return group_resume(f,old,proc_finish(&group,status));}
static int group_trap(struct trap_frame*f){unsigned old=group.current;uint64_t caps=jobs[old].grants;
 if(f->vector==32){int next=proc_tick(&group);if(group.slots[old].state==PROC_DONE){scratch_clear(&jobs[old].scratch);audit_add(&kernel_audit,2,PROC_TIMEOUT,group.slots[old].id);}return group_resume(f,old,next);}
 if(f->vector==128){int check=policy_check_mapped(caps,f->rax,f->rdi,f->rsi,jobs[old].vm.data!=0);if(check){audit_add(&kernel_audit,1,check,group.slots[old].id);f->rax=(uint64_t)(int64_t)check;return 0;}
 switch(f->rax){
 case SWP_SYS_EXIT:return group_end(f,(int)(f->rdi&255),4);
 case SWP_SYS_WRITE:clean_write((const char*)(uintptr_t)f->rdi,f->rsi);f->rax=f->rsi;break;
 case SWP_SYS_CAPS:f->rax=caps;break;
 case SWP_SYS_ID:f->rax=group.slots[old].id;break;
 case SWP_SYS_ROLE:f->rax=old;break;
 case SWP_SYS_YIELD:f->rax=0;return group_resume(f,old,proc_next(&group));
 case SWP_SYS_SEND:f->rax=f->rdx>=group.count||!(jobs[f->rdx].grants&CAP_IPC)?(uint64_t)-1:(uint64_t)proc_send(&group,(unsigned)f->rdx,(const void*)(uintptr_t)f->rdi,f->rsi);break;
 case SWP_SYS_RECV:f->rax=(uint64_t)proc_receive(&group,(void*)(uintptr_t)f->rdi,f->rsi);break;
 case SWP_SYS_SCRATCH_WRITE:f->rax=(uint64_t)scratch_write(&jobs[old].scratch,f->rdx,(const void*)(uintptr_t)f->rdi,f->rsi);break;
 case SWP_SYS_SCRATCH_READ:f->rax=(uint64_t)scratch_read(&jobs[old].scratch,f->rdx,(void*)(uintptr_t)f->rdi,f->rsi);break;
 default:f->rax=(uint64_t)-38;break;
 }return 0;}
 if(f->vector<32&&f->vector!=2&&f->vector!=8&&f->vector!=18)return group_end(f,0x100+(int)f->vector,3);
 return -1;
}
static void write(const char *s){size_t n=0;while(s[n])n++;console_write(s,n);}
void user_init(void){
#ifndef USER_HOST
 gdt[0]=0;gdt[1]=0x00cf9a000000ffffull;gdt[2]=0x00cf92000000ffffull;gdt[3]=0x00af9a000000ffffull;gdt[4]=0x00cff2000000ffffull;gdt[5]=0x00affa000000ffffull;
 tss.rsp0=(uintptr_t)(ring0_stack+sizeof ring0_stack);tss.iomap=sizeof tss;
 uint64_t b=(uintptr_t)&tss,l=sizeof tss-1;gdt[6]=(l&0xffff)|((b&0xffffff)<<16)|(0x89ull<<40)|((l&0xf0000)<<32)|((b&0xff000000)<<32);gdt[7]=b>>32;
 struct {uint16_t limit;uint64_t base;} __attribute__((packed)) gd={sizeof gdt-1,(uintptr_t)gdt};gdt_load(&gd);
 // Disable user writes to FS/GS bases until full per-process architectural state exists.
 uint64_t cr4;__asm__ volatile("mov %%cr4,%0":"=r"(cr4));cr4&=~(1ull<<16);__asm__ volatile("mov %0,%%cr4"::"r"(cr4):"memory");
 for(uint32_t msr=0xc0000100;msr<=0xc0000102;msr++)__asm__ volatile("wrmsr"::"c"(msr),"a"(0u),"d"(0u));
 // No FPU context switching yet: trap user x87/SIMD access (kernel uses GPR only).
 uint64_t cr0;__asm__ volatile("mov %%cr0,%0":"=r"(cr0));cr0=(cr0&~(1ull<<2))|(1ull<<3);__asm__ volatile("mov %0,%%cr0"::"r"(cr0):"memory");
#endif
}
static int gui_trap(struct trap_frame*f){
 if(f->vector==32){if(ticks()-gui_job.start<2)return 0;gui_job.frame=*f;return 1;}
 if(f->vector==128){int check=policy_check_mapped(CAP_GUI,f->rax,f->rdi,f->rsi,gui_job.vm.data!=0);
  if(check){audit_add(&kernel_audit,1,check,gui_job.id);f->rax=(uint64_t)(int64_t)check;return 0;}
  switch(f->rax){
   case SWP_SYS_EXIT:gui_job.result=(int)(f->rdi&255);return 1;
   case SWP_SYS_CAPS:f->rax=CAP_GUI;return 0;
   case SWP_SYS_ID:f->rax=gui_job.id;return 0;
   case SWP_SYS_ROLE:f->rax=0;return 0;
   case SWP_SYS_YIELD:f->rax=0;gui_job.frame=*f;return 1;
   case SWP_SYS_GUI_PUBLISH:{const unsigned char*p=(const void*)(uintptr_t)f->rdi;
    for(unsigned i=0;i<GUI_FRAME_BYTES;i++)if(p[i]>5){f->rax=(uint64_t)-22;return 0;}
    for(unsigned i=0;i<GUI_FRAME_BYTES;i++)gui_job.pixels[i]=p[i];
    gui_job.valid=1;f->rax=GUI_FRAME_BYTES;return 0;}
   default:f->rax=(uint64_t)-38;return 0;
  }
 }
 if(f->vector<32&&f->vector!=2&&f->vector!=8&&f->vector!=18){gui_job.result=0x100+(int)f->vector;return 1;}
 return -1;
}
int trap_dispatch(struct trap_frame *f){
 int from_user=(f->cs&3)==3;
 if(f->vector==32){timer_irq();if(from_user&&gui_job.running)return gui_trap(f);if(from_user&&batch_active)return group_trap(f);if(from_user&&active&&ticks()-started>=100){result=0x200;audit_add(&kernel_audit,2,result,0);active=0;return 1;}return 0;}
 if(from_user&&gui_job.running){int r=gui_trap(f);if(r>=0)return r;}
 if(from_user&&batch_active){int r=group_trap(f);if(r>=0)return r;}
 if(from_user&&active){
  if(f->vector==128){int check=policy_check_mapped(grants,f->rax,f->rdi,f->rsi,single_data);if(check){audit_add(&kernel_audit,1,check,f->rax);f->rax=(uint64_t)(int64_t)check;return 0;}switch(f->rax){case SWP_SYS_EXIT:result=(int)(f->rdi&255);active=0;return 1;case SWP_SYS_WRITE:clean_write((const char*)(uintptr_t)f->rdi,f->rsi);f->rax=f->rsi;return 0;case SWP_SYS_CAPS:f->rax=grants;return 0;case SWP_SYS_ID:f->rax=session.id;return 0;case SWP_SYS_DATA_READ:f->rax=(uint64_t)session_read(&session,f->rdx,(void*)(uintptr_t)f->rdi,f->rsi);return 0;
 case SWP_SYS_YIELD:case SWP_SYS_ROLE:f->rax=0;return 0;
 case SWP_SYS_SEND:case SWP_SYS_RECV:f->rax=(uint64_t)-38;return 0;
 case SWP_SYS_GUI_PUBLISH:f->rax=(uint64_t)gui_publish((const unsigned char*)(uintptr_t)f->rdi);return 0;
 case SWP_SYS_SCRATCH_WRITE:f->rax=(uint64_t)scratch_write(&single_scratch,f->rdx,(const void*)(uintptr_t)f->rdi,f->rsi);return 0;
 case SWP_SYS_SCRATCH_READ:f->rax=(uint64_t)scratch_read(&single_scratch,f->rdx,(void*)(uintptr_t)f->rdi,f->rsi);return 0;default:break;}}
  if(f->vector<32&&f->vector!=2&&f->vector!=8&&f->vector!=18){result=0x100+(int)f->vector;fault_error=f->error;audit_add(&kernel_audit,3,result,f->vector);active=0;return 1;}
 }
 write("KERNEL_TRAP vector=");console_number(f->vector);write(" error=");console_number(f->error);write("\n");
#ifdef USER_HOST
 __builtin_trap();
#else
 for(;;)__asm__ volatile("cli; hlt");
#endif
}
static int run_native(const struct user_image *image,const void *data,size_t len,unsigned char*out){
 struct space s;if(active||batch_active||gui_job.live||!next_id||!image||!!(image->grants&CAP_GUI)!=!!out||image->offset>=image->size||!vm_create_data(&s,image->code,image->size,image->data,image->data_files,image->data_mem))return -1;
 uint64_t caps=image->grants,offset=image->offset;single_data=s.data!=0;
 if(!session_begin(&session,next_id,caps,data,len)){vm_destroy(&s);return -1;}
 next_id++;gui_clear();gui_capture=!!out;scratch_clear(&single_scratch);result=-1;fault_error=0;grants=session.grants;IRQ_OFF();active=1;started=ticks();
 vm_activate(s.root);user_enter(USER_CODE+offset,USER_TOP);vm_activate(vm_kernel_root());active=0;grants=0;single_data=0;
 if(out&&result==0){if(gui_valid){for(unsigned i=0;i<GUI_FRAME_BYTES;i++)out[i]=gui_pending[i];}else result=-1;}
 gui_capture=0;gui_clear();scratch_clear(&single_scratch);session_clear(&session);vm_destroy(&s);audit_add(&kernel_audit,4,result,caps);IRQ_ON();return result;
}
int user_run_native(const struct user_image*image,const void*data,size_t len){return run_native(image,data,len,0);}
int user_run_gui(const struct user_image*image,unsigned char*out,size_t n){if(!out||n!=GUI_FRAME_BYTES||!image||image->grants!=CAP_GUI||active||batch_active||gui_job.live)return -1;for(unsigned i=0;i<GUI_FRAME_BYTES;i++)out[i]=0;return run_native(image,0,0,out);
}
int user_run_resource(const void*code,size_t n,uint64_t offset,uint64_t caps,const void*data,size_t len){struct user_image i={.code=code,.size=n,.offset=offset,.grants=caps};return user_run_native(&i,data,len);}
int user_run_image(const void *code,size_t n,uint64_t offset,uint64_t caps){return user_run_resource(code,n,offset,caps,0,0);}
static int run(char *entry,uint64_t caps){return user_run_image(user_blob_start,(size_t)(user_blob_end-user_blob_start),(uint64_t)(entry-user_blob_start),caps);}

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

int user_run_images(const struct user_image*images,unsigned count,int*out){
 if(active||batch_active||gui_job.live||!images||!out||!count||count>PROC_MAX||!next_id||next_id>UINT64_MAX-count)return 0;
 for(unsigned i=0;i<count;i++)if(!images[i].code||images[i].offset>=images[i].size||images[i].grants&~(CAP_CONSOLE|CAP_IPC|CAP_SCRATCH))return 0;
 if(!proc_init(&group,count,next_id))return 0;
 unsigned allocated=0;for(;allocated<count;allocated++){unsigned i=allocated;if(!vm_create_data(&jobs[i].vm,images[i].code,images[i].size,images[i].data,images[i].data_files,images[i].data_mem))break;jobs[i].frame=(struct trap_frame){.rip=USER_CODE+images[i].offset,.cs=0x2b,.rflags=0x202,.rsp=USER_TOP,.ss=0x23};jobs[i].grants=images[i].grants;scratch_clear(&jobs[i].scratch);}
 if(allocated!=count){while(allocated){unsigned i=--allocated;vm_destroy(&jobs[i].vm);jobs[i].frame=(struct trap_frame){0};jobs[i].grants=0;scratch_clear(&jobs[i].scratch);}proc_clear(&group);return 0;}
 next_id+=count;proc_next(&group);IRQ_OFF();batch_active=1;vm_activate(jobs[0].vm.root);user_enter(USER_CODE+images[0].offset,USER_TOP);vm_activate(vm_kernel_root());batch_active=0;
 for(unsigned i=0;i<count;i++){out[i]=group.slots[i].result;scratch_clear(&jobs[i].scratch);vm_destroy(&jobs[i].vm);jobs[i].frame=(struct trap_frame){0};jobs[i].grants=0;}
 proc_clear(&group);IRQ_ON();return 1;
}
int user_run_group(const void*code,size_t n,uint64_t offset,unsigned count,uint64_t caps,int*out){
 if(!count||count>PROC_MAX)return 0;
 struct user_image images[PROC_MAX];for(unsigned i=0;i<count;i++)images[i]=(struct user_image){.code=code,.size=n,.offset=offset,.grants=caps};return user_run_images(images,count,out);
}

int user_gui_active(void){return gui_job.live!=0;}
int user_gui_start(const struct user_image*i){
 if(gui_job.live||active||batch_active||!next_id||!i||i->grants!=CAP_GUI||i->offset>=i->size||!vm_create_data(&gui_job.vm,i->code,i->size,i->data,i->data_files,i->data_mem))return 0;
 gui_job.frame=(struct trap_frame){.rip=USER_CODE+i->offset,.rsp=USER_TOP,.cs=0x2b,.ss=0x23,.rflags=0x202};
 gui_job.id=next_id++;gui_job.steps=0;gui_job.valid=0;gui_job.result=-1;gui_job.live=1;return 1;
}
static void gui_job_end(int result_code){
 vm_destroy(&gui_job.vm);audit_add(&kernel_audit,4,result_code,gui_job.id);
 /* Wipe registers and all staged pixel bytes, including any unused frame. */
 volatile unsigned char*p=(void*)&gui_job;for(size_t i=0;i<sizeof gui_job;i++)p[i]=0;
}
void user_gui_cancel(void){if(gui_job.live&&!gui_job.running)gui_job_end(-125);}
int user_gui_step(unsigned char*out,size_t n,int*published){
 if(!gui_job.live||gui_job.running||!out||n!=GUI_FRAME_BYTES||!published)return -1;
 *published=0;IRQ_OFF();gui_job.running=1;gui_job.start=ticks();gui_job.steps++;
 vm_activate(gui_job.vm.root);user_resume(&gui_job.frame);vm_activate(vm_kernel_root());gui_job.running=0;
 int rc=0;
 if(gui_job.result>0|| (gui_job.result<0&&gui_job.steps>=100)){gui_job_end(gui_job.result>0?gui_job.result:PROC_TIMEOUT);rc=-1;}
 else {if(gui_job.valid){for(unsigned i=0;i<GUI_FRAME_BYTES;i++)out[i]=gui_job.pixels[i];gui_job.valid=0;*published=1;}
  if(gui_job.result==0){gui_job_end(0);rc=1;}}
 IRQ_ON();return rc;
}
