#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "process.h"
#include "scratch.h"
#include "policy.h"
#include "vm.h"
static void zero(const void*p,size_t n){const unsigned char*b=p;while(n--)assert(*b++==0);}
int main(void){struct proc_group g;struct scratch s;char out[256],data[256];memset(data,0x6a,sizeof data);assert(proc_selftest()&&scratch_selftest());
 assert(!proc_init(&g,0,1)&&!proc_init(&g,5,1)&&!proc_init(&g,2,UINT64_MAX));
 for(unsigned round=0;round<10000;round++){
  assert(proc_init(&g,4,1+round*4ull)&&proc_next(&g)==0);
  for(unsigned j=0;j<4;j++)assert(proc_send(&g,1,data,64)==64);
  assert(proc_send(&g,1,data,1)==-11&&proc_send(&g,0,data,1)==-22&&proc_send(&g,4,data,1)==-22&&proc_send(&g,2,data,65)==-22);
  assert(proc_tick(&g)==0&&proc_tick(&g)==1);assert(proc_receive(&g,out,63)==-90);
  for(unsigned j=0;j<4;j++){memset(out,0,sizeof out);assert(proc_receive(&g,out,64)==64&&!memcmp(out,data,64));zero(out+64,192);}
  assert(proc_receive(&g,out,64)==-11);zero(g.slots[1].queue,sizeof g.slots[1].queue);
  assert(proc_finish(&g,7)==2&&g.slots[1].result==7);assert(proc_send(&g,1,data,1)==-32);
  assert(proc_finish(&g,8)==3&&proc_finish(&g,9)==0);while(g.slots[0].state!=PROC_DONE)proc_tick(&g);assert(g.slots[0].result==PROC_TIMEOUT&&g.slots[0].spent==PROC_BUDGET);proc_clear(&g);zero(&g,sizeof g);
  scratch_clear(&s);for(unsigned i=0;i<4;i++)assert(scratch_write(&s,i*256,data,256)==256);assert(s.size==1024&&scratch_write(&s,1024,data,1)==-22&&scratch_write(&s,UINT64_MAX,data,1)==-22);assert(scratch_read(&s,1023,out,256)==1&&out[0]==0x6a);assert(scratch_read(&s,1024,out,1)==0);scratch_clear(&s);zero(&s,sizeof s);
  assert(policy_check(0,SWP_SYS_SEND,USER_CODE,1)==-1&&policy_check(CAP_IPC,SWP_SYS_RECV,USER_CODE,1)==-14&&policy_check(CAP_IPC,SWP_SYS_RECV,USER_TOP-1,2)==-14&&policy_check(CAP_IPC,SWP_SYS_RECV,USER_STACK,64)==0);
  assert(policy_check_mapped(CAP_SCRATCH,SWP_SYS_SCRATCH_READ,USER_DATA,1,1)==0&&policy_check_mapped(CAP_SCRATCH,SWP_SYS_SCRATCH_READ,USER_DATA,1,0)==-14&&policy_check_mapped(CAP_SCRATCH,SWP_SYS_SCRATCH_READ,USER_DATA+4095,2,1)==-14);
  assert(policy_check(0,SWP_SYS_SCRATCH_WRITE,USER_CODE,1)==-1&&policy_check(CAP_SCRATCH,SWP_SYS_SCRATCH_WRITE,USER_CODE,1)==0&&policy_check(CAP_SCRATCH,SWP_SYS_SCRATCH_READ,USER_GUARD,1)==-14);
 }
 puts("PASS: 10000 process groups, queue exhaustion/FIFO, timeout/fairness, 10000 scratch lifecycles, default deny and zeroization (UBSan)");return 0;}
