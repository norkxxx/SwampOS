#include "lineedit.h"
#include "hardware.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static void type(struct lineedit*s,const char*t){for(;*t;t++)assert(line_key(s,*t)==LINE_ECHO);}
int main(void){struct keyboard k={0};assert(!keyboard_decode(&k,0xe0)&&keyboard_decode(&k,0x48)==KEY_UP);assert(!keyboard_decode(&k,0xe0)&&keyboard_decode(&k,0x50)==KEY_DOWN);assert(!keyboard_decode(&k,0xe0)&&!keyboard_decode(&k,0xc8));
 struct lineedit s;line_init(&s);assert(line_key(&s,KEY_UP)==LINE_NONE);type(&s,"help");assert(line_key(&s,10)==LINE_SUBMIT);line_finish(&s);type(&s,"version");line_finish(&s);type(&s,"draft");assert(line_key(&s,KEY_UP)==LINE_REPAINT&&!strcmp(s.line,"version"));assert(line_key(&s,KEY_UP)==LINE_REPAINT&&!strcmp(s.line,"help"));assert(line_key(&s,KEY_UP)==LINE_NONE);assert(line_key(&s,KEY_DOWN)==LINE_REPAINT&&!strcmp(s.line,"version"));assert(line_key(&s,KEY_DOWN)==LINE_REPAINT&&!strcmp(s.line,"draft"));assert(line_key(&s,8)==LINE_ERASE&&!strcmp(s.line,"draf"));line_finish(&s);
 for(unsigned i=0;i<32;i++){char b[32];snprintf(b,sizeof b,"command%u",i);type(&s,b);line_finish(&s);}assert(s.count==16);for(unsigned i=0;i<16;i++)assert(line_key(&s,KEY_UP)==LINE_REPAINT);assert(!strcmp(s.line,"command16")&&line_key(&s,KEY_UP)==LINE_NONE);line_finish(&s);
 for(unsigned i=0;i<200;i++){line_key(&s,'a');}assert(s.len==127&&s.line[127]==0);assert(line_key(&s,'b')==LINE_NONE);line_finish(&s);unsigned seed=1;for(unsigned i=0;i<100000;i++){seed=seed*1664525u+1013904223u;int key=(int)(seed%260);if(line_key(&s,key)==LINE_SUBMIT)line_finish(&s);assert(s.count<=16&&s.next<16&&s.cursor<=s.count&&s.len<128&&s.line[s.len]==0&&strlen(s.line)==s.len);}puts("PASS: PS/2 extended arrows, 16-entry history, draft restoration, line bounds, 100000 key events (UBSan)");}
