#include "boot_hid.h"
#include "../../../kernel/lineedit.h"
static unsigned le16(const uint8_t*p){return (unsigned)p[0]|((unsigned)p[1]<<8);}
int usb_boot_config_parse(const uint8_t*p,size_t n,struct usb_boot_config*out){
 if(!out)return 0;
 *out=(struct usb_boot_config){0};
 if(!p||n<9||n>4096||p[0]!=9||p[1]!=2||le16(p+2)!=n||!p[4]||!p[5]||!(p[7]&0x80))return 0;
 struct usb_boot_config result={0};result.configuration=p[5];
 uint8_t seen[32]={0};unsigned count=0,expected=0,endpoints=0,mask=0,boot=0,active=0,have=0;
 struct usb_boot_interface current={0};
 for(size_t off=9;off<n;){
  if(n-off<2||p[off]<2||p[off]>n-off)return 0;
  const uint8_t*d=p+off;unsigned len=d[0],type=d[1];off+=len;
  if(type==4){
   if(len<9|| (active&&endpoints!=expected))return 0;
   if(active&&boot){if(!have||result.count==USB_BOOT_HID_MAX)return 0;result.interfaces[result.count++]=current;}
   active=1;expected=d[4];endpoints=mask=have=0;boot=0;
   unsigned id=d[2];if(!d[3]){if(seen[id/8]&(1u<<(id%8)))return 0;seen[id/8]|=(uint8_t)(1u<<(id%8));count++;}
   if(!d[3]&&d[5]==3&&d[6]==1&&(d[7]==1||d[7]==2)){boot=d[7];current=(struct usb_boot_interface){.number=d[2],.protocol=d[7]};}
  }else if(type==5){
   if(!active||len<7||++endpoints>expected)return 0;
   unsigned addr=d[2],ep=addr&15,bit=ep+((addr&128)?16:0);if(!ep||(addr&0x70)||(mask&(1u<<bit)))return 0;mask|=1u<<bit;
   unsigned packet=le16(d+4);
   if(boot&&(addr&128)&&(d[3]&3)==3){if(have||!d[6]||packet<(boot==1?8u:3u)||packet>64)return 0;current.endpoint=(uint8_t)addr;current.interval=d[6];current.packet_size=(uint16_t)packet;have=1;}
  }else if(type==2||type==1)return 0;
 }
 if(!active||endpoints!=expected||count!=p[4])return 0;
 if(boot){if(!have||result.count==USB_BOOT_HID_MAX)return 0;result.interfaces[result.count++]=current;}
 *out=result;return 1;
}
static int keycode(unsigned k,unsigned mod,unsigned caps){
 unsigned shift=!!(mod&0x22);
 if(k>=4&&k<=29)return ((shift!=!!caps)?'A':'a')+(int)k-4;
 if(k>=30&&k<=39)return (shift?"!@#$%^&*()":"1234567890")[k-30];
 switch(k){case 40:return '\n';case 41:return 27;case 42:return 8;case 43:return '\t';case 44:return ' ';case 45:return shift?'_':'-';case 46:return shift?'+':'=';case 47:return shift?'{':'[';case 48:return shift?'}':']';case 49:return shift?'|':'\\';case 51:return shift?':':';';case 52:return shift?'"':'\'';case 53:return shift?'~':'`';case 54:return shift?'<':',';case 55:return shift?'>':'.';case 56:return shift?'?':'/';case 81:return KEY_DOWN;case 82:return KEY_UP;default:return 0;}
}
int hid_keyboard_report(struct hid_keyboard*k,const uint8_t*p,size_t n,struct hid_keys*out){
 if(!out)return 0;
 *out=(struct hid_keys){0};if(!k||!p||n!=8||p[1])return 0;
 for(unsigned i=2;i<8;i++){if(p[i]&&p[i]<4)return 0;for(unsigned j=2;j<i;j++)if(p[i]&&p[i]==p[j])return 0;}
 for(unsigned i=2;i<8;i++){unsigned v=p[i];if(!v)continue;unsigned held=0;for(unsigned j=2;j<8;j++)held|=k->previous[j]==v;if(held)continue;if(v==57){k->caps^=1;continue;}int c=keycode(v,p[0],k->caps);if(c&&!(p[0]&0xdd))out->keys[out->count++]=c;}
 for(unsigned i=0;i<8;i++)k->previous[i]=p[i];
 return 1;
}
int hid_mouse_report(const uint8_t*p,size_t n,struct hid_mouse*out){if(!out)return 0;*out=(struct hid_mouse){0};if(!p||n!=3||(p[0]&~7u))return 0;out->buttons=p[0];out->dx=(int)p[1]-(p[1]&128?256:0);out->dy=(int)p[2]-(p[2]&128?256:0);return 1;}
int usb_hid_selftest(void){static const uint8_t config[]={9,2,25,0,1,1,0,0x80,50,9,4,0,0,1,3,1,1,0,7,5,0x81,3,8,0,10};struct usb_boot_config c;struct hid_keyboard k={0};struct hid_keys e;uint8_t key[8]={2,0,4};struct hid_mouse m;uint8_t mouse[3]={1,255,1};return usb_boot_config_parse(config,sizeof config,&c)&&c.count==1&&c.interfaces[0].endpoint==0x81&&hid_keyboard_report(&k,key,8,&e)&&e.count==1&&e.keys[0]=='A'&&hid_keyboard_report(&k,key,8,&e)&&!e.count&&hid_mouse_report(mouse,3,&m)&&m.dx==-1&&m.dy==1&&m.buttons==1;}
