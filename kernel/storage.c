#include "storage.h"
#include "persist.h"
#include "ext4.h"
#include "elf.h"
#include "user.h"
#include "vm.h"
#include "policy.h"
#include "guard.h"
#include "audit.h"
#include "gpt.h"
#include "vfs.h"
#include "session.h"
#include "process.h"
#include "ramdisk.h"
#include "../filesystem/ramfs/import.h"
#include "../security/onion/resource.h"
static struct ramdisk live_disk;
static int live_mode(void){return boot_uefi&&((const struct swamp_bootinfo*)SWAMP_UEFI_INFO)->reserved==1;}
extern void console_write(const char *,size_t),console_number(uint64_t);
static struct gpt partition_table;
static struct vfs mounted;
static struct guard_table launch_guard;
static uint64_t guard_revision=1;
static uint64_t gui_revision;
static uint32_t gui_inode;
static uint8_t gui_volume_id[16];
#define fs mounted.fs
#define ready mounted.mounted
static uint8_t file[FS_MAX_FILE];
/* Shell runs synchronously. Separate staging avoids large kernel-stack objects. */
static struct resource_snapshot selected_resource;
_Static_assert(SESSION_DATA_MAX == FS_MAX_FILE, "resource size contract");
static uint8_t group_code[PROC_MAX][4096],group_data[PROC_MAX][4096];
static void say(const char*s){size_t n=0;while(s[n])n++;console_write(s,n);}
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static const char*arg(const char*s,const char*p){while(*p&&*s==*p){s++;p++;}return !*p&&*s?s:0;}
static void safe(const char*s,size_t n){while(n--){unsigned char c=(unsigned char)*s++;char out=(c>=32&&c<=126)||c=='\n'||c=='\t'?(char)c:'.';console_write(&out,1);}}
int storage_init(void){guard_revision++;struct blockdev dev;vfs_unmount(&mounted);return (live_mode()?ramdisk_boot(&live_disk,&dev,(const struct swamp_bootinfo*)SWAMP_UEFI_INFO):ata_secondary_master(&dev))&&vfs_mount_demo(&mounted,&dev);}
static int load(const char*path,struct fs_inode*i){return ready&&ext4_lookup(&fs,path,i)&&(i->mode&0xf000)==0x8000&&ext4_read(&fs,i,0,file,i->size);}
/* Restrict explicit grants before creating a user process or copying its resource. */
static int grant_ok(uint32_t ino,uint64_t requested){
 if(ready&&guard_allows(&launch_guard,mounted.volume_id,ino,requested))return 1;
 audit_add(&kernel_audit,5,-1,((uint64_t)ino<<32)|requested);
 say("Launch denied by SwampGuard.\n");return 0;
}
static int guard_lookup(const char *path,struct fs_inode *i){struct elf_app a;return path&&load(path,i)&&elf_parse(file,i->size,&a);}
int storage_guard_read(const char *path,uint64_t *mask,uint64_t *revision){
 struct fs_inode i;if(!mask||!revision||!guard_lookup(path,&i))return 0;
 *mask=guard_ceiling(&launch_guard,mounted.volume_id,i.number);*revision=guard_revision;return 1;
}
static int guard_update(const uint8_t volume_id[16],uint32_t ino,uint64_t mask,int remove){
 if(!ready||!(remove?guard_remove(&launch_guard,volume_id,ino):guard_set(&launch_guard,volume_id,ino,mask)))return 0;
 guard_revision++;audit_add(&kernel_audit,6,0,((uint64_t)ino<<32)|(remove?CAP_ALL:mask));return 1;
}
int storage_guard_apply(const char *path,uint64_t mask,uint64_t revision){
 struct fs_inode i;if(revision!=guard_revision||(mask&~CAP_ALL)||!guard_lookup(path,&i))return 0;
 return guard_update(mounted.volume_id,i.number,mask,0);
}
static int guard_command(const char *s){
 const char *p;unsigned mode;
 if(eq(s,"guard-list")){static const char hex[]="0123456789abcdef";say("SwampGuard ceilings: console=1 data=2 IPC=4 scratch=8 GUI=32\n");for(unsigned j=0;j<GUARD_MAX;j++)if(launch_guard.entry[j].inode){say("volume=");for(unsigned k=0;k<16;k++){char b[2]={hex[launch_guard.entry[j].volume_id[k]>>4],hex[launch_guard.entry[j].volume_id[k]&15]};console_write(b,2);}say(" inode=");console_number(launch_guard.entry[j].inode);say(" ceiling=");console_number(launch_guard.entry[j].ceiling);say("\n");}return 1;}
 if((p=arg(s,"guard-set ")))mode=1;else if((p=arg(s,"guard-show ")))mode=2;else if((p=arg(s,"guard-reset ")))mode=3;else return 0;
 char path[128];size_t n=0;while(*p&&*p!=' '&&n+1<sizeof path)path[n++]=*p++;path[n]=0;
 uint64_t mask=0;int valid=n&& (mode==1?(*p==' '&&guard_parse_mask(p+1,&mask)):!*p);
 struct fs_inode i;
 if(!valid||!guard_lookup(path,&i)){say("Invalid guard request or ELF.\n");return 1;}
 if(mode==2){say("SwampGuard ceiling: ");console_number(guard_ceiling(&launch_guard,mounted.volume_id,i.number));say("\n");return 1;}
 if(guard_update(mounted.volume_id,i.number,mask,mode==3)){
  say("Guard policy updated.\n");
 }else say("Guard update rejected (full or absent).\n");return 1;
}
int storage_selftest(void){struct fs_inode i;struct elf_app a;if(!load("/pattern.bin",&i)||i.size!=9000)return 0;for(unsigned j=0;j<i.size;j++)if(file[j]!=(uint8_t)(j*31u+7u))return 0;if(!load("/bin/hello",&i)||!elf_parse(file,i.size,&a))return 0;return a.size>0;}
int storage_gui_demo(unsigned char*out,size_t n){struct fs_inode i;struct elf_app a;if(!out||n!=GUI_FRAME_BYTES||!load("/bin/gui-demo",&i)||!elf_parse(file,i.size,&a)||!grant_ok(i.number,CAP_GUI))return -1;struct user_image im={.code=a.code,.size=a.size,.offset=a.entry-USER_CODE,.grants=CAP_GUI,.data=a.data,.data_files=a.data_files,.data_mem=a.data_mem};return user_run_gui(&im,out,n);}
int storage_gui_start(void){struct fs_inode i;struct elf_app a;
 if(user_gui_active()||!load("/bin/gui-demo",&i)||!elf_parse(file,i.size,&a)||!grant_ok(i.number,CAP_GUI))return 0;
 struct user_image im={.code=a.code,.size=a.size,.offset=a.entry-USER_CODE,.grants=CAP_GUI,.data=a.data,.data_files=a.data_files,.data_mem=a.data_mem};
 if(!user_gui_start(&im))return 0;
 gui_inode=i.number;for(unsigned j=0;j<16;j++)gui_volume_id[j]=mounted.volume_id[j];gui_revision=guard_revision;return 1;
}
int storage_gui_poll(unsigned char*out,size_t n,int*published){
 if(!published)return -1;
 *published=0;
 if(!user_gui_active())return -1;
 unsigned volume_diff=0;for(unsigned j=0;j<16;j++)volume_diff|=(unsigned)(gui_volume_id[j]^mounted.volume_id[j]);
 if(gui_revision!=guard_revision||!ready||volume_diff||!guard_allows(&launch_guard,mounted.volume_id,gui_inode,CAP_GUI)){user_gui_cancel();audit_add(&kernel_audit,5,-125,gui_inode);return -1;}
 return user_gui_step(out,n,published);
}
static int visit(void *ctx,const char*name,size_t n,uint32_t ino){(void)ctx;safe(name,n);say("  inode=");console_number(ino);say("\n");return 1;}
int storage_command(const char*s){const char*p=0;struct fs_inode i;
 if(persist_command(s))return 1;
 if(guard_command(s))return 1;
 if(eq(s,"mount-live")){say(live_mode()&&storage_init()?"Live EXT4 mounted read-only.\n":"Live volume unavailable.\n");return 1;}
 if(live_mode()&&(eq(s,"gpt")||arg(s,"mount-gpt "))){say("Physical disk access disabled in Live mode.\n");return 1;}
 if(eq(s,"ata-info")){if(live_mode()){say("Physical disk access disabled in Live mode.\n");return 1;}struct blockdev dev;if(!ata_secondary_master(&dev)){say("No supported secondary IDE disk.\n");return 1;}say("Read-only ATA sectors: ");console_number(dev.sectors);say(" (512 bytes each)\n");return 1;}
 if(eq(s,"gpt")){struct blockdev dev;if(!ata_secondary_master(&dev)||!gpt_read(&dev,&partition_table)){say("No validated GPT on secondary IDE disk.\n");return 1;}for(unsigned j=0;j<partition_table.count;j++){say("Partition LBA ");console_number(partition_table.parts[j].first);say(" - ");console_number(partition_table.parts[j].last);say("\n");}return 1;}
 if(eq(s,"umount")){guard_revision++;vfs_unmount(&mounted);say("Volume unmounted.\n");return 1;}
 if((p=arg(s,"mount-gpt "))){guard_revision++;vfs_unmount(&mounted);unsigned index=0,digits=0;while(*p>='0'&&*p<='9'&&digits<3){index=index*10+(unsigned)(*p++-'0');digits++;}struct blockdev dev;if(*p||!digits||!index||index>128||!ata_secondary_master(&dev)||!gpt_read(&dev,&partition_table)||!vfs_mount_partition(&mounted,&dev,&partition_table,index-1))say("Mount denied or unsupported.\n");else say("GPT EXT4 mounted read-only.\n");return 1;}
 if(eq(s,"storage")){say(ready?(live_mode()?"EXT4 mounted: read-only RAM volume\n":"EXT4 mounted: read-only, secondary IDE master, lab data\n"):"No supported EXT4 volume.\n");return 1;}
 if((p=arg(s,"ramimport "))){char src[128],dst[RAMFS_NAME];size_t n=0,m=0;while(*p&&*p!=' '&&n+1<sizeof src)src[n++]=*p++;src[n]=0;if(*p!=' '){say("Usage: ramimport /source destination\n");return 1;}while(*p==' ')p++;while(*p&&*p!=' '&&m+1<sizeof dst)dst[m++]=*p++;dst[m]=0;if(*p||!n||!m||!ready){say("Import rejected or no volume.\n");return 1;}int r=ramfs_import_ext4(&live_ramfs,&fs,src,dst,file,sizeof file);say(r==0?"RAMIMPORT_OK\n":r==RAMFS_TOO_LARGE?"Import file too large.\n":"Import rejected.\n");return 1;}
 if(eq(s,"gui-async-test")){unsigned char out[GUI_FRAME_BYTES];unsigned count=0;int rc=-1;
  if(!user_gui_active()&&storage_gui_start()){for(unsigned i=0;i<100;i++){int published=0;rc=storage_gui_poll(out,sizeof out,&published);count+=(unsigned)published;if(rc)break;}if(user_gui_active())user_gui_cancel();}
  say(rc==1&&count==24?"GUI_ASYNC_TEST_OK\n":"GUI_ASYNC_TEST_FAIL\n");return 1;}
 if(eq(s,"gui-demo")){unsigned char out[GUI_FRAME_BYTES];say(storage_gui_demo(out,sizeof out)==0?"GUI_DEMO_OK\n":"GUI_DEMO_FAIL\n");return 1;}
 if(eq(s,"fscheck")){say(storage_selftest()?"STORAGE_TEST_OK\n":"STORAGE_TEST_FAIL\n");return 1;}
 if(eq(s,"ls")||(p=arg(s,"ls "))){if(eq(s,"ls"))p="/";if(!ready||!ext4_lookup(&fs,p,&i)||!ext4_list(&fs,&i,visit,0))say("Directory unavailable or unsupported.\n");return 1;}
 if((p=arg(s,"cat "))){if(!load(p,&i))say("File unavailable or unsupported.\n");else {size_t n=i.size>4096?4096:i.size;safe((const char*)file,n);say(i.size>n?"\n[truncated]\n":"\n");}return 1;}
 if((p=arg(s,"run-group "))){
  struct user_image images[PROC_MAX];int results[PROC_MAX];unsigned count=0;int ok=1;
  /* Each token has an explicit per-slot grant prefix: n=none, c=console, i=IPC, s=scratch; a=all three. */
  while(*p&&count<PROC_MAX){char mode=*p++;if(*p!=':'){ok=0;break;}p++;char path[128];size_t n=0;while(*p&&*p!=' '&&n<sizeof path-1)path[n++]=*p++;path[n]=0;
   uint64_t cap=mode=='n'?0:mode=='c'?CAP_CONSOLE:mode=='i'?CAP_IPC:mode=='s'?CAP_SCRATCH:mode=='a'?(CAP_CONSOLE|CAP_IPC|CAP_SCRATCH):UINT64_MAX;
   struct elf_app a;if(!n||(*p&&*p!=' ')||cap==UINT64_MAX||!load(path,&i)||!elf_parse(file,i.size,&a)||!grant_ok(i.number,cap)){ok=0;break;}for(size_t j=0;j<a.size;j++)group_code[count][j]=a.code[j];for(size_t j=0;j<a.data_files;j++)group_data[count][j]=a.data[j];images[count]=(struct user_image){.code=group_code[count],.size=a.size,.offset=a.entry-USER_CODE,.grants=cap,.data=group_data[count],.data_files=a.data_files,.data_mem=a.data_mem};count++;while(*p==' ')p++;
  }
  if(!ok||*p||count<2||!user_run_images(images,count,results))say("Group rejected.\n");else {say("Group results:");for(unsigned j=0;j<count;j++){say(" ");console_number((uint64_t)results[j]);}say("\n");}
  for(unsigned j=0;j<PROC_MAX;j++)for(unsigned k=0;k<4096;k++){((volatile uint8_t*)group_code[j])[k]=0;((volatile uint8_t*)group_data[j])[k]=0;}
  return 1;
 }
 if((p=arg(s,"run-pair "))){struct elf_app a;int results[2];if(!load(p,&i)||!elf_parse(file,i.size,&a)||!grant_ok(i.number,CAP_CONSOLE|CAP_IPC|CAP_SCRATCH)){say("Group rejected.\n");return 1;}struct user_image one={.code=a.code,.size=a.size,.offset=a.entry-USER_CODE,.grants=CAP_CONSOLE|CAP_IPC|CAP_SCRATCH,.data=a.data,.data_files=a.data_files,.data_mem=a.data_mem};struct user_image pair[2]={one,one};if(!user_run_images(pair,2,results)){say("Group rejected.\n");return 1;}say("Group results: ");console_number((uint64_t)results[0]);say(" ");console_number((uint64_t)results[1]);say("\n");return 1;}
 int console=0;if((p=arg(s,"run-resource-console ")))console=1;else p=arg(s,"run-resource ");
 if(p){struct resource_request req;struct elf_app a;
  if(!resource_parse(p,&req)||!load(req.app,&i)||!elf_parse(file,i.size,&a)){say("Resource launch rejected.\n");return 1;}
  if(!grant_ok(i.number,CAP_DATA|(console?CAP_CONSOLE:0)))return 1;
  if(!resource_prepare(&req,&live_ramfs,ready?&fs:0,&selected_resource)){say("Resource unavailable or too large.\n");return 1;}
  struct user_image im={.code=a.code,.size=a.size,.offset=a.entry-USER_CODE,.grants=CAP_DATA|(console?CAP_CONSOLE:0),.data=a.data,.data_files=a.data_files,.data_mem=a.data_mem};
  int r=user_run_native(&im,selected_resource.bytes,selected_resource.length);resource_clear(&selected_resource);say("App result: ");if(r<0)say("allocation failed");else console_number((uint64_t)r);say("\n");return 1;
 }
 uint64_t grant=0;if((p=arg(s,"run-scratch ")))grant=CAP_SCRATCH|CAP_CONSOLE;else if((p=arg(s,"run-data ")))grant=CAP_DATA|CAP_CONSOLE;else if((p=arg(s,"run-console ")))grant=CAP_CONSOLE;else p=arg(s,"run ");if(p){struct elf_app a;if(!load(p,&i)||!elf_parse(file,i.size,&a)){say("Rejected ELF.\n");return 1;}if(!grant_ok(i.number,grant))return 1;unsigned char data[SESSION_READ_MAX];size_t len=0;if(grant&CAP_DATA){struct fs_inode resource;if(!ext4_lookup(&fs,"/welcome.txt",&resource)||(resource.mode&0xf000)!=0x8000||resource.size>sizeof data||!ext4_read(&fs,&resource,0,data,resource.size)){say("Resource unavailable.\n");return 1;}len=resource.size;}struct user_image im={.code=a.code,.size=a.size,.offset=a.entry-USER_CODE,.grants=grant,.data=a.data,.data_files=a.data_files,.data_mem=a.data_mem};int r=user_run_native(&im,data,len);for(size_t j=0;j<len;j++)((volatile unsigned char*)data)[j]=0;say("App result: ");if(r<0)say("allocation failed");else console_number((uint64_t)r);say("\n");return 1;}return 0;
}

