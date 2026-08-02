#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H


typedef enum {
    STATE_INIT,
    STATE_LOGGING,  // normal CAN intake, SD intake, and RF trasnmit
    STATE_FAULT,    
    STATE_USB       // suspends RF, enumerated as usb storage device

} state_t;

void state_machine_init(void);
void app_state_poll(void);
state_t state_get(void);



#endif /* STATE_MACHINE_H */