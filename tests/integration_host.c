#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "session.h"
#include "policy.h"
#include "vm.h"
int main(void){struct app_session s;unsigned char source[256],out[256];for(unsigned j=0;j<256;j++)source[j]=(unsigned char)j;assert(session_selftest());
 for(unsigned i=1;i<=100000;i++){size_t len=i%257,off=i%269,n=1+i%256;assert(session_begin(&s,i,CAP_DATA,source,len));memset(out,0xa5,sizeof out);long expected=off>=len?0:(long)((len-off<n)?len-off:n);assert(session_read(&s,off,out,n)==expected);assert(!memcmp(out,source+(off<256?off:0),(size_t)expected));for(size_t j=(size_t)expected;j<sizeof out;j++)assert(out[j]==0xa5);assert(session_read(&s,UINT64_MAX,out,1)==0);session_clear(&s);for(size_t j=0;j<sizeof s;j++)assert(((unsigned char*)&s)[j]==0);assert(session_read(&s,0,out,1)==-1);assert(policy_check(0,SWP_SYS_DATA_READ,USER_STACK,1)==-1);assert(policy_check(CAP_DATA,SWP_SYS_DATA_READ,USER_CODE,1)==-14);assert(policy_check(CAP_DATA,SWP_SYS_DATA_READ,USER_TOP-n,n)==0);assert(policy_check(CAP_DATA,SWP_SYS_DATA_READ,USER_TOP-n+1,n)==-14);}
 assert(!session_begin(&s,0,CAP_DATA,source,1));assert(!session_begin(&s,1,CAP_CONSOLE,source,1));assert(!session_begin(&s,1,16,0,0));assert(!session_begin(&s,1,CAP_DATA,source,SESSION_DATA_MAX+1u));assert(!session_begin(&s,1,CAP_DATA,0,1));puts("PASS: 100000 session lifecycles, bounded resource reads, zeroization, default deny and writable range checks (UBSan)");}
