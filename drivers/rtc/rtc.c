#include "rtc.h"
struct raw_time {uint8_t sec,min,hour,day,month,year,mode;};
static int sample(rtc_read_reg read,void*c,struct raw_time*r){
 if(read(c,0x0a)&128)return 0;
 r->sec=read(c,0);r->min=read(c,2);r->hour=read(c,4);r->day=read(c,7);r->month=read(c,8);r->year=read(c,9);r->mode=read(c,0x0b);
 return !(read(c,0x0a)&128)&&!(r->mode&128);
}
static int same(const struct raw_time*a,const struct raw_time*b){return a->sec==b->sec&&a->min==b->min&&a->hour==b->hour&&a->day==b->day&&a->month==b->month&&a->year==b->year&&a->mode==b->mode;}
static int value(unsigned v,int binary){if(binary)return (int)v;if((v&15)>9||(v>>4)>9)return -1;return (int)((v>>4)*10+(v&15));}
int rtc_read_time(rtc_read_reg read,void*c,struct rtc_time*out){
 if(!out)return 0;
 *out=(struct rtc_time){0};if(!read||!(read(c,0x0d)&128))return 0;
 struct raw_time a,b;unsigned ok=0;for(unsigned i=0;i<2048;i++)if(sample(read,c,&a)&&sample(read,c,&b)&&same(&a,&b)){ok=1;break;}
 if(!ok||!(read(c,0x0d)&128))return 0;
 int bin=!!(a.mode&4),h24=!!(a.mode&2),pm=!!(a.hour&128);
 int s=value(a.sec,bin),m=value(a.min,bin),h=value(a.hour&127,bin),d=value(a.day,bin),mo=value(a.month,bin),y=value(a.year,bin);
 if(s<0||s>59||m<0||m>59||h<0||d<1||mo<1||mo>12||y<0||y>99)return 0;
 if(h24){if(pm||h>23)return 0;}else{if(h<1||h>12)return 0;h=h%12+(pm?12:0);}
 static const unsigned days[]={31,29,31,30,31,30,31,31,30,31,30,31};if((unsigned)d>days[mo-1])return 0;if(mo==2&&d==29&&y%4)return 0;
 *out=(struct rtc_time){(unsigned)y,(unsigned)mo,(unsigned)d,(unsigned)h,(unsigned)m,(unsigned)s};return 1;
}
#ifndef RTC_HOST
/* Single CPU, serialize against interrupts. CMOS index writes only, never data. */
static uint8_t read_cmos(void*c,uint8_t reg){(void)c;uint8_t v;__asm__ volatile("outb %0,$0x70; inb $0x71,%0":"+a"(reg)::"memory");v=reg;return v;}
int rtc_hardware_time(struct rtc_time*out){unsigned long flags;__asm__ volatile("pushfq; pop %0; cli":"=r"(flags)::"memory");int ok=rtc_read_time(read_cmos,0,out);if(flags&(1ul<<9))__asm__ volatile("sti":::"memory");return ok;}
#endif
