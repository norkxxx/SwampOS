#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../gui/surface.h"
#define LIVE_COLS 76
#define LIVE_ROWS 18
#define LIVE_FB_VA 0x8000000000ull
#define LIVE_FB_LIMIT (64ull*1024*1024)
struct live_state {
 char terminal[LIVE_ROWS][LIVE_COLS+1],note[513],expr[40],answer[40],files_text[512];
 unsigned row,col,view,note_len,expr_len,file,dirty;
 int mouse_x,mouse_y; unsigned mouse_visible,mouse_buttons;
};
struct live_stats {uint64_t ticks; unsigned free,total;};
int live_fb_validate(uint64_t base,uint64_t size,unsigned w,unsigned h,unsigned stride,unsigned format,uint64_t *bytes);
int live_calculate(const char *expr,int64_t *out);
void live_init(struct live_state*);
void live_put(struct live_state*,char);
void live_clear(struct live_state*);
int live_key(struct live_state*,int);
void live_mouse(struct live_state*,unsigned,unsigned,int,int,unsigned);
int live_render(struct surface*,const struct live_state*,struct live_stats,unsigned format);
void desktop_start(void);
void desktop_put(char);
void desktop_clear(void);
int desktop_key(int);
void desktop_refresh(void);
void desktop_mouse(int,int,unsigned);
int desktop_active(void);
int live_paint_region(struct surface *,struct screen_rect,const struct live_state *,struct live_stats,unsigned);
