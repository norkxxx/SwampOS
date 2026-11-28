#include "bootinfo.h"
.code64
.section .text.entry,"ax"
.global _entry
_entry:
 cld
 xor %r12d,%r12d
 movabs $SWAMP_UEFI_MAGIC,%rax
 cmp %rax,%rdx
 jne 2f
 mov $1,%r12d
 jmp 3f
2: mov $0x70000,%rsp
3:
 xor %ebp,%ebp
 lea __bss_start(%rip),%rdi
 lea __bss_end(%rip),%rcx
 sub %rdi,%rcx
 xor %eax,%eax
 rep stosb
 mov %r12d,boot_uefi(%rip)
 call kmain
1: cli
 hlt
 jmp 1b
.section .note.GNU-stack,"",@progbits
