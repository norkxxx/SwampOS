#include "../filesystem/ramfs/ramfs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct ramfs fs;
static uint8_t input[RAMFS_BYTES+1],output[RAMFS_BYTES+2];
static char console[8192];static size_t console_len;
void console_write(const char*s,size_t n){assert(n<sizeof console-console_len);memcpy(console+console_len,s,n);console_len+=n;console[console_len]=0;}
void console_number(uint64_t n){char b[32];int len=snprintf(b,sizeof b,"%llu",(unsigned long long)n);assert(len>0);console_write(b,(size_t)len);}
static void cmd(const char*s,const char*expected){console_len=0;console[0]=0;assert(ramfs_command(s));assert(strstr(console,expected));}
int main(void){ramfs_init(&fs);assert(!ramfs_count(&fs));size_t n=77;assert(ramfs_write(&fs,"../bad",input,1)==RAMFS_INVALID);assert(ramfs_write(&fs,"/disk",input,1)==RAMFS_INVALID);assert(ramfs_write(&fs,"",input,1)==RAMFS_INVALID);assert(ramfs_write(&fs,"a",input,RAMFS_BYTES+1)==RAMFS_TOO_LARGE);assert(!ramfs_count(&fs));for(unsigned i=0;i<RAMFS_BYTES;i++)input[i]=(uint8_t)(i*19);
 assert(!ramfs_write(&fs,"notes.txt",input,RAMFS_BYTES));assert(ramfs_read(&fs,"notes.txt",output,1,&n)==RAMFS_TOO_LARGE&&n==77);output[0]=0xa5;output[RAMFS_BYTES+1]=0x5a;assert(!ramfs_read(&fs,"notes.txt",output+1,RAMFS_BYTES,&n)&&n==RAMFS_BYTES&&!memcmp(output+1,input,n));assert(output[0]==0xa5&&output[RAMFS_BYTES+1]==0x5a);
 assert(!ramfs_copy(&fs,"notes.txt","copy"));assert(ramfs_copy(&fs,"notes.txt","copy")==RAMFS_EXISTS);assert(!ramfs_rename(&fs,"copy","copy2"));assert(ramfs_read(&fs,"copy",output,sizeof output,&n)==RAMFS_NOT_FOUND);assert(!ramfs_write(&fs,"notes.txt","A",1));for(unsigned i=1;i<RAMFS_BYTES;i++)assert(!fs.entries[0].data[i]);assert(!ramfs_remove(&fs,"notes.txt"));for(size_t i=0;i<sizeof fs.entries[0];i++)assert(!((unsigned char*)&fs.entries[0])[i]);assert(!ramfs_remove(&fs,"copy2"));
 char name[32];for(unsigned i=0;i<RAMFS_FILES;i++){snprintf(name,sizeof name,"file%u",i);assert(!ramfs_write(&fs,name,input,1));}assert(ramfs_count(&fs)==RAMFS_FILES);assert(ramfs_write(&fs,"overflow",input,1)==RAMFS_FULL);assert(ramfs_copy(&fs,"file0","overflow")==RAMFS_FULL);assert(!ramfs_rename(&fs,"file0","renamed"));ramfs_init(&fs);
 unsigned seed=1;for(unsigned i=0;i<50000;i++){seed=seed*1664525u+1013904223u;unsigned k=(seed>>16)%RAMFS_FILES;snprintf(name,sizeof name,"f%u",k);size_t len=(seed>>3)%RAMFS_BYTES;assert(!ramfs_write(&fs,name,input,len));assert(!ramfs_read(&fs,name,output,sizeof output,&n)&&n==len&&!memcmp(input,output,len));if(seed&1)assert(!ramfs_remove(&fs,name));assert(ramfs_count(&fs)<=RAMFS_FILES);}
 ramfs_init(&live_ramfs);cmd("ramwrite hello.txt hello swamp","OK");cmd("ramcat hello.txt","hello swamp");cmd("ramcp hello.txt copy.txt","OK");cmd("rammv copy.txt final.txt","OK");cmd("ramls","final.txt");cmd("ramstat","2 / 64");cmd("ramrm hello.txt","OK");cmd("ramcat hello.txt","File not found");cmd("ramwrite ../escape no","Invalid");cmd("rammv final.txt bad extra","Invalid");assert(!ramfs_command("unknown"));puts("PASS: RAMFS lifecycle, capacity, copy/rename/remove, scrub, bounds, 50000 data round trips and shell integration (UBSan)");}
