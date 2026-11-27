#pragma once
#include <stdint.h>
#include <stddef.h>
#define CAP_CONSOLE (1ull<<0)
#define SWP_SYS_EXIT 0ull
#define SWP_SYS_WRITE 1ull
#define SWP_SYS_CAPS 2ull
// Negative errno, or zero when policy permits the syscall.
int policy_check(uint64_t grants,uint64_t syscall,uint64_t address,size_t length);
