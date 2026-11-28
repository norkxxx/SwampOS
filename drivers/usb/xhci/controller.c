#include "controller.h"
#define HCH 1u
#define CNR (1u<<11)
#define HSE (1u<<2)
#define HCE (1u<<12)
static int rd(const struct xhci_io*i,uint32_t o,uint32_t*v){return i&&i->read32&&i->aperture>=4&&!(o&3)&&o<=i->aperture-4&&i->read32(i->ctx,o,v);}
static int wr(const struct xhci_io*i,uint32_t o,uint32_t v){return i&&i->write32&&i->aperture>=4&&!(o&3)&&o<=i->aperture-4&&i->write32(i->ctx,o,v);}
static int wr64(const struct xhci_io*i,uint32_t o,uint64_t v){return wr(i,o,(uint32_t)v)&&wr(i,o+4,(uint32_t)(v>>32));}
static int span(uint32_t off,uint32_t len,uint32_t max){return off<=max&&len<=max-off;}
int xhci_caps_read(const struct xhci_io*i,struct xhci_caps*out){uint32_t h,a,b,c,d,r;if(!out||!rd(i,0,&h)||!rd(i,4,&a)||!rd(i,8,&b)||!rd(i,0x10,&c)||!rd(i,0x14,&d)||!rd(i,0x18,&r))return 0;struct xhci_caps v={.op=h&255,.rt=r&~31u,.db=d&~3u,.slots=a&255,.ports=a>>24,.scratchpads=((b>>27)&31)|(((b>>21)&31)<<5),.ac64=c&1,.context_stride=(c&4)?64u:32u,.ext=(c>>16)*4u,.ppc=!!(c&8)};if((h>>16)<0x100||v.op<0x20||(v.op&3)||!v.slots||!v.ports||!(a&0x7ff00)||!span(v.op,0x400+16*v.ports,i->aperture)||v.rt<v.op+0x400+16*v.ports||!span(v.rt,0x40,i->aperture)||v.db<v.op+0x400+16*v.ports||!span(v.db,4,i->aperture)||!(v.db>=v.rt+0x40||v.db+4<=v.rt))return 0;*out=v;return 1;}
static int waitbits(struct xhci_controller*c,uint32_t off,uint32_t mask,uint32_t want){for(unsigned n=0;n<1000;n++){uint32_t v;if(!rd(&c->io,off,&v))return 0;if((v&mask)==want)return 1;c->io.delay_ms(c->io.ctx);}return 0;}
int xhci_controller_stop(struct xhci_controller*c){if(!c||!c->state)return 0;uint32_t op=c->caps.op;if(!wr(&c->io,op,0)||!waitbits(c,op+4,HCH,HCH))return 0;c->state=3;return 1;}
int xhci_controller_start_slots(struct xhci_controller *c,const struct xhci_io *io,
                                struct xhci_dma *m,uint64_t dma,unsigned slots){
 if(!c||c->state||!io||!io->write32||!io->delay_ms||!m||((uintptr_t)m&63)||
    !dma||(dma&4095)||dma>UINT64_MAX-sizeof(*m)||!slots||slots>2)return 0;
 struct xhci_caps caps;
 if(!xhci_caps_read(io,&caps)||slots>caps.slots||caps.scratchpads||
    !span(caps.db,4*(slots+1),io->aperture)||
    !(caps.db+4*(slots+1)<=caps.rt||caps.db>=caps.rt+0x40)||
    (!caps.ac64&&dma+sizeof(*m)>0x100000000ull))return 0;
 *c=(struct xhci_controller){.io=*io,.caps=caps,.memory=m,.state=1,.slot_limit=slots};
 uint32_t op=caps.op,ir=caps.rt+0x20,pages;
 if(!waitbits(c,op+4,CNR,0)||!xhci_controller_stop(c))return 0;
 c->state=1;
 if(!wr(io,op,2)||!waitbits(c,op,2,0)||!waitbits(c,op+4,CNR|HCH,HCH)||
    !rd(io,op+8,&pages)||!(pages&1))goto fail;
 uint64_t commands=dma+offsetof(struct xhci_dma,cmd),events=dma+offsetof(struct xhci_dma,event);
 uint64_t dcbaa=dma+offsetof(struct xhci_dma,dcbaa),erst=dma+offsetof(struct xhci_dma,erst);
 if(!xhci_cmd_init(&c->cmd,m->cmd,commands)||!xhci_event_init(&c->event,m->event,events))goto fail;
 for(unsigned n=0;n<256;n++)m->dcbaa[n]=0;
 m->erst[0]=(uint32_t)events;m->erst[1]=(uint32_t)(events>>32);m->erst[2]=64;m->erst[3]=0;
 __atomic_thread_fence(__ATOMIC_RELEASE);
 if(!wr(io,op+0x38,slots)||!wr64(io,op+0x30,dcbaa)||!wr64(io,op+0x18,commands|1)||
    !wr(io,ir+8,1)||!wr64(io,ir+0x10,erst)||!wr64(io,ir+0x18,events)||!wr(io,ir,0)||
    !wr(io,op,1)||!waitbits(c,op+4,HCH|HSE|HCE,0))goto fail;
 c->state=2;return 1;
fail:
 (void)xhci_controller_stop(c);return 0;
}
int xhci_controller_start(struct xhci_controller *c,const struct xhci_io *io,struct xhci_dma *m,uint64_t dma){
 return xhci_controller_start_slots(c,io,m,dma,1);
}

int xhci_controller_command(struct xhci_controller *c,unsigned type,unsigned slot,uint64_t arg){
 if(!c||c->state!=2||c->cmd.count||slot>c->slot_limit||(!c->caps.ac64&&arg>=0x100000000ull))return 0;
 unsigned state=xhci_slot_state(c,slot);
 if((type==9&&xhci_slots_full(c))||(type==10&&!state)||(type==11&&state!=1)||
    ((type==12||type==13)&&state!=2))return 0;
 uint64_t pointer;
 if(!xhci_cmd_submit(&c->cmd,type,slot,arg,&pointer))return 0;
 __atomic_thread_fence(__ATOMIC_RELEASE);
 if(!wr(&c->io,c->caps.db,0))goto fail;
 for(unsigned n=0;n<1000;n++){
  uint32_t status;uint64_t next;struct xhci_trb event;
  if(!rd(&c->io,c->caps.op+4,&status)||(status&(HCH|HSE|HCE)))goto fail;
  int got=xhci_event_pop(&c->event,&event,&next);
  if(got<0)goto fail;
  if(!got){c->io.delay_ms(c->io.ctx);continue;}
  if(!wr64(&c->io,c->caps.rt+0x38,next|8))goto fail;
  int done=xhci_cmd_complete(&c->cmd,&event);
  if(done!=1){if(done<0||((event.control>>10)&63)!=34)goto fail;continue;}
  unsigned id=event.control>>24;
  if(type==9){
   if(id<1||id>c->slot_limit||xhci_slot_state(c,id))goto fail;
   c->last_slot=id;
  }else if(id!=slot)goto fail;
  unsigned *target=id==2?&c->second_slot_state:&c->slot_state;
  if(type==9)*target=1;
  else if(type==11)*target=2;
  else if(type==10)*target=0;
  return 1;
 }
fail:
 c->state=1;(void)xhci_controller_stop(c);return 0;
}

int xhci_controller_noop(struct xhci_controller*c){return xhci_controller_command(c,23,0,0);}
