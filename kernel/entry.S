.code64
.section .text.entry,"ax"
.global _entry
_entry:
 cld
 mov $0x70000,%rsp
 xor %ebp,%ebp
 lea __bss_start(%rip),%rdi
 lea __bss_end(%rip),%rcx
 sub %rdi,%rcx
 xor %eax,%eax
 rep stosb
 call kmain
1: cli
 hlt
 jmp 1b
.section .note.GNU-stack,"",@progbits
