#pragma once
#include "block.h"
#define GPT_MAX_PARTS 128
struct gpt_part {uint64_t first,last;uint8_t type[16],id[16];};
struct gpt {unsigned count;struct gpt_part parts[GPT_MAX_PARTS];};
/* Strict laboratory GPT subset: 512-byte sectors, 128 x 128-byte entries, matching primary/backup. */
int gpt_read(const struct blockdev*,struct gpt*);
