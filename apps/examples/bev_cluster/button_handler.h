/****************************************************************************
 * apps/examples/bev_cluster/button_handler.h
 *
 * Button Handler - Manages button inputs and callbacks
 ****************************************************************************/

#ifndef __BUTTON_HANDLER_H
#define __BUTTON_HANDLER_H

#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Callback type for button press events */

typedef void (*button_callback_t)(uint8_t button_id);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: button_handler_init
 *
 * Description:
 *   Initialize button handler
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 ****************************************************************************/

int button_handler_init(void);

/****************************************************************************
 * Name: button_handler_register_callback
 *
 * Description:
 *   Register callback for button press events
 *
 * Input Parameters:
 *   callback - Function to call when button is pressed
 ****************************************************************************/

void button_handler_register_callback(button_callback_t callback);

/****************************************************************************
 * Name: button_handler_poll
 *
 * Description:
 *   Poll button states (non-blocking)
 *   Should be called periodically from main loop
 ****************************************************************************/

void button_handler_poll(void);

/****************************************************************************
 * Name: button_handler_get_count
 *
 * Description:
 *   Get number of buttons detected
 *
 * Returned Value:
 *   Number of buttons available
 ****************************************************************************/

int button_handler_get_count(void);

#endif /* __BUTTON_HANDLER_H */
