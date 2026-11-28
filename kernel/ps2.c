#include "ps2.h"
#include "hardware.h"
int ps2_mouse_decode(struct ps2_mouse*m,uint8_t v,struct mouse_event*e){
 if(!m||!e)return 0;
 if(m->used>=3)m->used=0;
 if(!m->used&&!(v&8))return 0;
 m->packet[m->used++]=v;if(m->used!=3)return 0;m->used=0;
 unsigned h=m->packet[0];e->buttons=h&7;
 e->dx=(h&0x40)?0:(int)m->packet[1]-((h&0x10)?256:0);
 e->dy=(h&0x80)?0:((h&0x20)?256:0)-(int)m->packet[2];return 1;
}
#ifdef PS2_MOCK
extern uint8_t ps2_in(uint16_t);extern void ps2_out(uint16_t,uint8_t);
#else
static uint8_t ps2_in(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static void ps2_out(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
#endif
static struct keyboard kb;
static struct ps2_mouse ms;
static unsigned ready;
static int wr(uint16_t p,uint8_t v){for(unsigned i=0;i<100000;i++){uint8_t s=ps2_in(0x64);if(s==255)return 0;if(!(s&2)){ps2_out(p,v);return 1;}}return 0;}
static int rd(int aux,uint8_t*v){for(unsigned i=0;i<100000;i++){uint8_t s=ps2_in(0x64);if(s==255)return 0;if(s&1){uint8_t x=ps2_in(0x60);if(!(s&0xc0)&&!!(s&32)==aux){*v=x;return 1;}}}return 0;}
static void flush(void){for(unsigned i=0;i<64;i++){uint8_t s=ps2_in(0x64);if(s==255||!(s&1))break;(void)ps2_in(0x60);}}
static int command(int aux,uint8_t v){for(unsigned i=0;i<3;i++){uint8_t a;if((aux&&!wr(0x64,0xd4))||!wr(0x60,v)||!rd(aux,&a))return 0;if(a==0xfa)return 1;if(a!=0xfe)return 0;}return 0;}
unsigned ps2_init(int mouse){uint8_t cfg;ready=0;kb=(struct keyboard){0};ms=(struct ps2_mouse){0};
 if(!wr(0x64,0xad)||!wr(0x64,0xa7))return 0;
 flush();if(!wr(0x64,0x20)||!rd(0,&cfg))goto fallback;
 /* Translation to set 1, IRQs off, first port enabled, auxiliary disabled until requested. */
 cfg=(uint8_t)((cfg&~0x13u)|0x60u);
 if(!wr(0x64,0x60)||!wr(0x60,cfg)||!wr(0x64,0xae))goto fallback;
 if(command(0,0xf5)&&command(0,0xf0)&&command(0,2)&&command(0,0xf4))ready|=1;
 else if(command(0,0xf4))ready|=1; /* firmware translation may already be configured */
 if(mouse&&wr(0x64,0xa8)&&command(1,0xf5)&&command(1,0xf6)&&command(1,0xf4))ready|=2;
 else (void)wr(0x64,0xa7);
 return ready;
fallback: (void)wr(0x64,0xae);(void)command(0,0xf4);return 0;
}
int ps2_poll(struct mouse_event*e,int*key){*key=0;uint8_t s=ps2_in(0x64);if(s==255||!(s&1))return 0;uint8_t v=ps2_in(0x60);if(s&0xc0){ms.used=0;kb.extended=0;return 0;}if(s&32)return (ready&2)?ps2_mouse_decode(&ms,v,e):0;*key=keyboard_decode(&kb,v);return 0;}
