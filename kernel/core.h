#pragma once
#include <stdint.h>
#include <stddef.h>
void core_init(void);
void timer_irq(void);
void *page_alloc(void);
int page_free(void *p);
void *heap_alloc(size_t n);
uint64_t ticks(void);
unsigned pages_used(void);
int core_test(void);
