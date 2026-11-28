#include "swamp.h"
/* Independent ELF program, no framebuffer pointer, no physical I/O. */
__attribute__((noreturn)) void _start(void){
 if(!(swamp_caps()&SWAMP_CAP_GUI))swamp_exit(13);
 unsigned char frame[SWAMP_GUI_BYTES];
 for(unsigned phase=0;phase<24;phase++){
 for(unsigned y=0;y<SWAMP_GUI_HEIGHT;y++)for(unsigned x=0;x<SWAMP_GUI_WIDTH;x++)frame[y*SWAMP_GUI_WIDTH+x]=(unsigned char)((x==0||x==15||y==0||y==7)?1:((x+y+phase+2)%5+1));
 if(swamp_gui_publish(frame)!=SWAMP_GUI_BYTES)swamp_exit(1);
 swamp_yield();}
 swamp_exit(0);
}
