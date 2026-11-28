#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../../filesystem/ramfs/ramfs.h"
#include "../../kernel/audit.h"
#define TOOL_RAM 5u
#define TOOL_HEX 6u
#define TOOL_PAINT 7u
#define TOOL_SETTINGS 8u
#define TOOL_GUARD 9u
#define GUARD_UI_APPS 6u
#define GUARD_UI_READ 6u
#define GUARD_UI_APPLY 7u
struct guard_editor {unsigned editing,app,valid,status;uint64_t actual,draft,revision;};
const char *tools_guard_path(unsigned);
uint64_t tools_guard_bit(unsigned);
unsigned tools_guard_key(struct guard_editor *,int);
void tools_guard_loaded(struct guard_editor *,int,uint64_t,uint64_t);
#define PAINT_W 24u
#define PAINT_H 12u
struct tools_state {
 struct guard_editor guard;
 unsigned selected, offset, theme, px, py, ink, status;
 unsigned count, audit_count, audit_offset;
 char names[RAMFS_FILES][RAMFS_NAME]; size_t sizes[RAMFS_FILES];
 unsigned char bytes[RAMFS_BYTES]; size_t length;
 unsigned char canvas[PAINT_W*PAINT_H];
 struct audit_event events[AUDIT_CAPACITY];
};
void tools_init(struct tools_state*);
void tools_sync(struct tools_state*,const struct ramfs*,const struct audit_log*);
void tools_key(struct tools_state*,unsigned,int);
void tools_paint_at(struct tools_state*,unsigned,unsigned);
int tools_save(struct tools_state*,struct ramfs*);
int tools_load(struct tools_state*,const struct ramfs*);
uint32_t tools_color(unsigned);
