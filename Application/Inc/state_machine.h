#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdbool.h>


typedef enum {
    STATE_INIT,
    STATE_LOGGING,  // normal CAN ingest, SD writes, and RF trasnmit
    STATE_SHUTDOWN, // suspend RF and CAN ingest, unmount sd
    STATE_OFF,      // shutdown sequence success
    STATE_FAULT,    
    STATE_USB       // suspends RF, enumerated as usb storage device

} state_t;

void state_machine_init(void);
void state_machine_poll(void);
state_t state_get(void);



#endif /* STATE_MACHINE_H */