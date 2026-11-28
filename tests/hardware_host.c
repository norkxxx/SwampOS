#include "hardware.h"
#include "net.h"
#include "handoff.h"
#include "vfs.h"
#include "device.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char ram[4096];
static int rd(void*c,uint64_t a,void*out,size_t n){(void)c;if(a>sizeof ram||n>sizeof ram-a)return 0;memcpy(out,ram+a,n);return 1;}
static void u32(unsigned char*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=v>>(i*8);}
static void fix(unsigned char*p,unsigned n,unsigned offset){p[offset]=0;unsigned char s=0;for(unsigned i=0;i<n;i++)s+=p[i];p[offset]=(unsigned char)-s;}
static void table(unsigned char*p,const char*sig,unsigned len){memcpy(p,sig,4);u32(p+4,len);p[8]=1;}
static void tables(void){memset(ram,0,sizeof ram);unsigned char*r=ram+64,*x=ram+256,*m=ram+512,*c=ram+768;memcpy(r,"RSD PTR ",8);r[15]=2;u32(r+20,36);u32(r+24,256);fix(r,20,8);fix(r,36,32);table(x,"XSDT",52);u32(x+36,512);u32(x+44,768);fix(x,52,9);table(m,"APIC",74);u32(m+36,0xfee00000);m[44]=0;m[45]=8;u32(m+48,1);m[52]=1;m[53]=12;m[64]=2;m[65]=10;fix(m,74,9);table(c,"MCFG",44);fix(c,44,9);}
static unsigned seed=0x5357414d;static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static int file_read(void*c,uint64_t lba,unsigned count,void*out){FILE*f=c;return !fseek(f,(long)(lba*512),SEEK_SET)&&fread(out,512,count,f)==count;}
static uint32_t config[64];
static uint32_t cfg(void*c,uint8_t b,uint8_t d,uint8_t f,uint8_t off){(void)c;(void)b;(void)d;(void)f;return config[off/4];}
int main(void){
 struct pci_function pf={.vendor=0x1af4,.product=0x1041};struct device_info di;
 config[0]=0x10411af4;config[1]=1u<<20;config[4]=0xc001;config[5]=0x1004;config[6]=1;config[0x34/4]=0x40;config[0x40/4]=0x5009;config[0x50/4]=0x11;
 assert(device_probe(cfg,0,&pf,&di)&&di.kind==DEV_VIRTIO&&di.virtio_vendor&&di.msix&&di.cap_count==2&&di.bars[0].address==0xc000&&di.bars[1].address==0x100001000ULL);
 config[0x50/4]=0x4011;assert(!device_probe(cfg,0,&pf,&di));config[0x50/4]=0x11;uint32_t saveconfig[64];memcpy(saveconfig,config,sizeof config);
 for(unsigned j=0;j<10000;j++){memcpy(config,saveconfig,sizeof config);config[rnd()%64]^=rnd();(void)device_probe(cfg,0,&pf,&di);}puts("PASS: PCI BAR/capability inventory, cycles, 10000 config mutations");

 struct acpi_info a;tables();assert(acpi_probe(rd,0,64,&a)&&a.cpus==1&&a.ioapics==1&&a.overrides==1&&a.mcfg==768&&a.lapic==0xfee00000);ram[64]^=1;assert(!acpi_probe(rd,0,64,&a));tables();ram[512+45]=1;fix(ram+512,74,9);assert(!acpi_probe(rd,0,64,&a));tables();unsigned char original[4096];memcpy(original,ram,sizeof ram);
 for(unsigned i=0;i<20000;i++){memcpy(ram,original,sizeof ram);unsigned x=rnd()%sizeof ram;ram[x]^=(uint8_t)(1+(rnd()%255));(void)acpi_probe(rd,0,64,&a);}puts("PASS: ACPI XSDT/MADT/MCFG, malformed length and 20000 mutations");
 struct keyboard k={0};assert(keyboard_decode(&k,30)=='a');assert(!keyboard_decode(&k,0x2a)&&keyboard_decode(&k,30)=='A'&&!keyboard_decode(&k,0xaa));assert(!keyboard_decode(&k,0x3a)&&keyboard_decode(&k,30)=='A');assert(!keyboard_decode(&k,0xe0)&&!keyboard_decode(&k,0x2a)&&keyboard_decode(&k,30)=='A');assert(!keyboard_decode(&k,0xe1));for(unsigned i=0;i<5;i++)assert(!keyboard_decode(&k,30));assert(keyboard_decode(&k,30)=='A');for(unsigned i=0;i<100000;i++)assert(keyboard_decode(&k,(uint8_t)rnd())>=0);puts("PASS: keyboard modifiers, extended sequences and 100000 scan bytes");
 struct swamp_bootinfo b={0};struct efi_mem map[4]={{.type=7,.physical=0,.pages=2048},{.type=2,.physical=0x800000,.pages=256},{.type=7,.physical=0x900000,.pages=256},{.type=7,.physical=0xa00000,.pages=512}};
 assert(handoff_map(&b,map,sizeof map,sizeof map[0])&&b.count==2&&b.map[0].base==0x400000&&b.map[0].length==0x400000&&b.map[1].length==0x300000);assert(!handoff_map(&b,map,sizeof map-1,sizeof map[0]));map[3].physical=0x900000;assert(!handoff_map(&b,map,sizeof map,sizeof map[0]));map[3].physical=0xa00000;
 for(unsigned i=0;i<20000;i++){struct efi_mem m[4];memcpy(m,map,sizeof m);((unsigned char*)m)[rnd()%sizeof m]^=(uint8_t)(1+rnd()%255);if(handoff_map(&b,m,sizeof m,sizeof m[0]))for(unsigned j=0;j<b.count;j++)assert(b.map[j].base>=SWAMP_UEFI_RESERVE&&b.map[j].length<=PHYS_LIMIT-b.map[j].base);}puts("PASS: UEFI RAM conversion, range limits and 20000 descriptor mutations");
 assert(net_selftest());struct net_endpoint ep={{2,0,0,0,0,1},{192,0,2,1}};struct net_stats st={0};unsigned char in[NET_MAX_FRAME],out[NET_MAX_FRAME+2];for(unsigned i=0;i<50000;i++){size_t n=rnd()%(sizeof in+1),cap=rnd()%(NET_MAX_FRAME+1);for(size_t j=0;j<n;j++)in[j]=(unsigned char)(rnd()>>24);out[0]=out[cap+1]=0x5a;size_t len=net_receive(&ep,in,n,out+1,cap,&st);assert(len<=cap&&out[0]==0x5a&&out[cap+1]==0x5a);}puts("PASS: ARP/ICMP/UDP positive and negative fixtures; 50000 random frames with output bounds");
 FILE*f=fopen("build/StorageLab-GPT.img","rb");assert(f&&!fseek(f,0,SEEK_END));struct blockdev d={f,(uint64_t)ftell(f)/512,file_read};struct gpt g;struct vfs v;struct fs_inode i;unsigned char buf[64];assert(gpt_read(&d,&g)&&g.count==1&&vfs_mount_partition(&v,&d,&g,0));assert(ext4_lookup(&v.fs,"/welcome.txt",&i)&&i.size<sizeof buf&&ext4_read(&v.fs,&i,0,buf,i.size)&&!memcmp(buf,"SwampOS Vault",13));assert(!block_read(&v.fs.dev,v.fs.dev.sectors,1,buf));g.parts[0].type[0]^=1;assert(!vfs_mount_partition(&v,&d,&g,0)&&!v.mounted);assert(!vfs_mount_partition(&v,&d,&g,1));struct block_slice s;struct blockdev child;assert(!block_slice_init(&s,&child,&d,UINT64_MAX,1)&&!block_slice_init(&s,&child,&d,1,UINT64_MAX));fclose(f);puts("PASS: real GPT -> partition view -> EXT4 mount; bounds and non-Linux GUID rejection");
}
