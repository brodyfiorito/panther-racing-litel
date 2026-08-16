// can_relay.c
#include "can_relay.h"
#include "ring_buffer.h"
#include "main.h"
#include "fdcan.h"

#include <stdbool.h>


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

static volatile uint32_t last_announce;
static volatile bool announced = false;


/* Private Function Protoypes */
static bool announce(void);



/* Public Function Definitions*/
void can_relay_init(void) {

    announce();
    last_announce = HAL_getTick();



}

void can_relay_poll(void) {

    bool bus_ok = can_check_bus_off();

    if (!announced) {
        if (rx_seen) {
            announced = true;
        } else if (bus_ok && HAL_GetTick() - last_announce >= ANNOUNCE_PERIOD_MS) {
            (void)announce();
            last_announce = HAL_GetTick();
        }

    }

    if (!bus_ok) {
        return;
    }

    if (check_mailbox()) {
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, );


    }

}



/* Private Helper Functions */

static bool announce(void) {   // Tells the VCU that the datalogger is installed, and should start sending messages

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &announce_header, announce_data) != HAL_OK) {
        return false;
    }
    
    return true;
}

static void can_relay_check_bus_off(void) {
    FDCAN_ProtocolStatusTypeDef ps;
    HAL_FDCAN_GetProtocolStatus(&hfdcan1, &ps);

    if (ps.BusOff) {
        // clearing INIT restarts the 128x11-bit recovery sequence
        CLEAR_BIT(hfdcan1.Instance->CCCR, FDCAN_CCCR_INIT);
    }
}

static bool check_mailbox(void) { // Returns true if theres a message waiting to be relayed




}