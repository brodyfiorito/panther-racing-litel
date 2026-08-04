// state_machine.c
#include "state_machine.h"
#include "app_config.h"
#include "can_ingest.h"
#include "sd_logger.h"
#include "rf_telemetry.h"
#include "usb_handler.h"
#include "ring_buffer.h"
#include "gpio.h"


/* Private Helper Prototypes */
void state_machine_update(void);
void state_machine_transition(state_t);




/* Private Variables */
static state_t state = STATE_INIT;
static ring_buffer_t can_rb;

static volatile uint8_t shutdown_possible_flag = 0;
static volatile uint32_t shutdown_isr_tick = 0;


// bitmask flags for each event; allows us to conduct bitwise operations to check/clear events
typedef enum {
    EVENT_SHUTDOWN      = (1 << 0),
    EVENT_CAN_RX        = (1 << 1),
    EVENT_WATCHDOG      = (1 << 2),
} state_machine_event_t;
static volatile uint32_t pending_events = 0;



/* Interrupt Callback*/

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == PWR_STATUS_Pin) {
        shutdown_isr_tick = HAL_GetTick();
        shutdown_possible_flag = 1;
    }
}


/* Public Functions */

void state_machine_init(void) {

    ring_buffer_init(&can_rb);
    can_ingest_init(&can_rb);
    sd_logger_status_t sd_status = sd_logger_init(&can_rb);
    rf_telemetry_init();
    usb_handler_init();


    state = (sd_status != SD_LOGGER_OK) ? STATE_FAULT : STATE_LOGGING;


}

void state_machine_poll(void) {

    state_machine_update();

    switch(state) {

        case STATE_LOGGING:
            can_ingest_poll();
            sd_logger_poll();
            rf_telemetry_poll();
            if (usb_is_enumerated()) {
                rf_telemetry_suspend();
                state = STATE_USB;
            }

            break;

        case STATE_USB:
            usb_handler_poll();
            if (!usb_is_enumerated()) {
                rf_telemetry_resume();
                state = STATE_LOGGING;
            }

            break;

        case STATE_SHUTDOWN:
            rf_telemetry_suspend();
            if (!sd_unmount()) {
                state = STATE_OFF;
            }

            break;

        case STATE_FAULT:



            break;

        case STATE_OFF:



            break;

        default:
            state = STATE_FAULT;
            break;

    }


}

state_t state_get(void) {
    return state;
}



/* Private Helpers */

void state_machine_update(void) {

    // power status pin hysteresis
    if (shutdown_possible_flag) {
        if((HAL_GetTick() - shutdown_isr_tick) > STATUS_PIN_HYST_MS) {
            // recheck pin status
            if (HAL_GPIO_ReadPin(PWR_STATUS_GPIO_Port, PWR_STATUS_Pin) == GPIO_PIN_RESET) {
                pending_events |= EVENT_SHUTDOWN;
            }
            shutdown_possible_flag = 0;
        }
    }

    if (pending_events & EVENT_SHUTDOWN) {
        pending_events &= ~EVENT_SHUTDOWN;
        state_machine_transition(STATE_SHUTDOWN);
    }

}

void state_machine_transition(state_t next_state) {
    state = next_state;
}