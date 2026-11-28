#pragma once
#include <stddef.h>
#include <stdint.h>
#define VQ_MAX 1024u
/* Split queue including the page-aligned used ring and optional event fields. */
#define VQ_BYTES 32768u
struct vq_desc {uint64_t addr;uint32_t len;uint16_t flags,next;};
struct vq_used {uint32_t id,len;};
struct vq {volatile uint8_t *base;uint16_t n,seen;size_t used_off;uint8_t pending[VQ_MAX];};
int vq_init(struct vq *,volatile void *,uint16_t);
void vq_desc_set(struct vq *,uint16_t,uint64_t,uint32_t,uint16_t);
int vq_submit(struct vq *,uint16_t);
/* -1 invalid device state, 0 none, 1 entry. Caller enforces buffer ownership. */
int vq_pop(struct vq *,struct vq_used *);
