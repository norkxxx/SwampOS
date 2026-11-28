#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../kernel/guard.h"
#include "../desktop/window.h"
#include "../kernel/guard_ui.h"
#include "../kernel/policy.h"
#include "../kernel/storage.h"
#include "../kernel/fixture_disk.h"
#include "../kernel/ramdisk.h"
#include "../kernel/user.h"
#include "../kernel/audit.h"
#include "../filesystem/ramfs/ramfs.h"
unsigned boot_uefi;
const char *fixture_open(void){return "fixture absent";}
void fixture_close(void){}
int fixture_io(enum fixture_op op,unsigned lba,void *p){(void)op;(void)lba;(void)p;return 0;}
struct ramfs live_ramfs;
static unsigned char disk[8*1024*1024];
static struct ramdisk rd;
static unsigned native_runs,group_runs,gui_runs;
static uint64_t last_grants;
static char output[4096];static size_t used;
void console_write(const char *p,size_t n){assert(n<sizeof output-used);memcpy(output+used,p,n);used+=n;output[used]=0;}
void console_number(uint64_t n){char b[32];int l=snprintf(b,sizeof b,"%llu",(unsigned long long)n);console_write(b,(size_t)l);}
int ata_secondary_master(struct blockdev *d){return ramdisk_bind(&rd,d,disk,sizeof disk);}
int user_run_native(const struct user_image *a,const void *p,size_t n){assert(a&&a->code&&(!n||p));native_runs++;last_grants=a->grants;return 0;}
int user_run_images(const struct user_image *a,unsigned n,int *out){assert(a&&n>=2);group_runs++;for(unsigned i=0;i<n;i++)out[i]=0;return 1;}
int user_run_gui(const struct user_image *a,unsigned char *out,size_t n){assert(a->grants==CAP_GUI&&n==GUI_FRAME_BYTES);gui_runs++;memset(out,1,n);return 0;}
static unsigned async_live,async_steps,async_cancel;
int user_gui_active(void){return async_live;}
int user_gui_start(const struct user_image*a){assert(!async_live&&a->grants==CAP_GUI&&a->code);async_live=1;return 1;}
void user_gui_cancel(void){assert(async_live);async_live=0;async_cancel++;}
int user_gui_step(unsigned char*out,size_t n,int*published){assert(async_live&&n==GUI_FRAME_BYTES);async_steps++;memset(out,2,n);*published=1;return 0;}
static void cmd(const char*s,const char*expected){used=0;output[0]=0;assert(storage_command(s));if(!strstr(output,expected)){fprintf(stderr,"%s => %s, expected %s\n",s,output,expected);abort();}}
static void unit(void){struct guard_table t={0};uint64_t x=99;uint8_t volume[16]={1},other[16]={2},zero[16]={0};
 assert(!guard_parse_mask("16",&x)&&x==99);assert(!guard_parse_mask("-1",&x));assert(!guard_parse_mask("18446744073709551615",&x));assert(!guard_parse_mask("",&x));assert(!guard_parse_mask("1 ",&x));assert(guard_parse_mask("47",&x)&&x==CAP_ALL);
 assert(!guard_allows(&t,zero,1,0)&&!guard_allows(&t,volume,0,0)&&!guard_allows(0,volume,1,0));
 for(unsigned i=1;i<=GUARD_MAX;i++)assert(guard_set(&t,volume,i,i&CAP_ALL));
 assert(!guard_set(&t,volume,GUARD_MAX+1,0));assert(!guard_set(&t,volume,1,16));assert(guard_set(&t,volume,1,0));assert(guard_remove(&t,volume,2));assert(guard_set(&t,volume,GUARD_MAX+1,0));
 for(unsigned i=0;i<100000;i++){uint32_t ino=1+i%GUARD_MAX;uint64_t mask=(i*313u)&CAP_ALL;uint64_t requested=(i*73u)&63u; if(!guard_set(&t,volume,ino,mask)){assert(ino==2);assert(guard_remove(&t,volume,GUARD_MAX+1));assert(guard_set(&t,volume,ino,mask));}assert(guard_ceiling(&t,volume,ino)==mask);assert(guard_allows(&t,volume,ino,requested)==(!(requested&~mask)));}
 /* The same inode number on a different EXT4 UUID must not inherit a profile. */
 struct guard_table isolated={0};assert(guard_set(&isolated,volume,42,CAP_CONSOLE));assert(guard_set(&isolated,other,42,CAP_DATA));
 assert(guard_ceiling(&isolated,volume,42)==CAP_CONSOLE);assert(guard_ceiling(&isolated,other,42)==CAP_DATA);
 assert(guard_allows(&isolated,volume,42,CAP_CONSOLE)&&!guard_allows(&isolated,volume,42,CAP_DATA));
 assert(guard_allows(&isolated,other,42,CAP_DATA)&&!guard_allows(&isolated,other,42,CAP_CONSOLE));
 assert(guard_remove(&isolated,volume,42));assert(guard_ceiling(&isolated,volume,42)==CAP_ALL&&guard_ceiling(&isolated,other,42)==CAP_DATA);
}
static uint32_t pixels[640*360+2];
static void editor_action(struct wm_state *w){if(w->action)assert(guard_editor_execute(&w->tools.guard,w->action));w->action=0;}
static void editor_key(struct wm_state*w,struct live_state*d,int k){assert(wm_key(w,d,k));editor_action(w);}
static void editor(void){
 struct wm_state w;struct live_state d;wm_init(&w);live_init(&d);wm_open(&w,TOOL_GUARD);struct guard_editor*g=&w.tools.guard;
 editor_key(&w,&d,'p');assert(g->editing&&g->valid&&g->actual==CAP_ALL);
 editor_key(&w,&d,'0');assert(!g->draft&&g->actual==CAP_ALL);uint64_t m=0,r=0;assert(storage_guard_read("/bin/hello",&m,&r)&&m==CAP_ALL);
 editor_key(&w,&d,'a');assert(g->valid&&g->status==1&&!g->actual);cmd("run-console /bin/hello","denied");
 editor_key(&w,&d,'1');cmd("guard-set /bin/hello 2","updated");editor_key(&w,&d,'a');assert(g->status==2&&!g->valid);cmd("guard-show /bin/hello","ceiling: 2");
 editor_key(&w,&d,'r');assert(g->actual==2&&g->valid);editor_key(&w,&d,'1');uint64_t prior=g->revision;cmd("umount","unmounted");assert(storage_init());assert(!storage_guard_apply("/bin/hello",3,prior));editor_key(&w,&d,'a');assert(!g->valid&&g->status==2);editor_key(&w,&d,'r');assert(g->actual==2);
 assert(!storage_guard_read("/welcome.txt",&m,&r));assert(!storage_guard_apply("/bin/hello",16,g->revision));
 cmd("guard-reset /bin/hello","updated");editor_key(&w,&d,'r');assert(g->actual==CAP_ALL);
 for(unsigned i=0;i<GUARD_UI_APPS;i++){assert(g->app==i&&g->valid);editor_key(&w,&d,'j');}assert(!g->app);
 // Mouse row toggles and applies only after a distinct click; right half denies all without applying.
 struct wm_rect q=w.windows[TOOL_GUARD].rect;w.mouse_visible=1;w.mouse_x=q.x+10;w.mouse_y=q.y+28+3*11+3;
 wm_mouse(&w,&d,640,360,0,0,1);editor_action(&w);assert(g->draft==(CAP_ALL^CAP_CONSOLE));wm_mouse(&w,&d,640,360,0,0,0);
 w.mouse_x=q.x+q.w-15;w.mouse_y=q.y+28+8*11+3;wm_mouse(&w,&d,640,360,0,0,1);editor_action(&w);assert(!g->draft&&g->actual==CAP_ALL);wm_mouse(&w,&d,640,360,0,0,0);
 w.mouse_x=q.x+10;wm_mouse(&w,&d,640,360,0,0,1);editor_action(&w);assert(!g->actual&&g->status==1);wm_mouse(&w,&d,640,360,0,0,0);
 cmd("guard-reset /bin/hello","updated");editor_key(&w,&d,'r');editor_key(&w,&d,'2');
 struct surface surf={pixels+1,640*360,640,360,640};pixels[0]=pixels[640*360+1]=0x12345678;
 unsigned seed=17;for(unsigned i=0;i<50000;i++){seed=seed*1664525u+1013904223u;const char *keys="123450jkrpa";struct guard_editor t=*g;unsigned action=tools_guard_key(&t,keys[(seed>>8)%11]);assert(t.app<GUARD_UI_APPS&&!(t.draft&~CAP_ALL)&&!(t.actual&~CAP_ALL));if(action==GUARD_UI_APPLY)assert(t.valid&&t.draft!=t.actual);if(action==GUARD_UI_READ)assert(!t.valid);if(i%500==0){struct wm_rect old=w.windows[TOOL_GUARD].rect;w.windows[TOOL_GUARD].rect=(struct wm_rect){0,0,160,100};assert(wm_render(&surf,&w,&d,(struct live_stats){0},1));w.windows[TOOL_GUARD].rect=old;}}
 assert(pixels[0]==0x12345678&&pixels[640*360+1]==0x12345678);assert(wm_render(&surf,&w,&d,(struct live_stats){0},1));FILE*f=fopen("build/awakens-guard.ppm","wb");assert(f);fprintf(f,"P6\n640 360\n255\n");for(unsigned i=1;i<=640*360;i++){uint32_t c=pixels[i];fputc(c>>16,f);fputc(c>>8,f);fputc(c,f);}assert(!fclose(f));
 wm_close(&w,TOOL_GUARD);assert(!g->editing&&!g->valid);puts("PASS: graphical policy editor, staged edits, key and mouse, stale shell/remount protection, 50000 UI transitions, framebuffer canaries");
}
int main(void){unit();FILE*f=fopen("build/vault.ext4","rb");assert(f&&fread(disk,1,sizeof disk,f)==sizeof disk);fclose(f);ramfs_init(&live_ramfs);assert(storage_init());
 cmd("guard-set /bin/hello 0","updated");cmd("guard-show /bin/hello","ceiling: 0");cmd("run-console /bin/hello","denied");cmd("run-data /bin/hello","denied");cmd("run-scratch /bin/hello","denied");cmd("run-resource /bin/hello ext:/welcome.txt","denied");cmd("run-resource-console /bin/hello ext:/welcome.txt","denied");cmd("run-pair /bin/hello","denied");cmd("run-group n:/bin/sdk-demo c:/bin/hello","denied");assert(!native_runs&&!group_runs);
 cmd("run /bin/hello","App result");assert(native_runs==1&&last_grants==0);cmd("guard-set /bin/hello 2","updated");cmd("run-resource /bin/hello ext:/welcome.txt","App result");assert(native_runs==2&&last_grants==CAP_DATA);cmd("run-resource-console /bin/hello ext:/welcome.txt","denied");assert(native_runs==2);
 cmd("guard-set /bin/gui-demo 0","updated");unsigned char frame[GUI_FRAME_BYTES];assert(storage_gui_demo(frame,sizeof frame)<0&&!gui_runs);cmd("guard-set /bin/gui-demo 32","updated");assert(storage_gui_demo(frame,sizeof frame)==0&&gui_runs==1);
 cmd("guard-set /bin/hello 16","Invalid");cmd("guard-set /bin/hello/ 1","Invalid");cmd("guard-set /bin/../bin/hello 1","Invalid");cmd("guard-show /bin/hello","ceiling: 2");cmd("umount","unmounted");assert(storage_init());cmd("run-console /bin/hello","denied");cmd("guard-reset /bin/hello","updated");cmd("run-console /bin/hello","App result");assert(native_runs==3&&last_grants==CAP_CONSOLE);cmd("run-group n:/bin/hello c:/bin/sdk-demo","Group results");assert(group_runs==1);
 struct audit_event e;int deny=0,change=0;for(unsigned i=0;audit_get(&kernel_audit,i,&e);i++){deny+=e.kind==5&&e.result==-1;change+=e.kind==6&&e.result==0;}assert(deny>=10&&change>=5);
 editor();
 int published=0;assert(storage_gui_start()&&!storage_gui_start());assert(storage_gui_poll(frame,sizeof frame,&published)==0&&published&&async_steps==1);
 cmd("guard-set /bin/gui-demo 0","updated");assert(storage_gui_poll(frame,sizeof frame,&published)<0&&!async_live&&async_cancel==1&&async_steps==1);assert(!storage_gui_start());
 cmd("guard-set /bin/gui-demo 32","updated");assert(storage_gui_start());cmd("umount","unmounted");assert(storage_gui_poll(frame,sizeof frame,&published)<0&&!async_live&&async_cancel==2);assert(storage_init());
 puts("PASS: asynchronous launch gate and cancellation on policy revision/remount before next slice");puts("PASS: 100000 capability ceiling cases; actual storage shell + EXT4, denied launches, no grant escalation, group atomic denial, GUI, remount, audit (mock CPU)");return 0;}
