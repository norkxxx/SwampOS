#include "../../kernel/bootinfo.h"
.text
.globl lab_jump
lab_jump:
 cli
 cld
 mov %cr4,%rax
 and $~0x200080,%rax
 mov %rax,%cr4
 mov $0x3e1000,%eax
 mov %rax,%cr3
 mov $SWAMP_UEFI_STACK,%esp
 lgdt 0x3e7020
 pushq $0x18
 leaq 1f(%rip),%rax
 pushq %rax
 lretq
1:
 mov $0x10,%ax
 mov %ax,%ds
 mov %ax,%es
 mov %ax,%ss
 xor %eax,%eax
 mov %ax,%fs
 mov %ax,%gs
 mov %cr0,%rax
 or $0x10000,%rax
 mov %rax,%cr0
 movabs $SWAMP_UEFI_MAGIC,%rdx
 mov $0x100000,%eax
 jmp *%rax
.section .rdata,"dr"
.globl kernel_payload,kernel_payload_end
kernel_payload:
.incbin "build/kernel.bin"
kernel_payload_end:
