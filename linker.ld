ENTRY(_entry)
PHDRS { code PT_LOAD FLAGS(5); ro PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS {
 . = 0x100000;
 __text_start = .;
 .text : { *(.text.entry) *(.text*) } :code
 . = ALIGN(4096); __text_end = .; __ro_start = .;
 .rodata : { *(.rodata*) } :ro
 . = ALIGN(4096); __ro_end = .;
 .data : { *(.data*) } :data
 .bss : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; } :data
 . = ALIGN(4096); __kernel_end = .;
 /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
 ASSERT(__kernel_end < 0x3e0000, "kernel exceeds reserved low memory")
}
