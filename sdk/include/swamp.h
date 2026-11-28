#pragma once
#include <stdint.h>
#include <stddef.h>
#define SWAMP_CAP_CONSOLE 1u
static inline long swamp_call(unsigned long n,unsigned long a,unsigned long b){register unsigned long r __asm__("rax")=n;__asm__ volatile("int $0x80":"+a"(r):"D"(a),"S"(b):"memory","cc");return (long)r;}
static inline unsigned long swamp_caps(void){return (unsigned long)swamp_call(2,0,0);}
static inline long swamp_write(const char *s,size_t n){return swamp_call(1,(uintptr_t)s,n);}
__attribute__((noreturn)) static inline void swamp_exit(unsigned status){swamp_call(0,status,0);__builtin_trap();}

#define SWAMP_CAP_DATA 2u
#define SWAMP_DATA_READ_MAX 256u
#define SWAMP_DATA_MAX (64u * 1024u)
static inline unsigned long swamp_id(void){return (unsigned long)swamp_call(3,0,0);}
static inline long swamp_data_read(void *dst,size_t n,unsigned long offset){register unsigned long r __asm__("rax")=4;__asm__ volatile("int $0x80":"+a"(r):"D"(dst),"S"(n),"d"(offset):"memory","cc");return (long)r;}
#define SWAMP_CAP_IPC 4u
#define SWAMP_CAP_SCRATCH 8u
static inline long swamp_call3(unsigned long n,unsigned long a,unsigned long b,unsigned long c){register unsigned long r __asm__("rax")=n;__asm__ volatile("int $0x80":"+a"(r):"D"(a),"S"(b),"d"(c):"memory","cc");return (long)r;}
static inline void swamp_yield(void){(void)swamp_call(5,0,0);}
static inline long swamp_send(unsigned dest,const void *p,size_t n){return swamp_call3(6,(uintptr_t)p,n,dest);}
static inline long swamp_recv(void *p,size_t n){return swamp_call(7,(uintptr_t)p,n);}
static inline long swamp_role(void){return swamp_call(8,0,0);}
static inline long swamp_scratch_write(const void*p,size_t n,unsigned long off){return swamp_call3(9,(uintptr_t)p,n,off);}
static inline long swamp_scratch_read(void*p,size_t n,unsigned long off){return swamp_call3(10,(uintptr_t)p,n,off);}

#define SWAMP_CAP_GUI 32u
#define SWAMP_GUI_WIDTH 16u
#define SWAMP_GUI_HEIGHT 8u
#define SWAMP_GUI_BYTES (SWAMP_GUI_WIDTH*SWAMP_GUI_HEIGHT)
static inline long swamp_gui_publish(const unsigned char *pixels){return swamp_call(11,(uintptr_t)pixels,SWAMP_GUI_BYTES);}
