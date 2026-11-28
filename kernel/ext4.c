#include "ext4.h"
/* Intentionally narrow read-only EXT4 profile; fail closed on unsupported features. */
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t u32(const uint8_t*p){return (uint32_t)u16(p)|((uint32_t)u16(p+2)<<16);}
static void copy(void *d,const void *s,size_t n){uint8_t *a=d;const uint8_t*b=s;while(n--)*a++=*b++;}
static int readblk(const struct ext4*f,uint32_t b,void*out){return b<f->blocks&&block_read(&f->dev,(uint64_t)b*8,8,out);}
int ext4_mount(struct ext4 *f,const struct blockdev*d){
 uint8_t b[FS_BLOCK];*f=(struct ext4){0};if(!d||!block_read(d,0,8,b))return 0;const uint8_t*s=b+1024;
 if(u16(s+56)!=0xef53||u32(s+24)!=2||u32(s+28)!=2||u32(s+20)!=0||u32(s+76)!=1||u16(s+58)!=1)return 0;
 if(u32(s+92)!=0||u32(s+96)!=0x42||(u32(s+100)&~3u)||u32(s+336))return 0;
 struct ext4 t={.dev=*d,.blocks=u32(s+4),.inodes=u32(s),.ipg=u32(s+40),.bpg=u32(s+32),.isize=u16(s+88)};
 copy(t.uuid,s+104,sizeof t.uuid);
 unsigned uuid_nonzero=0;for(unsigned j=0;j<sizeof t.uuid;j++)uuid_nonzero|=t.uuid[j];
 if(!uuid_nonzero||t.blocks<16||t.blocks>d->sectors/8||t.blocks>t.bpg||t.bpg>32768||t.inodes<11||t.inodes>t.ipg||t.ipg>32768||(t.isize!=128&&t.isize!=256))return 0;
 if(!readblk(&t,1,b))return 0;
 uint32_t bm=u32(b),im=u32(b+4);t.table=u32(b+8);uint64_t end=(uint64_t)t.table+((uint64_t)t.ipg*t.isize+4095)/4096;
 if(bm<2||im<2||bm==im||bm>=t.blocks||im>=t.blocks||t.table<2||end>t.blocks||(bm>=t.table&&bm<end)||(im>=t.table&&im<end))return 0;
 *f=t;struct fs_inode root;if(!ext4_inode(f,2,&root)||(root.mode&0xf000)!=0x4000){*f=(struct ext4){0};return 0;}return 1;
}
static int extent_valid(const struct ext4*f,const struct fs_inode*i){const uint8_t*e=i->extent;unsigned n=u16(e+2);if(u16(e)!=0xf30a||u16(e+4)!=4||u16(e+6)||n>4)return 0;uint64_t last=0;
 for(unsigned j=0;j<n;j++){const uint8_t*p=e+12+12*j;uint64_t lo=u32(p),len=u16(p+4),phys=u32(p+8);if(!len||len>32768||u16(p+6)||lo!=last||phys<2||phys>=f->blocks||len>f->blocks-phys)return 0;last=lo+len;if(last>FS_MAX_FILE/FS_BLOCK)return 0;}
 return last==((uint64_t)i->size+4095)/4096;
}
int ext4_inode(const struct ext4*f,uint32_t n,struct fs_inode*i){uint8_t b[FS_BLOCK];if(!f||!i||!n||n>f->inodes||!f->isize)return 0;uint64_t off=(uint64_t)(n-1)*f->isize;uint64_t block=f->table+off/4096;if(block>=f->blocks||!readblk(f,(uint32_t)block,b))return 0;
 const uint8_t*p=b+off%4096;uint16_t mode=u16(p);uint32_t size=u32(p+4);if(((mode&0xf000)!=0x8000&&(mode&0xf000)!=0x4000)||!u16(p+26)||u32(p+20)||u32(p+108)||size>FS_MAX_FILE||u32(p+32)!=0x80000)return 0;
 *i=(struct fs_inode){.number=n,.size=size,.mode=mode};copy(i->extent,p+40,60);return extent_valid(f,i);
}
int ext4_read(const struct ext4*f,const struct fs_inode*i,uint32_t off,void*out,size_t n){uint8_t b[FS_BLOCK];uint8_t *dst=out;if(!f||!i||(!out&&n)||off>i->size||n>i->size-off||!extent_valid(f,i))return 0;
 while(n){uint32_t logical=off/4096,phys=0;for(unsigned j=0;j<u16(i->extent+2);j++){const uint8_t*p=i->extent+12+12*j;uint32_t start=u32(p);if(logical>=start&&logical-start<u16(p+4)){phys=u32(p+8)+logical-start;break;}}
  if(!phys||!readblk(f,phys,b))return 0;
  unsigned pos=off%4096;size_t take=4096-pos;if(take>n)take=n;copy(dst,b+pos,take);dst+=take;off+=(uint32_t)take;n-=take;
 }return 1;
}
int ext4_list(const struct ext4*f,const struct fs_inode*i,fs_visit visit,void*ctx){uint8_t b[FS_BLOCK];if(!i||!visit||(i->mode&0xf000)!=0x4000||!i->size||i->size%4096)return 0;
 for(uint32_t off=0;off<i->size;off+=4096){if(!ext4_read(f,i,off,b,sizeof b))return 0;for(unsigned pos=0;pos<4096;){const uint8_t*p=b+pos;if(4096-pos<8)return 0;uint32_t ino=u32(p);unsigned rec=u16(p+4),len=p[6];if(rec<8||rec%4||rec>4096-pos||len>rec-8||ino>f->inodes||p[7]>7)return 0;
 if(ino){if(!len)return 0;for(unsigned j=0;j<len;j++)if(!p[8+j]||p[8+j]=='/')return 0;if(!visit(ctx,(const char*)p+8,len,ino))return 0;}pos+=rec;}}
 return 1;
}
struct find {const char *name;size_t len;uint32_t ino;};
static int find_entry(void *ctx,const char*name,size_t n,uint32_t ino){struct find*f=ctx;if(n!=f->len)return 1;for(size_t j=0;j<n;j++)if(name[j]!=f->name[j])return 1;if(f->ino)return 0;f->ino=ino;return 1;}
int ext4_lookup(const struct ext4*f,const char*path,struct fs_inode*i){if(!path||*path!='/'||!ext4_inode(f,2,i))return 0;path++;unsigned depth=0;while(*path){const char*p=path;size_t n=0;while(*path&&*path!='/'){if(++n>255)return 0;path++;}if(!n||++depth>8||(n==1&&p[0]=='.')||(n==2&&p[0]=='.'&&p[1]=='.'))return 0;struct find v={p,n,0};if(!ext4_list(f,i,find_entry,&v)||!v.ino||!ext4_inode(f,v.ino,i))return 0;if(*path=='/'){path++;if(!*path)return 0;}}return 1;}
