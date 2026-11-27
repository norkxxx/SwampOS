#include "core.h"
/* Reserved kernel BSS arena, not a firmware-wide physical allocator. */
#define N 32
static _Alignas(4096) unsigned char arena[N][4096];
static uint32_t used;
static _Alignas(16) unsigned char heap[8192];
static size_t cursor;
static volatile uint64_t timer;
struct gate {uint16_t lo,sel; uint8_t ist,attr; uint16_t mid; uint32_t hi,zero;} __attribute__((packed));
static struct gate idt[256];
extern void *exception_table[];
extern void irq_timer(void), irq_default(void), trap_syscall(void);
static void out(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static void pic(uint16_t p,uint8_t v){out(p,v);out(0x80,0);}
static void gate(unsigned i,void *p){uintptr_t a=(uintptr_t)p;idt[i]=(struct gate){a,24,0,0x8e,a>>16,a>>32,0};}
void core_init(void){for(unsigned i=0;i<256;i++)gate(i,irq_default);for(unsigned i=0;i<32;i++)gate(i,exception_table[i]);gate(32,irq_timer);gate(128,trap_syscall);idt[128].attr=0xee;struct {uint16_t limit;uintptr_t base;} __attribute__((packed)) d={sizeof(idt)-1,(uintptr_t)idt};__asm__ volatile("lidt %0"::"m"(d));pic(0x20,0x11);pic(0xa0,0x11);pic(0x21,0x20);pic(0xa1,0x28);pic(0x21,4);pic(0xa1,2);pic(0x21,1);pic(0xa1,1);pic(0x21,0xfe);pic(0xa1,0xff);out(0x43,0x36);out(0x40,11932&255);out(0x40,11932>>8);__asm__ volatile("sti");}
void timer_irq(void){timer++;out(0x20,0x20);}
uint64_t ticks(void){return timer;}
void *page_alloc(void){for(unsigned i=0;i<N;i++)if(!(used&(1u<<i))){used|=1u<<i;for(unsigned j=0;j<4096;j++)arena[i][j]=0;return arena[i];}return 0;}
int page_free(void *p){uintptr_t a=(uintptr_t)p,b=(uintptr_t)arena;if(a<b||a-b>=sizeof arena||(a-b)%4096)return 0;unsigned i=(a-b)/4096;if(!(used&(1u<<i)))return 0;used&=~(1u<<i);return 1;}
unsigned pages_used(void){unsigned n=0;for(unsigned i=0;i<N;i++)n+=(used>>i)&1;return n;}
void *heap_alloc(size_t n){if(!n||n>sizeof heap- cursor)return 0;n=(n+15)&~(size_t)15;if(n>sizeof heap-cursor)return 0;void *p=heap+cursor;cursor+=n;return p;}
int core_test(void){void *p[N];for(unsigned i=0;i<N;i++){p[i]=page_alloc();if(!p[i]||((uintptr_t)p[i]&4095))return 0;}if(page_alloc()||pages_used()!=N)return 0;for(unsigned i=0;i<N;i++)if(!page_free(p[i]))return 0;if(page_free(p[0])||page_free((char*)p[0]+1))return 0;unsigned char *a=page_alloc();if(!a)return 0;a[0]=42;if(!page_free(a))return 0;a=page_alloc();if(!a||a[0]||!page_free(a))return 0;size_t old=cursor;void *h=heap_alloc(17);int ok=h&& !((uintptr_t)h&15)&&!heap_alloc(SIZE_MAX)&&!heap_alloc(0);cursor=old;return ok;}
