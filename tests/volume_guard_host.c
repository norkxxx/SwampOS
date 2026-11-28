#include "guard.h"
#include "policy.h"
#include "vfs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISK_BYTES (8u*1024u*1024u)
struct disk { unsigned char bytes[DISK_BYTES]; };
static int read_disk(void *ctx,uint64_t lba,unsigned count,void *out){
 struct disk *d=ctx;if(!d||!out||!count||lba>DISK_BYTES/512||count>DISK_BYTES/512-lba)return 0;
 memcpy(out,d->bytes+lba*512,(size_t)count*512);return 1;
}
static void load(struct disk*d,const char*path){FILE*f=fopen(path,"rb");assert(f);assert(fread(d->bytes,1,sizeof d->bytes,f)==sizeof d->bytes);assert(fgetc(f)==EOF);assert(!fclose(f));}
int main(void){
 struct disk *a=calloc(1,sizeof *a),*b=calloc(1,sizeof *b),*bad=calloc(1,sizeof *bad);assert(a&&b&&bad);
 load(a,"build/vault.ext4");memcpy(b->bytes,a->bytes,sizeof b->bytes);memcpy(bad->bytes,a->bytes,sizeof bad->bytes);
 /* Simulate a distinct volume with a distinct on-disk EXT4 UUID. */
 memcpy(b->bytes+1024+104,"SwampOSVault0002",16);
 memset(bad->bytes+1024+104,0,16);
 struct blockdev da={a,DISK_BYTES/512,read_disk},db={b,DISK_BYTES/512,read_disk},dz={bad,DISK_BYTES/512,read_disk};
 struct vfs va,vb,vz,vpart;assert(vfs_mount_demo(&va,&da));assert(vfs_mount_demo(&vb,&db));assert(!vfs_mount_demo(&vz,&dz));
 assert(!memcmp(va.volume_id,"SwampOSVault0001",16));assert(!memcmp(vb.volume_id,"SwampOSVault0002",16));assert(memcmp(va.volume_id,vb.volume_id,16));
 struct gpt g={.count=1};const uint8_t linuxfs[16]={0xaf,0x3d,0xc6,0x0f,0x83,0x84,0x72,0x47,0x8e,0x79,0x3d,0x69,0xd8,0x47,0x7d,0xe4};
 memcpy(g.parts[0].type,linuxfs,16);g.parts[0].first=0;g.parts[0].last=da.sectors-1;
 assert(vfs_mount_partition(&vpart,&da,&g,0));assert(!memcmp(vpart.volume_id,va.volume_id,16));vfs_unmount(&vpart);
 struct fs_inode ia,ib;assert(ext4_lookup(&va.fs,"/bin/hello",&ia));assert(ext4_lookup(&vb.fs,"/bin/hello",&ib));assert(ia.number==ib.number);
 struct guard_table table={0};assert(guard_set(&table,va.volume_id,ia.number,CAP_CONSOLE));assert(guard_set(&table,vb.volume_id,ib.number,CAP_DATA));
 assert(guard_ceiling(&table,va.volume_id,ia.number)==CAP_CONSOLE);assert(guard_ceiling(&table,vb.volume_id,ib.number)==CAP_DATA);
 assert(!guard_allows(&table,va.volume_id,ia.number,CAP_DATA));assert(guard_allows(&table,vb.volume_id,ib.number,CAP_DATA));
 uint8_t saved[16];memcpy(saved,va.volume_id,16);vfs_unmount(&va);assert(!va.mounted);assert(vfs_mount_demo(&va,&da));assert(!memcmp(saved,va.volume_id,16));
 assert(guard_ceiling(&table,va.volume_id,ia.number)==CAP_CONSOLE);assert(guard_remove(&table,va.volume_id,ia.number));assert(guard_ceiling(&table,va.volume_id,ia.number)==CAP_ALL);assert(guard_ceiling(&table,vb.volume_id,ib.number)==CAP_DATA);
 vfs_unmount(&va);vfs_unmount(&vb);free(a);free(b);free(bad);
 puts("PASS: EXT4 UUID extraction, zero-UUID rejection, direct/GPT mount identity consistency, same-inode cross-volume SwampGuard isolation, remount-stable identity and independent profile removal");return 0;
}
