#include "../filesystem/ramfs/import.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct ramfs fs,backup;
static unsigned char scratch[RAMFS_BYTES],out[RAMFS_BYTES];
static int rd(void*c,uint64_t lba,unsigned count,void*o){FILE*f=c;return !fseek(f,(long)(lba*512),SEEK_SET)&&fread(o,512,count,f)==count;}
int main(void){
 ramfs_init(&fs);assert(RAMFS_FILES==64&&RAMFS_DIRS==64&&RAMFS_BYTES==8192);
 assert(!ramfs_mkdir(&fs,"a")&&!ramfs_mkdir(&fs,"a/b")&&!ramfs_mkdir(&fs,"a/b/c")&&!ramfs_mkdir(&fs,"other"));assert(!ramfs_write(&fs,"a/b/c/f","abc",3));assert(!ramfs_append(&fs,"a/b/c/f","def",3));size_t n;assert(!ramfs_read_at(&fs,"a/b/c/f",2,out,3,&n)&&n==3&&!memcmp(out,"cde",3));assert(!ramfs_read_at(&fs,"a/b/c/f",6,0,0,&n)&&!n);assert(ramfs_read_at(&fs,"a/b/c/f",7,out,1,&n)==RAMFS_INVALID);assert(ramfs_append(&fs,"a/b/c/f",scratch,RAMFS_BYTES)==RAMFS_TOO_LARGE);
 memcpy(&backup,&fs,sizeof fs);assert(ramfs_move_dir(&fs,"a","a/b/z")==RAMFS_INVALID&&!memcmp(&fs,&backup,sizeof fs));assert(ramfs_move_dir(&fs,"a","other")==RAMFS_EXISTS&&!memcmp(&fs,&backup,sizeof fs));assert(ramfs_move_dir(&fs,"a","missing/z")==RAMFS_NOT_FOUND&&!memcmp(&fs,&backup,sizeof fs));
 assert(!ramfs_move_dir(&fs,"a","other/moved"));assert(!ramfs_is_dir(&fs,"a")&&ramfs_is_dir(&fs,"other/moved/b/c"));assert(!ramfs_read(&fs,"other/moved/b/c/f",out,sizeof out,&n)&&n==6&&!memcmp(out,"abcdef",6));assert(!ramfs_move_dir(&fs,"other/moved","a"));
 assert(!ramfs_mkdir(&fs,"other/x")&&!ramfs_mkdir(&fs,"other/x/y")&&!ramfs_mkdir(&fs,"other/x/y/z")&&!ramfs_mkdir(&fs,"other/x/y/z/w")&&!ramfs_mkdir(&fs,"other/x/y/z/w/u"));memcpy(&backup,&fs,sizeof fs);assert(ramfs_move_dir(&fs,"a","other/x/y/z/w/u/new")==RAMFS_INVALID&&!memcmp(&fs,&backup,sizeof fs));
 for(unsigned i=0;i<20000;i++){assert(!ramfs_move_dir(&fs,"a","other/moved"));assert(!ramfs_read(&fs,"other/moved/b/c/f",out,sizeof out,&n)&&n==6&&!memcmp(out,"abcdef",6));assert(!ramfs_move_dir(&fs,"other/moved","a"));}
 puts("PASS: 40000 subtree moves, atomic rejection on collision/depth/descendant, append and bounded range reads");
 FILE*f=fopen("build/vault.ext4","rb");assert(f&&!fseek(f,0,SEEK_END));struct blockdev dev={f,(uint64_t)ftell(f)/512,rd};struct ext4 ext;assert(ext4_mount(&ext,&dev));memset(scratch,0xa5,sizeof scratch);assert(!ramfs_import_ext4(&fs,&ext,"/welcome.txt","a/welcome",scratch,sizeof scratch));assert(!ramfs_read(&fs,"a/welcome",out,sizeof out,&n)&&n>13&&!memcmp(out,"SwampOS Vault",13));for(size_t i=0;i<n;i++)assert(!scratch[i]);assert(ramfs_import_ext4(&fs,&ext,"/pattern.bin","a/welcome",scratch,sizeof scratch)==RAMFS_TOO_LARGE);assert(!ramfs_read(&fs,"a/welcome",out,sizeof out,&n)&&!memcmp(out,"SwampOS Vault",13));assert(ramfs_import_ext4(&fs,&ext,"/bin","a/welcome",scratch,sizeof scratch)==RAMFS_NOT_FOUND);assert(ramfs_import_ext4(&fs,&ext,"/missing","a/welcome",scratch,sizeof scratch)==RAMFS_NOT_FOUND);fclose(f);
 puts("PASS: actual EXT4 import, size/type/path rejection, unchanged destination on read rejection, staging cleanup");
}
