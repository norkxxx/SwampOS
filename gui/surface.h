#pragma once
#include <stdint.h>
#include <stddef.h>
struct surface {uint32_t *pixels;size_t capacity;unsigned width,height,stride;};
struct screen_rect { int x, y, w, h; };
/* Return the visible intersection and a bounded, stride-preserving subview. */
int surface_crop(const struct surface *, struct screen_rect *, struct surface *);
uint32_t surface_cursor_pixel(int dx, int dy, uint32_t under, unsigned format);
void surface_cursor(struct surface *, int x, int y, unsigned format);
int surface_valid(const struct surface*);
void surface_rect(struct surface*,int,int,int,int,uint32_t);
void surface_text(struct surface*,int,int,const char*,unsigned,uint32_t);
int swamp_desktop(struct surface*);
