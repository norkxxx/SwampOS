#include "pmm.h"
#include "vm.h"
#include "policy.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static struct pmm p;
static unsigned char seen[FRAME_COUNT];
int main(void){
 const struct e820_entry m[]={{0,PHYS_LIMIT,1,1},{0x200000,8192,2,1},{0x300001,1,2,1},{0x400000,4096,2,0},{PHYS_LIMIT,4096,1,1}};
 assert(pmm_init(&p,m,5,0x180001));unsigned total=p.total;assert(total==FRAME_COUNT-385-3);
 for(unsigned i=0;i<total;i++){uint64_t a=pmm_take(&p);assert(a>=0x181000&&a<PHYS_LIMIT&&a%4096==0&&a!=0x200000&&a!=0x201000&&a!=0x300000);assert(!seen[a/4096]);seen[a/4096]=1;}
 assert(!pmm_take(&p)&&!p.free);assert(!pmm_release(&p,0x200000)&&!pmm_release(&p,1)&&!pmm_release(&p,PHYS_LIMIT));
 for(unsigned i=0;i<FRAME_COUNT;i++)if(seen[i]){assert(pmm_release(&p,(uint64_t)i*4096));assert(!pmm_release(&p,(uint64_t)i*4096));}
 assert(p.free==total);
 unsigned seed=2026;for(unsigned i=0;i<100000;i++){uint64_t a=pmm_take(&p);assert(a&&pmm_release(&p,a));seed=seed*1664525u+1013904223u;uint64_t x=(uint64_t)(seed%FRAME_COUNT)*4096;assert(!pmm_release(&p,x));}
 struct e820_entry bad[]={{UINT64_MAX-1,4,1,1}};assert(!pmm_init(&p,bad,1,0x100000));
 assert(user_readable(USER_CODE,1)&&user_readable(USER_STACK,256)&&user_readable(USER_TOP-1,1));assert(!user_readable(USER_TOP,1)&&!user_readable(USER_GUARD,1)&&!user_readable(USER_CODE+4095,2)&&!user_readable(UINT64_MAX,2)&&!user_readable(USER_CODE,0)&&!user_readable(USER_STACK,257));
 assert(policy_check(0,SWP_SYS_WRITE,USER_CODE,1)==-1&&policy_check(CAP_CONSOLE,SWP_SYS_WRITE,USER_CODE,1)==0);
 assert(policy_check(CAP_CONSOLE,SWP_SYS_WRITE,USER_GUARD,1)==-14&&policy_check(0,99,0,0)==-38);
 assert(!policy_check(0,SWP_SYS_EXIT,0,0)&&!policy_check(0,SWP_SYS_CAPS,0,0));
 puts("PASS: physical frame exhaustion/reuse, E820 overlap/reservation, 100000 churn iterations, user pointer boundaries and default-deny policy (UBSan)");return 0;
}
