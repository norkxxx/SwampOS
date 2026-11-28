#pragma once
#include <stdint.h>
#include <stddef.h>
#define SESSION_DATA_MAX (64u * 1024u)
#define SESSION_READ_MAX 256u
struct app_session {uint64_t id, grants; size_t length; unsigned char data[SESSION_DATA_MAX];};
void session_clear(struct app_session *);
int session_begin(struct app_session *,uint64_t id,uint64_t grants,const void *,size_t);
long session_read(const struct app_session *,uint64_t offset,void *,size_t);
int session_selftest(void);
