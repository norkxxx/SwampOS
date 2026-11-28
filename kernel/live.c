#include "live.h"
static void zero(char*p,size_t n){while(n--)*p++=0;}
int live_fb_validate(uint64_t base,uint64_t size,unsigned w,unsigned h,unsigned stride,unsigned format,uint64_t *bytes){
 if(!bytes||base<128ull*1024*1024||base%4096||base>= (1ull<<52)||w<640||h<360||w>4096||h>4096||stride<w||stride>8192||format>1)return 0;
 uint64_t n=(uint64_t)stride*h*4,rounded=(n+4095)&~4095ull;
 if(n>size||rounded>LIVE_FB_LIMIT||rounded>(1ull<<52)-base)return 0;
 *bytes=rounded;return 1;
}
static int parse(const char **p,int64_t *v){int neg=0;while(**p==' ')++*p;if(**p=='-'){neg=1;++*p;}int64_t n=0;unsigned count=0;while(**p>='0'&&**p<='9'){n=n*10+(*(*p)++-'0');if(++count>9)return 0;}if(!count)return 0;*v=neg?-n:n;while(**p==' ')++*p;return 1;}
int live_calculate(const char *e,int64_t *out){int64_t a,b;if(!e||!out||!parse(&e,&a))return 0;char op=*e;if(!op){*out=a;return 1;}e++;if(!parse(&e,&b)||*e)return 0;switch(op){case '+':*out=a+b;break;case '-':*out=a-b;break;case '*':*out=a*b;break;case '/':if(!b)return 0;*out=a/b;break;default:return 0;}return 1;}
static void decimal(char *s,int64_t v){char b[24];unsigned n=0;uint64_t u;if(v<0){*s++='-';u=(uint64_t)(-(v+1))+1;}else u=(uint64_t)v;do{b[n++]=(char)('0'+u%10);u/=10;}while(u);while(n)*s++=b[--n];*s=0;}
void live_init(struct live_state*s){zero((char*)s,sizeof *s);s->dirty=1;}
void live_clear(struct live_state*s){zero((char*)s->terminal,sizeof s->terminal);s->row=s->col=0;s->dirty=1;}
static void line(struct live_state*s){s->col=0;if(s->row+1<LIVE_ROWS){s->row++;return;}for(unsigned i=1;i<LIVE_ROWS;i++)for(unsigned j=0;j<=LIVE_COLS;j++)s->terminal[i-1][j]=s->terminal[i][j];zero(s->terminal[LIVE_ROWS-1],LIVE_COLS+1);}
void live_put(struct live_state*s,char c){if(c=='\r')return;s->dirty=1;if(c=='\n'){line(s);return;}if(c=='\b'){if(s->col)s->terminal[s->row][--s->col]=0;return;}if(c=='\t')c=' ';if((unsigned char)c<32||(unsigned char)c>126)c='.';if(s->col==LIVE_COLS)line(s);s->terminal[s->row][s->col++]=c;s->terminal[s->row][s->col]=0;}
int live_key(struct live_state*s,int k){if(k==9){s->view=(s->view+1)%5;s->dirty=1;return 1;}if(k==27){s->view=0;s->dirty=1;return 1;}if(!s->view)return 0;s->dirty=1;
 if(s->view==1){if(k==8||k==127){if(s->note_len)s->note[--s->note_len]=0;}else if((k>=32&&k<=126)||k==10||k==13){if(s->note_len<512){s->note[s->note_len++]=k==13?'\n':(char)k;s->note[s->note_len]=0;}}}
 else if(s->view==2){if(k==8||k==127){if(s->expr_len)s->expr[--s->expr_len]=0;}else if(k==10||k==13){int64_t v;if(live_calculate(s->expr,&v))decimal(s->answer,v);else {zero(s->answer,sizeof s->answer);s->answer[0]='E';s->answer[1]='R';s->answer[2]='R';}s->expr_len=0;s->expr[0]=0;}else if(k>=32&&k<=126&&s->expr_len<sizeof s->expr-1){s->expr[s->expr_len++]=(char)k;s->expr[s->expr_len]=0;}}
 else if(s->view==4&&k>='1'&&k<='3'){s->file=(unsigned)(k-'1');}
 return 1;}
