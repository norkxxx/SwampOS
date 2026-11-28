#include "scratch.h"
void scratch_clear(struct scratch*s){volatile unsigned char*p=(void*)s;for(size_t i=0;i<sizeof *s;i++)p[i]=0;}
long scratch_write(struct scratch*s,uint64_t off,const void*buf,size_t n){if(!buf||!n||n>256||off>SCRATCH_SIZE||n>SCRATCH_SIZE-off||off>s->size)return -22;const unsigned char*p=buf;for(size_t i=0;i<n;i++)s->bytes[off+i]=p[i];if(off+n>s->size)s->size=off+n;return (long)n;}
long scratch_read(const struct scratch*s,uint64_t off,void*buf,size_t n){if(!buf||!n||n>256)return -22;if(off>=s->size)return 0;if(n>s->size-off)n=s->size-off;unsigned char*p=buf;for(size_t i=0;i<n;i++)p[i]=s->bytes[off+i];return (long)n;}
int scratch_selftest(void){struct scratch s;char b[4];scratch_clear(&s);int ok=scratch_write(&s,1,"x",1)==-22&&scratch_write(&s,0,"abc",3)==3&&scratch_read(&s,1,b,4)==2&&b[0]=='b'&&b[1]=='c'&&scratch_read(&s,3,b,1)==0;scratch_clear(&s);return ok&&!s.size&&!s.bytes[0];}
