#pragma once
#include <stdint.h>
#include <stddef.h>
void user_init(void);
int user_selftest(void);
struct trap_frame {uint64_t r15,r14,r13,r12,r11,r10,r9,r8,rdi,rsi,rbp,rbx,rdx,rcx,rax,vector,error,rip,cs,rflags,rsp,ss;};
int trap_dispatch(struct trap_frame *);

int user_run_image(const void *,size_t,uint64_t,uint64_t);

int user_run_resource(const void *,size_t,uint64_t,uint64_t,const void *,size_t);

int user_run_group(const void*,size_t,uint64_t,unsigned,uint64_t,int*);

struct user_image {const void *code;size_t size;uint64_t offset,grants;const void*data;size_t data_files,data_mem;};
int user_run_images(const struct user_image*,unsigned,int*);

int user_run_native(const struct user_image*,const void*,size_t);

/* Exact-size palette-indexed capture; valid only after successful native exit. */
int user_run_gui(const struct user_image*,unsigned char*,size_t);

/* Asynchronous GUI: one session, CAP_GUI only. step: -1 failed, 0 pending, 1 exited. */
int user_gui_start(const struct user_image*);
int user_gui_active(void);
void user_gui_cancel(void);
int user_gui_step(unsigned char*,size_t,int*);
