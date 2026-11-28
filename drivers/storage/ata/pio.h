#pragma once
#include <stdint.h>
#include "../../../kernel/block.h"
struct ata_io {
    void *ctx;
    uint8_t (*read8)(void *, uint16_t);
    uint16_t (*read16)(void *, uint16_t);
    void (*write8)(void *, uint16_t, uint8_t);
};
struct ata_ident { uint64_t sectors; unsigned lba48; };
struct ata_drive { struct ata_io io; struct ata_ident id; uint16_t base, alt; unsigned slave; };
int ata_ident_decode(const uint16_t words[256], struct ata_ident *out);
int ata_pio_probe(struct ata_drive *, struct blockdev *, struct ata_io, uint16_t, uint16_t, unsigned);
