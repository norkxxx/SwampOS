#pragma once
#include "block.h"
struct note_record {
    char magic[8];
    uint64_t generation;
    uint16_t length;
    char data[490];
    uint32_t checksum;
};
_Static_assert(sizeof(struct note_record)==512 && offsetof(struct note_record,checksum)==508,"record format");

struct note_volume {
    struct blockdev disk;
    int (*write)(void *,unsigned,const void *);
    int (*flush)(void *);
    struct note_record current;
    unsigned slot;
};
int vfs_note_mount(struct note_volume *, const struct blockdev *,
                   int (*write)(void *,unsigned,const void *), int (*flush)(void *));
/* 1 committed, 0 invalid input, -1 I/O failure requiring remount. */
int vfs_note_write(struct note_volume *, const char *, size_t);
