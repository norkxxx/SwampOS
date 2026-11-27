.code32
.section .text.entry,"ax"
.global _entry
_entry:
 cld
 mov $0x70000,%esp
 xor %ebp,%ebp
 call kmain
1: cli
 hlt
 jmp 1b
.section .note.GNU-stack,"",@progbits
