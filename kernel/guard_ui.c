#include "guard_ui.h"
#include "storage.h"
int guard_editor_execute(struct guard_editor *g,unsigned action){
 if(!g||!g->editing)return 0;
 uint64_t mask=0,rev=0;
 const char *path=tools_guard_path(g->app);
 if(action==GUARD_UI_READ){int ok=storage_guard_read(path,&mask,&rev);tools_guard_loaded(g,ok,mask,rev);return 1;}
 if(action!=GUARD_UI_APPLY)return 0;
 if(g->valid&&g->actual!=g->draft&&storage_guard_apply(path,g->draft,g->revision)){
  int ok=storage_guard_read(path,&mask,&rev);tools_guard_loaded(g,ok,mask,rev);if(ok)g->status=1;
 }else{g->status=2;g->valid=0;}
 return 1;
}
