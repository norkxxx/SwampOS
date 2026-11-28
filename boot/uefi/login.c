#include "efi.h"
static const ch16 *volatile banner = S("SWAMP LOGIN / PREBOOT\r\nSecurity has layers.\r\n\r\n");
status EFIAPI efi_main(handle self,system_table*s){(void)self;s->out->attribute(s->out,0x0a);s->out->clear(s->out);say(s,banner);say(s,secure_state(s)?S("Firmware Secure Boot: enabled\r\n"):S("Firmware Secure Boot: NOT enforced\r\n"));say(s,S("ROOT: LOCKED\r\nHOME: LOCKED\r\nSWAMPLOGIN_LOCKED\r\nNative key backend unavailable. No credentials accepted.\r\nNo data volume unlocked. No OS handoff.\r\n"));return SECURITY_VIOLATION;}
