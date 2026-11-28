#include "bootinfo.h"
#include "pci.h"
#include "fwcfg.h"
static uint8_t cfg_byte(void){uint8_t b;__asm__ volatile("inb %1,%0":"=a"(b):"Nd"((uint16_t)0x511));return b;}
static void cfg_select(uint16_t id){__asm__ volatile("outw %0,%1"::"a"(id),"Nd"((uint16_t)0x510));}
/* A guest operator opts in to whole-unit ownership with a QEMU fw_cfg file. */
int q35_live_option(const char *name){
 if(!boot_uefi||pci_config_read(0,0,0,0,0)!=0x29c08086)return 0;
 const struct swamp_bootinfo *b=(const void*)SWAMP_UEFI_INFO;
 if(b->reserved!=1||b->live_option!=0)return 0;
 cfg_select(0);
 const char *sig="QEMU";
 for(unsigned j=0;j<4;j++)if(cfg_byte()!=(uint8_t)sig[j])return 0;
 cfg_select(0x19);
 uint32_t count=0;
 for(unsigned j=0;j<4;j++)count=(count<<8)|cfg_byte();
 if(count>512)return 0;
 unsigned length=0;while(length<56 && name[length])length++;
 if(length==56)return 0;
 for(unsigned i=0;i<count;i++){
  uint8_t e[64];for(unsigned j=0;j<64;j++)e[j]=cfg_byte();
  unsigned j=0;while(j<=length&&e[8+j]==(uint8_t)name[j])j++;
  if(j!=length+1)continue;
  if(e[0]||e[1]||e[2]||e[3]!=1)return 0;
  cfg_select((uint16_t)e[4]<<8|e[5]);return cfg_byte()=='1';
 }
 return 0;
}
