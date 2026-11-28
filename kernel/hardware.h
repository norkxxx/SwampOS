#pragma once
#include <stdint.h>
#include <stddef.h>
struct cpu_info {char vendor[13];uint32_t leaf1_ecx,leaf1_edx,leaf7_ebx,ext_edx,logical;};
void cpu_probe(struct cpu_info *);
typedef int (*phys_reader)(void*,uint64_t,void*,size_t);
#define ACPI_DMAR_MAX_DRHD 8u
#define ACPI_DMAR_MAX_SCOPES 64u
struct acpi_dmar_unit {uint64_t reg_base;uint16_t segment;uint8_t flags,reserved;};
struct acpi_dmar_scope {uint8_t type,start_bus,device,function,path_count,unit_index;};
struct acpi_info {
 uint64_t rsdp,root,madt,mcfg,lapic,dmar;
 unsigned cpus,ioapics,overrides;
 unsigned dmar_drhd_count,dmar_rmrr_count,dmar_scope_count;
 uint8_t dmar_host_width,dmar_flags,dmar_bad,dmar_truncated;
 struct acpi_dmar_unit drhd[ACPI_DMAR_MAX_DRHD];
 struct acpi_dmar_scope scopes[ACPI_DMAR_MAX_SCOPES];
};
int acpi_probe(phys_reader,void*,uint64_t,struct acpi_info*);
int acpi_dmar_covers(const struct acpi_info*,uint16_t,uint8_t,uint8_t,uint8_t);
int hardware_command(const char *);
/* Set-1 translated keyboard, stateless printable key mapping with shift. */
struct keyboard {uint8_t shift,extended,caps;};
int keyboard_decode(struct keyboard *,uint8_t);
int keyboard_poll(void);

int acpi_get_info(struct acpi_info *);
