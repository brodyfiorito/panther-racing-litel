#ifndef USB_HANDLER_H
#define USB_HANDLER_H

#include <stdbool.h>


void usb_handler_init(void);
void usb_handler_poll(void);

bool usb_is_enumerated(void);


#endif /* USB_HANDLER_H */