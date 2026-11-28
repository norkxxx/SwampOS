#include "vfs.h"

static const uint8_t linuxfs[16]={0xaf,0x3d,0xc6,0x0f,0x83,0x84,0x72,0x47,0x8e,0x79,0x3d,0x69,0xd8,0x47,0x7d,0xe4};

void vfs_unmount(struct vfs *v){
 if(v)*v=(struct vfs){0};
}

int vfs_mount_demo(struct vfs *v,const struct blockdev *d){
 if(!v)return 0;
 vfs_unmount(v);
 if(!d||!ext4_mount(&v->fs,d))return 0;
 for(unsigned j=0;j<16;j++)v->volume_id[j]=v->fs.uuid[j];
 v->mounted=1;
 return 1;
}

int vfs_mount_partition(struct vfs *v,const struct blockdev *d,const struct gpt *g,unsigned i){
 if(!v)return 0;
 vfs_unmount(v);
 if(!d||!g||i>=g->count||i>=GPT_MAX_PARTS)return 0;
 const struct gpt_part *p=&g->parts[i];
 for(unsigned j=0;j<16;j++)if(p->type[j]!=linuxfs[j])return 0;
 if(p->first>p->last||p->last>=d->sectors)return 0;
 struct blockdev child;
 if(!block_slice_init(&v->slice,&child,d,p->first,p->last-p->first+1)||!ext4_mount(&v->fs,&child))return 0;
 /* The EXT4 UUID is the volume identity. Mount aliases of the same volume
    intentionally share policy; duplicate UUIDs must be treated as clones. */
 for(unsigned j=0;j<16;j++)v->volume_id[j]=v->fs.uuid[j];
 v->mounted=1;
 return 1;
}
