#include "ext4.h"
#include "elf.h"
#include "vm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned char disk[8*1024*1024], data[FS_MAX_FILE], original[FS_MAX_FILE];
static int rd(void *ctx,uint64_t lba,unsigned count,void *out){(void)ctx;assert(lba<=sizeof disk/512&&count<=sizeof disk/512-lba);memcpy(out,disk+lba*512,count*512);return 1;}
static void le32(unsigned char*p,uint32_t n){for(unsigned j=0;j<4;j++)p[j]=(unsigned char)(n>>(8*j));}
static unsigned count;
static int visit(void *ctx,const char *name,size_t n,uint32_t ino){(void)ctx;(void)name;assert(n&&ino);count++;return 1;}
int main(void){FILE *f=fopen("build/vault.ext4","rb");assert(f&&fread(disk,1,sizeof disk,f)==sizeof disk);fclose(f);struct blockdev d={0,sizeof disk/512,rd};struct ext4 fs;struct fs_inode i;struct elf_app a;
 assert(!block_read(&d,d.sectors,1,data)&&!block_read(&d,UINT64_MAX,1,data)&&!block_read(&d,0,0,data)&&!block_read(&d,0,129,data));assert(ext4_mount(&fs,&d));assert(!memcmp(fs.uuid,"SwampOSVault0001",16));assert(ext4_lookup(&fs,"/",&i)&&ext4_list(&fs,&i,visit,0)&&count>=5);assert(!ext4_lookup(&fs,"/../bin/hello",&i)&&!ext4_lookup(&fs,"/bin//hello",&i)&&!ext4_lookup(&fs,"/missing",&i)&&!ext4_lookup(&fs,"/welcome.txt/foo",&i));
 assert(ext4_lookup(&fs,"/pattern.bin",&i)&&i.size==9000&&ext4_read(&fs,&i,0,data,9000));for(unsigned j=0;j<9000;j++)assert(data[j]==(unsigned char)(j*31u+7u));assert(ext4_read(&fs,&i,4090,data,32));for(unsigned j=0;j<32;j++)assert(data[j]==(unsigned char)((4090+j)*31u+7u));assert(!ext4_read(&fs,&i,9000,data,1)&&!ext4_read(&fs,&i,UINT32_MAX,data,1));
 assert(ext4_lookup(&fs,"/bin/hello",&i)&&ext4_read(&fs,&i,0,data,i.size)&&elf_parse(data,i.size,&a)&&a.entry==USER_CODE);size_t n=i.size;memcpy(original,data,n);f=fopen("build/hello.elf","rb");assert(f&&fread(data,1,n,f)==n&&fgetc(f)==EOF&&!memcmp(original,data,n));fclose(f);
 for(size_t j=0;j<64;j++)assert(!elf_parse(data,j,&a));
 unsigned char save=data[68];data[68]=7;assert(!elf_parse(data,n,&a));data[68]=save;memset(data+24,255,8);assert(!elf_parse(data,n,&a));memcpy(data,original,n);memset(data+32,255,8);assert(!elf_parse(data,n,&a));
 // Deterministic parser mutation stress; rejection or bounded valid parse, no UB.
 unsigned seed=123456;for(unsigned j=0;j<20000;j++){memcpy(data,original,n);seed=seed*1664525u+1013904223u;data[seed%n]^=(unsigned char)(1u+(seed>>24));if(elf_parse(data,n,&a)){assert(a.code>=data&&a.code<data+n&&a.size<=4096&&a.size<=(size_t)(data+n-a.code));}}
 // Corrupt superblock feature flags, length and directory record.
 unsigned char *sb=disk+1024;sb[56]^=1;assert(!ext4_mount(&fs,&d));sb[56]^=1;sb[96]|=4;assert(!ext4_mount(&fs,&d));sb[96]&=~4u;unsigned char old[4];memcpy(old,sb+4,4);le32(sb+4,UINT32_MAX);assert(!ext4_mount(&fs,&d));memcpy(sb+4,old,4);assert(ext4_mount(&fs,&d));assert(!memcmp(fs.uuid,"SwampOSVault0001",16));assert(ext4_lookup(&fs,"/",&i));uint32_t phys=(uint32_t)i.extent[20]|((uint32_t)i.extent[21]<<8)|((uint32_t)i.extent[22]<<16)|((uint32_t)i.extent[23]<<24);assert(phys<fs.blocks);unsigned char *rec=disk+phys*4096;unsigned char lo=rec[4],hi=rec[5];rec[4]=rec[5]=0;assert(!ext4_list(&fs,&i,visit,0));rec[4]=lo;rec[5]=hi;
 unsigned char saved[60];memcpy(saved,i.extent,60);for(unsigned j=0;j<20000;j++){memcpy(i.extent,saved,60);seed=seed*1664525u+1013904223u;i.extent[seed%60]^=(unsigned char)(1u+(seed>>24));(void)ext4_read(&fs,&i,0,data,4096);}
 assert(ext4_lookup(&fs,"/bin/sdk-demo",&i)&&ext4_read(&fs,&i,0,data,i.size)&&elf_parse(data,i.size,&a));
 assert(ext4_lookup(&fs,"/bin/integration-demo",&i)&&ext4_read(&fs,&i,0,data,i.size)&&elf_parse(data,i.size,&a));
 f=fopen("build/integration-demo.elf","rb");assert(f&&fread(original,1,i.size,f)==i.size&&fgetc(f)==EOF&&!memcmp(original,data,i.size));fclose(f);
 puts("PASS: EXT4 volume, nested lookup, multi-block reads, block bounds, malformed metadata, ELF profile, 40000 deterministic parser mutations (UBSan)");return 0;}
