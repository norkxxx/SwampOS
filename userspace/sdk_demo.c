#include "swamp.h"
__attribute__((noreturn,section(".text.entry"))) void _start(void){
 static const char message[]="SWAMP_SDK_HELLO\n";
 if(!(swamp_caps()&SWAMP_CAP_CONSOLE))swamp_exit(13);
 swamp_exit(swamp_write(message,sizeof message-1)==sizeof message-1?0:14);
}
