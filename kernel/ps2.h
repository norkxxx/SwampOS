#pragma once
#include <stdint.h>
struct ps2_mouse {uint8_t packet[3],used;};
struct mouse_event {int dx,dy;unsigned buttons;};
int ps2_mouse_decode(struct ps2_mouse*,uint8_t,struct mouse_event*);
/* Bit 0 keyboard ACK, bit 1 mouse ACK. Polling, no IRQ1/IRQ12. */
unsigned ps2_init(int mouse);
int ps2_poll(struct mouse_event*,int *key);
