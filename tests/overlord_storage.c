#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ext4.h"
#include "elf.h"
#include "vm.h"
static unsigned char disk[8*1024*1024],data[FS_MAX_FILE],expected[FS_MAX_FILE];
static int read_disk(void *ctx,uint64_t lba,unsigned count,void *out){(void)ctx;if(lba>sizeof disk/512||count>sizeof disk/512-lba)return 0;memcpy(out,disk+lba*512,count*512);return 1;}
int main(void){FILE*f=fopen("build/vault.ext4","rb");assert(f&&fread(disk,1,sizeof disk,f)==sizeof disk);fclose(f);struct blockdev dev={0,sizeof disk/512,read_disk};struct ext4 fs;struct fs_inode i;struct elf_app a;assert(ext4_mount(&fs,&dev)&&ext4_lookup(&fs,"/bin/overlord-demo",&i)&&ext4_read(&fs,&i,0,data,i.size)&&elf_parse(data,i.size,&a)&&a.entry>=USER_CODE&&a.entry-USER_CODE<a.size);f=fopen("build/overlord-demo.elf","rb");assert(f&&fread(expected,1,i.size,f)==i.size&&fgetc(f)==EOF&&!memcmp(expected,data,i.size));fclose(f);assert(a.data_files==4&&a.data_mem>=12&&a.data_mem<=4096&&a.data[0]==0x5a&&a.data[1]==0x5a);size_t n=i.size;unsigned seed=7;for(unsigned j=0;j<20000;j++){memcpy(data,expected,n);seed=seed*1664525u+1013904223u;data[seed%n]^=(unsigned char)(1+(seed>>24));if(elf_parse(data,n,&a)){assert(a.size<=4096&&a.data_mem<=4096&&a.data_files<=a.data_mem);if(a.data_files)assert(a.data>=data&&a.data_files<=(size_t)(data+n-a.data));}}puts("PASS: Overlord ELF read-back, RW/BSS bounds, 20000 two-segment ELF mutations (UBSan)");return 0;}
