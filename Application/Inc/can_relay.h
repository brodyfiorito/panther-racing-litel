#ifndef CAN_RELAY_H
#define CAN_RELAY_H

#include <ring_buffer.h>

#include <stdbool.h>

// Announce related
#define CAN_ID_ANNOUNCE 0x000
#define ANNOUNCE_PERIOD_MS 250u

// Bus recovery releated
#define CAN_RELAY_BUS_OFF_WINDOW_MS 100u
#define CAN_RELAY_BUS_OFF_MAX_ATTEMPTS 10000u
#define CAN_RELAY_BUS_OFF_BACKOFF_MS 32u

// Peripheral related
#define CAN_RELAY_MAX_TX_FAILS 3u

typedef struct {
    // Message/peripheral related
    uint16_t tx_ok;                 // frames handed to the TX FIFO
    uint16_t tx_fail;               // AddMessageToTxFifoQ errors with space available
    uint16_t tx_fail_drops;         // head frames discarded after MAX_TX_FAILS
    uint16_t tx_max_fail_run;       // worst consecutive-failure run seen
    uint16_t id_rejects;            // frames dropped for out-of-range ID

    // CAN bus health related
    uint16_t recovery_attempts;     // bus-off recovery attempts
    bool bus_off_latched;           // 1 if recovery attempts past CAN_RELAY_BUS_OFF_MAX_ATTEMPT
    uint16_t bus_off_events;        // how many times
    uint32_t bus_off_ms_total;      // total bus off time
    uint32_t bus_off_ms_current;    // how long has the bus been off, active
    bool not_started;               // 1 if bus was never started


} can_relay_stats_t;

extern volatile bool rx_seen = false;

void can_relay_init(void);
void can_relay_poll(void);
void can_relay_submit(uint32_t id, const uint8_t *data, uint8_t len);
void can_relay_get_stats(can_relay_stats_t *out);


#endif /* CAN_RELAY_H */