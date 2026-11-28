#pragma once
#include "ps2.h"
void usb_input_auto(void);
void usb_input_start(void);
void usb_input_stop(void);
void usb_input_status(void);
int usb_input_poll(int *key,struct mouse_event *mouse);
