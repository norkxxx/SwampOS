#pragma once
#include "window.h"

#define SCENE_PIXELS (WM_WIDTH * WM_HEIGHT)
#define PRESENT_TICKS 2u  /* The PIT runs at 100 Hz. */

struct pointer_state { int x,y; unsigned visible; };
struct compositor {
 struct surface front, scene;
 unsigned format, scale, ready, presented, windowed, focus, menu, theme, view;
 int left, top;
 struct pointer_state pointer;
 struct wm_rect bounds[WM_COUNT]; /* Empty for a hidden window. */
 unsigned order[WM_COUNT],open[WM_COUNT];
 uint64_t last_tick, writes, paint_pixels;
};

int compositor_init(struct compositor *,struct surface front,unsigned format,uint32_t *back,size_t words);
int compositor_due(const struct compositor *,uint64_t tick);
/* Call after input dispatch. Each content bit identifies a changed client. */
int compositor_present(struct compositor *,const struct wm_state *,const struct live_state *,
                       struct live_stats,uint64_t tick,unsigned windowed,unsigned content);
