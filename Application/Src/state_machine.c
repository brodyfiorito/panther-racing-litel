// state_machine.c
#include "state_machine.h"
#include "app_config.h"
#include "can_ingest.h"
#include "sd_logger.h"
#include "rf_telemetry.h"
#include "usb_handler.h"
#include "ring_buffer.h"


/* Private Variables */
static state_t state = STATE_INIT;
static ring_buffer_t can_rb;




/* Public Function Defs */

void state_machine_init(void) {

    ring_buffer_init(&can_rb);
    can_ingest_init(&can_rb);
    sd_logger_status_t sd_status = sd_logger_init(&can_rb);
    rf_telemetry_init();
    usb_handler_init();


    state = (sd_status != SD_LOGGER_OK) ? STATE_FAULT : STATE_LOGGING;


}