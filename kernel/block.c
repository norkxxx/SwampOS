#include "block.h"
int block_read(const struct blockdev *d,uint64_t lba,unsigned count,void *out){
 return d&&d->read&&out&&count&&count<=128&&lba<d->sectors&&count<=d->sectors-lba&&d->read(d->ctx,lba,count,out);
}
static int slice_read(void *ctx,uint64_t lba,unsigned count,void *out){struct block_slice*s=ctx;if(!s||lba>=s->count||!count||count>s->count-lba)return 0;return block_read(&s->parent,s->first+lba,count,out);}
int block_slice_init(struct block_slice *s,struct blockdev*out,const struct blockdev *parent,uint64_t first,uint64_t count){if(!s||!out||!parent||!parent->read||!count||first>=parent->sectors||count>parent->sectors-first||parent==out)return 0;*s=(struct block_slice){*parent,first,count};*out=(struct blockdev){s,count,slice_read};return 1;}
