#pragma once
#include <stdint.h>
struct pci_function {uint8_t bus,device,function,class_code,subclass,prog_if,header;uint16_t vendor,product;};
typedef uint32_t (*pci_read_fn)(void*,uint8_t,uint8_t,uint8_t,uint8_t);
typedef int (*pci_visit_fn)(void*,const struct pci_function*);
/* Exhaustive legacy PCI config-space enumeration. No BAR writes, probing or enabling devices. */
unsigned pci_scan(pci_read_fn,void*,pci_visit_fn,void*);
#ifndef PCI_HOST
uint32_t pci_config_read(void*,uint8_t,uint8_t,uint8_t,uint8_t);
#endif
