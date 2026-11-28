#include "../drivers/usb/xhci/controller.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct mock_mmio { uint32_t regs[64]; unsigned reads; int fail_offset; };
static int read32(void *ctx,uint32_t offset,uint32_t *value) {
    struct mock_mmio *m=ctx;
    if(!m||!value||(offset&3u)||offset>=sizeof m->regs||offset==(uint32_t)m->fail_offset)return 0;
    *value=m->regs[offset/4];m->reads++;return 1;
}
static void valid_caps(struct mock_mmio *m) {
    memset(m,0,sizeof *m);m->fail_offset=-1;
    m->regs[0]=0x01000040u;             /* xHCI 1.0, CAPLENGTH 0x40 */
    m->regs[1]=(8u<<24)|(1u<<8)|32u;   /* 8 ports, 1 interrupter, 32 slots */
    m->regs[2]=0;
    m->regs[4]=1u|4u|8u;               /* AC64, 64-byte context, PPC */
    m->regs[5]=0x2000u;                /* DBOFF */
    m->regs[6]=0x1000u;                /* RTSOFF */
}
int main(void) {
    struct mock_mmio m;struct xhci_caps c;
    struct xhci_io io={.ctx=&m,.read32=read32,.write32=0,.delay_ms=0,.aperture=0x10000};
    valid_caps(&m);
    assert(xhci_caps_read(&io,&c));
    assert(c.op==0x40&&c.rt==0x1000&&c.db==0x2000&&c.slots==32&&c.ports==8);
    assert(c.ac64&&c.context_stride==64&&c.ppc&&c.scratchpads==0&&m.reads==6);

    valid_caps(&m);m.regs[0]=0x00ff0040u;assert(!xhci_caps_read(&io,&c));
    valid_caps(&m);m.regs[0]=0x01000021u;assert(!xhci_caps_read(&io,&c));
    valid_caps(&m);m.regs[1]&=~255u;assert(!xhci_caps_read(&io,&c));
    valid_caps(&m);m.regs[1]&=~(255u<<24);assert(!xhci_caps_read(&io,&c));
    valid_caps(&m);m.regs[1]&=~(0x7ffu<<8);assert(!xhci_caps_read(&io,&c));
    valid_caps(&m);m.regs[6]=0xfffe0u;assert(!xhci_caps_read(&io,&c));
    valid_caps(&m);m.regs[5]=0x1010u;assert(!xhci_caps_read(&io,&c));
    valid_caps(&m);m.fail_offset=0x14;assert(!xhci_caps_read(&io,&c));

    puts("XHCI_CAPS_PROBE_HOST_OK: read-only CAP register probe, version/CAPLENGTH, HCS/HCC, runtime/doorbell bounds and overlap rejection");
    return 0;
}
