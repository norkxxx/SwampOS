#include "task.h"
#include <stdint.h>
static _Alignas(16) unsigned char stacks[2][4096];
static uintptr_t contexts[2], host;
static unsigned steps[2], current, done[2];
extern void task_switch(uintptr_t *old, uintptr_t next);
static void yield(void){task_switch(&contexts[current],host);}
static void worker(void){unsigned id=current; for(unsigned i=0;i<5;i++){steps[id]++;yield();}done[id]=1;yield();for(;;)__asm__ volatile("cli; hlt");}
void task_demo(void){for(unsigned i=0;i<2;i++){uintptr_t *s=(uintptr_t*)(stacks[i]+sizeof stacks[i]);*--s=0;*--s=(uintptr_t)worker;for(unsigned j=0;j<6;j++)*--s=0;contexts[i]=(uintptr_t)s;steps[i]=done[i]=0;}while(!done[0]||!done[1]){for(current=0;current<2;current++)if(!done[current])task_switch(&host,contexts[current]);}}
unsigned task_steps(unsigned i){return i<2?steps[i]:0;}
