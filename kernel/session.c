#include "session.h"
#include "policy.h"
void session_clear(struct app_session *s){volatile unsigned char *p=(void*)s;for(size_t i=0;i<sizeof *s;i++)p[i]=0;}
int session_begin(struct app_session *s,uint64_t id,uint64_t grants,const void *data,size_t n){
 session_clear(s);if(!id||grants&~CAP_ALL||n>SESSION_DATA_MAX||(n&&(!data||!(grants&CAP_DATA))))return 0;
 s->id=id;s->grants=grants;s->length=n;const unsigned char *p=data;for(size_t i=0;i<n;i++)s->data[i]=p[i];return 1;
}
long session_read(const struct app_session *s,uint64_t offset,void *dst,size_t n){
 if(!s->id||!(s->grants&CAP_DATA))return -1;
 if(!dst||!n||n>SESSION_READ_MAX)return -22;
 if(offset>=s->length)return 0;
 if(n>s->length-offset)n=s->length-offset;
 unsigned char *p=dst;for(size_t i=0;i<n;i++)p[i]=s->data[offset+i];return (long)n;
}
int session_selftest(void){static struct app_session s;unsigned char out[4]={0};int ok=session_begin(&s,1,CAP_DATA,"abc",3)&&session_read(&s,1,out,4)==2&&out[0]=='b'&&out[1]=='c'&&session_read(&s,3,out,1)==0;session_clear(&s);return ok&&session_read(&s,0,out,1)==-1&&!s.data[0];}
