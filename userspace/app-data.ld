ENTRY(_start)
PHDRS { app PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS {
 . = 0x40000000; .text : { *(.text*) *(.rodata*) } :app
 . = 0x40004000; .data : { *(.data*) } :data
 .bss : { *(.bss*) *(COMMON) } :data
 /DISCARD/ : { *(.note*) *(.comment*) *(.eh_frame*) }
 ASSERT(SIZEOF(.text) <= 4096, "user text exceeds page")
 ASSERT(SIZEOF(.data)+SIZEOF(.bss) <= 4096, "user data exceeds page")
}
