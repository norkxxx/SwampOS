#include <stdint.h>
#include "core.h"
#include "task.h"
#include "vm.h"
#include "user.h"
static volatile uint16_t *const vga=(void*)0xb8000;
static unsigned pos;
static inline void out(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t in(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static void serial_init(void){out(0x3f9,0);out(0x3fb,0x80);out(0x3f8,3);out(0x3f9,0);out(0x3fb,3);out(0x3fa,0xc7);out(0x3fc,0x0b);}
static void put(char c){for(unsigned i=0;i<100000;i++)if(in(0x3fd)&32){out(0x3f8,c);break;}if(c=='\n')pos=(pos/80+1)*80;else if(c=='\b'){if(pos){--pos;vga[pos]=0x0720;}}else if(c!='\r'){if(pos<2000)vga[pos++]=0x0a00|(uint8_t)c;}if(pos>=2000){for(unsigned i=0;i<1920;i++)vga[i]=vga[i+80];for(unsigned i=1920;i<2000;i++)vga[i]=0x0720;pos=1920;}}
static void say(const char*s){while(*s)put(*s++);}
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void clear(void){for(unsigned i=0;i<2000;i++)vga[i]=0x0720;pos=0;}
static void number(uint64_t n){char b[21];unsigned i=0;do{b[i++]='0'+n%10;n/=10;}while(n);while(i)put(b[--i]);}
void console_write(const char*s,size_t n){while(n--)put(*s++);}
void console_number(uint64_t n){number(n);}
static void command(char*s){if(eq(s,"help"))say("help clear version onion shrek status mem ticks selftest tasks vmtest sandbox pmm halt\n");else if(eq(s,"clear"))clear();else if(eq(s,"version"))say("SwampOS 0.5.0 SwampBox / x86_64\n");else if(eq(s,"onion"))say("Security has layers. Experimental ring 3 isolation.\n");else if(eq(s,"shrek"))say("WHAT ARE YOU DOING IN MY SWAMP?!\n");else if(eq(s,"status"))say("Long mode; 4 KiB paging; NX; experimental ring 3. No app ecosystem.\n");else if(eq(s,"mem")){say("Arena pages used: ");number(pages_used());say(" / 32\n");}else if(eq(s,"ticks")){number(ticks());say("\n");}else if(eq(s,"selftest"))say(core_test()?"CORE_TEST_OK\n":"CORE_TEST_FAIL\n");else if(eq(s,"tasks")){task_demo();say("Task A: ");number(task_steps(0));say(" Task B: ");number(task_steps(1));say("\n");}else if(eq(s,"pmm")){say("Free frames: ");number(frames.free);say(" / ");number(frames.total);say("\n");}else if(eq(s,"vmtest"))say(vm_selftest()?"VM_TEST_OK\n":"VM_TEST_FAIL\n");else if(eq(s,"sandbox"))say(user_selftest()?"USER_TEST_OK\n":"USER_TEST_FAIL\n");else if(eq(s,"halt")){say("Swamp halted.\n");for(;;)__asm__ volatile("cli; hlt");}else if(*s)say("Unknown command. Type help.\n");}
void kmain(void){serial_init();clear();core_init();if(!vm_init()){say("VM_INIT_FAIL\n");for(;;)__asm__ volatile("cli; hlt");}user_init();if(!vm_selftest()){say("VM_TEST_FAIL\n");goto fatal;}say("VM_TEST_OK\n");if(!user_selftest()){say("USER_TEST_FAIL\n");goto fatal;}say("USER_TEST_OK\n");if(!core_test()){say("CORE_TEST_FAIL\n");goto fatal;}say("CORE_TEST_OK\n");say("SwampOS 0.5.0 SwampBox\nSecurity has layers.\nBOOT_OK\n");char buf[128];unsigned n=0;say("swamp> ");for(;;){if(!(in(0x3fd)&1))continue;char c=in(0x3f8);if(c=='\r'||c=='\n'){put('\n');buf[n]=0;command(buf);n=0;say("swamp> ");}else if(c==8||c==127){if(n){n--;put('\b');}}else if(c>=32&&c<=126&&n<sizeof(buf)-1){buf[n++]=c;put(c);}}fatal: for(;;)__asm__ volatile("cli; hlt");}
