#include "audit.h"
struct audit_log kernel_audit;
void audit_add(struct audit_log*l,uint32_t kind,int32_t result,uint64_t detail){if(l->next==UINT64_MAX)return;l->event[l->head]=(struct audit_event){++l->next,kind,result,detail};l->head=(l->head+1)%AUDIT_CAPACITY;if(l->count<AUDIT_CAPACITY)l->count++;}
int audit_get(const struct audit_log*l,unsigned i,struct audit_event*out){if(!out||i>=l->count)return 0;*out=l->event[(l->head+AUDIT_CAPACITY-l->count+i)%AUDIT_CAPACITY];return 1;}
int audit_selftest(void){struct audit_log l={0};struct audit_event e;for(unsigned i=0;i<100;i++)audit_add(&l,1,-1,i);return l.count==64&&audit_get(&l,0,&e)&&e.seq==37&&audit_get(&l,63,&e)&&e.detail==99&&!audit_get(&l,64,&e);}
