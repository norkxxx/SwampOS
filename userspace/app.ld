ENTRY(_start)
PHDRS { app PT_LOAD FLAGS(5); }
SECTIONS { . = 0x40000000; .text : { *(.text*) *(.rodata*) } :app /DISCARD/ : { *(.note*) *(.comment*) *(.eh_frame*) } }
