// can_ingest.c
#include "can_ingest.h"
#include "fdcan.h"
#include "main.h"
#include "ring_buffer.h"
#include "can_relay.h"

#include <string.h>


/* Private Variables */
static can_stats_t stats;
static ring_buffer_t *s_rb;
static FDCAN_HandleTypeDef *s_hfdcan;


/* Private Function Protoypes */
static volatile uint32_t s_frames_rx;
static volatile uint32_t s_frames_dropped;



/* Public Function Definitions*/
void can_ingest_init(FDCAN_HandleTypeDef *hfdcan, ring_buffer_t *rb) {

    s_hfdcan = hfdcan;
    s_rb = rb;
    s_frames_rx      = 0u;
    s_frames_dropped = 0u;

    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, 
    FDCAN_ACCEPT_IN_RX_FIFO0,   // non-matching standard IDs
    FDCAN_ACCEPT_IN_RX_FIFO0,   // non-matching extended IDs
    FDCAN_REJECT_REMOTE,
    FDCAN_REJECT_REMOTE);

    HAL_FDCAN_ActivateNotification(&hfdcan1,
    FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO0_MESSAGE_LOST, 0);
    HAL_FDCAN_Start(&hfdcan1);

    

}

void can_ingest_poll(void) {




    can_ingest_stats(&stats);
}

void can_ingest_stats(can_stats_t *out) {
    out->frames_rx = s_frames_rx;
    out->frames_dropped = s_frames_dropped;
}

/* Private Helper Functions */

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan != s_hfdcan) {
        return;
    }

    if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_MESSAGE_LOST) {
        s_frames_dropped++;
    }
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0u) {
        return;
    }

    /* Drain the whole FIFO per entry, not one frame per interrupt. */
    while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0u) {
        FDCAN_RxHeaderTypeDef hdr;

        /* 64 bytes because the HAL writes DataLength bytes and CAN2.0 allows 64.
           Zeroed so short frames don't relay stack garbage in the unused
           tail — ring_buffer_push always copies 8. */
        uint8_t data[64] = { 0 };

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &hdr, data) != HAL_OK) {
            break;
        }

        ring_buffer_push(s_rb, hdr.Identifier, data);
        s_frames_rx++;
        rx_seen = true;
    }
}