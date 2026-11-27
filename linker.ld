ENTRY(_entry)
PHDRS { code PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS { . = 0x100000; .text : { *(.text.entry) *(.text*) } :code .rodata : { *(.rodata*) } :code . = ALIGN(4096); .data : { *(.data*) } :data .bss : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; } :data /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) } }
