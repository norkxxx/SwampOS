#include "swamp.h"
/* An ordinary Ring 3 client. FNV-1a is a diagnostic checksum, not a signature. */
static void hex(char *dst,uint32_t n){
    const char digits[]="0123456789ABCDEF";
    for(unsigned i=0;i<8;i++)dst[i]=digits[(n>>(28-4*i))&15];
}
__attribute__((noreturn,section(".text.entry"))) void _start(void){
    unsigned long caps=swamp_caps();
    if(!(caps&SWAMP_CAP_DATA))swamp_exit(13);
    if(!(caps&SWAMP_CAP_CONSOLE))swamp_exit(25);
    unsigned char buf[SWAMP_DATA_READ_MAX];
    uint32_t hash=2166136261u;unsigned long off=0;
    for(;;){
        long n=swamp_data_read(buf,sizeof buf,off);
        if(n<0||n>(long)sizeof buf||(unsigned long)n>SWAMP_DATA_MAX-off)swamp_exit(23);
        if(!n)break;
        for(long i=0;i<n;i++)hash=(hash^buf[i])*16777619u;
        off+=(unsigned long)n;
    }
    char line[]="RESOURCE_SCAN_OK bytes=0x00000000 fnv1a=0x00000000\n";
    hex(line+25,(uint32_t)off);hex(line+42,hash);
    swamp_exit(swamp_write(line,sizeof line-1)==sizeof line-1?0:25);
}
