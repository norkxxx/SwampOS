#include <stdint.h>
static volatile uint16_t *const vga=(void*)0xb8000;
static unsigned pos;
static inline void out(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t in(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static void serial_init(void){out(0x3f9,0);out(0x3fb,0x80);out(0x3f8,3);out(0x3f9,0);out(0x3fb,3);out(0x3fa,0xc7);out(0x3fc,0x0b);}
static void put(char c){for(unsigned i=0;i<100000;i++)if(in(0x3fd)&32){out(0x3f8,c);break;}if(c=='\n')pos=(pos/80+1)*80;else if(c=='\b'){if(pos){--pos;vga[pos]=0x0720;}}else if(c!='\r'){if(pos<2000)vga[pos++]=0x0a00|(uint8_t)c;}if(pos>=2000){for(unsigned i=0;i<1920;i++)vga[i]=vga[i+80];for(unsigned i=1920;i<2000;i++)vga[i]=0x0720;pos=1920;}}
static void say(const char*s){while(*s)put(*s++);}
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void clear(void){for(unsigned i=0;i<2000;i++)vga[i]=0x0720;pos=0;}
static void command(char*s){if(eq(s,"help"))say("help clear version onion shrek status halt\n");else if(eq(s,"clear"))clear();else if(eq(s,"version"))say("SwampOS 0.2.0 Onion / x86_64\n");else if(eq(s,"onion"))say("Security has layers. Isolation not implemented yet.\n");else if(eq(s,"shrek"))say("WHAT ARE YOU DOING IN MY SWAMP?!\n");else if(eq(s,"status"))say("Long mode; identity paging; supervisor-only mappings. No userspace or sandbox.\n");else if(eq(s,"halt")){say("Swamp halted.\n");for(;;)__asm__ volatile("cli; hlt");}else if(*s)say("Unknown command. Type help.\n");}
void kmain(void){serial_init();clear();say("SwampOS 0.2.0 Onion\nSecurity has layers.\nBOOT_OK\n");char buf[128];unsigned n=0;say("swamp> ");for(;;){if(!(in(0x3fd)&1))continue;char c=in(0x3f8);if(c=='\r'||c=='\n'){put('\n');buf[n]=0;command(buf);n=0;say("swamp> ");}else if(c==8||c==127){if(n){n--;put('\b');}}else if(c>=32&&c<=126&&n<sizeof(buf)-1){buf[n++]=c;put(c);}}}
