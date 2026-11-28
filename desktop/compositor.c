#include "compositor.h"

#define RECT_LIMIT (2 * WM_COUNT + 6)
struct damage {struct screen_rect list[RECT_LIMIT]; unsigned count;};

static int minimum(int a,int b){return a<b?a:b;}
static int maximum(int a,int b){return a>b?a:b;}
static void add(struct damage *d,struct screen_rect r,int width,int height){
 int right=minimum(width,r.x+r.w),bottom=minimum(height,r.y+r.h);
 r.x=maximum(0,r.x);r.y=maximum(0,r.y);r.w=right-r.x;r.h=bottom-r.y;
 if(r.w<=0||r.h<=0)return;
 /* Merge touching regions only if their bounding box is economical. */
 for(unsigned i=0;i<d->count;){
  struct screen_rect b=d->list[i];
  int left=minimum(r.x,b.x),top=minimum(r.y,b.y);
  int rw=maximum(r.x+r.w,b.x+b.w)-left,rh=maximum(r.y+r.h,b.y+b.h)-top;
  if(r.x<=b.x+b.w&&b.x<=r.x+r.w&&r.y<=b.y+b.h&&b.y<=r.y+r.h&&rw*rh<=r.w*r.h+b.w*b.h){
   r=(struct screen_rect){left,top,rw,rh};
   d->list[i]=d->list[--d->count];i=0;
  }else i++;
 }
 if(d->count==RECT_LIMIT){d->list[0]=(struct screen_rect){0,0,width,height};d->count=1;}
 else d->list[d->count++]=r;
}
static void scene_add(struct damage *d,struct wm_rect r){
 add(d,(struct screen_rect){r.x,r.y,r.w,r.h},WM_WIDTH,WM_HEIGHT);
}
static struct wm_rect visible(const struct wm_window *w){
 return w->open&&!w->minimized?w->rect:(struct wm_rect){0};
}
static int differs(struct wm_rect a,struct wm_rect b){
 return a.x!=b.x||a.y!=b.y||a.w!=b.w||a.h!=b.h;
}
int compositor_init(struct compositor *c,struct surface front,unsigned format,uint32_t *back,size_t words){
 if(!c)return 0;
 *c=(struct compositor){0};
 if(!surface_valid(&front)||front.width<WM_WIDTH||front.height<WM_HEIGHT||format>1||
    !back||words<SCENE_PIXELS||((uintptr_t)back|(uintptr_t)front.pixels)%sizeof(uint32_t))return 0;
 uintptr_t a=(uintptr_t)front.pixels,b=(uintptr_t)back;
 size_t bytes=((size_t)(front.height-1)*front.stride+front.width)*sizeof(*back);
 if(a>UINTPTR_MAX-bytes||b>UINTPTR_MAX-SCENE_PIXELS*sizeof(*back)||
    (a<b+SCENE_PIXELS*sizeof(*back)&&b<a+bytes))return 0;
 c->front=front;c->scene=(struct surface){back,SCENE_PIXELS,WM_WIDTH,WM_HEIGHT,WM_WIDTH};
 c->scale=front.width>=1280&&front.height>=720?2:1;
 c->left=(int)(front.width-WM_WIDTH*c->scale)/2;c->top=(int)(front.height-WM_HEIGHT*c->scale)/2;
 c->format=format;c->ready=1;
 return 1;
}
int compositor_due(const struct compositor *c,uint64_t tick){
 return c&&c->ready&&(!c->presented||tick-c->last_tick>=PRESENT_TICKS);
}
static void window_damage(struct compositor *c,struct damage *scene,const struct wm_state *w,unsigned content){
 unsigned bar=c->focus!=w->focus;
 for(unsigned i=0;i<WM_COUNT;i++){
  struct wm_rect old=c->bounds[i],now=visible(&w->windows[i]);
  if(differs(old,now)){
   scene_add(scene,old);scene_add(scene,now);
  }
  /* Minimized windows keep their task button. */
  unsigned opened=w->windows[i].open;
  if(c->open[i]!=opened)bar=1;
  if((content&(1u<<i))||(c->focus!=w->focus&&(i==c->focus||i==w->focus)))
   scene_add(scene,now);
  unsigned before=c->order[i],after=w->order[i];
  if(before!=after){
   scene_add(scene,visible(&w->windows[before]));
   scene_add(scene,visible(&w->windows[after]));
  }
  c->bounds[i]=now;c->order[i]=after;c->open[i]=opened;
 }
 if(bar)scene_add(scene,(struct wm_rect){0,WM_WORK,WM_WIDTH,WM_HEIGHT-WM_WORK});
 if(c->menu!=w->menu)scene_add(scene,(struct wm_rect){0,180,300,150});
}
static void scanout(struct compositor *c,struct screen_rect r,struct pointer_state ptr){
 const int z=(int)c->scale, width=WM_WIDTH*z,height=WM_HEIGHT*z;
 uint32_t border=c->format?0x07110c:0x0c1107;
 for(int y=r.y;y<r.y+r.h;y++){
  volatile uint32_t *out=c->front.pixels+(size_t)y*c->front.stride;
  int sy=y-c->top,py=y-ptr.y;
  const uint32_t *row=sy>=0&&sy<height?c->scene.pixels+(size_t)(sy/z)*WM_WIDTH:0;
  int cursor_row=ptr.visible&&py>=0&&py<13;
  for(int x=r.x;x<r.x+r.w;x++){
   int sx=x-c->left;
   uint32_t pixel=row&&sx>=0&&sx<width?row[(unsigned)sx>>(z-1)]:border;
   out[x]=cursor_row?surface_cursor_pixel(x-ptr.x,py,pixel,c->format):pixel;
  }
 }
 c->writes+=(unsigned)r.w*(unsigned)r.h;
}
int compositor_present(struct compositor *c,const struct wm_state *w,const struct live_state *d,
                       struct live_stats stats,uint64_t tick,unsigned windowed,unsigned content){
 if(!c||!c->ready||!d||d->view>=5||(windowed&&!wm_valid(w)))return 0;
 struct pointer_state ptr=windowed?(struct pointer_state){w->mouse_x,w->mouse_y,w->mouse_visible}:
                                    (struct pointer_state){d->mouse_x,d->mouse_y,d->mouse_visible};
 if(ptr.visible&&(ptr.x<0||ptr.y<0||(unsigned)ptr.x>=c->front.width||(unsigned)ptr.y>=c->front.height))return 0;
 if(!ptr.visible)ptr=(struct pointer_state){0};
 struct damage scene={0},output={0};
 int full=!c->presented||c->windowed!=windowed;
 if(windowed){
  window_damage(c,&scene,w,content);
  if(c->theme!=w->tools.theme)full=1;
  c->theme=w->tools.theme;c->focus=w->focus;c->menu=w->menu;
 }else if(c->view!=d->view)full=1;
 else if(content&(1u<<d->view))scene_add(&scene,(struct wm_rect){10,76,620,242});
 if(full){scene.count=1;scene.list[0]=(struct screen_rect){0,0,WM_WIDTH,WM_HEIGHT};}
 c->writes=c->paint_pixels=0;
 for(unsigned i=0;i<scene.count;i++){
  struct screen_rect r=scene.list[i];
  if(windowed)wm_paint_region(&c->scene,r,w,d,stats,c->format);
  else live_paint_region(&c->scene,r,d,stats,c->format);
  c->paint_pixels+=(unsigned)r.w*(unsigned)r.h;
  if(!full)add(&output,(struct screen_rect){c->left+r.x*(int)c->scale,c->top+r.y*(int)c->scale,
                     r.w*(int)c->scale,r.h*(int)c->scale},c->front.width,c->front.height);
 }
 if(full)add(&output,(struct screen_rect){0,0,(int)c->front.width,(int)c->front.height},c->front.width,c->front.height);
 else if(c->pointer.x!=ptr.x||c->pointer.y!=ptr.y||c->pointer.visible!=ptr.visible){
  if(c->pointer.visible)add(&output,(struct screen_rect){c->pointer.x,c->pointer.y,8,13},c->front.width,c->front.height);
  if(ptr.visible)add(&output,(struct screen_rect){ptr.x,ptr.y,8,13},c->front.width,c->front.height);
 }
 for(unsigned i=0;i<output.count;i++)scanout(c,output.list[i],ptr);
 c->pointer=ptr;c->presented=1;c->windowed=windowed;c->view=d->view;
 if(output.count||full)c->last_tick=tick;
 return 1;
}
