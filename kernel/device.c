#include "device.h"
static enum device_kind classify(const struct pci_function*p){if(p->vendor==0x1af4)return DEV_VIRTIO;if(p->class_code==1&&p->subclass==6&&p->prog_if==1)return DEV_AHCI;if(p->class_code==1&&p->subclass==8&&p->prog_if==2)return DEV_NVME;if(p->class_code==12&&p->subclass==3){switch(p->prog_if){case 0:return DEV_UHCI;case 0x10:return DEV_OHCI;case 0x20:return DEV_EHCI;case 0x30:return DEV_XHCI;default:break;}}if(p->class_code==2&&p->subclass==0)return DEV_ETHERNET;if(p->class_code==3)return DEV_DISPLAY;return DEV_OTHER;}
const char *device_name(enum device_kind k){switch(k){case DEV_AHCI:return "AHCI";case DEV_NVME:return "NVMe";case DEV_XHCI:return "xHCI";case DEV_UHCI:return "UHCI";case DEV_OHCI:return "OHCI";case DEV_EHCI:return "EHCI";case DEV_ETHERNET:return "Ethernet";case DEV_DISPLAY:return "Display";case DEV_VIRTIO:return "VirtIO";default:return "Other";}}
int device_probe(pci_read_fn read,void*c,const struct pci_function*p,struct device_info*out){
 if(!read||!p||!out||p->device>=32||p->function>=8)return 0;
 struct device_info a={0};a.kind=classify(p);
 unsigned h=p->header&127;if(h>1)return 0;unsigned count=h==0?6:2;
 #define READ(reg) read(c,p->bus,p->device,p->function,(reg))
 if((READ(0)&65535)!=p->vendor)return 0;
 for(unsigned i=0;i<count;i++){uint32_t x=READ(0x10+4*i);if(!x)continue;if(x==UINT32_MAX)return 0;if(x&1){a.bars[i]=(struct pci_bar){x&~3u,1};continue;}
  unsigned t=(x>>1)&3;if(t==0){a.bars[i]=(struct pci_bar){x&~15u,2};continue;}if(t!=2||i+1>=count)return 0;uint64_t addr=(x&~15u)|((uint64_t)READ(0x10+4*(i+1))<<32);a.bars[i]=(struct pci_bar){addr,3};i++;
 }
 if(READ(4)&(1u<<20)){unsigned pos=READ(0x34)&255;uint64_t seen=0;while(pos){if(pos<0x40||pos>0xfc||(pos&3)||(seen&(1ull<<(pos/4))))return 0;seen|=1ull<<(pos/4);uint32_t cword=READ(pos);unsigned id=cword&255;if(id==255||!id)return 0;a.cap_count++;if(id==0x10)a.pcie=1;if(id==5)a.msi=1;if(id==0x11)a.msix=1;if(id==9&&a.kind==DEV_VIRTIO)a.virtio_vendor=1;pos=(cword>>8)&255;}}
 #undef READ
 *out=a;return 1;
}
