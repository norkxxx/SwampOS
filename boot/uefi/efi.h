/* Minimal UEFI x64 ABI declarations. No firmware library dependency. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#define EFIAPI __attribute__((ms_abi))
typedef uint64_t status;
typedef void *handle;
typedef uint16_t ch16;
#define ERR(n) (0x8000000000000000ULL|(n))
#define EFI_ERROR(s) ((s)>>63)
#define SUCCESS 0
#define NOT_FOUND ERR(14)
#define SECURITY_VIOLATION ERR(26)
#define INVALID_PARAMETER ERR(2)
#define UNSUPPORTED ERR(3)
typedef struct {uint32_t a;uint16_t b,c;uint8_t d[8];} guid;
typedef struct {uint64_t signature;uint32_t revision,header_size,crc32,reserved;} hdr;
typedef struct textout textout;
struct textout {void *reset;status(EFIAPI *output)(textout*,const ch16*);void *test,*query,*setmode;status(EFIAPI *attribute)(textout*,size_t);status(EFIAPI *clear)(textout*);void *cursor,*enable,*mode;};
typedef struct {hdr h;void *get_time,*set_time,*get_wakeup,*set_wakeup,*set_virtual,*convert;status(EFIAPI *get_variable)(const ch16*,const guid*,uint32_t*,size_t*,void*);} runtime;
typedef struct {hdr h;void *raise,*restore;status(EFIAPI *alloc_pages)(uint32_t,uint32_t,size_t,uint64_t*);status(EFIAPI *free_pages)(uint64_t,size_t);status(EFIAPI *get_map)(size_t*,void*,size_t*,size_t*,uint32_t*);status(EFIAPI *allocate_pool)(uint32_t,size_t,void**);status(EFIAPI *free_pool)(void*);void *create_event,*set_timer,*wait,*signal,*close,*check,*install,*reinstall,*uninstall;status(EFIAPI *handle_protocol)(handle,const guid*,void**);void *reserved,*register_notify,*locate_handle,*locate_device_path,*install_config;status(EFIAPI *load_image)(uint8_t,handle,void*,void*,size_t,handle*);status(EFIAPI *start_image)(handle,size_t*,ch16**);void *exit;status(EFIAPI *unload_image)(handle);status(EFIAPI *exit_boot)(handle,size_t);void *monotonic;status(EFIAPI *stall)(size_t);void *watchdog,*connect,*disconnect,*open,*close_protocol,*open_info,*protocols_per_handle;status(EFIAPI *locate_handle_buffer)(uint32_t,const guid*,void*,size_t*,handle**);status(EFIAPI *locate_protocol)(const guid*,void*,void**);} boot;
typedef struct {hdr h;ch16 *vendor;uint32_t revision;handle cin;void *in;handle cout;textout *out;handle cerr;textout *err;runtime *rt;boot *bs;size_t entries;void *config;} system_table;
typedef struct {uint32_t revision;handle parent;system_table *system;handle device;void *path;} loaded_image;
_Static_assert(offsetof(boot,exit_boot)==232,"UEFI ExitBootServices ABI");
_Static_assert(offsetof(boot,locate_protocol)==320,"UEFI LocateProtocol ABI");
_Static_assert(offsetof(boot,load_image)==200,"UEFI boot ABI");
_Static_assert(offsetof(boot,locate_handle_buffer)==312,"UEFI boot ABI");
_Static_assert(offsetof(system_table,bs)==96,"UEFI system table ABI");
#define S(x) ((const ch16*)L##x)
static const guid global_var={0x8be4df61,0x93ca,0x11d2,{0xaa,0x0d,0x00,0xe0,0x98,0x03,0x2b,0x8c}};
static const guid loaded_guid={0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static const guid dp_guid={0x09576e91,0x6d3f,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static const guid fs_guid={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static inline void say(system_table *s,const ch16 *t){s->out->output(s->out,t);}
static inline int secure_state(system_table *s){uint8_t sb=0,setup=1;size_t n=1;status r=s->rt->get_variable(S("SecureBoot"),&global_var,0,&n,&sb);if(r||n!=1||sb!=1)return 0;n=1;r=s->rt->get_variable(S("SetupMode"),&global_var,0,&n,&setup);return !r&&n==1&&setup==0;}
