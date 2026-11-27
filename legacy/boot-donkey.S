.code16
.section .text
.global _start
_start:
 cli
 xor %ax,%ax
 mov %ax,%ds
 mov %ax,%es
 mov %ax,%ss
 mov $0x7c00,%sp
 sti
 mov %dl,drive
 mov $dap,%si
 mov $0x42,%ah
 int $0x13
 jc fail
 cli
 in $0x92,%al
 or $2,%al
 out %al,$0x92
 lgdt gdtptr
 mov %cr0,%eax
 or $1,%eax
 mov %eax,%cr0
 ljmp $8,$pm
fail:
 mov $0x0e45,%ax
 int $0x10
 cli
1: hlt
 jmp 1b
.code32
pm:
 mov $16,%ax
 mov %ax,%ds
 mov %ax,%es
 mov %ax,%ss
 mov $0x90000,%esp
 cld
 mov $0x10000,%esi
 mov $0x100000,%edi
 mov $8192,%ecx
 rep movsl
 # Clear three page-table pages, then identity map first 1 GiB.
 xor %eax,%eax
 mov $0x1000,%edi
 mov $3072,%ecx
 rep stosl
 movl $0x2003,0x1000
 movl $0x3003,0x2000
 mov $0x3000,%edi
 mov $0x83,%eax
 mov $512,%ecx
map:
 mov %eax,(%edi)
 add $0x200000,%eax
 add $8,%edi
 loop map
 mov %cr4,%eax
 or $0x20,%eax
 mov %eax,%cr4
 mov $0x1000,%eax
 mov %eax,%cr3
 mov $0xc0000080,%ecx
 rdmsr
 or $0x100,%eax
 wrmsr
 mov %cr0,%eax
 or $0x80010000,%eax
 mov %eax,%cr0
 ljmp $24,$lm
.code64
lm:
 mov $16,%ax
 mov %ax,%ds
 mov %ax,%es
 mov %ax,%ss
 mov $0x70000,%rsp
 mov $0x100000,%rax
 jmp *%rax
.align 8
gdt: .quad 0,0x00cf9a000000ffff,0x00cf92000000ffff,0x00af9a000000ffff
gdtptr: .word 31
 .long gdt
drive: .byte 0
.align 4
dap: .byte 16,0
 .word 64,0,0x1000
 .quad 1
.org 510
.word 0xaa55
