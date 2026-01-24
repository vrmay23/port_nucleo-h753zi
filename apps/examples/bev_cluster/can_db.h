/****************************************************************************
 * apps/examples/bev_cluster/can_db.h
 *
 * CAN Database - Message and Signal Definitions
 ****************************************************************************/

#ifndef __CAN_DB_H
#define __CAN_DB_H

#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Vehicle data structure - contains all decoded CAN signals */

struct vehicle_data_s
{
  /* Signal 0: Speed */

  uint8_t speed_kmh;

  /* Signal 1-2: Battery voltage (16-bit, units: 0.1V) */

  uint16_t battery_voltage;

  /* Signal 3-4: Battery current (16-bit, units: 0.1A, signed) */

  int16_t battery_current;

  /* Signal 5: Direction (0=Forward, 1=Reverse, 2=Neutral) */

  uint8_t direction;

  /* Signal 6: Ambient temperature */

  int8_t temperature_c;

  /* Signal 7-9: Reserved for future use */

  uint16_t reserved_signal_7;
  uint16_t reserved_signal_8;
  uint16_t reserved_signal_9;

  /* Computed value: Power (Voltage * Current) */

  float power_kw;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: can_db_decode_speed
 *
 * Description:
 *   Decode speed message (CAN_MSG_SPEED)
 *
 * Input Parameters:
 *   data   - CAN frame data bytes
 *   vdata  - Vehicle data structure to update
 ****************************************************************************/

void can_db_decode_speed(const uint8_t *data,
                         struct vehicle_data_s *vdata);

/****************************************************************************
 * Name: can_db_decode_battery
 *
 * Description:
 *   Decode battery message (CAN_MSG_BATTERY)
 *
 * Input Parameters:
 *   data   - CAN frame data bytes
 *   vdata  - Vehicle data structure to update
 ****************************************************************************/

void can_db_decode_battery(const uint8_t *data,
                           struct vehicle_data_s *vdata);

/****************************************************************************
 * Name: can_db_decode_temperature
 *
 * Description:
 *   Decode temperature message (CAN_MSG_TEMPERATURE)
 *
 * Input Parameters:
 *   data   - CAN frame data bytes
 *   vdata  - Vehicle data structure to update
 ****************************************************************************/

void can_db_decode_temperature(const uint8_t *data,
                               struct vehicle_data_s *vdata);

/****************************************************************************
 * Name: can_db_encode_button
 *
 * Description:
 *   Encode button press into CAN message
 *
 * Input Parameters:
 *   button_id - Button number (0-9)
 *   action    - Action code (BTN_ACTION_xxx)
 *   data      - Output buffer (8 bytes)
 ****************************************************************************/

void can_db_encode_button(uint8_t button_id, uint8_t action,
                          uint8_t *data);

#endif /* __CAN_DB_H */
