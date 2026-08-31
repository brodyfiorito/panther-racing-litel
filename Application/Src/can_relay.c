// can_relay.c
#include "can_relay.h"
#include "main.h"
#include "fdcan.h"



/* TODO:
    - 

*/

/* Private Variables */
static FDCAN_TxHeaderTypeDef announce_header = {
    .Identifier          = CAN_ID_ANNOUNCE,      // defined in can_relay.h
    .IdType              = FDCAN_STANDARD_ID,
    .TxFrameType         = FDCAN_DATA_FRAME,
    .DataLength          = FDCAN_DLC_BYTES_8,
    .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
    .BitRateSwitch       = FDCAN_BRS_OFF,
    .FDFormat            = FDCAN_CLASSIC_CAN,
    .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
    .MessageMarker       = 0,
};

static uint8_t announce_data[8] = { 0xFF };
static uint32_t last_announce;
static bool announced = false;

static ring_buffer_t mailbox;

static FDCAN_TxHeaderTypeDef relay_header = {
    .IdType              = FDCAN_STANDARD_ID,
    .TxFrameType         = FDCAN_DATA_FRAME,
    .DataLength          = FDCAN_DLC_BYTES_0,
    .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
    .BitRateSwitch       = FDCAN_BRS_OFF,
    .FDFormat            = FDCAN_CLASSIC_CAN,
    .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
    .MessageMarker       = 0,
};

static const uint32_t dlc_lut[9] = {
    FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2,
    FDCAN_DLC_BYTES_3, FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7, FDCAN_DLC_BYTES_8,
};

static volatile can_relay_stats_t stats;
static uint8_t tx_fail_run;
static bool bus_off;
static uint32_t bus_off_entry_tick;
static uint32_t next_attempt_tick;
static uint32_t window_start_tick;
static uint32_t attempts_in_window;


/* Private Function Protoypes */
static bool announce(void);
static bool check_can_bus_ok(void);



/* Public Function Definitions*/


void can_relay_init(void) {

    ring_buffer_init(&mailbox);

    memset(&stats, 0, sizeof(stats));
    tx_fail_run = 0u;
    bus_off = false;
    attempts_in_window = 0u;
    window_start_tick = HAL_GetTick();

    last_announce = HAL_GetTick() - ANNOUNCE_PERIOD_MS;

}

void can_relay_poll(void) {

    if (!check_can_bus_ok()) {
        return;
    }

    if (!announced) {
        if (rx_seen) {
            announced = true;
        } else if (HAL_GetTick() - last_announce >= ANNOUNCE_PERIOD_MS) {
            (void)announce();
            last_announce = HAL_GetTick();
        }

    }

    uint32_t budget = HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1);

    can_frame_t relay_frame;
    while (budget-- && ring_buffer_peek(&mailbox, &relay_frame)) {
        relay_header.Identifier = relay_frame.id & 0x7FFu;
        relay_header.DataLength = dlc_lut[(relay_frame.len > 8u) ? 8u : relay_frame.len];
        if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &relay_header, relay_frame.data) == HAL_OK) {
            ring_buffer_drop(&mailbox);
            stats.tx_ok++;
            tx_fail_run = 0u;
            continue;
        }

        // something is erroring
        stats.tx_fail++;
        if (tx_fail_run < UINT8_MAX) {
            tx_fail_run++;
        }
        if (tx_fail_run > stats.tx_max_fail_run) {
            stats.tx_max_fail_run = tx_fail_run;
        }

        if (tx_fail_run >= CAN_RELAY_MAX_TX_FAILS) {
            ring_buffer_drop(&mailbox);      // head is probably bad, no reason to not delete
            stats.tx_fail_drops++;
            tx_fail_run = 0u;
        }

        break;   // peripheral is unhappy; stop hammering it with this poll
    }

}

void can_relay_submit(uint32_t id, const uint8_t *data, uint8_t len) {
    ring_buffer_push(&mailbox, id, data, len);
}

void can_relay_get_stats(can_relay_stats_t *out) {
    if (out == NULL) return;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    *out = stats;
    __set_PRIMASK(primask);
}


/* Private Helper Functions */

static bool announce(void) {   // Tells the VCU that the datalogger is installed, and should start sending messages

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &announce_header, announce_data) != HAL_OK) {
        return false;
    }
    
    return true;
}

static bool check_can_bus_ok(void) {
    FDCAN_ProtocolStatusTypeDef ps;
    if (HAL_FDCAN_GetProtocolStatus(&hfdcan1, &ps) != HAL_OK) {
        return false;
    }

    if (!bus_off && (hfdcan1.Instance->CCCR & FDCAN_CCCR_INIT)) {
    stats.not_started = true;    /* INIT set but no bus-off — never started */
    return false;
}

    if (!ps.BusOff) {
        if (bus_off) {                       /* recovery completed */
            bus_off = false;
            stats.bus_off_ms_total += HAL_GetTick() - bus_off_entry_tick;
            stats.bus_off_ms_current = 0u;
            tx_fail_run = 0u;                   /* not the frame's fault */
        }
        return true;
    }

    uint32_t now = HAL_GetTick();

    if (!bus_off) {                          /* --- entry transition --- */
        bus_off = true;
        bus_off_entry_tick = now;
        stats.bus_off_events++;
        tx_fail_run = 0u;
        next_attempt_tick = now;                /* first attempt immediately */
    }

    stats.bus_off_ms_current = now - bus_off_entry_tick;

    if (stats.bus_off_latched) {
        return false;
    }
    if ((hfdcan1.Instance->CCCR & FDCAN_CCCR_INIT) == 0u) {
        return false;                           // sequence running, just wait
    }
    if ((int32_t)(now - next_attempt_tick) < 0) {
        return false;                           // backoff
    }

    if (now - window_start_tick > CAN_RELAY_BUS_OFF_WINDOW_MS) {
        window_start_tick = now;
        attempts_in_window = 0u;
    }
    if (++attempts_in_window > CAN_RELAY_BUS_OFF_MAX_ATTEMPTS) {
        stats.bus_off_latched = true;           // give up, bus probably broken wait for power cycle
        return false;
    }

    CLEAR_BIT(hfdcan1.Instance->CCCR, FDCAN_CCCR_INIT);
    stats.recovery_attempts++;
    next_attempt_tick = now + CAN_RELAY_BUS_OFF_BACKOFF_MS;
    return false;
}