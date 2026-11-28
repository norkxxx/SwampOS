#include "../kernel/session.h"
#include "../kernel/policy.h"
#include "../kernel/ext4.h"
#include "../kernel/elf.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
static struct app_session s;
static unsigned char data[SESSION_DATA_MAX],exe[FS_MAX_FILE];
static unsigned status,reads;
static int fail;
static jmp_buf done;
static char output[128];
void resource_scan_entry(void);
unsigned long swamp_caps(void){return s.grants;}
long swamp_data_read(void *p,size_t n,unsigned long off){reads++;if(fail==1)return -14;return session_read(&s,off,p,n);}
long swamp_write(const char *p,size_t n){assert(n<sizeof output);memcpy(output,p,n);output[n]=0;return fail==2?-1:(long)n;}
__attribute__((noreturn)) void swamp_exit(unsigned code){status=code;longjmp(done,1);}
static void run(unsigned expected){reads=0;output[0]=0;if(!setjmp(done))resource_scan_entry();assert(status==expected);}
static int read_disk(void *ctx,uint64_t lba,unsigned n,void *out){FILE*f=ctx;return !fseek(f,(long)(lba*512),SEEK_SET)&&fread(out,512,n,f)==n;}
int main(void){
 for(size_t i=0;i<sizeof data;i++)data[i]=(unsigned char)(31*i+7);
 unsigned sizes[]={0,1,255,256,257,4096,8192,9000,65535,65536};
 for(unsigned k=0;k<sizeof sizes/sizeof *sizes;k++){
  unsigned n=sizes[k];assert(session_begin(&s,1,CAP_DATA|CAP_CONSOLE,data,n));run(0);
  uint32_t h=2166136261u;for(unsigned i=0;i<n;i++)h=(h^data[i])*16777619u;
  char expected[128];snprintf(expected,sizeof expected,"RESOURCE_SCAN_OK bytes=0x%08X fnv1a=0x%08X\n",n,h);assert(!strcmp(output,expected));assert(reads==(n+255)/256+1);
  assert(session_read(&s,0,exe,257)==-22);assert(session_read(&s,UINT64_MAX,exe,1)==0);
  session_clear(&s);for(size_t i=0;i<sizeof s;i++)assert(!((unsigned char*)&s)[i]);
 }
 assert(!session_begin(&s,1,CAP_DATA,data,sizeof data+1));
 assert(session_begin(&s,1,0,0,0));run(13);assert(!reads);
 assert(session_begin(&s,1,CAP_DATA,data,1));run(25);assert(!reads);
 assert(session_begin(&s,1,CAP_DATA|CAP_CONSOLE,data,1));fail=1;run(23);fail=2;run(25);fail=0;session_clear(&s);
 FILE*f=fopen("build/vault.ext4","rb");assert(f&&!fseek(f,0,SEEK_END));struct blockdev d={f,(uint64_t)ftell(f)/512,read_disk};struct ext4 fs;struct fs_inode i;struct elf_app a;
 assert(ext4_mount(&fs,&d)&&ext4_lookup(&fs,"/bin/resource-scan",&i)&&ext4_read(&fs,&i,0,exe,i.size)&&elf_parse(exe,i.size,&a));fclose(f);
 f=fopen("build/resource-scan.elf","rb");assert(f&&fread(data,1,i.size,f)==i.size&&fgetc(f)==EOF&&!memcmp(exe,data,i.size));fclose(f);
 puts("RESOURCE_SCAN_HOST_OK: 10 boundary sizes, checksum and read count, permissions, I/O errors, scrubbing, EXT4 ELF read-back (mock syscalls)");
}
