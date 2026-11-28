#include "fixture_hw.h"

/* ICH9 AHCI, one directly attached disk on port zero, command slot zero. */
enum { GHC=4, PI=12, CAP2=36, PORT=0x100, CLB=0, FB=8, IS=16,
       IE=20, CMD=24, TFD=32, SIG=36, SSTS=40, SERR=48, SACT=52, CI=56 };
enum { ST=1, FRE=16, FR=1<<14, CR=1<<15 };

static int port_stop(unsigned port) {
    *fixture_reg(port+CMD) &= ~ST;
    if (!fixture_wait(port+CMD,CR,0)) return 0;
    *fixture_reg(port+CMD) &= ~FRE;
    return fixture_wait(port+CMD,FR,0);
}

int ahci_idle(void) {
    unsigned ports = *fixture_reg(PI);
    if (!(ports & 1) || (ports >> 6) || (*fixture_reg(CAP2) & 1)) return 0;
    *fixture_reg(GHC) = 1u<<31; /* AHCI enabled, interrupts disabled */
    for (unsigned i=0;i<6;i++) {
        if (!(ports & (1u<<i))) continue;
        unsigned port = PORT + i*128;
        if (i && (*fixture_reg(port+SSTS) & 15)) return 0;
        if (!port_stop(port)) return 0;
    }
    return 1;
}

static int issue(struct fixture_hw *h, unsigned opcode, unsigned lba, int write, int data) {
    if ((*fixture_reg(PORT+CI) | *fixture_reg(PORT+SACT)) ||
        !fixture_wait(PORT+TFD,0x88,0)) return 0;
    uint32_t *header = (void *)h->dma;
    uint8_t *fis = h->dma+512;
    clear_bytes(header,32);
    clear_bytes(fis,144);
    header[0] = 5u | (write ? 64u : 0u) | (data ? 1u<<16 : 0u);
    header[2] = (uint32_t)h->iova + 512;
    fis[0]=0x27; fis[1]=0x80; fis[2]=opcode;
    if (opcode==0x25 || opcode==0x35) { fis[4]=lba; fis[7]=0x40; fis[12]=1; }
    if (data) {
        uint32_t *prdt = (void *)(fis+128);
        prdt[0] = (uint32_t)h->iova + DATA_PAGE*4096;
        prdt[3] = FIXTURE_BLOCK-1;
    }
    *fixture_reg(PORT+IS) = 0xffffffff;
    dma_fence();
    *fixture_reg(PORT+CI)=1;
    if (!fixture_wait(PORT+CI,1,0)) return 0;
    dma_fence();
    return !(*fixture_reg(PORT+IS) & 0x7d000000u) && !(*fixture_reg(PORT+TFD) & 0x21) &&
           (!data || ((volatile uint32_t *)header)[1]==FIXTURE_BLOCK);
}

int ahci_start(struct fixture_hw *h) {
    if ((*fixture_reg(PORT+SSTS) & 15)!=3 || *fixture_reg(PORT+SIG)!=0x101) return 0;
    *fixture_reg(PORT+IE)=0;
    *fixture_reg(PORT+CLB)=h->iova;
    *fixture_reg(PORT+CLB+4)=0;
    *fixture_reg(PORT+FB)=h->iova+4096;
    *fixture_reg(PORT+FB+4)=0;
    *fixture_reg(PORT+SERR)=0xffffffff;
    *fixture_reg(PORT+CMD) |= FRE;
    *fixture_reg(PORT+CMD) |= ST;
    if (!issue(h,0xec,0,0,1)) return 0;
    uint16_t *id=(void *)(h->dma+DATA_PAGE*4096);
    static const char serial[21]=FIXTURE_SERIAL "  ";
    for (unsigned i=0;i<20;i++)
        if ((uint8_t)(id[10+i/2] >> (i%2 ? 0 : 8)) != serial[i]) return 0;
    uint64_t sectors=(uint64_t)id[103]<<48 | (uint64_t)id[102]<<32 | (uint32_t)id[101]<<16 | id[100];
    return sectors==FIXTURE_SECTORS && (id[83] & (1u<<10)) && (id[83] & (1u<<13)) &&
           ((id[106] & 0xd000u)!=0x5000u || (id[117]==256 && !id[118]));
}

int ahci_io(struct fixture_hw *h,enum fixture_op op,unsigned lba) {
    if (op==FIXTURE_FLUSH) return issue(h,0xea,0,0,0);
    return issue(h,op==FIXTURE_WRITE?0x35:0x25,lba,op==FIXTURE_WRITE,1);
}
