#include "gpt.h"
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t u64(const uint8_t*p){return u32(p)|((uint64_t)u32(p+4)<<32);}
static uint32_t step(uint32_t c,const uint8_t*p,unsigned n){while(n--){c^=*p++;for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u& (0u-(c&1)));}return c;}
static int eq(const uint8_t*a,const uint8_t*b,unsigned n){while(n--)if(*a++!=*b++)return 0;return 1;}
static int zero(const uint8_t*p,unsigned n){while(n--)if(*p++)return 0;return 1;}
static int hdr(uint8_t*b,uint64_t here,uint64_t other,uint64_t table,uint64_t cap){if(!eq(b,(const uint8_t*)"EFI PART",8)||u32(b+8)!=0x10000||u32(b+12)!=92||u32(b+20)||u64(b+24)!=here||u64(b+32)!=other||u64(b+40)<34||u64(b+48)>cap-34||u64(b+40)>u64(b+48)||zero(b+56,16)||u64(b+72)!=table||u32(b+80)!=128||u32(b+84)!=128)return 0;uint32_t c=u32(b+16);for(unsigned i=16;i<20;i++)b[i]=0;return ~step(~0u,b,92)==c;}
int gpt_read(const struct blockdev*d,struct gpt*out){if(!out)return 0;out->count=0;if(!d||d->sectors<68)return 0;uint8_t a[512],b[512],x[512],y[512];uint64_t end=d->sectors-1;
 if(!block_read(d,0,1,x)||x[510]!=0x55||x[511]!=0xaa||x[450]!=0xee||u32(x+454)!=1||u32(x+458)!=(end>0xffffffffu?0xffffffffu:(uint32_t)end)||!zero(x+462,48))return 0;
 if(!block_read(d,1,1,a)||!block_read(d,end,1,b)||!hdr(a,1,end,2,d->sectors)||!hdr(b,end,1,end-32,d->sectors)||!eq(a+40,b+40,32)||u32(a+88)!=u32(b+88))return 0;
 uint32_t c=~0u;unsigned count=0;
 for(unsigned i=0;i<32;i++){if(!block_read(d,2+i,1,x)||!block_read(d,end-32+i,1,y)||!eq(x,y,512))return 0;c=step(c,x,512);
 for(unsigned j=0;j<4;j++){const uint8_t*p=x+j*128;if(zero(p,16))continue;uint64_t lo=u64(p+32),hi=u64(p+40);if(zero(p+16,16)||lo<u64(a+40)||hi>u64(a+48)||lo>hi)return 0;for(unsigned k=0;k<count;k++)if(eq(out->parts[k].id,p+16,16)||!(hi<out->parts[k].first||lo>out->parts[k].last))return 0;struct gpt_part*q=&out->parts[count++];q->first=lo;q->last=hi;for(unsigned k=0;k<16;k++){q->type[k]=p[k];q->id[k]=p[k+16];}}}
 if(~c!=u32(a+88))return 0;
 out->count=count;return 1;
}
