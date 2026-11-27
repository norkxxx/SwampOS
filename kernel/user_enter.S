.text
.global gdt_load,user_enter,user_escape
 gdt_load:
 lgdt (%rdi)
 pushq $0x18
 leaq 1f(%rip),%rax
 pushq %rax
 lretq
1: mov $0x10,%ax
 mov %ax,%ds
 mov %ax,%es
 mov %ax,%ss
 xor %eax,%eax
 mov %ax,%fs
 mov %ax,%gs
 mov $0x30,%ax
 ltr %ax
 ret
user_enter:
 cli
 push %rbp
 push %rbx
 push %r12
 push %r13
 push %r14
 push %r15
 mov %rsp,saved_rsp(%rip)
 pushq $0x23
 push %rsi
 pushq $0x202
 pushq $0x2b
 push %rdi
 mov $0x23,%ax
 mov %ax,%ds
 mov %ax,%es
 xor %eax,%eax
 xor %ebx,%ebx
 xor %ecx,%ecx
 xor %edx,%edx
 xor %edi,%edi
 xor %esi,%esi
 xor %ebp,%ebp
 xor %r8d,%r8d
 xor %r9d,%r9d
 xor %r10d,%r10d
 xor %r11d,%r11d
 xor %r12d,%r12d
 xor %r13d,%r13d
 xor %r14d,%r14d
 xor %r15d,%r15d
 iretq
user_escape:
 cli
 cld
 mov $0x10,%ax
 mov %ax,%ds
 mov %ax,%es
 mov %ax,%ss
 mov saved_rsp(%rip),%rsp
 pop %r15
 pop %r14
 pop %r13
 pop %r12
 pop %rbx
 pop %rbp
 ret
.bss
.align 8
saved_rsp: .quad 0
.section .note.GNU-stack,"",@progbits
