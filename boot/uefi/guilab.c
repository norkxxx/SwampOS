/* Standalone unsigned firmware graphics lab. No disk access, kernel handoff or authentication. */
#include "efi.h"
#include "../../gui/surface.h"
static const guid gop_guid={0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
struct gop_info {uint32_t version,width,height,format,masks[4],stride;};
struct gop_mode {uint32_t max,mode;struct gop_info *info;size_t size;uint64_t fb;size_t fbsize;};
struct gop {void *query,*set;status(EFIAPI *blt)(struct gop*,void*,uint32_t,size_t,size_t,size_t,size_t,size_t,size_t,size_t);struct gop_mode *mode;};
static uint32_t pixels[640*360];
static uint32_t *volatile canvas=pixels;
status EFIAPI efi_main(handle self,system_table*s){(void)self;uint8_t sb=1;size_t n=1;if(s->rt->get_variable(S("SecureBoot"),&global_var,0,&n,&sb)||n!=1||sb!=0)return SECURITY_VIOLATION;
 struct gop*g=0;status r=s->bs->locate_protocol(&gop_guid,0,(void**)&g);if(r)return r;if(!g||!g->blt||!g->mode||!g->mode->info||g->mode->size<sizeof(struct gop_info))return UNSUPPORTED;
 unsigned w=g->mode->info->width,h=g->mode->info->height;if(w<640||h<360||w>16384||h>16384)return UNSUPPORTED;
 struct surface v={canvas,640*360,640,360,640};if(!swamp_desktop(&v))return INVALID_PARAMETER;
 /* EFI_GRAPHICS_OUTPUT_BLT_PIXEL is B,G,R,reserved in memory, matching little endian 0x00RRGGBB. */
 r=g->blt(g,canvas,2,0,0,(w-640)/2,(h-360)/2,640,360,640*4);if(r)return r;
 /* Stay on the static preview, firmware reboot returns to menu. No credentials or input accepted. */
 for(;;)__asm__ volatile("pause");
}