void live_mouse(struct live_state*s,unsigned w,unsigned h,int dx,int dy,unsigned buttons){
 if(!s||w<640||h<360||w>4096||h>4096)return;
 if(!s->mouse_visible){s->mouse_x=(int)w/2;s->mouse_y=(int)h/2;s->mouse_visible=1;}
 int64_t x=(int64_t)s->mouse_x+dx,y=(int64_t)s->mouse_y+dy;
 s->mouse_x=x<0?0:x>=w?(int)w-1:(int)x;s->mouse_y=y<0?0:y>=h?(int)h-1:(int)y;
 unsigned old_view=s->view,old_file=s->file;
 if((buttons&1)&&!(s->mouse_buttons&1)){
  int z=w>=1280&&h>=720?2:1,ox=((int)w-640*z)/2,oy=((int)h-360*z)/2;
  int px=s->mouse_x-ox,py=s->mouse_y-oy;
  if(px>=0&&py>=45*z&&py<70*z){px/=z;for(unsigned i=0;i<5;i++)if(px>=12+(int)i*124&&px<132+(int)i*124)s->view=i;}
  else if(s->view==4&&px>=20*z&&px<260*z&&py>=108*z&&py<126*z){px/=z;s->file=px<116?0:px<212?1:2;}
 }
 s->mouse_buttons=buttons&7;
 if(old_view!=s->view||old_file!=s->file)s->dirty=1;
}
static uint32_t color(uint32_t c,unsigned f){return f==0?((c&255)<<16)|(c&0xff00)|((c>>16)&255):c;}
#define RECT(x,y,w,h,c) surface_rect(s,ox+(x)*z,oy+(y)*z,(w)*z,(h)*z,color(c,format))
#define TEXT(x,y,t,k,c) surface_text(s,ox+(x)*z,oy+(y)*z,t,(k)*z,color(c,format))
static void wrap(struct surface*s,int ox,int oy,int z,unsigned format,const char*t){char b[77];unsigned i=0,row=0;while(*t&&row<16){char c=*t++;if(c=='\n'||i==76){b[i]=0;TEXT(20,86+row*12,b,1,0xc0e4c9);row++;i=0;if(c=='\n')continue;if(row==16)break;}b[i++]=c;}if(i&&row<16){b[i]=0;TEXT(20,86+row*12,b,1,0xc0e4c9);}}
static void paint_scene(struct surface*s,const struct live_state*d,struct live_stats st,unsigned format,int ox,int oy,int z){surface_rect(s,0,0,(int)s->width,(int)s->height,color(0x07110c,format));RECT(0,0,640,360,0x102218);RECT(0,0,640,40,0x244632);TEXT(15,9,"SWAMPOS / FARKUAD",2,0xbce8c8);TEXT(465,14,"LIVE / RAM ONLY",1,0xf0cd91);
 static const char *names[]={"TERMINAL","NOTES","CALCULATOR","MONITOR","FILES"};for(unsigned i=0;i<5;i++){RECT(12+(int)i*124,45,120,25,i==d->view?0x467c53:0x203b2a);TEXT(20+(int)i*124,54,names[i],1,0xc0e4c9);}RECT(10,76,620,242,0x0b1912);
 if(d->view==0){for(unsigned i=0;i<LIVE_ROWS;i++)TEXT(20,83+(int)i*12,d->terminal[i],1,0x9bd6a7);RECT(20+(int)d->col*6,91+(int)d->row*12,5,1,0xbce8c8);}
 if(d->view==1){wrap(s,ox,oy,z,format,d->note);TEXT(20,296,"VOLATILE NOTES / 512 BYTES / BACKSPACE TO EDIT",1,0xf0cd91);}
 if(d->view==2){TEXT(20,88,"INTEGER CALCULATOR",2,0xbce8c8);TEXT(20,120,"ENTER TWO INTEGERS WITH + - * / THEN ENTER",1,0x9bd6a7);TEXT(20,150,d->expr,2,0xc0e4c9);TEXT(20,190,"RESULT",1,0x9bd6a7);TEXT(20,210,d->answer,2,0xc0e4c9);TEXT(20,285,"UP TO NINE DIGITS PER OPERAND / INTEGER DIVISION",1,0xf0cd91);}
 if(d->view==3){char b[32];TEXT(20,90,"KERNEL MONITOR",2,0xbce8c8);TEXT(20,130,"TIMER TICKS",1,0x9bd6a7);decimal(b,(int64_t)(st.ticks&0x7fffffffffffffffull));TEXT(220,130,b,1,0xc0e4c9);TEXT(20,155,"FREE 4K FRAMES",1,0x9bd6a7);decimal(b,st.free);TEXT(220,155,b,1,0xc0e4c9);TEXT(20,180,"TOTAL 4K FRAMES",1,0x9bd6a7);decimal(b,st.total);TEXT(220,180,b,1,0xc0e4c9);TEXT(20,235,"NETWORK OFF / NO DISK WRITES",1,0xf0cd91);}
 if(d->view==4){TEXT(20,90,"LIVE EXT4 / SELECT 1 2 3",1,0x9bd6a7);TEXT(20,112,"1 WELCOME.TXT   2 PATTERN.BIN   3 /BIN",1,0xc0e4c9);char b[77];const char*p=d->files_text;unsigned row=0;while(*p&&row<12){unsigned n=0;while(*p&&*p!='\n'&&n<76)b[n++]=*p++;b[n]=0;TEXT(20,144+row*13,b,1,0xc0e4c9);row++;if(*p=='\n')p++;}}
 RECT(0,324,640,36,0x244632);TEXT(15,337,"TAB / CLICK: SWITCH / ESC: TERMINAL",1,0xc0e4c9);TEXT(427,337,"EXPERIMENTAL LIVE",1,0xf0cd91);
}

int live_paint_region(struct surface *scene,struct screen_rect r,const struct live_state *d,struct live_stats st,unsigned f){
 struct surface part;
 if(!d||d->view>=5||f>1||!surface_crop(scene,&r,&part))return 0;
 paint_scene(&part,d,st,f,-r.x,-r.y,1);
 return 1;
}
int live_render(struct surface *s,const struct live_state *d,struct live_stats st,unsigned f){
 if(!surface_valid(s)||s->width<640||s->height<360||!d||d->view>=5||f>1)return 0;
 int z=s->width>=1280&&s->height>=720?2:1;
 paint_scene(s,d,st,f,((int)s->width-640*z)/2,((int)s->height-360*z)/2,z);
 if(d->mouse_visible)surface_cursor(s,d->mouse_x,d->mouse_y,f);
 return 1;
}
