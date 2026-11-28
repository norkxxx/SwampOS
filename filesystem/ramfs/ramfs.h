#pragma once
#include <stddef.h>
#include <stdint.h>
#define RAMFS_FILES 64
#define RAMFS_NAME 96
#define RAMFS_DIRS 64
#define RAMFS_COMPONENT 31
#define RAMFS_DEPTH 8
#define RAMFS_BYTES 8192
/* Canonical relative paths, bounded volatile hierarchy; no physical block I/O. */
enum ramfs_result { RAMFS_OK=0, RAMFS_INVALID=-1, RAMFS_NOT_FOUND=-2,
 RAMFS_FULL=-3, RAMFS_EXISTS=-4, RAMFS_TOO_LARGE=-5, RAMFS_NOT_EMPTY=-6 };
struct ramfs_entry {char name[RAMFS_NAME];size_t length;unsigned used;uint8_t data[RAMFS_BYTES];};
struct ramfs {struct ramfs_entry entries[RAMFS_FILES];char dirs[RAMFS_DIRS][RAMFS_NAME];};
void ramfs_init(struct ramfs*);
int ramfs_write(struct ramfs*,const char*,const void*,size_t);
int ramfs_append(struct ramfs*,const char*,const void*,size_t);
int ramfs_read_at(const struct ramfs*,const char*,size_t,void*,size_t,size_t*);
int ramfs_move_dir(struct ramfs*,const char*,const char*);
int ramfs_read(const struct ramfs*,const char*,void*,size_t,size_t*);
int ramfs_remove(struct ramfs*,const char*);
int ramfs_rename(struct ramfs*,const char*,const char*);
int ramfs_copy(struct ramfs*,const char*,const char*);
int ramfs_entry(const struct ramfs*,unsigned,char*,size_t,size_t*);
unsigned ramfs_count(const struct ramfs*);
int ramfs_mkdir(struct ramfs*,const char*);
int ramfs_rmdir(struct ramfs*,const char*);
int ramfs_dir_entry(const struct ramfs*,unsigned,char*,size_t);
int ramfs_is_dir(const struct ramfs*,const char*);
extern struct ramfs live_ramfs;
int ramfs_command(const char*);
