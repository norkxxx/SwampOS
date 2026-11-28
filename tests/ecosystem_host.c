#include "surface.h"
#include "audit.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t pixels[642*360+2];
int main(void){
 struct surface s={pixels+1,642*360,640,360,642};pixels[0]=pixels[642*360+1]=0x12345678;
 for(unsigned y=0;y<360;y++)pixels[1+y*642+640]=pixels[1+y*642+641]=0x87654321;
 assert(swamp_desktop(&s));assert(pixels[1]==0x254532);assert(pixels[1+359*642+639]==0x254532);
 unsigned seed=0x9a82;for(unsigned i=0;i<20000;i++){seed=seed*1664525u+1013904223u;int x=(int)(seed%1000)-500;seed=seed*1664525u+1013904223u;int y=(int)(seed%800)-400;surface_rect(&s,x,y,1+(int)(seed%40),1+(int)(seed%30),seed);surface_text(&s,x,y,"SWAMPOS 010",1+seed%4,seed);}
 surface_rect(&s,INT_MIN,INT_MIN,INT_MAX,INT_MAX,1);surface_rect(&s,INT_MAX,INT_MAX,INT_MAX,INT_MAX,2);surface_text(&s,INT_MIN,INT_MAX,"TEST",8,0);assert(pixels[0]==0x12345678&&pixels[642*360+1]==0x12345678);for(unsigned y=0;y<360;y++)assert(pixels[1+y*642+640]==0x87654321&&pixels[1+y*642+641]==0x87654321);
 struct surface bad=s;bad.capacity=1;assert(!surface_valid(&bad)&&!swamp_desktop(&bad));surface_rect(&bad,0,0,100,100,0);assert(audit_selftest());struct audit_log log={0};struct audit_event e;for(unsigned i=0;i<100000;i++){audit_add(&log,4,(int)i,i);assert(audit_get(&log,log.count-1,&e)&&e.seq==i+1&&e.detail==i);if(i>=63)assert(audit_get(&log,0,&e)&&e.seq==i-62);}
 assert(!audit_get(&log,64,&e));log.next=UINT64_MAX;audit_add(&log,1,1,1);assert(log.next==UINT64_MAX);
 assert(swamp_desktop(&s));FILE*f=fopen("build/gui-preview.ppm","wb");assert(f);fprintf(f,"P6\n640 360\n255\n");for(unsigned y=0;y<360;y++)for(unsigned x=0;x<640;x++){uint32_t c=s.pixels[y*s.stride+x];unsigned char rgb[]={c>>16,c>>8,c};assert(fwrite(rgb,1,3,f)==3);}assert(!fclose(f));
 puts("PASS: 20,000 clipped drawing cases, stride/capacity canaries, 100,000 audit appends, rendered host preview (UBSan)");return 0;
}
