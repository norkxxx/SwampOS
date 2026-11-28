#include "hardware.h"
#include "lineedit.h"
int keyboard_decode(struct keyboard*k,uint8_t s){
 if(!k)return 0;
 if(k->extended>=2){if(--k->extended==1)k->extended=0;return 0;}
 if(k->extended==1){k->extended=0;if(s==0x48)return KEY_UP;if(s==0x50)return KEY_DOWN;return 0;}
 if(s==0xe0){k->extended=1;return 0;}if(s==0xe1){k->extended=6;return 0;}
 if(s==0x2a||s==0x36){k->shift|=s==0x2a?1:2;return 0;}if(s==0xaa||s==0xb6){k->shift&=s==0xaa?~1u:~2u;return 0;}
 if(s==0x3a){k->caps^=1;return 0;}if(s&0x80)return 0;
 static const char normal[128]={[1]=27,[2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',[12]='-',[13]='=',[14]=8,[15]='\t',[16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\n',[30]='a',[31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'',[41]='`',[43]='\\',[44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[57]=' '};
 static const char shifted[128]={[2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',[12]='_',[13]='+',[26]='{',[27]='}',[39]=':',[40]='"',[41]='~',[43]='|',[51]='<',[52]='>',[53]='?'};
 int ch=normal[s];if(ch>='a'&&ch<='z'){if(!!k->shift!=!!k->caps)ch-=32;}else if(k->shift&&shifted[s])ch=shifted[s];return ch;
}
#ifndef HARDWARE_HOST
static uint8_t in8(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
int keyboard_poll(void){static struct keyboard k;uint8_t s=in8(0x64);if(!(s&1)||s==255)return 0;uint8_t code=in8(0x60);if(s&0xe0)return 0;return keyboard_decode(&k,code);}
#endif
