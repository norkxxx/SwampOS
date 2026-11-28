#pragma once
#include <stdint.h>
#define AUDIT_CAPACITY 64u
struct audit_event {uint64_t seq;uint32_t kind;int32_t result;uint64_t detail;};
struct audit_log {uint64_t next;unsigned count,head;struct audit_event event[AUDIT_CAPACITY];};
extern struct audit_log kernel_audit;
void audit_add(struct audit_log*,uint32_t,int32_t,uint64_t);
int audit_get(const struct audit_log*,unsigned,struct audit_event*);
int audit_selftest(void);
