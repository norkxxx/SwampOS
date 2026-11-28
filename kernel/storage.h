#pragma once
int storage_init(void);
int storage_command(const char *);
int storage_selftest(void);

#include <stddef.h>
#include <stdint.h>
/* Trusted desktop/shell interface; no userspace syscall. */
int storage_guard_read(const char *,uint64_t *,uint64_t *);
int storage_guard_apply(const char *,uint64_t,uint64_t);
void storage_live_preview(unsigned,char*,size_t);
int storage_gui_demo(unsigned char*,size_t);

int storage_gui_start(void);
int storage_gui_poll(unsigned char*,size_t,int*);
