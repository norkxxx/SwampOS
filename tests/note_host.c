#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../kernel/vfs_note.h"

struct disk { unsigned char sectors[3][512]; int bad_read, bad_write, bad_flush; };
static int read_one(void *ctx,uint64_t lba,unsigned count,void *out) {
    struct disk *d=ctx;
    if (d->bad_read || count!=1 || lba>=3) return 0;
    memcpy(out,d->sectors[lba],512); return 1;
}
static int write_one(void *ctx,unsigned lba,const void *data) {
    struct disk *d=ctx;
    if (d->bad_write || lba<1 || lba>2) return 0;
    memcpy(d->sectors[lba],data,512); return 1;
}
static int flush(void *ctx) { return !((struct disk *)ctx)->bad_flush; }
static void seed(struct disk *d,unsigned slot,uint64_t generation) {
    struct note_record r={.magic="SWPNOTE1",.generation=generation};
    const uint8_t *p=(const void *)&r;
    uint32_t crc=~0u;
    for(unsigned i=0;i<508;i++) {
        crc ^= p[i];
        for(unsigned bit=0;bit<8;bit++) crc=(crc&1)?(crc>>1)^0xedb88320u:crc>>1;
    }
    r.checksum=~crc; memcpy(d->sectors[slot],&r,512);
}
int main(void) {
    struct disk d={0};
    struct blockdev dev={&d,8192,read_one};
    struct note_volume v={0};
    memcpy(d.sectors[0],"SWAMPOS-DISPOSABLE-V1",21);
    assert(!vfs_note_mount(&v,&dev,write_one,flush));
    seed(&d,1,1); seed(&d,2,1);
    assert(!vfs_note_mount(&v,&dev,write_one,flush));
    seed(&d,2,2);
    assert(vfs_note_mount(&v,&dev,write_one,flush) && v.slot==2);
    char bytes[491]; memset(bytes,'a',sizeof bytes);
    assert(!vfs_note_write(&v,bytes,sizeof bytes) && !vfs_note_write(&v,"\n",1));
    assert(vfs_note_write(&v,bytes,490)==1 && v.current.length==490);
    for(unsigned fault=0;fault<3;fault++) {
        d.bad_write=fault==0; d.bad_flush=fault==1; d.bad_read=fault==2;
        assert(vfs_note_write(&v,"try",3)==-1 && v.slot==0);
        assert(vfs_note_write(&v,"again",5)==0);
        d.bad_write=d.bad_flush=d.bad_read=0;
        assert(vfs_note_mount(&v,&dev,write_one,flush));
    }
    seed(&d,1,UINT64_MAX);
    assert(vfs_note_mount(&v,&dev,write_one,flush) && !vfs_note_write(&v,"x",1));
    puts("PASS note volume: size, characters, ambiguous slots, counter overflow, failed writes, flushes and readback");
}
