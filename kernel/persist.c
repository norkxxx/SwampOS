#include "persist.h"
#include "vfs_note.h"
#include "fixture_disk.h"
#include <stddef.h>

extern void console_write(const char *,size_t);
static struct note_volume volume;
static int read_sector(void *ctx,uint64_t lba,unsigned count,void *out) {
    (void)ctx;
    return count==1 && lba<3 && fixture_io(FIXTURE_READ,(unsigned)lba,out);
}
static int write_sector(void *ctx,unsigned lba,const void *in) {
    (void)ctx;
    return fixture_io(FIXTURE_WRITE,lba,(void *)in);
}
static int flush_disk(void *ctx) { (void)ctx; return fixture_io(FIXTURE_FLUSH,0,0); }
static size_t length(const char *s) { size_t n=0; while(s[n]) n++; return n; }
static void say(const char *s) { console_write(s,length(s)); }
static int prefix(const char *s,const char *p) {
    while (*p) if (*s++!=*p++) return 0;
    return 1;
}
static int is(const char *s,const char *p) { return length(s)==length(p) && prefix(s,p); }
int persist_command(const char *s) {
    if (is(s,"persist-mount")) {
        if (volume.slot) { say("PERSIST_MOUNTED\n"); return 1; }
        const struct blockdev disk={0,FIXTURE_SECTORS,read_sector};
        const char *reason=fixture_open();
        if (reason) { say("PERSIST_DENIED: "); say(reason); say("\n"); }
        else if (!vfs_note_mount(&volume,&disk,write_sector,flush_disk)) { fixture_close(); say("PERSIST_DENIED: invalid label or records\n"); }
        else say("PERSIST_MOUNTED\n");
        return 1;
    }
    if (is(s,"persist-umount")) {
        fixture_close(); volume=(struct note_volume){0};
        say("PERSIST_UNMOUNTED\n"); return 1;
    }
    int listing=is(s,"ls /persist"),reading=is(s,"cat /persist/note");
    const char *write_prefix="write /persist/note ";
    if (!listing && !reading && !prefix(s,write_prefix)) return 0;
    if (!volume.slot) { say("PERSIST_NOT_MOUNTED\n"); return 1; }
    if (listing) { say("note\n"); return 1; }
    if (reading) { say("PERSIST_NOTE "); console_write(volume.current.data,volume.current.length); say("\n"); return 1; }
    const char *data=s+length(write_prefix);
    size_t n=length(data);
    int result=vfs_note_write(&volume,data,n);
    if (result<0) { fixture_close(); say("PERSIST_IO_ERROR: remount required\n"); }
    else say(result ? "PERSIST_COMMITTED\n" : "PERSIST_WRITE_DENIED\n");
    return 1;
}
