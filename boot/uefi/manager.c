#include "efi.h"
#include "path.h"
#ifndef LAB_MODE
#define LAB_MODE 0
#endif
static const ch16 *volatile banner = S("\r\nSWAMPBOOT 0.7.0 / x64 UEFI\r\n");
status EFIAPI efi_main(handle self,system_table*s){
 say(s,banner);
 if(!secure_state(s)){say(s,S("Secure Boot not enforced.\r\n"));if(!LAB_MODE)return SECURITY_VIOLATION;say(s,S("LAB MODE ONLY - NO TRUST GUARANTEE\r\n"));}
 loaded_image*im=0;uint8_t*origin=0;struct disk_path op;
 if(s->bs->handle_protocol(self,&loaded_guid,(void**)&im)||!im||s->bs->handle_protocol(im->device,&dp_guid,(void**)&origin)||!parse_disk_path(origin,4096,&op)){say(s,S("BOOT_SOURCE_REJECTED\r\n"));return UNSUPPORTED;}
 handle*hs=0;size_t n=0;if(s->bs->locate_handle_buffer(2,&fs_guid,0,&n,&hs))return NOT_FOUND;
 uint8_t*selected=0;struct disk_path sp;size_t matches=0;
 for(size_t i=0;i<n;i++){uint8_t*p=0;struct disk_path q;if(!s->bs->handle_protocol(hs[i],&dp_guid,(void**)&p)&&parse_disk_path(p,4096,&q)&&same_disk(origin,&op,p,&q)&&login_partition(p,&q)){selected=p;sp=q;matches++;}}
 s->bs->free_pool(hs);if(matches!=1){say(s,S("LOGIN_PARTITION_REJECTED\r\n"));return SECURITY_VIOLATION;}
 uint8_t path[4096];if(!append_login_path(path,sizeof(path),selected,&sp))return INVALID_PARAMETER;
 handle child=0;status r=s->bs->load_image(0,self,path,0,0,&child);if(r){if(child)s->bs->unload_image(child);say(s,S("LOGIN_IMAGE_REJECTED\r\n"));return r;}
 say(s,S("LOGIN_CHAINLOAD\r\n"));r=s->bs->start_image(child,0,0);s->bs->unload_image(child);say(s,S("RETURNED LOCKED - NO KERNEL HANDOFF\r\n"));return r?r:SECURITY_VIOLATION;
}
