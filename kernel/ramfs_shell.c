#include "../filesystem/ramfs/ramfs.h"
struct ramfs live_ramfs;
extern void console_write(const char*,size_t),console_number(uint64_t);
static void say(const char*s){size_t n=0;while(s[n])n++;console_write(s,n);}
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static const char*arg(const char*s,const char*p){while(*p&&*s==*p){s++;p++;}return !*p?s:0;}
static int token(const char**s,char*n){unsigned i=0;while(**s==' ')++*s;while(**s&&**s!=' '){if(i+1>=RAMFS_NAME)return 0;n[i++]=*(*s)++;}n[i]=0;while(**s==' ')++*s;return i!=0;}
static void result(int r){static const char*const text[]={"OK","Invalid name or arguments","File not found","RAMFS full","Destination exists","File too large","Directory not empty"};say(r<=0&&r>=-6?text[-r]:"Error");say("\n");}
static int direct_child(const char*path,const char*dir){if(*dir){while(*dir&&*dir==*path){dir++;path++;}if(*dir||*path!='/')return 0;path++;}if(!*path)return 0;while(*path)if(*path++=='/')return 0;return 1;}
int ramfs_command(const char*s){const char*p;char a[RAMFS_NAME],b[RAMFS_NAME];size_t len;
 if(eq(s,"ramdirs")){for(unsigned i=0;i<RAMFS_DIRS;i++)if(!ramfs_dir_entry(&live_ramfs,i,a,sizeof a)){say(a);say("/\n");}return 1;}
 if((p=arg(s,"rammkdir "))){result(token(&p,a)&&!*p?ramfs_mkdir(&live_ramfs,a):RAMFS_INVALID);return 1;}
 if((p=arg(s,"ramrmdir "))){result(token(&p,a)&&!*p?ramfs_rmdir(&live_ramfs,a):RAMFS_INVALID);return 1;}
 if((p=arg(s,"ramls "))){if(!token(&p,b)||*p){result(RAMFS_INVALID);return 1;}if(eq(b,"/"))b[0]=0;if(!ramfs_is_dir(&live_ramfs,b)){result(RAMFS_NOT_FOUND);return 1;}for(unsigned i=0;i<RAMFS_DIRS;i++)if(!ramfs_dir_entry(&live_ramfs,i,a,sizeof a)&&direct_child(a,b)){say(a);say("/\n");}for(unsigned i=0;i<RAMFS_FILES;i++)if(!ramfs_entry(&live_ramfs,i,a,sizeof a,&len)&&direct_child(a,b)){say(a);say("  ");console_number(len);say(" bytes\n");}return 1;}
 if(eq(s,"ramls")){for(unsigned i=0;i<RAMFS_FILES;i++)if(!ramfs_entry(&live_ramfs,i,a,sizeof a,&len)){say(a);say("  ");console_number(len);say(" bytes\n");}return 1;}
 if(eq(s,"ramstat")){say("Volatile files: ");console_number(ramfs_count(&live_ramfs));say(" / ");console_number(RAMFS_FILES);say("; ");console_number(RAMFS_BYTES);say(" bytes per file; no disk writes\n");return 1;}
 if((p=arg(s,"ramappend "))){if(!token(&p,a)){result(RAMFS_INVALID);return 1;}len=0;while(p[len])len++;result(ramfs_append(&live_ramfs,a,p,len));return 1;}
 if((p=arg(s,"ramwrite "))){if(!token(&p,a)){result(RAMFS_INVALID);return 1;}len=0;while(p[len])len++;result(ramfs_write(&live_ramfs,a,p,len));return 1;}
 if((p=arg(s,"ramcat "))){if(!token(&p,a)||*p){result(RAMFS_INVALID);return 1;}static uint8_t data[RAMFS_BYTES];int r=ramfs_read(&live_ramfs,a,data,sizeof data,&len);if(r){result(r);return 1;}for(size_t i=0;i<len;i++){char c=(char)data[i];if((unsigned char)c<32&&c!='\n')c='.';if((unsigned char)c>126)c='.';console_write(&c,1);((volatile uint8_t*)data)[i]=0;}say("\n");return 1;}
 if((p=arg(s,"ramrm "))){result(token(&p,a)&&!*p?ramfs_remove(&live_ramfs,a):RAMFS_INVALID);return 1;}
 if((p=arg(s,"ramcp "))){result(token(&p,a)&&token(&p,b)&&!*p?ramfs_copy(&live_ramfs,a,b):RAMFS_INVALID);return 1;}
 if((p=arg(s,"rammv "))){result(token(&p,a)&&token(&p,b)&&!*p?(ramfs_is_dir(&live_ramfs,a)?ramfs_move_dir(&live_ramfs,a,b):ramfs_rename(&live_ramfs,a,b)):RAMFS_INVALID);return 1;}
 return 0;
}
