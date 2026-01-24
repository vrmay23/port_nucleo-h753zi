/****************************************************************************
 * apps/examples/bev_cluster/can_db.c
 ****************************************************************************/

#include "can_db.h"
#include "app_config.h"
#include <string.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void can_db_decode_speed(const uint8_t *data,
                         struct vehicle_data_s *vdata)
{
  /* Extract speed signal using defines */

  vdata->speed_kmh = data[SIGNAL_SPEED_BYTE];
}

void can_db_decode_battery(const uint8_t *data,
                           struct vehicle_data_s *vdata)
{
  /* Extract voltage (16-bit, big-endian, units: 0.1V) */

  vdata->battery_voltage = (data[SIGNAL_BATTERY_V_BYTE] << 8) |
                           data[SIGNAL_BATTERY_V_BYTE + 1];

  /* Extract current (16-bit, big-endian, signed, units: 0.1A) */

  vdata->battery_current = (int16_t)((data[SIGNAL_BATTERY_I_BYTE] << 8) |
                                      data[SIGNAL_BATTERY_I_BYTE + 1]);

  /* Extract direction */

  vdata->direction = data[SIGNAL_DIRECTION_BYTE];

  /* Compute power = V * I (in kW) */

  float voltage = vdata->battery_voltage * 0.1f;
  float current = vdata->battery_current * 0.1f;
  vdata->power_kw = (voltage * current) / 1000.0f;
}

void can_db_decode_temperature(const uint8_t *data,
                               struct vehicle_data_s *vdata)
{
  /* Extract temperature signal using define */

  vdata->temperature_c = (int8_t)data[SIGNAL_TEMP_BYTE];
}

void can_db_encode_button(uint8_t button_id, uint8_t action,
                          uint8_t *data)
{
  memset(data, 0, 8);
  data[0] = 0xBB;          /* Button message marker */
  data[1] = button_id;     /* Which button (0-9) */
  data[2] = action;        /* Action code */
  data[3] = 0x00;
  data[4] = 0xBE;          /* Signature */
  data[5] = 0xEF;
  data[6] = 0x00;
  data[7] = 0x00;
}
