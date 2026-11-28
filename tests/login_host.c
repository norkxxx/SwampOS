#include "efi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned printed,cleared;static int locked,secure;
status EFIAPI efi_main(handle,system_table*);
static status EFIAPI output(textout*t,const ch16*s){(void)t;printed++;const ch16*mark=S("SWAMPLOGIN_LOCKED");for(size_t i=0;s[i];i++){size_t j=0;while(mark[j]&&s[i+j]==mark[j])j++;if(!mark[j])locked=1;}return 0;}
static status EFIAPI attr(textout*t,size_t a){(void)t;assert(a==10);return 0;}
static status EFIAPI clear(textout*t){(void)t;cleared++;return 0;}
static status EFIAPI var(const ch16*n,const guid*g,uint32_t*a,size_t*z,void*v){(void)g;(void)a;assert(*z==1);*(uint8_t*)v=n[2]=='c'?secure:0;return 0;}
int main(void){textout out={.output=output,.attribute=attr,.clear=clear};runtime r={.get_variable=var};system_table s={.out=&out,.rt=&r};for(int i=0;i<1000;i++){secure=i&1;locked=0;assert(efi_main(0,&s)==SECURITY_VIOLATION&&locked);}assert(cleared==1000&&printed==3000);puts("PASS: SwampLogin 1000 mock invocations, locked in both firmware states, no boot services needed");}
