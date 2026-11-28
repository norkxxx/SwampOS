#include "ramfs.h"
static void wipe(void *p,size_t n){volatile uint8_t*q=p;while(n--)*q++=0;}
static void copy(void *dst,const void*src,size_t n){uint8_t*d=dst;const uint8_t*s=src;while(n--)*d++=*s++;}
static size_t name_length(const char*n){
 if(!n)return 0;
 size_t i=0,part=0;unsigned depth=1;
 while(i<RAMFS_NAME&&n[i]){unsigned char c=n[i++];if(c=='/'){if(!part||++depth>RAMFS_DEPTH)return 0;part=0;continue;}
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||(c=='.'&&part)))return 0;
  if(++part>RAMFS_COMPONENT)return 0;
 }return i<RAMFS_NAME&&part?i:0;
}
static int equal(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static int locate(const struct ramfs*f,const char*n){for(unsigned i=0;i<RAMFS_FILES;i++)if(f->entries[i].used&&equal(f->entries[i].name,n))return (int)i;return -1;}
static int dir_locate(const struct ramfs*f,const char*n){for(unsigned i=0;i<RAMFS_DIRS;i++)if(f->dirs[i][0]&&equal(f->dirs[i],n))return (int)i;return -1;}
static int parent_exists(const struct ramfs*f,const char*n,size_t len){while(len&&n[len-1]!='/')len--;if(!len)return 1;char p[RAMFS_NAME];copy(p,n,len-1);p[len-1]=0;return dir_locate(f,p)>=0;}
static int descendant(const char*path,const char*parent){while(*parent&&*path==*parent){path++;parent++;}return !*parent&&*path=='/';}
static int vacant(const struct ramfs*f){for(unsigned i=0;i<RAMFS_FILES;i++)if(!f->entries[i].used)return (int)i;return -1;}
void ramfs_init(struct ramfs*f){if(f)wipe(f,sizeof *f);}
int ramfs_write(struct ramfs*f,const char*n,const void*d,size_t len){size_t nl=name_length(n);if(!f||!nl||(!d&&len))return RAMFS_INVALID;if(len>RAMFS_BYTES)return RAMFS_TOO_LARGE;if(!parent_exists(f,n,nl))return RAMFS_NOT_FOUND;if(dir_locate(f,n)>=0)return RAMFS_EXISTS;int i=locate(f,n);if(i<0)i=vacant(f);if(i<0)return RAMFS_FULL;struct ramfs_entry*e=&f->entries[i];if(!e->used){wipe(e,sizeof *e);copy(e->name,n,nl);e->used=1;}if(len)copy(e->data,d,len);wipe(e->data+len,RAMFS_BYTES-len);e->length=len;return RAMFS_OK;}
int ramfs_read(const struct ramfs*f,const char*n,void*d,size_t cap,size_t*len){if(!f||!name_length(n)||!len||(!d&&cap))return RAMFS_INVALID;int i=locate(f,n);if(i<0)return RAMFS_NOT_FOUND;const struct ramfs_entry*e=&f->entries[i];if(e->length>cap)return RAMFS_TOO_LARGE;if(e->length)copy(d,e->data,e->length);*len=e->length;return RAMFS_OK;}
int ramfs_remove(struct ramfs*f,const char*n){if(!f||!name_length(n))return RAMFS_INVALID;int i=locate(f,n);if(i<0)return RAMFS_NOT_FOUND;wipe(&f->entries[i],sizeof f->entries[i]);return RAMFS_OK;}
int ramfs_rename(struct ramfs*f,const char*a,const char*b){size_t nl=name_length(b);if(!f||!name_length(a)||!nl)return RAMFS_INVALID;int i=locate(f,a);if(i<0)return RAMFS_NOT_FOUND;if(equal(a,b))return RAMFS_OK;if(locate(f,b)>=0||dir_locate(f,b)>=0)return RAMFS_EXISTS;if(!parent_exists(f,b,nl))return RAMFS_NOT_FOUND;struct ramfs_entry*e=&f->entries[i];wipe(e->name,sizeof e->name);copy(e->name,b,nl);return RAMFS_OK;}
int ramfs_copy(struct ramfs*f,const char*a,const char*b){if(!f||!name_length(a)||!name_length(b))return RAMFS_INVALID;int i=locate(f,a);if(i<0)return RAMFS_NOT_FOUND;if(locate(f,b)>=0)return RAMFS_EXISTS;return ramfs_write(f,b,f->entries[i].data,f->entries[i].length);}
int ramfs_entry(const struct ramfs*f,unsigned i,char*n,size_t cap,size_t*len){if(!f||i>=RAMFS_FILES||!n||cap<RAMFS_NAME||!len)return RAMFS_INVALID;const struct ramfs_entry*e=&f->entries[i];if(!e->used)return RAMFS_NOT_FOUND;copy(n,e->name,RAMFS_NAME);*len=e->length;return RAMFS_OK;}
unsigned ramfs_count(const struct ramfs*f){unsigned n=0;if(f)for(unsigned i=0;i<RAMFS_FILES;i++)n+=!!f->entries[i].used;return n;}

int ramfs_is_dir(const struct ramfs*f,const char*n){return f&&n&&(!n[0]||(name_length(n)&&dir_locate(f,n)>=0));}
int ramfs_mkdir(struct ramfs*f,const char*n){size_t nl=name_length(n);if(!f||!nl)return RAMFS_INVALID;if(dir_locate(f,n)>=0||locate(f,n)>=0)return RAMFS_EXISTS;if(!parent_exists(f,n,nl))return RAMFS_NOT_FOUND;for(unsigned i=0;i<RAMFS_DIRS;i++)if(!f->dirs[i][0]){wipe(f->dirs[i],RAMFS_NAME);copy(f->dirs[i],n,nl);return RAMFS_OK;}return RAMFS_FULL;}
int ramfs_rmdir(struct ramfs*f,const char*n){if(!f||!name_length(n))return RAMFS_INVALID;int i=dir_locate(f,n);if(i<0)return RAMFS_NOT_FOUND;for(unsigned j=0;j<RAMFS_DIRS;j++)if(f->dirs[j][0]&&descendant(f->dirs[j],n))return RAMFS_NOT_EMPTY;for(unsigned j=0;j<RAMFS_FILES;j++)if(f->entries[j].used&&descendant(f->entries[j].name,n))return RAMFS_NOT_EMPTY;wipe(f->dirs[i],RAMFS_NAME);return RAMFS_OK;}
int ramfs_dir_entry(const struct ramfs*f,unsigned i,char*n,size_t cap){if(!f||i>=RAMFS_DIRS||!n||cap<RAMFS_NAME)return RAMFS_INVALID;if(!f->dirs[i][0])return RAMFS_NOT_FOUND;copy(n,f->dirs[i],RAMFS_NAME);return RAMFS_OK;}

int ramfs_append(struct ramfs*f,const char*n,const void*d,size_t len){
 if(!f||!name_length(n)||(!d&&len))return RAMFS_INVALID;
 int i=locate(f,n);if(i<0)return ramfs_write(f,n,d,len);
 struct ramfs_entry*e=&f->entries[i];if(e->length>RAMFS_BYTES||len>RAMFS_BYTES-e->length)return RAMFS_TOO_LARGE;
 if(len)copy(e->data+e->length,d,len);
 e->length+=len;return RAMFS_OK;
}
int ramfs_read_at(const struct ramfs*f,const char*n,size_t off,void*d,size_t cap,size_t*len){
 if(!f||!name_length(n)||!len||(!d&&cap))return RAMFS_INVALID;
 int i=locate(f,n);if(i<0)return RAMFS_NOT_FOUND;
 const struct ramfs_entry*e=&f->entries[i];if(off>e->length)return RAMFS_INVALID;
 size_t available=e->length-off;if(cap>available)cap=available;
 if(cap)copy(d,e->data+off,cap);
 *len=cap;return RAMFS_OK;
}
/* Preflight every descendant. No allocation, no writes until all paths are valid.
 * Since the new directory root is absent, a valid namespace cannot have an
 * existing descendant beneath it. The shell/desktop path is serialized. */
static int rebase(char out[RAMFS_NAME],const char*path,size_t oldlen,const char*dest,size_t newlen){
 size_t suffix=0;while(path[oldlen+suffix])suffix++;
 if(suffix>=RAMFS_NAME-newlen)return 0;
 copy(out,dest,newlen);copy(out+newlen,path+oldlen,suffix+1);
 return name_length(out)!=0;
}
int ramfs_move_dir(struct ramfs*f,const char*a,const char*b){
 size_t al=name_length(a),bl=name_length(b);if(!f||!al||!bl)return RAMFS_INVALID;
 int i=dir_locate(f,a);if(i<0)return RAMFS_NOT_FOUND;if(equal(a,b))return RAMFS_OK;
 if(descendant(b,a))return RAMFS_INVALID;
 if(dir_locate(f,b)>=0||locate(f,b)>=0)return RAMFS_EXISTS;
 if(!parent_exists(f,b,bl))return RAMFS_NOT_FOUND;
 /* Snapshot both names before editing storage (they may point into the store). */
 char src[RAMFS_NAME],dst[RAMFS_NAME],temp[RAMFS_NAME];copy(src,a,al+1);copy(dst,b,bl+1);
 for(unsigned j=0;j<RAMFS_DIRS;j++)if(f->dirs[j][0]&&descendant(f->dirs[j],src)&&!rebase(temp,f->dirs[j],al,dst,bl))return RAMFS_INVALID;
 for(unsigned j=0;j<RAMFS_FILES;j++)if(f->entries[j].used&&descendant(f->entries[j].name,src)&&!rebase(temp,f->entries[j].name,al,dst,bl))return RAMFS_INVALID;
 for(unsigned j=0;j<RAMFS_DIRS;j++)if(f->dirs[j][0]&&descendant(f->dirs[j],src)){(void)rebase(temp,f->dirs[j],al,dst,bl);wipe(f->dirs[j],RAMFS_NAME);copy(f->dirs[j],temp,name_length(temp));}
 for(unsigned j=0;j<RAMFS_FILES;j++)if(f->entries[j].used&&descendant(f->entries[j].name,src)){(void)rebase(temp,f->entries[j].name,al,dst,bl);wipe(f->entries[j].name,RAMFS_NAME);copy(f->entries[j].name,temp,name_length(temp));}
 wipe(f->dirs[i],RAMFS_NAME);copy(f->dirs[i],dst,bl);return RAMFS_OK;
}
