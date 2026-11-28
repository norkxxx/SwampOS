#include "fixture_hw.h"
#include "core.h"

/* Single outstanding command per queue, polled phase-tagged completions. */
enum { CC=20, CSTS=28, AQA=36, ASQ=40, ACQ=48, DB=0x1000, DEPTH=8 };
struct queue { unsigned tail,head,phase,sequence; };
static struct queue queues[2];
static unsigned stride;

int nvme_idle(void) {
    *fixture_reg(CC)=0;
    return fixture_wait(CSTS,1,0);
}

static int submit(struct fixture_hw *h,unsigned q,unsigned opcode,unsigned ns,
                  uint64_t buffer,uint32_t cdw10,uint32_t cdw11) {
    struct queue *s=&queues[q];
    volatile uint32_t *sq=(void *)(h->dma + q*8192 + s->tail*64);
    volatile uint32_t *cq=(void *)(h->dma + q*8192 + 4096 + s->head*16);
    unsigned cid=++s->sequence & 0xffff;
    for (unsigned i=0;i<16;i++) sq[i]=0;
    sq[0]=opcode | cid<<16;
    sq[1]=ns;
    sq[6]=(uint32_t)buffer; sq[7]=buffer>>32;
    sq[10]=cdw10; sq[11]=cdw11;
    s->tail=(s->tail+1)%DEPTH;
    dma_fence();
    *fixture_reg(DB+2*q*stride)=s->tail;
    uint64_t begin=ticks();
    uint32_t result;
    while ((((result=cq[3])>>16) & 1)!=s->phase) {
        if ((*fixture_reg(CSTS)&2) || ticks()-begin>=150) return 0;
        __asm__ volatile("pause");
    }
    dma_fence();
    if ((result>>17) || (result&0xffff)!=cid || cq[2]>>16!=q || (cq[2]&0xffff)!=s->tail) return 0;
    s->head=(s->head+1)%DEPTH;
    if (!s->head) s->phase ^= 1;
    *fixture_reg(DB+(2*q+1)*stride)=s->head;
    return 1;
}

int nvme_start(struct fixture_hw *h,unsigned aperture) {
    uint32_t cap=*fixture_reg(0),high=*fixture_reg(4);
    stride=4u<<(high&15);
    if ((cap&0xffff)<DEPTH-1 || !(high&(1u<<5)) || ((high>>16)&15) ||
        DB+3*stride+4>aperture) return 0;
    queues[0]=(struct queue){.phase=1}; queues[1]=queues[0];
    *fixture_reg(12)=0xffffffff;
    *fixture_reg(AQA)=(DEPTH-1)<<16 | (DEPTH-1);
    *fixture_reg(ASQ)=h->iova; *fixture_reg(ASQ+4)=0;
    *fixture_reg(ACQ)=h->iova+4096; *fixture_reg(ACQ+4)=0;
    *fixture_reg(CC)=(4u<<20)|(6u<<16)|1;
    if (!fixture_wait(CSTS,3,1)) return 0;
    uint64_t addr=h->iova+DATA_PAGE*4096;
    uint8_t *id=h->dma+DATA_PAGE*4096;
    if (!submit(h,0,6,0,addr,1,0)) return 0;
    static const char serial[21]=FIXTURE_SERIAL "  ";
    for (unsigned i=0;i<20;i++) if (id[4+i]!=serial[i]) return 0;
    if (!submit(h,0,6,1,addr,0,0)) return 0;
    unsigned format=id[26]&15;
    if (*(uint64_t *)(void *)id!=FIXTURE_SECTORS || (id[26]&0xf0) || format>id[25] ||
        id[128+4*format] || id[129+4*format] || id[130+4*format]!=9 || (id[29]&7)) return 0;
    uint32_t size=(DEPTH-1)<<16|1;
    return submit(h,0,5,0,h->iova+3*4096,size,1) &&
           submit(h,0,1,0,h->iova+2*4096,size,1u<<16|1);
}

int nvme_io(struct fixture_hw *h,enum fixture_op op,unsigned lba) {
    if (op==FIXTURE_FLUSH) return submit(h,1,0,1,0,0,0);
    return submit(h,1,op==FIXTURE_WRITE?1:2,1,h->iova+DATA_PAGE*4096,lba,0);
}
