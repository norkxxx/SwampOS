#pragma once
#include <stdint.h>
#include <stddef.h>
#define SWAMP_CAP_CONSOLE 1u
#define SWAMP_CAP_DATA 2u
#define SWAMP_DATA_READ_MAX 256u
#define SWAMP_DATA_MAX (64u * 1024u)
unsigned long swamp_caps(void);
long swamp_data_read(void *,size_t,unsigned long);
long swamp_write(const char *,size_t);
__attribute__((noreturn)) void swamp_exit(unsigned);
