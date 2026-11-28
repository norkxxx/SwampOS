#include "../desktop/window.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
static uint32_t pixels[640*360+2];
static void move(struct wm_state*w,struct live_state*d,int x,int y,unsigned b){wm_mouse(w,d,640,360,x-w->mouse_x,y-w->mouse_y,b);}
static void click(struct wm_state*w,struct live_state*d,int x,int y){move(w,d,x,y,0);move(w,d,x,y,1);move(w,d,x,y,0);}
static void preview(void){FILE*f=fopen("build/awakens-preview.ppm","wb");assert(f);fprintf(f,"P6\n640 360\n255\n");for(unsigned i=1;i<=640*360;i++){uint32_t c=pixels[i];fputc(c>>16,f);fputc(c>>8,f);fputc(c,f);}assert(!fclose(f));}
int main(void){struct wm_state w;struct live_state d;wm_init(&w);live_init(&d);assert(wm_valid(&w)&&w.focus==0);wm_mouse(&w,&d,640,360,0,0,0);
 click(&w,&d,30,80);assert(w.focus==1&&w.windows[0].open);assert(wm_key(&w,&d,'a')&&d.note[0]=='a');click(&w,&d,w.windows[1].rect.x+20,w.windows[1].rect.y+32);assert(w.action==1);w.action=0;click(&w,&d,w.windows[1].rect.x+70,w.windows[1].rect.y+32);assert(w.action==2);w.action=0;
 struct wm_rect r=w.windows[1].rect;move(&w,&d,r.x+10,r.y+10,0);move(&w,&d,r.x+10,r.y+10,1);move(&w,&d,r.x+40,r.y+20,1);move(&w,&d,r.x+40,r.y+20,0);assert(w.windows[1].rect.x==r.x+30&&w.windows[1].rect.y==r.y+10);
 r=w.windows[1].rect;move(&w,&d,r.x+r.w-5,r.y+r.h-5,0);move(&w,&d,r.x+r.w-5,r.y+r.h-5,1);move(&w,&d,r.x+r.w-35,r.y+r.h-35,1);move(&w,&d,r.x+r.w-35,r.y+r.h-35,0);assert(w.windows[1].rect.w==r.w-30&&w.windows[1].rect.h==r.h-30);
 r=w.windows[1].rect;click(&w,&d,r.x+r.w-30,r.y+10);assert(w.windows[1].maximized&&w.windows[1].rect.w==640);click(&w,&d,610,10);assert(!w.windows[1].maximized&&w.windows[1].rect.w==r.w);
 click(&w,&d,r.x+r.w-50,r.y+10);assert(w.windows[1].minimized&&w.focus==0);click(&w,&d,200,342);assert(!w.windows[1].minimized&&w.focus==1);click(&w,&d,r.x+r.w-10,r.y+10);assert(!w.windows[1].open&&w.focus==0);
 click(&w,&d,20,342);assert(w.menu);click(&w,&d,30,250);assert(w.focus==2&&!w.menu);for(const char*p="19*23";*p;p++)assert(wm_key(&w,&d,*p));wm_key(&w,&d,10);assert(!strcmp(d.answer,"437"));
 struct surface s={pixels+1,640*360,640,360,640};pixels[0]=pixels[640*360+1]=0x12345678;wm_init(&w);live_clear(&d);for(const char*p="SWAMPOS 0.20.0 DEVELOPMENT\nSecurity has layers.\nswamp> ";*p;p++)live_put(&d,*p);wm_open(&w,2);w.windows[2].rect=(struct wm_rect){280,145,330,170};assert(wm_render(&s,&w,&d,(struct live_stats){0},1));preview();
 unsigned seed=1;for(unsigned i=0;i<100000;i++){seed=seed*1664525u+1013904223u;switch(seed%8){case 0:wm_open(&w,(seed>>8)%WM_COUNT);break;case 1:wm_close(&w,(seed>>8)%WM_COUNT);break;case 2:wm_minimize(&w,(seed>>8)%WM_COUNT);break;case 3:wm_maximize(&w,(seed>>8)%WM_COUNT);break;default:wm_mouse(&w,&d,640,360,(int)((seed>>8)%1000)-500,(int)((seed>>18)%1000)-500,(seed>>5)&7);break;}assert(wm_valid(&w));if(i%1000==0)assert(wm_render(&s,&w,&d,(struct live_stats){i,i,50000},i&1));}wm_mouse(&w,&d,640,360,INT_MAX,INT_MIN,0);assert(wm_valid(&w));assert(pixels[0]==0x12345678&&pixels[640*360+1]==0x12345678);puts("PASS: window drag/resize/focus/minimize/maximize/restore/close, taskbar, launcher, calculator, 100000 window events, framebuffer canaries");}
