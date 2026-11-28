#pragma once
#include <stdint.h>
#include <stddef.h>
#define CAP_CONSOLE (1ull<<0)
#define CAP_DATA (1ull<<1)
#define CAP_IPC (1ull<<2)
#define CAP_SCRATCH (1ull<<3)
#define CAP_GUI (1ull<<5) /* bit 4 remains reserved */
#define CAP_ALL (CAP_CONSOLE|CAP_DATA|CAP_IPC|CAP_SCRATCH|CAP_GUI)
#define SWP_SYS_GUI_PUBLISH 11ull
#define GUI_FRAME_BYTES 128u
#define SWP_SYS_YIELD 5ull
#define SWP_SYS_SEND 6ull
#define SWP_SYS_RECV 7ull
#define SWP_SYS_ROLE 8ull
#define SWP_SYS_SCRATCH_WRITE 9ull
#define SWP_SYS_SCRATCH_READ 10ull
#define SWP_SYS_ID 3ull
#define SWP_SYS_DATA_READ 4ull
#define SWP_SYS_EXIT 0ull
#define SWP_SYS_WRITE 1ull
#define SWP_SYS_CAPS 2ull
// Negative errno, or zero when policy permits the syscall.
int policy_check(uint64_t grants,uint64_t syscall,uint64_t address,size_t length);

int policy_check_mapped(uint64_t,uint64_t,uint64_t,size_t,int);
