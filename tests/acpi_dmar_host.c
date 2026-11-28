#include "../kernel/hardware.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned char mem[0x100000];
static int rd(void *ctx,uint64_t a,void *out,size_t n){(void)ctx;if(a>sizeof mem||n>sizeof mem-a)return 0;memcpy(out,mem+a,n);return 1;}
static void w16(unsigned char*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(unsigned char*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static void w64(unsigned char*p,uint64_t v){w32(p,(uint32_t)v);w32(p+4,(uint32_t)(v>>32));}
static void checksum(unsigned char*p,size_t n,size_t at){p[at]=0;uint8_t sum=0;for(size_t i=0;i<n;i++)sum=(uint8_t)(sum+p[i]);p[at]=(uint8_t)(0u-sum);}
static void scope(unsigned char*p,uint8_t type,uint8_t bus,uint8_t dev,uint8_t fn){p[0]=type;p[1]=8;p[2]=p[3]=p[4]=0;p[5]=bus;p[6]=dev;p[7]=fn;}
static size_t build(unsigned char *dmar,uint8_t drhd_flags,unsigned scopes){
 memset(dmar,0,4096);memcpy(dmar,"DMAR",4);w32(dmar+4,48+16+scopes*8);dmar[8]=1;dmar[36]=47;dmar[37]=1;
 unsigned off=48;w16(dmar+off,0);w16(dmar+off+2,(uint16_t)(16+scopes*8));dmar[off+4]=drhd_flags;w16(dmar+off+6,0);w64(dmar+off+8,0xfed90000ull);
 for(unsigned i=0;i<scopes;i++)scope(dmar+off+16+i*8,1,0,20,0);
 checksum(dmar,48+16+scopes*8,9);return 48+16+scopes*8;
}
static size_t build_multihop(unsigned char *dmar){
 memset(dmar,0,4096);memcpy(dmar,"DMAR",4);w32(dmar+4,74);dmar[8]=1;dmar[36]=47;dmar[37]=1;
 unsigned off=48;w16(dmar+off,0);w16(dmar+off+2,26);dmar[off+4]=0;w16(dmar+off+6,0);w64(dmar+off+8,0xfed90000ull);
 unsigned char *s=dmar+off+16;s[0]=1;s[1]=10;s[5]=0;s[6]=2;s[7]=0;s[8]=20;s[9]=0;
 checksum(dmar,74,9);return 74;
}
static size_t build_with_rmrr_atsr(unsigned char *dmar){
 size_t n=build(dmar,0,1);unsigned off=(unsigned)n;
 w16(dmar+off,1);w16(dmar+off+2,32);w16(dmar+off+6,0);w64(dmar+off+8,0xfed00000ull);w64(dmar+off+16,0xfed00fffull);scope(dmar+off+24,1,0,20,0);off+=32;
 w16(dmar+off,2);w16(dmar+off+2,16);dmar[off+4]=0;w16(dmar+off+6,0);scope(dmar+off+8,1,0,20,0);off+=16;
 w32(dmar+4,(uint32_t)off);checksum(dmar,off,9);return off;
}
static void setup_root(uint64_t dmar_addr,size_t dmar_len){
 unsigned char *xsdt=mem+0x1000;memset(xsdt,0,44);memcpy(xsdt,"XSDT",4);w32(xsdt+4,44);w64(xsdt+36,dmar_addr);checksum(xsdt,44,9);
 unsigned char *r=mem+0x80000;memset(r,0,36);memcpy(r,"RSD PTR ",8);r[15]=2;w32(r+20,36);w64(r+24,0x1000);checksum(r,20,8);checksum(r,36,32);
 (void)dmar_len;
}
int main(void){
 struct acpi_info info;unsigned char *dmar=mem+0x2000;size_t n=build(dmar,0,1);setup_root(0x2000,n);
 assert(acpi_probe(rd,0,0x80000,&info));
 assert(info.dmar==0x2000&&!info.dmar_bad&&!info.dmar_truncated);
 assert(info.dmar_host_width==47&&info.dmar_flags==1&&info.dmar_drhd_count==1&&info.dmar_scope_count==1);
 assert(info.drhd[0].reg_base==0xfed90000ull&&info.drhd[0].segment==0&&info.drhd[0].flags==0);
 assert(acpi_dmar_covers(&info,0,0,20,0));
 assert(!acpi_dmar_covers(&info,0,1,20,0));
 assert(!acpi_dmar_covers(&info,1,0,20,0));

 /* DMAR encodes device and function in two distinct bytes. */
 n=build(dmar,0,1);
 scope(dmar+64,1,0,2,3);checksum(dmar,n,9);
 assert(acpi_probe(rd,0,0x80000,&info)&&acpi_dmar_covers(&info,0,0,2,3));
 assert(!acpi_dmar_covers(&info,0,0,0,2));
 dmar[71]=8;checksum(dmar,n,9);
 assert(acpi_probe(rd,0,0x80000,&info)&&info.dmar_bad&&!acpi_dmar_covers(&info,0,0,2,3));
 dmar[70]=32;dmar[71]=0;checksum(dmar,n,9);
 assert(acpi_probe(rd,0,0x80000,&info)&&info.dmar_bad);

 n=build_with_rmrr_atsr(dmar);setup_root(0x2000,n);assert(acpi_probe(rd,0,0x80000,&info));
 assert(info.dmar_rmrr_count==1&&info.dmar_scope_count==3&&acpi_dmar_covers(&info,0,0,20,0));

 n=build_multihop(dmar);setup_root(0x2000,n);assert(acpi_probe(rd,0,0x80000,&info));
 assert(info.dmar_scope_count==1&&!acpi_dmar_covers(&info,0,0,20,0));

 n=build(dmar,1,1);setup_root(0x2000,n);assert(acpi_probe(rd,0,0x80000,&info));
 assert(acpi_dmar_covers(&info,0,200,31,7)); /* include-all DRHD */

 n=build(dmar,0,65);setup_root(0x2000,n);assert(acpi_probe(rd,0,0x80000,&info));
 assert(info.dmar_truncated&&!acpi_dmar_covers(&info,0,0,20,0));

 n=build(dmar,0,1);dmar[48+2]=3;checksum(dmar,n,9);setup_root(0x2000,n);
 assert(acpi_probe(rd,0,0x80000,&info));assert(info.dmar_bad&&!acpi_dmar_covers(&info,0,0,20,0));

 n=build(dmar,0,1);dmar[9]^=1;setup_root(0x2000,n);
 assert(acpi_probe(rd,0,0x80000,&info));assert(info.dmar_bad&&!acpi_dmar_covers(&info,0,0,20,0));

 n=build(dmar,0,1);dmar[48+2]=0;dmar[48+3]=0;checksum(dmar,n,9);setup_root(0x2000,n);
 assert(acpi_probe(rd,0,0x80000,&info));assert(info.dmar_bad);

 puts("ACPI_DMAR_HOST_OK: RSDP/XSDT/DMAR checksums, DRHD, include-all, direct endpoint scopes, segment/BDF matching, multi-hop non-claim, truncation and malformed lengths");
 return 0;
}
