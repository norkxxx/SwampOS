.code64
.text
.global irq_timer,irq_default
irq_default:
 cli
1: hlt
 jmp 1b
irq_timer:
 cld
 push %rax
 push %rcx
 push %rdx
 push %rsi
 push %rdi
 push %r8
 push %r9
 push %r10
 push %r11
 push %rbx
 mov %rsp,%rbx
 and $-16,%rsp
 call timer_irq
 mov %rbx,%rsp
 pop %rbx
 pop %r11
 pop %r10
 pop %r9
 pop %r8
 pop %rdi
 pop %rsi
 pop %rdx
 pop %rcx
 pop %rax
 iretq
exception_0:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_1:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_2:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_3:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_4:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_5:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_6:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_7:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_8:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_9:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_10:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_11:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_12:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_13:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_14:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_15:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_16:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_17:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_18:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_19:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_20:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_21:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_22:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_23:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_24:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_25:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_26:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_27:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_28:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_29:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_30:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
exception_31:
 cli
 mov $0xb8000,%rax
 movw $0x4f45,(%rax)
 movw $0x4f58,2(%rax)
 jmp irq_default
.section .rodata
.global exception_table
exception_table:
 .quad exception_0
 .quad exception_1
 .quad exception_2
 .quad exception_3
 .quad exception_4
 .quad exception_5
 .quad exception_6
 .quad exception_7
 .quad exception_8
 .quad exception_9
 .quad exception_10
 .quad exception_11
 .quad exception_12
 .quad exception_13
 .quad exception_14
 .quad exception_15
 .quad exception_16
 .quad exception_17
 .quad exception_18
 .quad exception_19
 .quad exception_20
 .quad exception_21
 .quad exception_22
 .quad exception_23
 .quad exception_24
 .quad exception_25
 .quad exception_26
 .quad exception_27
 .quad exception_28
 .quad exception_29
 .quad exception_30
 .quad exception_31
.section .note.GNU-stack,"",@progbits