struct preview {char *out;size_t cap,n;};
static void preview_char(struct preview*p,char c){if(p->n+1<p->cap){p->out[p->n++]=c;p->out[p->n]=0;}}
static void preview_str(struct preview*p,const char*s){while(*s)preview_char(p,*s++);}
static int preview_entry(void *ctx,const char*name,size_t n,uint32_t ino){(void)ino;struct preview*p=ctx;for(size_t i=0;i<n;i++)preview_char(p,name[i]>=32&&name[i]<=126?name[i]:'.');preview_char(p,'\n');return 1;}
void storage_live_preview(unsigned which,char*out,size_t cap){if(!out||!cap)return;struct preview p={out,cap,0};out[0]=0;struct fs_inode i;
 if(!live_mode()||!ready){preview_str(&p,"No mounted Live EXT4 volume.");return;}
 if(which==2){if(!ext4_lookup(&fs,"/bin",&i)||!ext4_list(&fs,&i,preview_entry,&p))preview_str(&p,"Directory unavailable.");return;}
 if(!load(which==1?"/pattern.bin":"/welcome.txt",&i)){preview_str(&p,"File unavailable.");return;}
 if(which==1){static const char hex[]="0123456789ABCDEF";preview_str(&p,"HEX PREVIEW / FIRST 64 BYTES\n");for(unsigned j=0;j<i.size&&j<64;j++){preview_char(&p,hex[file[j]>>4]);preview_char(&p,hex[file[j]&15]);preview_char(&p,j%16==15?'\n':' ');}}
 else for(unsigned j=0;j<i.size&&j<256;j++){unsigned c=file[j];preview_char(&p,(c>=32&&c<=126)||c=='\n'?(char)c:'.');}
}
