#pragma once
#include "pmm.h"
#define USER_CODE 0x40000000ull
#define USER_GUARD 0x40001000ull
#define USER_STACK 0x40002000ull
#define USER_TOP (USER_STACK+4096)
struct space {uint64_t root, pdpt, pd, pt, code, stack;};
extern struct pmm frames;
int vm_init(void);
int vm_create(struct space *,const void *,size_t);
void vm_destroy(struct space *);
void vm_activate(uint64_t);
uint64_t vm_kernel_root(void);
int user_readable(uint64_t,size_t);
int vm_selftest(void);
