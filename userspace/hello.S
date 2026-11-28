.section .text,"ax"
.global _start
_start:
 mov $2,%eax
 int $0x80
 test $1,%eax
 jz denied
 mov $1,%eax
 lea message(%rip),%rdi
 mov $message_end-message,%esi
 int $0x80
 cmp $message_end-message,%rax
 jne denied
 xor %edi,%edi
 jmp finish
denied:
 mov $13,%edi
finish:
 xor %eax,%eax
 int $0x80
 ud2
message: .ascii "VAULT_APP_HELLO\n"
message_end:
.section .note.GNU-stack,"",@progbits
