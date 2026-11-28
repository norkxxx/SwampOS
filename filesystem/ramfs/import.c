#include "import.h"
int ramfs_import_ext4(struct ramfs*f,const struct ext4*fs,const char*src,const char*dst,void*scratch,size_t cap){
 if(!f||!fs||!src||*src!='/'||!dst||!scratch||cap<RAMFS_BYTES)return RAMFS_INVALID;
 struct fs_inode inode;
 if(!ext4_lookup(fs,src,&inode)||(inode.mode&0xf000)!=0x8000)return RAMFS_NOT_FOUND;
 if(inode.size>RAMFS_BYTES)return RAMFS_TOO_LARGE;
 int result=RAMFS_INVALID;
 if(ext4_read(fs,&inode,0,scratch,inode.size))result=ramfs_write(f,dst,scratch,inode.size);
 volatile uint8_t*p=scratch;for(size_t i=0;i<inode.size;i++)p[i]=0;
 return result;
}
