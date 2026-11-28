#pragma once
#include <stdint.h>
#include <stddef.h>
#define SCRATCH_SIZE 1024u
struct scratch {size_t size;unsigned char bytes[SCRATCH_SIZE];};
void scratch_clear(struct scratch*);
long scratch_write(struct scratch*,uint64_t,const void*,size_t);
long scratch_read(const struct scratch*,uint64_t,void*,size_t);
int scratch_selftest(void);
