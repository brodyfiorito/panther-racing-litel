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
static void state_machine_update(void);
static void state_machine_transition(state_t next_state);
static void can_suspend(void);
static void can_resume(void);
static bool power_recovered(void);


/* Event Handling */
static _Atomic uint32_t pending_events = 0;

static inline void event_set(uint32_t e)  { __atomic_fetch_or(&pending_events, e, __ATOMIC_RELAXED); }
static inline uint32_t event_take(uint32_t e) { return __atomic_fetch_and(&pending_events, ~e, __ATOMIC_RELAXED) & e; }



/* Private Variables */
static state_t state = STATE_INIT;
static ring_buffer_t can_rb;

static volatile uint8_t shutdown_possible_flag = 0;
static volatile uint32_t shutdown_isr_tick = 0;



/* Interrupt Callback*/

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == PWR_STATUS_Pin && !shutdown_possible_flag) {
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
                state_machine_transition(STATE_USB);
            }

            break;

        case STATE_USB:
            usb_handler_poll();
            if (!usb_is_enumerated()) {
                if (HAL_GPIO_ReadPin(PWR_STATUS_GPIO_Port, PWR_STATUS_Pin) != GPIO_PIN_SET) {
                    state_machine_transition(STATE_DARK);   // card already unmounted
                    break;
                }
                sd_logger_status_t sd_status = sd_logger_init(&can_rb);
                if (sd_status != SD_LOGGER_OK) {
                    state_machine_transition(STATE_FAULT);
                } else {
                    rf_telemetry_resume();
                    can_resume();
                    can_ingest_flush_rx();
                    state_machine_transition(STATE_LOGGING);
                }
            }

            break;

        case STATE_FAULT:

            break;

        case STATE_SHUTDOWN:
            sd_logger_flush();
            (void)sd_unmount();
            state_machine_transition(STATE_DARK);

            break;
        
        case STATE_DARK:
            if (usb_is_enumerated()) {
                state_machine_transition(STATE_USB);
                break;
            }

            if (power_recovered()) {
                sd_logger_status_t sd_status = sd_logger_init(&can_rb);
                shutdown_possible_flag = 0;
                (void)event_take(EVENT_SHUTDOWN);
                if (sd_status != SD_LOGGER_OK) {
                    state_machine_transition(STATE_FAULT);
                } else {
                    rf_telemetry_resume();
                    can_resume();
                    can_ingest_flush_rx();
                    state_machine_transition(STATE_LOGGING);
                    log_write(get_log_time(), LOG_WARN, "STATE", "Power recovered, returning to logging");
                }
            }

            break;

        default:
            log_write(get_log_time(), LOG_FAULT, "STATE", "Unhandled state: %u", (unsigned)state);
            state_machine_transition(STATE_FAULT);
            break;

    }


}

state_t state_get(void) {
    return state;
}



/* Private Helpers */

static void state_machine_update(void) {

    /* power status pin hysteresis */
    if (shutdown_possible_flag) {
        if ((HAL_GetTick() - shutdown_isr_tick) > STATUS_PIN_HYST_MS) {
            if (HAL_GPIO_ReadPin(PWR_STATUS_GPIO_Port, PWR_STATUS_Pin) == GPIO_PIN_RESET) {
                event_set(EVENT_SHUTDOWN);
            }
            shutdown_possible_flag = 0;
        }
    }

    if (event_take(EVENT_SHUTDOWN)) {
        if (state == STATE_LOGGING) {
            state_machine_transition(STATE_SHUTDOWN);
        }
    }
}

static void state_machine_transition(state_t next_state) {
    state = next_state;
    switch (next_state) {
    case STATE_SHUTDOWN:
        can_suspend();
        rf_telemetry_suspend();
        log_write(get_log_time(), LOG_WARN, "STATE", "brownout, draining buffer and unmounting...");
        break;
    case STATE_FAULT:
        log_write(get_log_time(), LOG_FAULT, "STATE", "entered fault");
        break;
    case STATE_USB:
        rf_telemetry_suspend();
        can_suspend();
        sd_power_on();
        sd_logger_flush();
        (void)sd_unmount();
        log_write(get_log_time(), LOG_WARN, "STATE", "entered usb mode");
        break;
    case STATE_DARK:
        sd_power_off();
        break;
    default:
        break;
    }
}

static void can_suspend(void) {
    // set can transciever to standby


}

static void can_resume(void) {
    // set can transciever to active


}

static bool power_recovered(void) {


    if (HAL_GPIO_ReadPin(PWR_STATUS_GPIO_Port, PWR_STATUS_Pin) != GPIO_PIN_SET) {
        return false;
    }

    uint64_t rail_high_since = get_log_time();
    while ((get_log_time() - rail_high_since) < RECOVERY_HYST_US) {
        if (HAL_GPIO_ReadPin(PWR_STATUS_GPIO_Port, PWR_STATUS_Pin) != GPIO_PIN_SET) {
            return false;
        }
    }

    return true;
}