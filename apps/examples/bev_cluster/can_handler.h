/****************************************************************************
 * apps/examples/bev_cluster/can_handler.h
 *
 * CAN Bus Handler - Manages CAN communication
 ****************************************************************************/

#ifndef __CAN_HANDLER_H
#define __CAN_HANDLER_H

#include <stdint.h>
#include "can_db.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Callback type for CAN data updates */

typedef void (*can_update_callback_t)(struct vehicle_data_s *vdata);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: can_handler_init
 *
 * Description:
 *   Initialize CAN handler and open CAN interface
 *
 * Input Parameters:
 *   interface - CAN interface name (e.g. "can0")
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 ****************************************************************************/

int can_handler_init(const char *interface);

/****************************************************************************
 * Name: can_handler_register_callback
 *
 * Description:
 *   Register callback to be called when vehicle data is updated
 *
 * Input Parameters:
 *   callback - Function to call with updated data
 ****************************************************************************/

void can_handler_register_callback(can_update_callback_t callback);

/****************************************************************************
 * Name: can_handler_poll
 *
 * Description:
 *   Poll CAN bus for new messages (non-blocking)
 *   Should be called periodically from main loop
 ****************************************************************************/

void can_handler_poll(void);

/****************************************************************************
 * Name: can_handler_send_button
 *
 * Description:
 *   Send button press message via CAN
 *
 * Input Parameters:
 *   button_id - Button number (0-9)
 *   action    - Action code (BTN_ACTION_xxx)
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 ****************************************************************************/

int can_handler_send_button(uint8_t button_id, uint8_t action);

/****************************************************************************
 * Name: can_handler_get_data
 *
 * Description:
 *   Get current vehicle data
 *
 * Returned Value:
 *   Pointer to vehicle data structure (read-only)
 ****************************************************************************/

const struct vehicle_data_s *can_handler_get_data(void);

#endif /* __CAN_HANDLER_H */
