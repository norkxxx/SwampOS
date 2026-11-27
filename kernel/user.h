#pragma once
#include <stdint.h>
void user_init(void);
int user_selftest(void);
struct trap_frame {uint64_t r15,r14,r13,r12,r11,r10,r9,r8,rdi,rsi,rbp,rbx,rdx,rcx,rax,vector,error,rip,cs,rflags,rsp,ss;};
int trap_dispatch(struct trap_frame *);
