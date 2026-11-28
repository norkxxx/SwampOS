#pragma once
#define SWAMP_UEFI_MAGIC 0x5357414d50554649
#define SWAMP_UEFI_INFO 0x3e0000
#define SWAMP_UEFI_RESERVE 0x400000
#define SWAMP_UEFI_STACK 0x3ff000
#define SWAMP_XHCI_RESOURCE_MAX 4u
#define SWAMP_XHCI_HANDOFF_MAGIC 0x58484349u
#define SWAMP_XHCI_HANDOFF_VERSION 1u
#ifndef __ASSEMBLER__
#include "pmm.h"
struct swamp_xhci_resource {uint64_t base,length;uint16_t segment,vendor,product,command;uint8_t bus,device,function,bar;uint32_t flags;};
struct swamp_bootinfo {uint32_t count, reserved; uint64_t rsdp, fb; uint32_t width,height,stride,pixel_format; struct e820_entry map[64]; uint64_t fb_size,ramdisk,ramdisk_size; uint32_t live_option; uint32_t xhci_magic,xhci_count,xhci_version; struct swamp_xhci_resource xhci[SWAMP_XHCI_RESOURCE_MAX]; uint32_t phys_count,phys_complete; struct e820_entry phys_map[64]; struct swamp_xhci_resource storage_resource;};
_Static_assert(sizeof(struct swamp_xhci_resource)==32,"xHCI handoff record ABI");
_Static_assert(sizeof(struct swamp_bootinfo)<=4096,"UEFI bootinfo must fit its reserved page");
extern unsigned boot_uefi;
#endif
