/****************************************************************************
 * apps/examples/bev_cluster/app_config.h
 *
 * Global configuration and constants for BEV Cluster Application
 ****************************************************************************/

#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Display Configuration */

#define DISPLAY_WIDTH           480
#define DISPLAY_HEIGHT          320
#define DISPLAY_REFRESH_MS      5

/* CAN Configuration */

#define CAN_INTERFACE           "can0"
#define CAN_POLL_INTERVAL_MS    10
#define CAN_USE_EXTENDED_ID     1     /* 1=Extended (29-bit), 0=Standard */

/* Button Configuration */

#define MAX_BUTTONS             10
#define BUTTON_POLL_INTERVAL_MS 50

/* Notification System */

#define NOTIFICATION_TIMEOUT_MS 5000  /* 5 seconds for temporary notifs */
#define MAX_NOTIFICATIONS       5

/****************************************************************************
 * CAN Database Configuration
 *
 * EASY EDIT SECTION - Customize your CAN messages and signals here
 *
 * How to add a new signal:
 *   1. Define CAN_MSG_xxx with your message ID (Extended 29-bit)
 *   2. Define SIGNAL_xxx with message ID and byte position
 *   3. Add decode function in can_db.c
 *   4. Add case in can_handler.c
 ****************************************************************************/

/* === CAN Message IDs (Extended 29-bit) === */

#define CAN_MSG_SPEED           0x18FEF100  /* Speed message */
#define CAN_MSG_BATTERY         0x18FEF200  /* Battery pack status */
#define CAN_MSG_TEMPERATURE     0x18FEF300  /* Temperature sensors */
#define CAN_MSG_RESERVED_3      0x18FEF400  /* Reserved */
#define CAN_MSG_RESERVED_4      0x18FEF500  /* Reserved */
#define CAN_MSG_RESERVED_5      0x18FEF600  /* Reserved */
#define CAN_MSG_RESERVED_6      0x18FEF700  /* Reserved */
#define CAN_MSG_RESERVED_7      0x18FEF800  /* Reserved */
#define CAN_MSG_RESERVED_8      0x18FEF900  /* Reserved */
#define CAN_MSG_RESERVED_9      0x18FEFA00  /* Reserved */

/* === Signal 0: Speed (CAN_MSG_SPEED) === */

#define SIGNAL_SPEED_MSG        CAN_MSG_SPEED
#define SIGNAL_SPEED_BYTE       0     /* Byte position */
#define SIGNAL_SPEED_LEN        1     /* Length in bytes */
/* Value = raw byte (0-255 km/h) */

/* === Signal 1: Battery Voltage (CAN_MSG_BATTERY) === */

#define SIGNAL_BATTERY_V_MSG    CAN_MSG_BATTERY
#define SIGNAL_BATTERY_V_BYTE   0     /* Start byte (16-bit big-endian) */
#define SIGNAL_BATTERY_V_LEN    2     /* Length in bytes */
/* Value = (raw * 0.1) V, range 0-6553.5V */

/* === Signal 2: Battery Current (CAN_MSG_BATTERY) === */

#define SIGNAL_BATTERY_I_MSG    CAN_MSG_BATTERY
#define SIGNAL_BATTERY_I_BYTE   2     /* Start byte (16-bit signed) */
#define SIGNAL_BATTERY_I_LEN    2     /* Length in bytes */
/* Value = (raw * 0.1) A, range -3276.8 to +3276.7A */

/* === Signal 3: Direction (CAN_MSG_BATTERY) === */

#define SIGNAL_DIRECTION_MSG    CAN_MSG_BATTERY
#define SIGNAL_DIRECTION_BYTE   4     /* Byte position */
#define SIGNAL_DIRECTION_LEN    1     /* Length in bytes */
/* Value: 0=Forward, 1=Reverse, 2=Neutral */

/* === Signal 4: Temperature (CAN_MSG_TEMPERATURE) === */

#define SIGNAL_TEMP_MSG         CAN_MSG_TEMPERATURE
#define SIGNAL_TEMP_BYTE        0     /* Byte position */
#define SIGNAL_TEMP_LEN         1     /* Length in bytes */
/* Value = signed byte (-128 to +127°C) */

/* === Signal 5-9: Reserved (customize as needed) === */

#define SIGNAL_RESERVED_5_MSG   CAN_MSG_RESERVED_5
#define SIGNAL_RESERVED_5_BYTE  0
#define SIGNAL_RESERVED_5_LEN   1

#define SIGNAL_RESERVED_6_MSG   CAN_MSG_RESERVED_6
#define SIGNAL_RESERVED_6_BYTE  0
#define SIGNAL_RESERVED_6_LEN   1

#define SIGNAL_RESERVED_7_MSG   CAN_MSG_RESERVED_7
#define SIGNAL_RESERVED_7_BYTE  0
#define SIGNAL_RESERVED_7_LEN   1

#define SIGNAL_RESERVED_8_MSG   CAN_MSG_RESERVED_8
#define SIGNAL_RESERVED_8_BYTE  0
#define SIGNAL_RESERVED_8_LEN   1

#define SIGNAL_RESERVED_9_MSG   CAN_MSG_RESERVED_9
#define SIGNAL_RESERVED_9_BYTE  0
#define SIGNAL_RESERVED_9_LEN   1

/* Button Action IDs */

#define BTN_ACTION_FORWARD      0x01
#define BTN_ACTION_REVERSE      0x02
#define BTN_ACTION_NEUTRAL      0x03
#define BTN_ACTION_HORN         0x04
#define BTN_ACTION_LIGHTS       0x05
#define BTN_ACTION_RESERVED_5   0x06
#define BTN_ACTION_RESERVED_6   0x07
#define BTN_ACTION_RESERVED_7   0x08
#define BTN_ACTION_RESERVED_8   0x09
#define BTN_ACTION_RESERVED_9   0x0A

/* Button TX CAN Message ID */

#define CAN_TX_BUTTON_MSG       0x200

/* Colors */

#define COLOR_BACKGROUND        0x000000
#define COLOR_SPEED_BOX         0x1a1a1a
#define COLOR_SPEED_TEXT        0x00ff00
#define COLOR_BATTERY_HIGH      0x00ff00
#define COLOR_BATTERY_MED       0xffaa00
#define COLOR_BATTERY_LOW       0xff0000
#define COLOR_TEMP_NORMAL       0x00aaff
#define COLOR_TEMP_HIGH         0xff0000
#define COLOR_NOTIF_BG          0x333333
#define COLOR_NOTIF_TEXT        0xffffff

#endif /* __APP_CONFIG_H */
