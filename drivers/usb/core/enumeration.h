#pragma once
#include "../hid/boot_hid.h"
/* Caller has already reset and addressed a USB 1.x/2.x device, configured EP0,
 * and exclusively owns its control endpoint. Callback enforces deadline and
 * returns a completed transfer with actual byte count (0 for status-only). */
typedef int (*usb_control_fn)(void*,const uint8_t setup[8],uint8_t*,size_t,size_t*);
struct usb_enumerated {uint16_t vendor,product,bcd_usb;uint8_t ep0_size;struct usb_boot_config config;};
/* 1 configured; 0 read-only rejection; -1 configuration may have changed,
 * caller must reset the port before reuse. No endpoint data transfers here. */
int usb_enumerate_boot(usb_control_fn,void*,uint8_t*,size_t,struct usb_enumerated*);
int usb_enum_selftest(void);
