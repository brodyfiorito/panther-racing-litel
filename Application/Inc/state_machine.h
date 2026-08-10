#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdbool.h>


typedef enum {
    STATE_INIT,
    STATE_LOGGING,  // normal CAN ingest, SD writes, and RF trasnmit
    STATE_SHUTDOWN, // suspend RF and CAN ingest, unmount sd
    STATE_DARK,      // shutdown sequence success, see if rail comes back before caps run out
    STATE_FAULT,    
    STATE_USB       // suspends RF, enumerated as usb storage device

} state_t;

// bitmask flags for each event; allows us to conduct bitwise operations to check/clear events
typedef enum {
    EVENT_SHUTDOWN      = (1 << 0),
    EVENT_CAN_RX        = (1 << 1),
    EVENT_WATCHDOG      = (1 << 2),
} state_machine_event_t;


void state_machine_init(void);
void state_machine_poll(void);
state_t state_get(void);
void state_machine_event_set(uint32_t e);



#endif /* STATE_MACHINE_H */