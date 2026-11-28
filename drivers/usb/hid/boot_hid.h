#pragma once
#include <stddef.h>
#include <stdint.h>
#define USB_BOOT_HID_MAX 8u
struct usb_boot_interface {uint8_t number,protocol,endpoint,interval;uint16_t packet_size;};
struct usb_boot_config {uint8_t configuration,count;struct usb_boot_interface interfaces[USB_BOOT_HID_MAX];};
/* Strict bounded USB 2.0 boot-protocol subset. Returns 1 on structurally valid config. */
int usb_boot_config_parse(const uint8_t*,size_t,struct usb_boot_config*);
struct hid_keyboard {uint8_t previous[8],caps;};
struct hid_keys {unsigned count;int keys[6];};
/* Reports in boot protocol only, not report IDs / NKRO. Emits newly pressed keys. */
int hid_keyboard_report(struct hid_keyboard*,const uint8_t*,size_t,struct hid_keys*);
struct hid_mouse {int dx,dy;unsigned buttons;};
int hid_mouse_report(const uint8_t*,size_t,struct hid_mouse*);
int usb_hid_selftest(void);
