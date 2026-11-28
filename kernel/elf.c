#include "elf.h"
#include "vm.h"
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t u32(const uint8_t*p){return (uint32_t)u16(p)|((uint32_t)u16(p+2)<<16);}
static uint64_t u64(const uint8_t*p){return (uint64_t)u32(p)|((uint64_t)u32(p+4)<<32);}
/* Static ET_EXEC: one RX page and optionally one RW/NX page, fixed disjoint VAs.
 * No interpreter, TLS, relocations, or executable writable segment. */
int elf_parse(const void*buf,size_t n,struct elf_app*a){
 if(!a)return 0;
 *a=(struct elf_app){0};const uint8_t*p=buf;
 if(!p||n<64||p[0]!=127||p[1]!='E'||p[2]!='L'||p[3]!='F'||p[4]!=2||p[5]!=1||p[6]!=1||p[7]||p[8]||u16(p+16)!=2||u16(p+18)!=62||u32(p+20)!=1||u32(p+48)||u16(p+52)!=64||u16(p+54)!=56)return 0;
 unsigned count=u16(p+56);uint64_t off=u64(p+32),entry=u64(p+24);if((count!=1&&count!=2)||off<64||off>n||56u*count>n-off)return 0;
 const uint8_t*h=p+off;uint64_t pos=u64(h+8),va=u64(h+16),files=u64(h+32),mem=u64(h+40);
 if(u32(h)!=1||u32(h+4)!=5||va!=USER_CODE||u64(h+24)!=va||!files||files!=mem||files>4096||pos<off+56u*count||pos>n||files>n-pos||u64(h+48)!=4096||pos%4096||entry<va||entry-va>=files)return 0;
 struct elf_app t={.code=p+pos,.size=(size_t)files,.entry=entry};
 if(count==2){h+=56;uint64_t d=u64(h+8),df=u64(h+32),dm=u64(h+40);if(u32(h)!=1||u32(h+4)!=6||u64(h+16)!=USER_DATA||u64(h+24)!=USER_DATA||!dm||df>dm||dm>4096||d<off+112||d>n||df>n-d||d%4096||u64(h+48)!=4096||(df&&d<pos+files&&pos<d+df))return 0;t.data=p+d;t.data_files=(size_t)df;t.data_mem=(size_t)dm;}
 *a=t;return 1;
}
