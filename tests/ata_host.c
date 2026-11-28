#include "../drivers/storage/ata/pio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x170
#define ALT 0x376
struct regwrite {uint16_t port;uint8_t value;};
struct mock {uint16_t id[256];unsigned state,pos,n;int absent,busy,error,packet;struct regwrite writes[256];};
static uint8_t rd(void *ctx,uint16_t p){struct mock*m=ctx;if(m->absent)return 255;if(m->busy)return 0x80;if(p==BASE+4&&m->packet)return 0x14;if(p==BASE+5&&m->packet)return 0xeb;if(p==BASE+7||p==ALT)return m->error?0x41:m->state?0x48:0x40;return 0;}
static uint16_t r16(void *ctx,uint16_t p){struct mock*m=ctx;assert(p==BASE&&m->state&&m->pos<256);unsigned i=m->pos++;uint16_t v=m->state==1?m->id[i]:(uint16_t)(i^0x5aa5);if(m->pos==256)m->state=0;return v;}
static void wr(void *ctx,uint16_t p,uint8_t v){struct mock*m=ctx;assert(m->n<256);m->writes[m->n++]=(struct regwrite){p,v};if(p==BASE+7){assert(v==0xec||v==0x20||v==0x24);m->state=v==0xec?1:2;m->pos=0;}}
static void init(struct mock*m,int ext){memset(m,0,sizeof *m);m->id[49]=1<<9;m->id[60]=0xffff;m->id[61]=0x0fff;if(ext){m->id[83]=0x4400;m->id[100]=0x1234;m->id[101]=0x5678;m->id[102]=0x12;}}
static int probe(struct mock*m,struct ata_drive*d,struct blockdev*b){return ata_pio_probe(d,b,(struct ata_io){m,rd,r16,wr},BASE,ALT,0);}
int main(void){struct mock m;struct ata_drive d;struct blockdev b;struct ata_ident id;uint8_t buf[1024];
 init(&m,1);assert(probe(&m,&d,&b)&&b.sectors==0x1256781234ull);m.n=0;
 assert(block_read(&b,0x0fffffffull,2,buf));assert(m.n==19);assert(m.writes[0].value==0xef&&m.writes[6].value==0x20);assert(m.writes[7].value==0xe0&&m.writes[10].value==0x10&&m.writes[18].value==0x24);
 for(unsigned i=0;i<512;i+=2){unsigned v=(i/2)^0x5aa5;assert(buf[i]==(v&255)&&buf[i+1]==(v>>8)&&buf[i+512]==buf[i]);}
 m.n=0;assert(block_read(&b,0x123456789aull,1,buf));assert(m.writes[3].value==0x34&&m.writes[4].value==0x12&&m.writes[5].value==0&&m.writes[8].value==0x9a&&m.writes[9].value==0x78&&m.writes[10].value==0x56);
 m.n=0;assert(!block_read(&b,b.sectors,1,buf)&&!m.n);assert(!b.read(b.ctx,0,129,buf)&&!m.n);m.error=1;assert(!block_read(&b,0,1,buf)&&!m.n);
 init(&m,0);assert(probe(&m,&d,&b)&&b.sectors==0x0fffffffull);m.n=0;assert(!block_read(&b,0x10000000,1,buf)&&!m.n);assert(block_read(&b,1,1,buf));
 init(&m,0);m.absent=1;assert(!probe(&m,&d,&b)&&!b.read);init(&m,0);m.busy=1;assert(!probe(&m,&d,&b));init(&m,0);m.packet=1;assert(!probe(&m,&d,&b));
 init(&m,1);m.id[106]=0x5000;m.id[117]=2048;assert(!ata_ident_decode(m.id,&id));m.id[117]=256;assert(ata_ident_decode(m.id,&id));m.id[103]=2;assert(!ata_ident_decode(m.id,&id));
 uint32_t rng=0x5a4d;for(unsigned i=0;i<50000;i++){init(&m,i&1);rng=rng*1664525u+1013904223u;m.id[(rng>>16)&255]^=(uint16_t)rng;if(ata_ident_decode(m.id,&id))assert(id.sectors&&id.sectors<=(1ull<<(id.lba48?48:28)));else assert(!id.sectors);}
 puts("ATA_PIO_HOST_OK: 50000 IDENTIFY mutations; LBA28/48 task-file order, bounds, 4Kn rejection, missing/busy/packet/error");return 0;}
