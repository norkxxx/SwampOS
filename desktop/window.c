#include "window.h"
static const char *names[WM_COUNT]={"TERMINAL","NOTES","CALC","MONITOR","FILES","RAM FILES","HEX","PAINT","SETTINGS","GUARD"};
static int clamp(int64_t v,int lo,int hi){return v<lo?lo:v>hi?hi:(int)v;}
static int hit(struct wm_rect r,int x,int y){return x>=r.x&&y>=r.y&&x<r.x+r.w&&y<r.y+r.h;}
static int shown(const struct wm_window*w){return w->open&&!w->minimized;}
static void refocus(struct wm_state*s){s->focus=WM_NONE;for(unsigned i=WM_COUNT;i;i--)if(shown(&s->windows[s->order[i-1]])){s->focus=s->order[i-1];break;}}
static void raise(struct wm_state*s,unsigned id){unsigned i=0;while(i<WM_COUNT&&s->order[i]!=id)i++;if(i==WM_COUNT)return;for(;i+1<WM_COUNT;i++)s->order[i]=s->order[i+1];s->order[WM_COUNT-1]=id;s->focus=id;s->dirty=1;}
void wm_open(struct wm_state*s,unsigned id){if(id>=WM_COUNT)return;s->windows[id].open=1;s->windows[id].minimized=0;s->menu=0;raise(s,id);}
void wm_close(struct wm_state*s,unsigned id){if(id>=WM_COUNT)return;s->windows[id].open=0;s->windows[id].minimized=0;if(id==TOOL_GUARD)s->tools.guard=(struct guard_editor){0};if(s->focus==id)refocus(s);s->drag=s->resize=WM_NONE;s->dirty=1;}
void wm_minimize(struct wm_state*s,unsigned id){if(id>=WM_COUNT||!s->windows[id].open)return;s->windows[id].minimized=1;if(s->focus==id)refocus(s);s->drag=s->resize=WM_NONE;s->dirty=1;}
void wm_maximize(struct wm_state*s,unsigned id){if(id>=WM_COUNT||!shown(&s->windows[id]))return;struct wm_window*w=&s->windows[id];if(w->maximized){w->rect=w->restore;w->maximized=0;}else{w->restore=w->rect;w->rect=(struct wm_rect){0,0,WM_WIDTH,WM_WORK};w->maximized=1;}s->drag=s->resize=WM_NONE;s->dirty=1;}
void wm_init(struct wm_state*s){*s=(struct wm_state){0};for(unsigned i=0;i<WM_COUNT;i++){s->order[i]=i;s->windows[i].rect=(struct wm_rect){76+(int)(i%5)*22,22+(int)(i%5)*14,448,242};}s->windows[0].rect=(struct wm_rect){75,25,550,290};s->drag=s->resize=s->focus=WM_NONE;tools_init(&s->tools);wm_open(s,0);}
int wm_valid(const struct wm_state*s){if(!s||s->focus>WM_NONE||s->drag>WM_NONE||s->resize>WM_NONE)return 0;unsigned seen=0;for(unsigned i=0;i<WM_COUNT;i++){unsigned id=s->order[i];if(id>=WM_COUNT||(seen&(1u<<id)))return 0;seen|=1u<<id;struct wm_rect r=s->windows[i].rect;if(r.x<0||r.y<0||r.w<160||r.h<100||r.x>WM_WIDTH-r.w||r.y>WM_WORK-r.h)return 0;}return s->focus==WM_NONE||shown(&s->windows[s->focus]);}
int wm_key(struct wm_state*s,struct live_state*d,int k){if(k==9){wm_open(s,s->focus==WM_NONE?0:(s->focus+1)%WM_COUNT);return 1;}if(k==27){wm_open(s,0);return 1;}if(s->focus==WM_NONE)return 1;if(s->focus>=5){if(s->focus==TOOL_PAINT&&(k=='S'||k=='L'||k=='G'||k=='X'))s->action=k=='S'?3:k=='L'?4:k=='G'?5:8;else if(s->focus==TOOL_GUARD&&(s->tools.guard.editing||k=='p'||k=='P'))s->action=tools_guard_key(&s->tools.guard,k);else tools_key(&s->tools,s->focus,k);s->dirty=1;return 1;}d->view=s->focus;return live_key(d,k);}
void wm_mouse(struct wm_state*s,struct live_state*d,unsigned width,unsigned height,int dx,int dy,unsigned buttons){
 if(width<640||height<360||width>4096||height>4096)return;
 if(!s->mouse_visible){s->mouse_x=(int)width/2;s->mouse_y=(int)height/2;s->mouse_visible=1;}
 s->mouse_x=clamp((int64_t)s->mouse_x+dx,0,(int)width-1);s->mouse_y=clamp((int64_t)s->mouse_y+dy,0,(int)height-1);
 int z=width>=1280&&height>=720?2:1,ox=((int)width-640*z)/2,oy=((int)height-360*z)/2;
 int x=(s->mouse_x-ox)/z,y=(s->mouse_y-oy)/z;int inside=s->mouse_x>=ox&&s->mouse_y>=oy&&x<640&&y<360;
 unsigned down=buttons&1,edge=down&&!(s->buttons&1);s->buttons=buttons&7;
 if(!down){s->drag=s->resize=WM_NONE;return;}
 if(s->drag<WM_COUNT){s->dirty=1;struct wm_rect*r=&s->windows[s->drag].rect;r->x=clamp((int64_t)s->origin.x+x-s->anchor_x,0,640-r->w);r->y=clamp((int64_t)s->origin.y+y-s->anchor_y,0,WM_WORK-r->h);return;}
 if(s->resize<WM_COUNT){s->dirty=1;struct wm_rect*r=&s->windows[s->resize].rect;r->w=clamp((int64_t)s->origin.w+x-s->anchor_x,160,640-r->x);r->h=clamp((int64_t)s->origin.h+y-s->anchor_y,100,WM_WORK-r->y);return;}
 if(!edge||!inside)return;
 s->dirty=1;
 if(y>=WM_WORK){if(x<80){s->menu^=1;return;}unsigned id=(unsigned)(x-80)/110+(s->focus>=5&&s->focus<WM_COUNT?5:0);if(id<WM_COUNT&&s->windows[id].open){if(s->focus==id&&shown(&s->windows[id]))wm_minimize(s,id);else wm_open(s,id);}return;}
 if(s->menu){if(x<300&&y>=180&&y<330){unsigned id=(unsigned)(y-180)/30+(x>=150?5:0);if(id<WM_COUNT)wm_open(s,id);}s->menu=0;return;}
 for(unsigned i=WM_COUNT;i;i--){unsigned id=s->order[i-1];struct wm_window*w=&s->windows[id];struct wm_rect r=w->rect;if(!shown(w)||!hit(r,x,y))continue;raise(s,id);
  if(y<r.y+20){if(x>=r.x+r.w-20)wm_close(s,id);else if(x>=r.x+r.w-40)wm_maximize(s,id);else if(x>=r.x+r.w-60)wm_minimize(s,id);else if(!w->maximized)s->drag=id;}
  else if(!w->maximized&&x>=r.x+r.w-12&&y>=r.y+r.h-12)s->resize=id;
  else if(id==1&&y>=r.y+24&&y<r.y+43&&x>=r.x+8&&x<r.x+110){s->action=x<r.x+58?1:2;}
  else if(id==4&&y>=r.y+30&&y<r.y+48){int col=(x-r.x-8)/42;if(col>=0&&col<3)d->file=(unsigned)col;d->dirty=1;}
  else if(id==TOOL_GUARD&&x>=r.x+8&&x<r.x+r.w-8){
   int row=(y-r.y-28)/11;unsigned rows=(unsigned)(r.h-35)/11;
   if(y>=r.y+28&&row>=0&&(unsigned)row<rows){int k=0;
    if(row==0)k='p';else if(s->tools.guard.editing&&r.w>=220){if(row==1)k=x<r.x+r.w/2?'k':'j';else if(row>=3&&row<=7)k='1'+row-3;else if(row==8)k=x<r.x+r.w/2?'a':'0';else if(row==9)k='r';}
    if(k)s->action=tools_guard_key(&s->tools.guard,k);
   }
  }
  else if(id==TOOL_PAINT&&x>=r.x+8&&y>=r.y+48){unsigned px=(unsigned)(x-r.x-8)/5,py=(unsigned)(y-r.y-48)/3;if(px<PAINT_W&&py<PAINT_H)tools_paint_at(&s->tools,px,py);}
  s->origin=r;s->anchor_x=x;s->anchor_y=y;return;
 }
 if(x<70){unsigned id=(unsigned)y/60;if(id<5)wm_open(s,id);}
}
static uint32_t col(uint32_t c,unsigned f){return f==0?((c&255)<<16)|(c&0xff00)|((c>>16)&255):c;}
struct painter{struct surface*s;int ox,oy,z;unsigned f;};
static void rect(struct painter*p,int x,int y,int w,int h,uint32_t c){surface_rect(p->s,p->ox+x*p->z,p->oy+y*p->z,w*p->z,h*p->z,col(c,p->f));}
static void text(struct painter*p,int x,int y,const char*t,unsigned limit,uint32_t c){char b[110];unsigned i=0;if(limit>=sizeof b)limit=sizeof b-1;while(i<limit&&t[i]){b[i]=t[i];i++;}b[i]=0;surface_text(p->s,p->ox+x*p->z,p->oy+y*p->z,b,p->z,col(c,p->f));}
static void number(char*b,uint64_t v){char tmp[24];unsigned n=0;do{tmp[n++]=(char)('0'+v%10);v/=10;}while(v);unsigned i=0;while(n)b[i++]=tmp[--n];b[i]=0;}
static void lines(struct painter*p,const char*t,int x,int y,unsigned cols,unsigned rows){char b[110];if(cols>=sizeof b)cols=sizeof b-1;for(unsigned r=0;*t&&r<rows;r++){unsigned n=0;while(*t&&*t!='\n'&&n<cols)b[n++]=*t++;b[n]=0;text(p,x,y+(int)r*11,b,cols,0xbce8c8);if(*t=='\n')t++;}}
static void client(struct painter*p,const struct live_state*d,struct live_stats st,unsigned id,struct wm_rect r,unsigned note_status){int x=r.x+8,y=r.y+28;unsigned cols=(unsigned)(r.w-16)/6,rows=(unsigned)(r.h-35)/11;
 if(id==0){unsigned start=d->row+1>rows?d->row+1-rows:0;for(unsigned i=start;i<=d->row&&i<LIVE_ROWS;i++)text(p,x,y+(int)(i-start)*11,d->terminal[i],cols,0x9bd6a7);}
 else if(id==1){rect(p,x,y-4,46,16,0x345f43);text(p,x+3,y,"SAVE",6,0xbce8c8);rect(p,x+50,y-4,46,16,0x345f43);text(p,x+53,y,"LOAD",6,0xbce8c8);if(note_status)text(p,x,y+14,note_status==1?"SAVED IN RAM":note_status==2?"LOADED FROM RAM":"RAM FILE ERROR",cols,0xe6c690);if(rows>3)lines(p,d->note,x,y+33,cols,rows-3); }
 else if(id==2){text(p,x,y,"EXPRESSION:",cols,0x8db49a);text(p,x,y+12,d->expr,cols,0xbce8c8);text(p,x,y+30,"RESULT:",cols,0x8db49a);text(p,x,y+42,d->answer,cols,0xbce8c8);}
 else if(id==3){char b[24];text(p,x,y,"TIMER TICKS",cols,0x8db49a);number(b,st.ticks);text(p,x,y+11,b,cols,0xbce8c8);text(p,x,y+24,"FREE / TOTAL FRAMES",cols,0x8db49a);number(b,st.free);text(p,x,y+35,b,cols,0xbce8c8);number(b,st.total);text(p,x+66,y+35,b,cols>11?cols-11:0,0xbce8c8);}
 else if(id==4){text(p,x,y+6,"1:TXT  2:BIN  3:DIR",cols,0x8db49a);if(rows>2)lines(p,d->files_text,x,y+22,cols,rows-2);}
}
static void extra_client(struct painter*p,const struct tools_state*t,unsigned id,struct wm_rect r){
 int x=r.x+8,y=r.y+28;unsigned cols=(unsigned)(r.w-16)/6,rows=(unsigned)(r.h-35)/11;char b[110];
 if(id==TOOL_RAM){text(p,x,y,"J/K SELECT - TAB TO HEX",cols,0x8db49a);if(rows<2)return;unsigned first=t->selected>=rows-1?t->selected-(rows-2):0;for(unsigned i=first;i<t->count&&i-first<rows-1;i++){text(p,x,y+11*(int)(i-first+1),t->names[i],cols>9?cols-9:cols,i==t->selected?0xe6c690:0xbce8c8);if(cols>10){number(b,t->sizes[i]);text(p,x+(int)(cols-8)*6,y+11*(int)(i-first+1),b,8,0x8db49a);}}}
 else if(id==TOOL_HEX){text(p,x,y,t->count?t->names[t->selected]:"NO RAM FILES",cols,0xbce8c8);if(rows<3)return;text(p,x,y+11,"J/K FILE  N/P OFFSET",cols,0x8db49a);static const char h[]="0123456789ABCDEF";unsigned per=cols>=28?8:4;for(unsigned row=0;row<rows-2;row++){size_t off=t->offset+row*per;if(off>=t->length)break;unsigned k=0;for(int z=3;z>=0;z--)b[k++]=h[(off>>(z*4))&15];b[k++]=' ';for(unsigned i=0;i<per&&off+i<t->length;i++){unsigned v=t->bytes[off+i];b[k++]=h[v>>4];b[k++]=h[v&15];b[k++]=' ';}b[k]=0;text(p,x,y+22+(int)row*11,b,cols,0xbce8c8);}}
 else if(id==TOOL_PAINT){text(p,x,y,"0-5 INK / WASD / SPACE",cols,0x8db49a);text(p,x,y+10,"C CLEAR / S L G X STOP",cols,0x8db49a);for(unsigned yy=0;yy<PAINT_H;yy++)for(unsigned xx=0;xx<PAINT_W;xx++)rect(p,x+(int)xx*5,r.y+48+(int)yy*3,5,3,tools_color(t->canvas[yy*PAINT_W+xx]));rect(p,x+(int)t->px*5,r.y+48+(int)t->py*3,5,1,0xffffff);if(r.h>=112)text(p,x,r.y+90,t->status==1?"SAVED":t->status==2?"LOADED":t->status==3?"FILE / DEMO ERROR":t->status==4?"RING 3 FRAME":t->status==5?"RING 3 RUNNING":t->status==6?"CLIENT STOPPED":"RAM ONLY",cols,0xe6c690);}
 else if(id==TOOL_SETTINGS){text(p,x,y,"1 SWAMP  2 MATRIX  3 SLATE",cols,0xbce8c8);if(rows>=2)text(p,x,y+11,t->theme==0?"SWAMP":t->theme==1?"MATRIX":"SLATE",cols,0xe6c690);if(rows>=3)text(p,x,y+22,"VOLATILE THEME",cols,0x8db49a);}
 else if(id==TOOL_GUARD&&t->guard.editing){const struct guard_editor*g=&t->guard;
  static const char *const labels[]={"1 CONSOLE","2 FILE RESOURCE","3 IPC","4 SCRATCH","5 GUI"};
  text(p,x,y,"P: AUDIT / POLICY EDITOR",cols,0x8db49a);
  if(rows>1)text(p,x,y+11,"K PREV / J NEXT APP",cols,0xbce8c8);
  if(rows>2)text(p,x,y+22,tools_guard_path(g->app)?tools_guard_path(g->app):"INVALID APP",cols,0xe6c690);
  if(g->valid)for(unsigned i=0;i<5&&i+3<rows;i++){
   unsigned row=i+3;uint64_t bit=tools_guard_bit(i);text(p,x,y+(int)row*11,labels[i],cols>9?cols-9:cols,0xbce8c8);
   if(cols>=19)text(p,x+(int)(cols-8)*6,y+(int)row*11,g->draft&bit?"ALLOW":"DENY",8,g->draft&bit?0x9bd6a7:0xe6c690);
  }
  if(rows>8){text(p,x,y+88,"A APPLY",cols/2,0xbce8c8);text(p,r.x+r.w/2+4,y+88,"0 DENY ALL",cols/2,0xe6c690);}
  if(rows>9)text(p,x,y+99,"R RELOAD / DISCARD",cols,0xbce8c8);
  if(rows>10)text(p,x,y+110,g->status==2?"STALE / RELOAD":!g->valid?"UNAVAILABLE / RELOAD":g->status==1?"APPLIED":g->draft!=g->actual?"UNSAVED CHANGES":"CURRENT CEILING",cols,0xe6c690);
  if(rows>11)text(p,x,y+121,"CEILING ONLY - NO AUTO GRANTS",cols,0x8db49a);
 }
 else if(id==TOOL_GUARD){text(p,x,y,"P: POLICY / AUDIT LOG",cols,0x8db49a);if(rows<2)return;for(unsigned i=t->audit_offset;i<t->audit_count&&i-t->audit_offset<rows-1;i++){int yy=y+11*(int)(i-t->audit_offset+1);number(b,t->events[i].seq);text(p,x,yy,b,8,0xbce8c8);number(b,t->events[i].kind);text(p,x+54,yy,b,5,0x8db49a);text(p,x+90,yy,t->events[i].result<0?"ERR":"OK",cols>15?cols-15:0,t->events[i].result<0?0xe6c690:0xbce8c8);}}
}
static void paint_scene(struct surface*s,const struct wm_state*w,const struct live_state*d,struct live_stats st,unsigned f,int ox,int oy,int z){
 struct painter p={s,ox,oy,z,f};surface_rect(s,0,0,(int)s->width,(int)s->height,col(0x07110c,f));rect(&p,0,0,640,360,w->tools.theme==1?0x071b0b:w->tools.theme==2?0x16232c:0x102218);
 for(unsigned i=0;i<5;i++){rect(&p,20,7+(int)i*60,27,25,0x345f43);text(&p,9,38+(int)i*60,names[i],10,0xbce8c8);}
 for(unsigned i=0;i<WM_COUNT;i++){unsigned id=w->order[i];const struct wm_window*a=&w->windows[id];if(!shown(a))continue;struct wm_rect r=a->rect;
 struct screen_rect wr={ox+r.x*z,oy+r.y*z,r.w*z,r.h*z};
 struct surface inside;
 if(!surface_crop(s,&wr,&inside))continue;
 struct painter wp={&inside,ox-wr.x,oy-wr.y,z,f};
 rect(&p,r.x,r.y,r.w,r.h,0x0b1912);rect(&p,r.x,r.y,r.w,20,w->focus==id?0x467c53:0x244632);text(&p,r.x+6,r.y+6,names[id],(unsigned)(r.w-66)/6,0xbce8c8);text(&p,r.x+r.w-56,r.y+6,"-   +  X",9,0xc0e4c9);rect(&p,r.x+r.w-9,r.y+r.h-9,7,7,0x345f43);if(id<5)client(&wp,d,st,id,r,w->note_status);else extra_client(&wp,&w->tools,id,r);}
 rect(&p,0,WM_WORK,640,30,0x244632);text(&p,9,341,"SWAMP",10,0xbce8c8);unsigned page=w->focus>=5&&w->focus<WM_COUNT?5:0;for(unsigned j=0;j<5;j++){unsigned i=page+j;if(w->windows[i].open){rect(&p,80+(int)j*110,334,106,22,w->focus==i?0x467c53:0x203b2a);text(&p,85+(int)j*110,341,names[i],16,0xc0e4c9);}}
 if(w->menu){rect(&p,0,180,300,150,0x183323);for(unsigned i=0;i<WM_COUNT;i++){int xx=(int)(i/5)*150,yy=(int)(i%5)*30;rect(&p,xx+2,182+yy,146,27,0x244632);text(&p,xx+10,191+yy,names[i],20,0xbce8c8);}}
}

int wm_paint_region(struct surface *scene,struct screen_rect r,const struct wm_state *w,
                    const struct live_state *d,struct live_stats st,unsigned f){
 struct surface part;
 if(!w||!d||f>1||!surface_crop(scene,&r,&part))return 0;
 paint_scene(&part,w,d,st,f,-r.x,-r.y,1);
 return 1;
}
int wm_render(struct surface *s,const struct wm_state *w,const struct live_state *d,struct live_stats st,unsigned f){
 if(!surface_valid(s)||s->width<640||s->height<360||f>1||!wm_valid(w)||!d)return 0;
 int z=s->width>=1280&&s->height>=720?2:1;
 paint_scene(s,w,d,st,f,((int)s->width-640*z)/2,((int)s->height-360*z)/2,z);
 if(w->mouse_visible)surface_cursor(s,w->mouse_x,w->mouse_y,f);
 return 1;
}
