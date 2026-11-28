#include "swamp.h"
static volatile unsigned initialized=0x5a5a;
static char b[8];
__attribute__((noreturn,section(".text.entry"))) void _start(void){
 unsigned long caps=swamp_caps();
 if(initialized!=0x5a5a||b[0]!=0||b[7]!=0)swamp_exit(5);
 initialized=0x1234;
 if(!(caps&SWAMP_CAP_SCRATCH))swamp_exit(13);
 if(swamp_scratch_read(b,sizeof b,0)!=0||swamp_scratch_write("test",4,0)!=4||swamp_scratch_read(b,4,0)!=4||b[0]!='t'||b[3]!='t')swamp_exit(1);
 if(!(caps&SWAMP_CAP_IPC)){swamp_write("SCRATCH_OK\n",11);swamp_exit(0);}
 if(swamp_role()==0){if(swamp_send(1,"ping",4)!=4)swamp_exit(2);long n;do{n=swamp_recv(b,4);}while(n==-11);if(n!=4||b[0]!='p'||b[1]!='o')swamp_exit(3);swamp_write("IPC_PONG_OK\n",12);}
 else {long n;do{n=swamp_recv(b,4);}while(n==-11);if(n!=4||b[0]!='p'||b[1]!='i'||swamp_send(0,"pong",4)!=4)swamp_exit(4);swamp_yield();swamp_write("IPC_PING_OK\n",12);}
 swamp_exit(0);
}
