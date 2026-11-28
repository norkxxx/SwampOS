#include "pio.h"
#define ATA_LBA28_LIMIT (1ull << 28)
#define ATA_LBA48_LIMIT (1ull << 48)
#define ATA_POLL_LIMIT 1000000u
int ata_ident_decode(const uint16_t w[256], struct ata_ident *out)
{
    if (!out) return 0;
    *out = (struct ata_ident){0};
    if (!w || (w[0] & 0x8000) || !(w[49] & (1u << 9))) return 0;
    /* Only 512-byte logical sectors: the block API has a fixed sector size. */
    if ((w[106] & 0xc000) == 0x4000 && (w[106] & 0x1000)) {
        uint32_t words = (uint32_t)w[117] | ((uint32_t)w[118] << 16);
        if (words != 256) return 0;
    }
    uint64_t n28 = (uint32_t)w[60] | ((uint32_t)w[61] << 16);
    if (!n28 || n28 > ATA_LBA28_LIMIT) return 0;
    if ((w[83] & 0xc400) == 0x4400) {
        uint64_t n48 = (uint64_t)w[100] | ((uint64_t)w[101] << 16) |
                       ((uint64_t)w[102] << 32) | ((uint64_t)w[103] << 48);
        if (!n48 || n48 > ATA_LBA48_LIMIT || n48 < n28) return 0;
        *out = (struct ata_ident){n48, 1};
    } else *out = (struct ata_ident){n28, 0};
    return 1;
}
static uint8_t rd(struct ata_drive *d, unsigned off) { return d->io.read8(d->io.ctx, d->base + off); }
static void wr(struct ata_drive *d, unsigned off, uint8_t v) { d->io.write8(d->io.ctx, d->base + off, v); }
static void delay(struct ata_drive *d) { for (unsigned i=0;i<4;i++) (void)d->io.read8(d->io.ctx,d->alt); }
static int wait_status(struct ata_drive *d, int data, int ignore_error)
{
    for (unsigned i=0;i<ATA_POLL_LIMIT;i++) {
        uint8_t s=rd(d,7);
        if (!s || s==255) return 0;
        if (s&0x80) continue;
        if (!ignore_error && (s&0x21)) return 0;
        if (!!(s&8)==!!data) return 1;
    }
    return 0;
}
static int read_sectors(void *ctx,uint64_t lba,unsigned count,void *dst)
{
    struct ata_drive *d=ctx; uint8_t *p=dst;
    if (!d || !p || !count || count>128 || lba>=d->id.sectors || count>d->id.sectors-lba) return 0;
    for (unsigned k=0;k<count;k++,lba++) {
        int ext=lba>=ATA_LBA28_LIMIT;
        if (ext && !d->id.lba48) return 0;
        if (!wait_status(d,0,0)) return 0;
        wr(d,6,(uint8_t)(0xe0 | (d->slave<<4) | (ext?0:lba>>24))); delay(d);
        if (!wait_status(d,0,0)) return 0;
        if (ext) {
            /* High-order task-file bytes first, then low-order bytes. */
            wr(d,1,0); wr(d,2,0); wr(d,3,lba>>24); wr(d,4,lba>>32); wr(d,5,lba>>40);
        }
        wr(d,1,0); wr(d,2,1); wr(d,3,lba); wr(d,4,lba>>8); wr(d,5,lba>>16);
        wr(d,7,ext?0x24:0x20); delay(d);
        if (!wait_status(d,1,0)) return 0;
        for (unsigned i=0;i<256;i++) { uint16_t v=d->io.read16(d->io.ctx,d->base); *p++=v; *p++=v>>8; }
        delay(d); if (!wait_status(d,0,0)) return 0;
    }
    return 1;
}
int ata_pio_probe(struct ata_drive *d,struct blockdev *out,struct ata_io io,uint16_t base,uint16_t alt,unsigned slave)
{
    if (!out) return 0;
    *out=(struct blockdev){0};
    if (!d || !io.read8 || !io.read16 || !io.write8 || slave>1 || base>65528 || !base || !alt) return 0;
    struct ata_drive a={.io=io,.base=base,.alt=alt,.slave=slave};
    io.write8(io.ctx,alt,2); /* nIEN: polling only */
    wr(&a,6,0xa0|(slave<<4)); delay(&a);
    if (!wait_status(&a,0,1)) return 0;
    wr(&a,2,0); wr(&a,3,0); wr(&a,4,0); wr(&a,5,0); wr(&a,7,0xec); delay(&a);
    if (!wait_status(&a,1,0) || rd(&a,4) || rd(&a,5)) return 0;
    uint16_t w[256]; for (unsigned i=0;i<256;i++) w[i]=io.read16(io.ctx,base);
    delay(&a); if (!wait_status(&a,0,0) || !ata_ident_decode(w,&a.id)) return 0;
    *d=a; *out=(struct blockdev){d,a.id.sectors,read_sectors}; return 1;
}
