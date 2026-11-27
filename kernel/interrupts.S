.code64
.text
.global irq_timer,irq_default,trap_syscall
irq_default:
 cli
1: hlt
 jmp 1b
irq_timer:
 pushq $0
 pushq $32
 jmp trap_common
trap_syscall:
 pushq $0
 pushq $128
 jmp trap_common
exception_0:
 pushq $0
 pushq $0
 jmp trap_common
exception_1:
 pushq $0
 pushq $1
 jmp trap_common
exception_2:
 pushq $0
 pushq $2
 jmp trap_common
exception_3:
 pushq $0
 pushq $3
 jmp trap_common
exception_4:
 pushq $0
 pushq $4
 jmp trap_common
exception_5:
 pushq $0
 pushq $5
 jmp trap_common
exception_6:
 pushq $0
 pushq $6
 jmp trap_common
exception_7:
 pushq $0
 pushq $7
 jmp trap_common
exception_8:
 pushq $8
 jmp trap_common
exception_9:
 pushq $0
 pushq $9
 jmp trap_common
exception_10:
 pushq $10
 jmp trap_common
exception_11:
 pushq $11
 jmp trap_common
exception_12:
 pushq $12
 jmp trap_common
exception_13:
 pushq $13
 jmp trap_common
exception_14:
 pushq $14
 jmp trap_common
exception_15:
 pushq $0
 pushq $15
 jmp trap_common
exception_16:
 pushq $0
 pushq $16
 jmp trap_common
exception_17:
 pushq $17
 jmp trap_common
exception_18:
 pushq $0
 pushq $18
 jmp trap_common
exception_19:
 pushq $0
 pushq $19
 jmp trap_common
exception_20:
 pushq $0
 pushq $20
 jmp trap_common
exception_21:
 pushq $21
 jmp trap_common
exception_22:
 pushq $0
 pushq $22
 jmp trap_common
exception_23:
 pushq $0
 pushq $23
 jmp trap_common
exception_24:
 pushq $0
 pushq $24
 jmp trap_common
exception_25:
 pushq $0
 pushq $25
 jmp trap_common
exception_26:
 pushq $0
 pushq $26
 jmp trap_common
exception_27:
 pushq $0
 pushq $27
 jmp trap_common
exception_28:
 pushq $0
 pushq $28
 jmp trap_common
exception_29:
 pushq $29
 jmp trap_common
exception_30:
 pushq $30
 jmp trap_common
exception_31:
 pushq $0
 pushq $31
 jmp trap_common
trap_common:
 cld
 push %rax
 push %rcx
 push %rdx
 push %rbx
 push %rbp
 push %rsi
 push %rdi
 push %r8
 push %r9
 push %r10
 push %r11
 push %r12
 push %r13
 push %r14
 push %r15
 mov %rsp,%rbx
 mov %rsp,%rdi
 and $-16,%rsp
 call trap_dispatch
 test %eax,%eax
 jnz user_escape
 mov %rbx,%rsp
 pop %r15
 pop %r14
 pop %r13
 pop %r12
 pop %r11
 pop %r10
 pop %r9
 pop %r8
 pop %rdi
 pop %rsi
 pop %rbp
 pop %rbx
 pop %rdx
 pop %rcx
 pop %rax
 add $16,%rsp
 iretq
.section .rodata
.global exception_table
.align 8
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
