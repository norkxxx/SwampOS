#include "guard.h"
#include "policy.h"
static int valid_key(const uint8_t id[16],uint32_t ino){if(!id||!ino)return 0;unsigned any=0;for(unsigned i=0;i<16;i++)any|=id[i];return any!=0;}
static int same(const uint8_t a[16],const uint8_t b[16]){unsigned d=0;for(unsigned i=0;i<16;i++)d|=(unsigned)(a[i]^b[i]);return d==0;}
int guard_set(struct guard_table *t,const uint8_t id[16],uint32_t ino,uint64_t mask){
 if(!t||!valid_key(id,ino)||(mask&~CAP_ALL))return 0;
 unsigned slot=GUARD_MAX;
 for(unsigned i=0;i<GUARD_MAX;i++){
  if(t->entry[i].inode==ino&&same(t->entry[i].volume_id,id)){t->entry[i].ceiling=mask;return 1;}
  if(!t->entry[i].inode&&slot==GUARD_MAX)slot=i;
 }
 if(slot==GUARD_MAX)return 0;
 for(unsigned i=0;i<16;i++)t->entry[slot].volume_id[i]=id[i];
 t->entry[slot].inode=ino;t->entry[slot].ceiling=mask;return 1;
}
int guard_remove(struct guard_table *t,const uint8_t id[16],uint32_t ino){
 if(!t||!valid_key(id,ino))return 0;
 for(unsigned i=0;i<GUARD_MAX;i++)if(t->entry[i].inode==ino&&same(t->entry[i].volume_id,id)){t->entry[i]=(struct guard_entry){0};return 1;}
 return 0;
}
uint64_t guard_ceiling(const struct guard_table *t,const uint8_t id[16],uint32_t ino){
 if(!t||!valid_key(id,ino))return 0;
 for(unsigned i=0;i<GUARD_MAX;i++)if(t->entry[i].inode==ino&&same(t->entry[i].volume_id,id))return t->entry[i].ceiling;
 return CAP_ALL; /* ceiling only, never a grant */
}
int guard_allows(const struct guard_table *t,const uint8_t id[16],uint32_t ino,uint64_t req){return t&&valid_key(id,ino)&&!(req&~guard_ceiling(t,id,ino));}
int guard_parse_mask(const char *p,uint64_t *out){
 if(!p||!out||!*p)return 0;
 uint64_t n=0;unsigned digits=0;
 while(*p){if(*p<'0'||*p>'9'||++digits>2)return 0;n=n*10+(unsigned)(*p++-'0');}
 if(n&~CAP_ALL)return 0;
 *out=n;return 1;
}
