#pragma once
#include "../kernel/live.h"
#include "../applications/system/tools.h"
#define WM_COUNT 10
#define WM_NONE WM_COUNT
#define WM_WIDTH 640
#define WM_HEIGHT 360
#define WM_WORK 330
struct wm_rect { int x,y,w,h; };
struct wm_window {struct wm_rect rect,restore; unsigned open,minimized,maximized;};
struct wm_state {
 struct wm_window windows[WM_COUNT];
 unsigned order[WM_COUNT],focus,menu,buttons,drag,resize;
 int mouse_x,mouse_y,anchor_x,anchor_y;
 struct wm_rect origin;
 unsigned mouse_visible,dirty,action,note_status;
 struct tools_state tools;
};
void wm_init(struct wm_state*);
void wm_open(struct wm_state*,unsigned);
void wm_close(struct wm_state*,unsigned);
void wm_minimize(struct wm_state*,unsigned);
void wm_maximize(struct wm_state*,unsigned);
int wm_key(struct wm_state*,struct live_state*,int);
void wm_mouse(struct wm_state*,struct live_state*,unsigned,unsigned,int,int,unsigned);
int wm_valid(const struct wm_state*);
int wm_render(struct surface*,const struct wm_state*,const struct live_state*,struct live_stats,unsigned);
int wm_paint_region(struct surface *,struct screen_rect,const struct wm_state *,const struct live_state *,struct live_stats,unsigned);
