/* Host tool: create an actual EXT4 volume using upstream libext2fs. */
#include <ext2fs/ext2fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static void check(errcode_t e,const char *op){if(e){fprintf(stderr,"%s: %s (%ld)\n",op,error_message(e),(long)e);exit(1);}}
static void add(ext2_filsys fs,ext2_ino_t dir,const char *name,const void *data,unsigned len){
 ext2_ino_t ino;struct ext2_inode i={0};ext2_file_t f;unsigned written;
 check(ext2fs_new_inode(fs,dir,0100644,0,&ino),"new inode");ext2fs_inode_alloc_stats2(fs,ino,1,0);
 i.i_mode=0100644;i.i_links_count=1;i.i_flags=EXT4_EXTENTS_FL;i.i_atime=i.i_mtime=i.i_ctime=fs->now;
 struct ext3_extent_header *h=(void*)i.i_block;h->eh_magic=EXT3_EXT_MAGIC;h->eh_max=4;
 check(ext2fs_write_new_inode(fs,ino,&i),"write inode");check(ext2fs_file_open(fs,ino,EXT2_FILE_WRITE,&f),"open file");check(ext2fs_file_write(f,data,len,&written),"write file");if(written!=len)exit(1);check(ext2fs_file_close(f),"close file");check(ext2fs_link(fs,dir,name,ino,EXT2_FT_REG_FILE),"link file");
}
int main(int argc,char **argv){if(argc<3||argc>8)return 2;FILE *app=fopen(argv[2],"rb");if(!app)return 1;unsigned char code[65536];size_t n=fread(code,1,sizeof code,app);if(ferror(app)||!feof(app)||!n)return 1;fclose(app);
 FILE *out=fopen(argv[1],"wb");if(!out)return 1;if(fseek(out,8*1024*1024-1,SEEK_SET)||fputc(0,out)==EOF||fclose(out))return 1;
 struct ext2_super_block s={0};s.s_blocks_count=2048;s.s_inodes_count=128;s.s_log_block_size=2;s.s_log_cluster_size=2;s.s_rev_level=1;s.s_inode_size=256;s.s_feature_incompat=EXT2_FEATURE_INCOMPAT_FILETYPE|EXT3_FEATURE_INCOMPAT_EXTENTS;s.s_state=EXT2_VALID_FS;
 ext2_filsys fs;check(ext2fs_initialize(argv[1],EXT2_FLAG_EXCLUSIVE,&s,unix_io_manager,&fs),"initialize");fs->now=1767225600;fs->super->s_mkfs_time=fs->super->s_wtime=fs->super->s_lastcheck=fs->now;memcpy(fs->super->s_uuid,"SwampOSVault0001",16);memcpy(fs->super->s_volume_name,"SWAMPVAULT-RO",13);
 check(ext2fs_allocate_tables(fs),"allocate tables");check(ext2fs_mkdir(fs,EXT2_ROOT_INO,EXT2_ROOT_INO,0),"root");for(unsigned i=1;i<EXT2_FIRST_INODE(fs->super);i++)if(i!=EXT2_ROOT_INO)ext2fs_inode_alloc_stats2(fs,i,1,0);
 check(ext2fs_mkdir(fs,EXT2_ROOT_INO,0,"bin"),"bin");ext2_ino_t dir;check(ext2fs_lookup(fs,EXT2_ROOT_INO,"bin",3,0,&dir),"lookup bin");add(fs,dir,"hello",code,(unsigned)n);if(argc>=4){app=fopen(argv[3],"rb");if(!app)return 1;n=fread(code,1,sizeof code,app);if(ferror(app)||!feof(app)||!n)return 1;fclose(app);add(fs,dir,"sdk-demo",code,(unsigned)n);}if(argc>=5){app=fopen(argv[4],"rb");if(!app)return 1;n=fread(code,1,sizeof code,app);if(ferror(app)||!feof(app)||!n)return 1;fclose(app);add(fs,dir,"integration-demo",code,(unsigned)n);}if(argc>=6){app=fopen(argv[5],"rb");if(!app)return 1;n=fread(code,1,sizeof code,app);if(ferror(app)||!feof(app)||!n)return 1;fclose(app);add(fs,dir,"overlord-demo",code,(unsigned)n);}if(argc>=7){app=fopen(argv[6],"rb");if(!app)return 1;n=fread(code,1,sizeof code,app);if(ferror(app)||!feof(app)||!n)return 1;fclose(app);add(fs,dir,"gui-demo",code,(unsigned)n);}if(argc==8){app=fopen(argv[7],"rb");if(!app)return 1;n=fread(code,1,sizeof code,app);if(ferror(app)||!feof(app)||!n)return 1;fclose(app);add(fs,dir,"resource-scan",code,(unsigned)n);}const char msg[]="SwampOS Vault: read-only EXT4 demo volume. NOT encrypted.\n";add(fs,EXT2_ROOT_INO,"welcome.txt",msg,sizeof msg-1);
 unsigned char bytes[9000];for(unsigned i=0;i<sizeof bytes;i++)bytes[i]=(unsigned char)(i*31u+7u);add(fs,EXT2_ROOT_INO,"pattern.bin",bytes,sizeof bytes);
 fs->super->s_state=EXT2_VALID_FS;check(ext2fs_calculate_summary_stats(fs,0),"summary");check(ext2fs_close(fs),"close filesystem");return 0;
}
