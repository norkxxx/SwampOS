#include "ps2.h"
#include "live.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static unsigned absent,stuck,no_mouse,config_pending,aux_pending,head,tail,retry,mouse_commands,keyboard_commands;
static uint8_t cfg;static struct {uint8_t status,data;} q[256];
static void enqueue(uint8_t status,uint8_t data){assert(tail<256);q[tail].status=status|1;q[tail++].data=data;}
uint8_t ps2_in(uint16_t p){if(absent)return 255;if(p==0x64)return stuck?2:(head<tail?q[head].status:0);assert(p==0x60&&head<tail);return q[head++].data;}
void ps2_out(uint16_t p,uint8_t v){if(p==0x64){switch(v){case 0xad:cfg|=16;break;case 0xa7:cfg|=32;break;case 0xae:cfg&=~16;break;case 0xa8:cfg&=~32;break;case 0x20:enqueue(0,cfg);break;case 0x60:config_pending=1;break;case 0xd4:aux_pending=1;break;default:assert(0);}return;}assert(p==0x60);if(config_pending){cfg=v;config_pending=0;return;}if(aux_pending){aux_pending=0;mouse_commands++;if(!no_mouse)enqueue(32,0xfa);}else{keyboard_commands++;if(retry){retry=0;enqueue(0,0xfe);}else enqueue(0,0xfa);}}
static void reset(void){absent=stuck=no_mouse=config_pending=aux_pending=head=tail=retry=mouse_commands=keyboard_commands=0;cfg=3;}
int main(void){struct mouse_event e;int k;reset();retry=1;assert(ps2_init(1)==3);assert(cfg==0x40&&keyboard_commands==5&&mouse_commands==3);
 enqueue(0,0x1e);assert(!ps2_poll(&e,&k)&&k=='a');enqueue(32,0x29);enqueue(32,7);enqueue(32,0xfe);assert(!ps2_poll(&e,&k)&&!k);assert(!ps2_poll(&e,&k)&&!k);assert(ps2_poll(&e,&k)&&!k&&e.dx==7&&e.dy==2&&e.buttons==1);
 enqueue(0x80,0x1e);assert(!ps2_poll(&e,&k)&&!k);enqueue(0,0x0f);assert(!ps2_poll(&e,&k)&&k==9);
 reset();assert(ps2_init(0)==1&&mouse_commands==0);reset();no_mouse=1;assert(ps2_init(1)==1);reset();absent=1;assert(!ps2_init(1)&&!ps2_poll(&e,&k)&&!k);reset();stuck=1;assert(!ps2_init(1));
 struct ps2_mouse m={0};assert(!ps2_mouse_decode(&m,0,&e));assert(!ps2_mouse_decode(&m,0x18,&e));assert(!ps2_mouse_decode(&m,0xff,&e));assert(ps2_mouse_decode(&m,10,&e)&&e.dx==-1&&e.dy==-10);assert(!ps2_mouse_decode(&m,0xc8,&e));assert(!ps2_mouse_decode(&m,255,&e));assert(ps2_mouse_decode(&m,255,&e)&&!e.dx&&!e.dy);
 struct live_state s;live_init(&s);live_mouse(&s,640,360,-120,-125,1);assert(s.view==1&&s.mouse_visible);live_mouse(&s,640,360,124,0,1);assert(s.view==1);live_mouse(&s,640,360,0,0,0);live_mouse(&s,640,360,0,0,1);assert(s.view==2);live_mouse(&s,640,360,INT_MAX,INT_MIN,0);assert(s.mouse_x==639&&s.mouse_y==0);
 static uint32_t buf[640*360+2];buf[0]=buf[640*360+1]=0x12345678;struct surface f={buf+1,640*360,640,360,640};assert(live_render(&f,&s,(struct live_stats){0},1));assert(buf[0]==0x12345678&&buf[640*360+1]==0x12345678);
 unsigned seed=1;for(unsigned i=0;i<100000;i++){seed=seed*1664525u+1013904223u;if(ps2_mouse_decode(&m,(uint8_t)(seed>>16),&e)){live_mouse(&s,640,360,e.dx,e.dy,e.buttons);assert(s.mouse_x>=0&&s.mouse_x<640&&s.mouse_y>=0&&s.mouse_y<360&&s.view<5);}}
 puts("PASS: mock PS/2 init, ACK/resend, absent/stuck controller, keyboard/aux demux, mouse packets, clicks, bounds, 100000 decoder bytes (UBSan)");}
