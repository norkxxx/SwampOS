#include "../desktop/compositor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CANARY 0xdeadbeefu
struct fixture {
 struct compositor c;
 struct wm_state w;
 struct live_state d;
 struct surface target, reference;
 uint32_t *front,*expected,*scene;
 unsigned windowed;
 uint64_t tick;
};
static const struct live_stats sample={12345,1777,2024};

static void check_frame(struct fixture *f,unsigned content){
 assert(compositor_present(&f->c,&f->w,&f->d,sample,f->tick,f->windowed,content));
 int ok=f->windowed?wm_render(&f->reference,&f->w,&f->d,sample,f->c.format):
                     live_render(&f->reference,&f->d,sample,f->c.format);
 assert(ok);
 for(unsigned y=0;y<f->target.height;y++){
  size_t start=(size_t)y*f->target.stride;
  if(memcmp(f->front+1+start,f->expected+1+start,f->target.width*4)){
   fprintf(stderr,"pixel mismatch at row %u, tick %llu, mode %u, focus %u\n",
           y,(unsigned long long)f->tick,f->windowed,f->w.focus);abort();
  }
  for(unsigned x=f->target.width;x<f->target.stride;x++)assert(f->front[start+x+1]==CANARY);
 }
 assert(f->front[0]==CANARY&&f->front[f->target.capacity+1]==CANARY);
 assert(f->scene[0]==CANARY&&f->scene[SCENE_PIXELS+1]==CANARY);
 f->tick+=PRESENT_TICKS;
}
static void move(struct fixture *f,int x,int y,unsigned buttons){
 wm_mouse(&f->w,&f->d,f->target.width,f->target.height,x-f->w.mouse_x,y-f->w.mouse_y,buttons);
}
static void sequence(unsigned width,unsigned height,unsigned format){
 struct fixture f={0};size_t size=(size_t)(width+7)*height;
 f.front=malloc((size+2)*4);f.expected=calloc(size+2,4);f.scene=malloc((SCENE_PIXELS+2)*4);
 assert(f.front&&f.scene&&f.expected);
 for(size_t i=0;i<size+2;i++)f.front[i]=CANARY;
 f.scene[0]=f.scene[SCENE_PIXELS+1]=CANARY;
 f.target=(struct surface){f.front+1,size,width,height,width+7};
 f.reference=(struct surface){f.expected+1,size,width,height,width+7};
 assert(!compositor_init(&f.c,f.target,format,f.scene+1,SCENE_PIXELS-1));
 assert(!compositor_due(&f.c,0));
 assert(!compositor_init(&f.c,f.target,format,f.front+1,size));
 assert(!compositor_init(&f.c,f.target,2,f.scene+1,SCENE_PIXELS));
 struct surface short_row=f.target;short_row.capacity-=8;
 assert(!compositor_init(&f.c,short_row,format,f.scene+1,SCENE_PIXELS));
 assert(compositor_init(&f.c,f.target,format,f.scene+1,SCENE_PIXELS));
 wm_init(&f.w);live_init(&f.d);f.windowed=1;
 assert(compositor_due(&f.c,0));check_frame(&f,0);
 assert(f.c.writes==(uint64_t)width*height&&!compositor_due(&f.c,1)&&compositor_due(&f.c,2));
 wm_mouse(&f.w,&f.d,width,height,0,0,0);check_frame(&f,0);
 assert(f.c.writes==104&&!f.c.paint_pixels);
 move(&f,20,20,0);check_frame(&f,0);
 assert(f.c.writes==208&&!f.c.paint_pixels);
 for(unsigned i=0;i<10000;i++)wm_mouse(&f.w,&f.d,width,height,(i&1)?2:-1,0,0);
 check_frame(&f,0);assert(!f.c.paint_pixels&&f.c.writes<=208);
 check_frame(&f,0);assert(!f.c.writes&&!f.c.paint_pixels);
 int z=(int)f.c.scale, ox=f.c.left,oy=f.c.top;
 wm_open(&f.w,1);check_frame(&f,0);
 struct wm_rect r=f.w.windows[1].rect;
 int x=ox+(r.x+10)*z,y=oy+(r.y+8)*z;
 move(&f,x,y,0);move(&f,x,y,1);move(&f,x+24*z,y+12*z,1);move(&f,x+24*z,y+12*z,0);
 /* No presentation between press, motion and release. */
 assert(f.w.drag==WM_NONE&&f.w.windows[1].rect.x==r.x+24);
 check_frame(&f,0);assert(f.c.paint_pixels<SCENE_PIXELS);
 r=f.w.windows[1].rect;x=ox+(r.x+r.w-4)*z;y=oy+(r.y+r.h-4)*z;
 move(&f,x,y,0);move(&f,x,y,1);move(&f,x-45*z,y-20*z,1);move(&f,x-45*z,y-20*z,0);
 assert(f.w.resize==WM_NONE&&f.w.windows[1].rect.w==r.w-45);
 check_frame(&f,0);
 wm_key(&f.w,&f.d,'A');wm_key(&f.w,&f.d,'B');assert(!strcmp(f.d.note,"AB"));check_frame(&f,1u<<1);
 for(unsigned i=0;i<WM_COUNT;i++){wm_open(&f.w,i);check_frame(&f,0);}
 wm_maximize(&f.w,TOOL_GUARD);check_frame(&f,0);
 wm_minimize(&f.w,TOOL_GUARD);check_frame(&f,0);
 wm_close(&f.w,TOOL_GUARD);check_frame(&f,0);
 f.w.tools.theme=2;check_frame(&f,0);assert(f.c.paint_pixels==SCENE_PIXELS);
 f.w.menu=1;check_frame(&f,0);f.w.menu=0;check_frame(&f,0);
 /* Several raises inside the same PIT interval, including an unfocused overlap. */
 wm_open(&f.w,2);wm_open(&f.w,0);wm_open(&f.w,3);check_frame(&f,0);
 move(&f,(int)width-1,(int)height-1,0);check_frame(&f,0);
 assert(f.c.writes<=208);
 f.windowed=0;check_frame(&f,0);
 for(unsigned i=0;i<5;i++){live_key(&f.d,9);check_frame(&f,0);}
 f.d.view=0;live_put(&f.d,'q');check_frame(&f,1);
 live_mouse(&f.d,width,height,INT_MIN,INT_MAX,0);check_frame(&f,0);
 live_mouse(&f.d,width,height,INT_MAX,INT_MIN,0);check_frame(&f,0);assert(!f.c.paint_pixels);
 f.c.last_tick=UINT64_MAX;assert(!compositor_due(&f.c,0)&&compositor_due(&f.c,1));
 printf("PASS %ux%u format %u: full %u pixels, disjoint cursor 208 writes, 0 scene pixels; 10000 motion events coalesced\n",width,height,format,width*height);
 free(f.front);free(f.expected);free(f.scene);
}
int main(void){
 sequence(640,360,1);sequence(801,601,0);sequence(1291,733,0);sequence(1280,720,1);
 return 0;
}
