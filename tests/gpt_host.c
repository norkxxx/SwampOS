#define _FILE_OFFSET_BITS 64
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gpt.h"
struct source{FILE*f;uint64_t sector;unsigned byte;int mutate;};
static int read_disk(void*v,uint64_t l,unsigned n,void*p){struct source*s=v;if(l>0x7fffffffu||fseek(s->f,(long)(l*512),SEEK_SET)||fread(p,512,n,s->f)!=n)return 0;if(s->mutate&&s->sector>=l&&s->sector-l<n)((unsigned char*)p)[(s->sector-l)*512+s->byte]^=1;return 1;}
int main(void){FILE*f=fopen("build/SwampBoot-GPT.img","rb");assert(f);assert(!fseek(f,0,SEEK_END));long len=ftell(f);assert(len>0&&len%512==0);struct source s={.f=f};struct blockdev d={&s,(uint64_t)len/512,read_disk};struct gpt g;assert(gpt_read(&d,&g)&&g.count==5&&g.parts[0].first==2048);for(unsigned i=0;i<5;i++)assert(g.parts[i].last>=g.parts[i].first);
 uint64_t targets[]={1,2,3,33,d.sectors-1,d.sectors-33};unsigned accepted=0,rejected=0;s.mutate=1;for(unsigned i=0;i<6000;i++){s.sector=targets[i%6];s.byte=(i*313u+(i/6)*19u)%512;if(gpt_read(&d,&g)){/* Header padding is not CRC protected. */assert((s.sector==1||s.sector==d.sectors-1)&&s.byte>=92);accepted++;}else{assert(g.count==0);rejected++;}}
 s.sector=0;s.byte=510;assert(!gpt_read(&d,&g));fclose(f);printf("PASS: GPT parser, 6000 mutations (%u rejected, %u header-padding only)\n",rejected,accepted);}
