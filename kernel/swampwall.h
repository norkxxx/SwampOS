#pragma once
#include <stdint.h>
#include <stddef.h>
/* Exact endpoint grants, no wildcard or implicit permission. Host/kernel policy primitive;
 * not yet connected to a user socket syscall. */
#define WALL_MAX 16
struct wall_rule {uint32_t app; uint8_t ip[4],proto; uint16_t port;};
struct wall {struct wall_rule rules[WALL_MAX]; size_t count;};
int wall_grant(struct wall *,const struct wall_rule *);
int wall_revoke(struct wall *,uint32_t);
int wall_allow(const struct wall *,uint32_t,const uint8_t[4],uint8_t,uint16_t);
int wall_selftest(void);
