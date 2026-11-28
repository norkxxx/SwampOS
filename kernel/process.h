#pragma once
#include <stdint.h>
#include <stddef.h>
#define PROC_MAX 4u
#define PROC_MSG_MAX 64u
#define PROC_QUEUE 4u
#define PROC_QUANTUM 2u
#define PROC_BUDGET 100u
#define PROC_TIMEOUT 0x200
#define PROC_READY 1u
#define PROC_RUNNING 2u
#define PROC_DONE 3u
struct proc_message {uint64_t sender;size_t len;unsigned char data[PROC_MSG_MAX];};
struct proc_slot {uint64_t id;unsigned state,spent,slice,head,count;int result;struct proc_message queue[PROC_QUEUE];};
struct proc_group {unsigned count,current;struct proc_slot slots[PROC_MAX];};
int proc_init(struct proc_group*,unsigned,uint64_t);
int proc_next(struct proc_group*);
int proc_finish(struct proc_group*,int);
int proc_tick(struct proc_group*);
long proc_send(struct proc_group*,unsigned,const void*,size_t);
long proc_receive(struct proc_group*,void*,size_t);
void proc_clear(struct proc_group*);
int proc_selftest(void);
