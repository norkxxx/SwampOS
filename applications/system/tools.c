#include "tools.h"
#include "../../kernel/policy.h"
const char *tools_guard_path(unsigned i){static const char *const paths[]={"/bin/hello","/bin/sdk-demo","/bin/integration-demo","/bin/overlord-demo","/bin/gui-demo","/bin/resource-scan"};return i<GUARD_UI_APPS?paths[i]:0;}
uint64_t tools_guard_bit(unsigned i){static const uint64_t bits[]={CAP_CONSOLE,CAP_DATA,CAP_IPC,CAP_SCRATCH,CAP_GUI};return i<5?bits[i]:0;}
void tools_guard_loaded(struct guard_editor*g,int ok,uint64_t mask,uint64_t rev){
 g->valid=ok&&!(mask&~CAP_ALL);g->actual=g->draft=g->valid?mask:0;g->revision=g->valid?rev:0;g->status=g->valid?0:3;
}
unsigned tools_guard_key(struct guard_editor*g,int k){
 if(k=='p'||k=='P'){g->editing^=1;g->valid=0;g->status=0;return g->editing?GUARD_UI_READ:0;}
 if(!g->editing)return 0;
 if(k=='j'||k=='k'){g->app=(g->app+(k=='j'?1:GUARD_UI_APPS-1))%GUARD_UI_APPS;g->valid=0;return GUARD_UI_READ;}
 if(k=='r'||k=='R'){g->valid=0;return GUARD_UI_READ;}
 if(!g->valid)return 0;
 if(k>='1'&&k<='5'){g->draft^=tools_guard_bit((unsigned)(k-'1'));g->status=0;}
 if(k=='0'){g->draft=0;g->status=0;}
 if(k=='a'||k=='A')return g->draft!=g->actual?GUARD_UI_APPLY:0;
 return 0;
}
static void zero(void *p,size_t n){unsigned char *b=p;while(n--)*b++=0;}
void tools_init(struct tools_state*t){zero(t,sizeof *t);t->ink=1;}
void tools_sync(struct tools_state*t,const struct ramfs*f,const struct audit_log*a){
 t->count=0;for(unsigned i=0;i<RAMFS_FILES;i++){size_t n;char name[RAMFS_NAME];if(!ramfs_entry(f,i,name,sizeof name,&n)){unsigned j=t->count++;for(unsigned k=0;k<RAMFS_NAME;k++)t->names[j][k]=name[k];t->sizes[j]=n;}}
 if(t->selected>=t->count)t->selected=t->count?t->count-1:0;
 zero(t->bytes,sizeof t->bytes);t->length=0;
 if(t->count)ramfs_read(f,t->names[t->selected],t->bytes,sizeof t->bytes,&t->length);
 unsigned max=t->length?(unsigned)((t->length-1)/8)*8:0;if(t->offset>max)t->offset=max;
 t->audit_count=0;for(unsigned i=0;i<AUDIT_CAPACITY;i++){struct audit_event e;if(!audit_get(a,i,&e))break;t->events[t->audit_count++]=e;}
 if(t->audit_offset>=t->audit_count)t->audit_offset=t->audit_count?t->audit_count-1:0;
}
void tools_paint_at(struct tools_state*t,unsigned x,unsigned y){if(x<PAINT_W&&y<PAINT_H){t->px=x;t->py=y;t->canvas[y*PAINT_W+x]=(unsigned char)t->ink;}}
void tools_key(struct tools_state*t,unsigned id,int k){
 if(id==TOOL_RAM){if(k=='j'&&t->selected+1<t->count){t->selected++;t->offset=0;}if(k=='k'&&t->selected){t->selected--;t->offset=0;}}
 else if(id==TOOL_HEX){if(k=='n'&&t->offset+8<t->length)t->offset+=8;if(k=='p'&&t->offset>=8)t->offset-=8;if(k=='j'&&t->selected+1<t->count){t->selected++;t->offset=0;}if(k=='k'&&t->selected){t->selected--;t->offset=0;}}
 else if(id==TOOL_PAINT){if(k>='0'&&k<='5')t->ink=(unsigned)(k-'0');if(k=='a'&&t->px)t->px--;if(k=='d'&&t->px+1<PAINT_W)t->px++;if(k=='w'&&t->py)t->py--;if(k=='s'&&t->py+1<PAINT_H)t->py++;if(k==' ')tools_paint_at(t,t->px,t->py);if(k=='c')zero(t->canvas,sizeof t->canvas);}
 else if(id==TOOL_SETTINGS){if(k>='1'&&k<='3')t->theme=(unsigned)(k-'1');}
 else if(id==TOOL_GUARD){if(k=='j'&&t->audit_offset+1<t->audit_count)t->audit_offset++;if(k=='k'&&t->audit_offset)t->audit_offset--;}
}
int tools_save(struct tools_state*t,struct ramfs*f){int r=ramfs_write(f,"paint.swpaint",t->canvas,sizeof t->canvas);t->status=r?3:1;return r;}
int tools_load(struct tools_state*t,const struct ramfs*f){unsigned char b[PAINT_W*PAINT_H];size_t n=0;int r=ramfs_read(f,"paint.swpaint",b,sizeof b,&n);if(!r&&n!=sizeof b)r=RAMFS_INVALID;for(unsigned i=0;!r&&i<sizeof b;i++)if(b[i]>5)r=RAMFS_INVALID;if(!r)for(unsigned i=0;i<sizeof b;i++)t->canvas[i]=b[i];t->status=r?3:2;return r;}
uint32_t tools_color(unsigned i){static const uint32_t c[]={0x0b1912,0x9bd6a7,0x5184aa,0xe6c690,0xd88b8b,0xe3f5e8};return c[i<6?i:0];}
