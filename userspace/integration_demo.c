#include "swamp.h"
__attribute__((noreturn,section(".text.entry"))) void _start(void){
 char buf[128];if(!swamp_id())swamp_exit(20);
 if(!(swamp_caps()&SWAMP_CAP_DATA)){swamp_exit(swamp_data_read(buf,1,0)==-1?13:21);}
 if(swamp_data_read((void*)0x40000000,1,0)!=-14)swamp_exit(22);
 long n=swamp_data_read(buf,sizeof buf,0);if(n<1||n>(long)sizeof buf)swamp_exit(23);
 if(swamp_data_read(buf,1,(unsigned long)n)!=0)swamp_exit(24);
 swamp_exit(swamp_write(buf,(size_t)n)==n?0:25);
}
