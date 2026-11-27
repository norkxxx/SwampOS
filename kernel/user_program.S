.section .rodata.user,"a",@progbits
.global user_blob_start,user_blob_end,user_normal,user_kernel_read,user_code_write,user_nx,user_io,user_guard,user_spin,user_deny,user_fpu
user_blob_start:
user_normal:
 mov $1,%eax
 lea message(%rip),%rdi
 mov $message_end-message,%esi
 int $0x80
 cmp $message_end-message,%rax
 jne bad
 mov $1,%eax
 mov $0x100000,%edi
 mov $1,%esi
 int $0x80
 cmp $-14,%rax
 jne bad
 mov $1,%eax
 mov $0x40000fff,%edi
 mov $2,%esi
 int $0x80
 cmp $-14,%rax
 jne bad
 mov $99,%eax
 int $0x80
 cmp $-38,%rax
 jne bad
 xor %edi,%edi
 xor %eax,%eax
 int $0x80
bad:
 mov $1,%edi
 xor %eax,%eax
 int $0x80
 ud2
user_kernel_read:
 mov $0x100000,%eax
 mov (%rax),%rax
 jmp bad
user_code_write:
 mov $0x40000000,%eax
 movb $0,(%rax)
 jmp bad
user_nx:
 mov $0x40002000,%eax
 movb $0xc3,(%rax)
 call *%rax
 jmp bad
user_io:
 mov $0x80,%dx
 xor %eax,%eax
 out %al,%dx
 jmp bad
user_guard:
 mov $0x40001000,%eax
 mov (%rax),%rax
 jmp bad
user_deny:
 mov $2,%eax
 int $0x80
 test %rax,%rax
 jne bad
 mov $1,%eax
 lea message(%rip),%rdi
 mov $message_end-message,%esi
 int $0x80
 cmp $-1,%rax
 jne bad
 xor %edi,%edi
 xor %eax,%eax
 int $0x80
 ud2
user_fpu:
 fninit
 jmp bad
user_spin:
 jmp user_spin
message: .ascii "SwampBox: ring 3 hello\n"
message_end:
user_blob_end:
.section .note.GNU-stack,"",@progbits
