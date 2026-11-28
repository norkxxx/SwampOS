#include "live_menu.h"
#include <assert.h>
#include <stdio.h>
static unsigned stalls,reads,draws,mode,slogan;
static status EFIAPI output(textout*t,const ch16*c){(void)t;const char *needle="ITS YOUR SWAMP - ITS YOUR LIFE";for(unsigned i=0;c[i];i++){unsigned j=0;while(needle[j]&&c[i+j]==(ch16)needle[j])j++;if(!needle[j])slogan=1;}return 0;}
static status EFIAPI clear(textout*t){(void)t;draws++;return 0;}
static status EFIAPI stall(size_t n){assert(n==100000);stalls++;return 0;}
static status EFIAPI readkey(struct menu_input*i,struct menu_key*k){(void)i;reads++;if(mode==1&&reads==1){k->scan=2;return 0;}if(mode==1&&reads==2){k->unicode=13;return 0;}if(mode==2){k->unicode='3';return 0;}if(mode==3){k->scan=23;return 0;}return ERR(6);}
int main(void){textout out={.output=output,.clear=clear};boot bs={.stall=stall};struct menu_input in={.read=readkey};system_table s={.out=&out,.bs=&bs,.in=&in};assert(live_menu(&s)==0&&stalls==100&&draws==10&&slogan);mode=1;reads=stalls=0;assert(live_menu(&s)==1&&stalls==1);mode=2;assert(live_menu(&s)==2);mode=3;assert(live_menu(&s)==3);unsigned n=0;assert(!menu_select(&n,(struct menu_key){1,0})&&n==3);assert(!menu_select(&n,(struct menu_key){2,0})&&n==0);puts("PASS: UEFI menu mock, 10-second default, arrow/Enter, direct selection, Esc, slogan");}
