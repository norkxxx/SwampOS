#include "vfs_note.h"
static int equal(const void *a,const void *b,size_t n) {
    const unsigned char *x=a,*y=b; while(n--) if(*x++!=*y++) return 0; return 1;
}
static uint32_t crc(const void *p,size_t n) {
    const uint8_t *bytes=p; uint32_t sum=~0u;
    while (n--) { sum^=*bytes++; for(unsigned j=0;j<8;j++) sum=(sum>>1)^(0xedb88320u & -(sum&1)); }
    return ~sum;
}
static int valid(const struct note_record *r) {
    if (!equal(r->magic,"SWPNOTE1",8) || !r->generation || r->length>sizeof r->data ||
        r->checksum!=crc(r,508)) return 0;
    for(unsigned i=0;i<r->length;i++) if((unsigned char)r->data[i]<32 || (unsigned char)r->data[i]>126) return 0;
    return 1;
}
int vfs_note_mount(struct note_volume *v, const struct blockdev *disk,
                   int (*write)(void *,unsigned,const void *), int (*flush)(void *)) {
    if (!v) return 0;
    v->slot=0;
    if (!disk || disk->sectors!=8192 || !write || !flush) return 0;
    uint8_t label[512];
    if (!block_read(disk,0,1,label) || !equal(label,"SWAMPOS-DISPOSABLE-V1",21)) return 0;
    for (unsigned i=21;i<512;i++) if(label[i]) return 0;
    struct note_record slots[2];
    if (!block_read(disk,1,1,&slots[0]) || !block_read(disk,2,1,&slots[1])) return 0;
    int a=valid(&slots[0]),b=valid(&slots[1]);
    if ((!a && !b) || (a && b && slots[0].generation==slots[1].generation)) return 0;
    v->disk=*disk; v->write=write; v->flush=flush;
    v->slot=b && (!a || slots[1].generation>slots[0].generation) ? 2 : 1;
    v->current=slots[v->slot-1];
    return 1;
}

int vfs_note_write(struct note_volume *v,const char *data,size_t n) {
    if (!v || !v->slot || !data) return 0;
    struct note_record next={.magic={'S','W','P','N','O','T','E','1'},.generation=v->current.generation+1};
    if (!next.generation || n>sizeof next.data) return 0;
    for (unsigned i=0;i<n;i++) {
        if ((unsigned char)data[i]<32 || (unsigned char)data[i]>126) return 0;
        next.data[i]=data[i];
    }
    next.length=n; next.checksum=crc(&next,508);
    unsigned other=3-v->slot;
    struct note_record readback;
    if (!v->write(v->disk.ctx,other,&next) || !v->flush(v->disk.ctx) ||
        !block_read(&v->disk,other,1,&readback) || !equal(&next,&readback,sizeof next)) {
        v->slot=0; return -1;
    }
    v->slot=other; v->current=next; return 1;
}
