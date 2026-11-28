#include "hardware.h"
static uint16_t le16(const uint8_t*p){return (uint16_t)p[0]|(uint16_t)p[1]<<8;}
static uint32_t le32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t le64(const uint8_t*p){return le32(p)|(uint64_t)le32(p+4)<<32;}
static int signature(const uint8_t*p,const char*s,unsigned n){for(unsigned i=0;i<n;i++)if(p[i]!=(uint8_t)s[i])return 0;return 1;}
static int checksum(phys_reader read,void*ctx,uint64_t a,uint32_t n){uint8_t tmp[64],sum=0;if(!n||n>65536||a>UINT64_MAX-n)return 0;while(n){unsigned len=n>sizeof tmp?sizeof tmp:n;if(!read(ctx,a,tmp,len))return 0;for(unsigned i=0;i<len;i++)sum+=tmp[i];a+=len;n-=len;}return sum==0;}
static uint32_t header(phys_reader read,void*c,uint64_t a,uint8_t*h){if(!a||a>UINT64_MAX-36||!read(c,a,h,36))return 0;uint32_t n=le32(h+4);return n>=36&&checksum(read,c,a,n)?n:0;}

static int dmar_scopes(phys_reader read,void*c,uint64_t table,uint32_t off,uint32_t end,uint8_t unit,struct acpi_info*x){
 while(off<end){
  uint8_t h[6];if(end-off<6||!read(c,table+off,h,6))return 0;
  unsigned len=h[1];if(len<6||len>end-off||((len-6)&1u)||h[0]<1||h[0]>5)return 0;
  unsigned paths=(len-6)/2;uint8_t dev=0xff,fn=0xff;
  for(unsigned k=0;k<paths;k++){uint8_t path[2];if(!read(c,table+off+6+k*2,path,2))return 0;dev=path[0];fn=path[1];if(dev>31||fn>7)return 0;}
  if(x->dmar_scope_count<ACPI_DMAR_MAX_SCOPES){
   x->scopes[x->dmar_scope_count]=(struct acpi_dmar_scope){h[0],h[5],dev,fn,(uint8_t)paths,unit};
  }else x->dmar_truncated=1;
  x->dmar_scope_count++;off+=len;
 }
 return off==end;
}
static int parse_dmar(phys_reader read,void*c,uint64_t a,uint32_t n,struct acpi_info*x){
 if(n<48)return 0;
 uint8_t fixed[12];
 if(!read(c,a+36,fixed,sizeof fixed))return 0;
 if(fixed[0]>63)return 0;
 x->dmar_host_width=fixed[0];x->dmar_flags=fixed[1];
 for(uint32_t off=48;off<n;){
  uint8_t h[4];if(n-off<4||!read(c,a+off,h,4))return 0;
  uint16_t type=le16(h),len=le16(h+2);if(len<4||len>n-off)return 0;
  if(type==0){
   uint8_t d[12];if(len<16||!read(c,a+off+4,d,sizeof d))return 0;
   uint16_t seg=le16(d+2);uint64_t reg=le64(d+4);
   if(!reg||(reg&4095u)||reg>=(1ull<<52))return 0;
   uint8_t unit=0xff;
   if(x->dmar_drhd_count<ACPI_DMAR_MAX_DRHD){unit=(uint8_t)x->dmar_drhd_count;x->drhd[x->dmar_drhd_count]=(struct acpi_dmar_unit){reg,seg,d[0],0};}
   else x->dmar_truncated=1;
   x->dmar_drhd_count++;
   if(!dmar_scopes(read,c,a,off+16,off+len,unit,x))return 0;
  }else if(type==1){
   uint8_t d[20];if(len<24||!read(c,a+off+4,d,sizeof d))return 0;
   uint16_t seg=le16(d+2);uint64_t base=le64(d+4),limit=le64(d+12);
   (void)seg;if(base>limit||limit>=(1ull<<52))return 0;
   x->dmar_rmrr_count++;
   if(!dmar_scopes(read,c,a,off+24,off+len,0xff,x))return 0;
  }else if(type==2||type==5){
   uint8_t d[4];if(len<8||!read(c,a+off+4,d,sizeof d))return 0;
   if(!dmar_scopes(read,c,a,off+8,off+len,0xff,x))return 0;
  }else if(type==3){
   if(len<20)return 0;
  }else if(type==4){
   if(len<9)return 0;
   uint8_t name[1];int term=0;
   for(uint32_t j=off+8;j<off+len;j++){
    if(!read(c,a+j,name,1))return 0;
    if(!name[0]){term=1;break;}
   }
   if(!term)return 0;
  }
  off+=len;
 }
 return 1;
}
int acpi_dmar_covers(const struct acpi_info*x,uint16_t segment,uint8_t bus,uint8_t device,uint8_t function){
 if(!x||!x->dmar||x->dmar_bad||x->dmar_truncated||device>31||function>7)return 0;
 unsigned units=x->dmar_drhd_count<ACPI_DMAR_MAX_DRHD?x->dmar_drhd_count:ACPI_DMAR_MAX_DRHD;
 for(unsigned i=0;i<units;i++){
  const struct acpi_dmar_unit*u=&x->drhd[i];if(u->segment!=segment)continue;
  if(u->flags&1u)return 1;
  unsigned scopes=x->dmar_scope_count<ACPI_DMAR_MAX_SCOPES?x->dmar_scope_count:ACPI_DMAR_MAX_SCOPES;
  for(unsigned j=0;j<scopes;j++){const struct acpi_dmar_scope*s=&x->scopes[j];if(s->unit_index==i&&s->type==1&&s->path_count==1&&s->start_bus==bus&&s->device==device&&s->function==function)return 1;}
 }
 return 0;
}
int acpi_probe(phys_reader read,void*c,uint64_t rsdp,struct acpi_info*out){
 if(!read||!out||rsdp>UINT64_MAX-36)return 0;
 struct acpi_info x={0};uint8_t r[36],h[36];if(!read(c,rsdp,r,20)||!signature(r,"RSD PTR ",8)||!checksum(read,c,rsdp,20))return 0;
 uint64_t root=le32(r+16);unsigned stride=4;
 if(r[15]>=2){if(!read(c,rsdp,r,36))return 0;uint32_t n=le32(r+20);if(n<36||!checksum(read,c,rsdp,n))return 0;if(le64(r+24)){root=le64(r+24);stride=8;}}
 uint32_t len=header(read,c,root,h);if(!len||!signature(h,stride==8?"XSDT":"RSDT",4)||(len-36)%stride||(len-36)/stride>256)return 0;
 x.rsdp=rsdp;x.root=root;
 for(uint32_t off=36;off<len;off+=stride){uint8_t p[8];if(!read(c,root+off,p,stride))return 0;uint64_t a=stride==8?le64(p):le32(p);for(unsigned z=0;z<sizeof h;z++)h[z]=0;uint32_t n=header(read,c,a,h);if(!n){if(signature(h,"DMAR",4)){x.dmar=a;x.dmar_bad=1;}continue;}
  if(signature(h,"MCFG",4)&&n>=44)x.mcfg=a;
  if(signature(h,"DMAR",4)){
   if(x.dmar){x.dmar_bad=1;continue;}
   x.dmar=a;if(!parse_dmar(read,c,a,n,&x))x.dmar_bad=1;continue;
  }
  if(!signature(h,"APIC",4))continue;
  if(x.madt||n<44||!read(c,a+36,p,8))return 0;
  x.madt=a;x.lapic=le32(p);
  for(uint32_t j=44;j<n;){uint8_t e[16];if(n-j<2||!read(c,a+j,e,2)||e[1]<2||e[1]>n-j)return 0;unsigned size=e[1];
   if(e[0]==0||e[0]==1||e[0]==2||e[0]==5||e[0]==9){unsigned need=e[0]==0?8:e[0]==1?12:e[0]==2?10:e[0]==5?12:16;if(size<need||!read(c,a+j,e,need))return 0;
    switch(e[0]){case 0:if(le32(e+4)&1)x.cpus++;break;case 1:x.ioapics++;break;case 2:x.overrides++;break;case 5:x.lapic=le64(e+4);break;case 9:if(le32(e+8)&1)x.cpus++;break;}}
   j+=size;
  }
 }
 *out=x;return 1;
}
