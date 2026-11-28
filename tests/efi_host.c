#include "efi.h"
#include "path.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef LAB_MODE
#define LAB_MODE 0
#endif
status EFIAPI efi_main(handle,system_table*);
static uint8_t paths[3][64];static handle handles[3]={(handle)1,(handle)2,(handle)3};static loaded_image im={.device=(handle)1};static int secure=1,setup=0,mode=0,loaded=0,started=0,unloaded=0;static unsigned num=2;
static status EFIAPI output(textout*t,const ch16*s){(void)t;(void)s;return 0;}
static status EFIAPI variable(const ch16*n,const guid*g,uint32_t*a,size_t*z,void*v){(void)g;(void)a;if(*z!=1)return INVALID_PARAMETER;*(uint8_t*)v=n[1]=='e'?secure:setup;/* SecureBoot and SetupMode share first two characters. */if(n[2]=='t')*(uint8_t*)v=setup;return 0;}
static status EFIAPI hp(handle h,const guid*g,void**out){if(g->a==loaded_guid.a){*out=&im;return 0;}if(g->a==dp_guid.a&&(uintptr_t)h>=1&&(uintptr_t)h<=3){*out=paths[(uintptr_t)h-1];return 0;}return NOT_FOUND;}
static status EFIAPI locate(uint32_t t,const guid*g,void*k,size_t*n,handle**out){(void)t;(void)g;(void)k;*n=num;*out=handles;return 0;}
static status EFIAPI freepool(void*p){(void)p;return 0;}
static status EFIAPI load(uint8_t policy,handle h,void*p,void*buf,size_t n,handle*out){(void)h;assert(!policy&&!buf&&!n);uint8_t*b=p;assert(!memcmp(b,paths[1],50)&&b[50]==4&&b[51]==4);loaded++;*out=(handle)4;return mode==1?SECURITY_VIOLATION:0;}
static status EFIAPI start(handle h,size_t*z,ch16**p){(void)z;(void)p;assert(h==(handle)4);started++;return mode==2?0:SECURITY_VIOLATION;}
static status EFIAPI unload(handle h){assert(h==(handle)4);unloaded++;return 0;}
static void makepath(uint8_t*p,int disk,int login){memset(p,0,64);p[0]=1;p[1]=1;p[2]=8;p[4]=disk;p[8]=4;p[9]=1;p[10]=42;p[48]=2;p[49]=2;uint8_t id[]={0x02,0x00,0x70,0x5a,0x4d,0x1b,0x07,0x47,0x91,0x10,0x53,0x57,0x41,0x4d,0x50,0x02};memcpy(p+32,id,16);if(!login)p[32]=1;p[50]=0x7f;p[51]=0xff;p[52]=4;}
int main(void){textout out={.output=output};runtime rt={.get_variable=variable};boot bs={.handle_protocol=hp,.free_pool=freepool,.locate_handle_buffer=locate,.load_image=load,.start_image=start,.unload_image=unload};system_table s={.out=&out,.rt=&rt,.bs=&bs};makepath(paths[0],1,0);makepath(paths[1],1,1);makepath(paths[2],1,1);
 for(int i=0;i<1000;i++){secure=1;setup=0;num=2;mode=0;loaded=started=unloaded=0;assert(efi_main((handle)9,&s)==SECURITY_VIOLATION&&loaded==1&&started==1&&unloaded==1);mode=1;assert(efi_main((handle)9,&s)==SECURITY_VIOLATION&&loaded==2&&started==1&&unloaded==2);mode=2;assert(efi_main((handle)9,&s)==SECURITY_VIOLATION&&loaded==3&&started==2);num=3;assert(efi_main((handle)9,&s)==SECURITY_VIOLATION&&loaded==3);num=2;paths[1][4]=2;assert(efi_main((handle)9,&s)==SECURITY_VIOLATION&&loaded==3);paths[1][4]=1;secure=0;int before=loaded;assert(efi_main((handle)9,&s)==SECURITY_VIOLATION&&loaded==before+LAB_MODE);secure=1;setup=1;before=loaded;assert(efi_main((handle)9,&s)==SECURITY_VIOLATION&&loaded==before+LAB_MODE);}
 struct disk_path p;uint8_t tmp[64];for(unsigned i=0;i<10000;i++){memcpy(tmp,paths[1],64);tmp[i%64]^=(uint8_t)(1+(i%255));(void)parse_disk_path(tmp,sizeof(tmp),&p);}assert(!parse_disk_path(paths[1],49,&p));printf("PASS: EFI manager mock (%s), 7000 policy/chainload cases, 10000 device-path mutations\n",LAB_MODE?"lab":"strict");}
