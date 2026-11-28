#pragma once
#include "efi.h"
/* UEFI Simple Text Input protocol. */
struct menu_key {uint16_t scan;ch16 unicode;};
struct menu_input {status(EFIAPI *reset)(struct menu_input*,uint8_t);status(EFIAPI *read)(struct menu_input*,struct menu_key*);void *wait;};
static inline int menu_select(unsigned *selected,struct menu_key k){
 if(k.unicode>='1'&&k.unicode<='4'){*selected=(unsigned)(k.unicode-'1');return 1;}
 if(k.unicode==13)return 1;
 if(k.scan==23){*selected=3;return 1;}
 if(k.scan==1)*selected=(*selected+3)%4;
 if(k.scan==2)*selected=(*selected+1)%4;
 return 0;
}
static inline void menu_draw(system_table*s,unsigned selected,unsigned seconds){
 if(s->out->attribute)s->out->attribute(s->out,0x0a);
 if(s->out->clear)s->out->clear(s->out);
 say(s,S("\r\n"
 "              _______________\r\n"
 "         ___ /               \\ ___\r\n"
 "        (___)  _          _   (___)\r\n"
 "            | (o)        (o) |\r\n"
 "            |       __       |\r\n"
 "            |    ._______.   |\r\n"
 "             \\     -----    /\r\n"
 "              '------------'\r\n"
 "\r\n"
 "         SWAMPOS / AWAKENS DEV 38 R1\r\n"
 "       ITS YOUR SWAMP - ITS YOUR LIFE\r\n\r\n"));
 static const ch16 *const opts[]={S("1. Live Desktop (keyboard + PS/2 mouse)"),S("2. Classic Desktop (keyboard only)"),S("3. Serial recovery console (COM1)"),S("4. Return to firmware")};
 for(unsigned i=0;i<4;i++){say(s,i==selected?S("     > "):S("       "));say(s,opts[i]);say(s,S("\r\n"));}
 say(s,S("\r\n   Up/Down + Enter | 1-4 | Esc: firmware\r\n"));
 ch16 timer[]={ ' ', ' ', ' ', 'A','u','t','o','b','o','o','t',' ', 'i','n',' ',(ch16)('0'+seconds/10),(ch16)('0'+seconds%10),' ','s','e','c','o','n','d','s','\r','\n',0};say(s,timer);
 say(s,S("   UNSIGNED / LIVE ROOT IN RAM / DISK WRITES OFF BY DEFAULT\r\n"));
}
static inline unsigned live_menu(system_table*s){
 unsigned selected=0,previous=4,last=11;struct menu_input*in=s->in;
 for(unsigned tick=0;tick<100;tick++){
  unsigned seconds=(100-tick+9)/10;
  if(previous!=selected||last!=seconds){menu_draw(s,selected,seconds);previous=selected;last=seconds;}
  if(in&&in->read){struct menu_key k={0};if(!in->read(in,&k)&&menu_select(&selected,k))return selected;}
  if(!s->bs->stall||s->bs->stall(100000))break;
 }
 return selected;
}
