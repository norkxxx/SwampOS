#include "live.h"
#include "../desktop/compositor.h"
#include "../filesystem/ramfs/ramfs.h"
#include "bootinfo.h"
#include "vm.h"
#include "core.h"
#include "storage.h"
#include "policy.h"
#include "guard_ui.h"
#include "user.h"
static struct live_state state;
static uint32_t scene_pixels[SCENE_PIXELS];
static struct compositor display;
static unsigned active,windowed,pending,contents;
static struct live_stats monitor;
static void changed(unsigned id){if(id<WM_COUNT)contents|=1u<<id;pending=1;}
static struct wm_state wm;

int desktop_active(void){return active;}
void desktop_start(void){if(!boot_uefi)return;const struct swamp_bootinfo*b=(const void*)SWAMP_UEFI_INFO;if(b->reserved!=1||b->live_option==2)return;uint64_t bytes;
 if(!live_fb_validate(b->fb,b->fb_size,b->width,b->height,b->stride,b->pixel_format,&bytes)||!vm_map_live_fb(b->fb,bytes))return;
 struct surface fb=(struct surface){(uint32_t*)LIVE_FB_VA,(size_t)(bytes/4),b->width,b->height,b->stride};
 if(!compositor_init(&display,fb,b->pixel_format,scene_pixels,SCENE_PIXELS))return;
 pending=1;live_init(&state);wm_init(&wm);windowed=b->live_option==0;active=1;}
void desktop_put(char c){if(active){live_put(&state,c);changed(0);}}
void desktop_clear(void){if(active){live_clear(&state);changed(0);}}
static void tool_action(void){if(wm.action==3)tools_save(&wm.tools,&live_ramfs);else if(wm.action==4)tools_load(&wm.tools,&live_ramfs);else if(wm.action==5){if(!user_gui_active())wm.tools.status=storage_gui_start()?5:3;}else if(wm.action==8){user_gui_cancel();wm.tools.status=6;}else if(wm.action==GUARD_UI_READ||wm.action==GUARD_UI_APPLY)guard_editor_execute(&wm.tools.guard,wm.action);
 if(wm.action>=3)wm.action=0;}
int desktop_key(int k){
 if(!active)return 0;
 int handled=windowed?wm_key(&wm,&state,k):live_key(&state,k);
 if(windowed)tool_action();
 if(handled)changed(windowed?wm.focus:state.view);
 return handled;
}
static unsigned visible(unsigned id){return wm.windows[id].open&&!wm.windows[id].minimized;}
void desktop_refresh(void){
 if(!active)return;
 uint64_t t=ticks();
 static uint64_t gui_tick;
 if(user_gui_active()){
  if(!wm.windows[TOOL_PAINT].open){
   user_gui_cancel();wm.tools.status=6;changed(TOOL_PAINT);
  }else if(t-gui_tick>=5){
   unsigned char frame[GUI_FRAME_BYTES];int published=0;
   gui_tick=t;
   int rc=storage_gui_poll(frame,sizeof frame,&published);
   if(published)for(unsigned y=0;y<PAINT_H;y++)for(unsigned x=0;x<PAINT_W;x++)
    wm.tools.canvas[y*PAINT_W+x]=frame[(y*8/PAINT_H)*16+x*16/PAINT_W];
   wm.tools.status=rc<0?3:rc==1?4:5;changed(TOOL_PAINT);
  }
 }
 if((windowed?visible(3):state.view==3)&&(!monitor.total||t-monitor.ticks>=100)){
  monitor=(struct live_stats){t,frames.free,frames.total};changed(3);
 }
 if(!pending||!compositor_due(&display,t))return;
 unsigned dirty=contents;
 if(dirty&&windowed){
  unsigned tools=(1u<<TOOL_RAM)|(1u<<TOOL_HEX)|(1u<<TOOL_GUARD);
  if(visible(TOOL_RAM)||visible(TOOL_HEX)||visible(TOOL_GUARD)){
   tools_sync(&wm.tools,&live_ramfs,&kernel_audit);dirty|=tools;
  }
 }
 if((windowed?visible(4):state.view==4)&&(dirty&((1u<<4)|1u))){
  storage_live_preview(state.file,state.files_text,sizeof state.files_text);dirty|=1u<<4;
 }
 if(compositor_present(&display,&wm,&state,monitor,t,windowed,dirty)){
  contents=pending=state.dirty=wm.dirty=0;
 }
}
void desktop_mouse(int dx,int dy,unsigned buttons){if(active){pending=1;if(windowed){unsigned press=(buttons&1)&&!(wm.buttons&1);wm_mouse(&wm,&state,display.front.width,display.front.height,dx,dy,buttons);if(press)changed(wm.focus);if(wm.action==1){wm.note_status=ramfs_write(&live_ramfs,"notes.txt",state.note,state.note_len)==0?1:3;}else if(wm.action==2){size_t n;if(!ramfs_read(&live_ramfs,"notes.txt",state.note,512,&n)){for(size_t i=0;i<n;i++)if(((unsigned char)state.note[i]<32&&state.note[i]!='\n')||(unsigned char)state.note[i]>126)state.note[i]='.';state.note_len=(unsigned)n;state.note[n]=0;wm.note_status=2;}else wm.note_status=3;}tool_action();wm.action=0;}else {live_mouse(&state,display.front.width,display.front.height,dx,dy,buttons);if(state.dirty)changed(state.view);}}}
