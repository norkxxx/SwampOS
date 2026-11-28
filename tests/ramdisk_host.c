#include "ramdisk.h"
#include "ext4.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char disk[8*1024*1024];
int main(void){FILE*f=fopen("build/vault.ext4","rb");assert(f&&fread(disk,1,sizeof disk,f)==sizeof disk);fclose(f);struct ramdisk r;struct blockdev d;assert(ramdisk_bind(&r,&d,disk,sizeof disk));unsigned char b[512];assert(block_read(&d,d.sectors-1,1,b)&&!block_read(&d,d.sectors,1,b)&&!block_read(&d,d.sectors-1,2,b));struct ext4 fs;struct fs_inode i;assert(ext4_mount(&fs,&d)&&ext4_lookup(&fs,"/welcome.txt",&i)&&i.size<sizeof b&&ext4_read(&fs,&i,0,b,i.size));b[i.size]=0;assert(strstr((char*)b,"NOT encrypted"));assert(ext4_lookup(&fs,"/bin/overlord-demo",&i));struct swamp_bootinfo bi={.reserved=1,.ramdisk=0x800000,.ramdisk_size=sizeof disk,.count=1,.map={{0x1000000,0x100000,1,1}}};assert(ramdisk_boot(&r,&d,&bi));bi.map[0].base=0x900000;assert(!ramdisk_boot(&r,&d,&bi));bi.map[0].base=0x1000000;bi.ramdisk=0x800001;assert(!ramdisk_boot(&r,&d,&bi));puts("PASS: embedded RAM EXT4 block adapter, boundary checks, firmware overlap rejection and file read-back");}
