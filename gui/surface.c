#include "surface.h"
int surface_valid(const struct surface*s){return s&&s->pixels&&s->width&&s->height&&s->width<=4096&&s->height<=4096&&s->stride>=s->width&&s->stride<=8192&&s->capacity>=s->width && (s->height-1)<=(s->capacity-s->width)/s->stride;}
int surface_crop(const struct surface *parent, struct screen_rect *r, struct surface *out){
 if(!surface_valid(parent)||!r||!out||r->w<=0||r->h<=0)return 0;
 int64_t right=(int64_t)r->x+r->w, bottom=(int64_t)r->y+r->h;
 if(right>parent->width)right=parent->width;
 if(bottom>parent->height)bottom=parent->height;
 int x=r->x<0?0:r->x, y=r->y<0?0:r->y;
 if(right<=x||bottom<=y)return 0;
 *r=(struct screen_rect){x,y,(int)right-x,(int)bottom-y};
 size_t offset=(size_t)y*parent->stride+(unsigned)x;
 *out=(struct surface){parent->pixels+offset,parent->capacity-offset,(unsigned)r->w,(unsigned)r->h,parent->stride};
 return 1;
}

uint32_t surface_cursor_pixel(int dx,int dy,uint32_t under,unsigned format){
 if(dx<0||dy<0||dy>=13||dx>=(dy<8?dy+1:4))return under;
 if(dx==0)return format?0x06100a:0x0a1006;
 return format?0xe3f5e8:0xe8f5e3;
}
void surface_cursor(struct surface *s,int x,int y,unsigned format){
 struct screen_rect r={x,y,8,13};struct surface part;
 if(!surface_crop(s,&r,&part))return;
 for(int j=0;j<r.h;j++)for(int i=0;i<r.w;i++){
  uint32_t *p=part.pixels+(size_t)j*part.stride+i;
  *p=surface_cursor_pixel(r.x+i-x,r.y+j-y,*p,format);
 }
}
void surface_rect(struct surface*s,int x,int y,int w,int h,uint32_t c){if(!surface_valid(s)||w<=0||h<=0)return;int64_t x0=x,y0=y,x1=x0+w,y1=y0+h;if(x0<0)x0=0;if(y0<0)y0=0;if(x1>s->width)x1=s->width;if(y1>s->height)y1=s->height;for(int64_t j=y0;j<y1;j++)for(int64_t i=x0;i<x1;i++)s->pixels[(size_t)j*s->stride+(size_t)i]=c&0xffffffu;}
/* Original small 5x7 bitmap glyphs, uppercase ASCII and digits. */
static const uint8_t font[36][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
static const uint8_t punctuation[][7]={{0,0,0,31,0,0,0},{0,4,4,31,4,4,0},{1,2,2,4,8,8,16},{0,0,0,0,0,12,12},{0,12,12,0,12,12,0},{0,0,31,0,31,0,0},{0,21,14,31,14,21,0},{0,0,0,0,0,0,31},{0,0,12,12,0,12,12},{0,0,0,0,0,4,8},{4,4,4,4,4,0,4},{14,17,1,2,4,0,4},{2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{14,8,8,8,8,8,14},{14,2,2,2,2,2,14},{0,16,8,4,2,1,0},{0,4,2,1,2,4,0},{0,4,8,16,8,4,0}};
static const uint8_t*punct(unsigned k){const char*p="-+/.:=*_;,!?()[]\\><";for(unsigned i=0;p[i];i++)if((unsigned char)p[i]==k)return punctuation[i];return 0;}
void surface_text(struct surface*s,int x,int y,const char*t,unsigned scale,uint32_t c){if(!surface_valid(s)||!t||scale<1||scale>8)return;int64_t xx=x;for(unsigned i=0;i<256&&t[i];i++,xx+=6*scale){if(xx>=s->width)break;unsigned k=(unsigned char)t[i];if(k>='a'&&k<='z')k-=32;const uint8_t*g=k>='A'&&k<='Z'?font[k-'A']:k>='0'&&k<='9'?font[26+k-'0']:punct(k);if(!g||xx<-(int64_t)(5*scale)||y>=(int)s->height||(int64_t)y+7*scale<=0)continue;for(unsigned r=0;r<7;r++)for(unsigned col=0;col<5;col++)if(g[r]&(1u<<(4-col)))surface_rect(s,(int)(xx+col*scale),(int)((int64_t)y+r*scale),(int)scale,(int)scale,c);}}
int swamp_desktop(struct surface*s){if(!surface_valid(s)||s->width<640||s->height<360)return 0;surface_rect(s,0,0,(int)s->width,(int)s->height,0x0b1912);surface_rect(s,0,0,640,34,0x254532);surface_text(s,16,9,"SWAMPOS  ECOSYSTEM",2,0xc0e4c9);surface_text(s,500,12,"GUI LAB",1,0x9bd6a7);
 surface_rect(s,18,54,390,248,0x183323);surface_rect(s,18,54,390,32,0x345f43);surface_text(s,29,64,"SWAMP SHELL",2,0xc0e4c9);surface_text(s,33,105,"SECURITY HAS LAYERS",2,0x9bd6a7);surface_text(s,33,145,"NATIVE ELF SDK",2,0x9bd6a7);surface_text(s,33,180,"SIGNED SWP PACKAGES",2,0x9bd6a7);surface_text(s,33,221,"FIRMWARE GRAPHICS DEMO",1,0x8db49a);surface_text(s,33,247,"NO LOGIN OR DISK UNLOCK",1,0xf0cd91);
 surface_rect(s,424,54,198,116,0x183323);surface_text(s,439,70,"SWAMP GUARD",2,0xc0e4c9);surface_text(s,439,106,"LAB MODE",2,0xf0cd91);surface_text(s,439,143,"NOT A SECURITY UI",1,0x8db49a);
 surface_rect(s,424,184,198,118,0x183323);surface_text(s,439,201,"SWAMP PKG",2,0xc0e4c9);surface_text(s,439,236,"HOST STAGING",1,0x9bd6a7);surface_text(s,439,264,"NO NATIVE INSTALL",1,0x8db49a);
 surface_rect(s,0,324,640,36,0x254532);surface_text(s,16,335,"SWAMP MATRIX",2,0xc0e4c9);surface_text(s,440,338,"STATIC PREVIEW  010",1,0x9bd6a7);return 1;}
