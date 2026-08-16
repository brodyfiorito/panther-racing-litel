#ifndef CAN_RELAY_H
#define CAN_RELAY_H

#include <stdbool.h>

#define CAN_ID_ANNOUNCE 0x000
#define ANNOUNCE_PERIOD_MS 250

volatile bool rx_seen = false;

void can_relay_init(void);
void can_relay_poll(void);


#endif /* CAN_RELAY_H */