#include "lab_jump.S"
.section .rdata,"dr"
.globl live_volume_start,live_volume_end
live_volume_start:
.incbin "build/vault.ext4"
live_volume_end:
