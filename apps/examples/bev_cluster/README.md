# BEV Cluster Application

Complete instrument cluster for Battery Electric Vehicles with CAN bus 
integration, graphical display, and button handling.

## Architecture

```
┌─────────────┐
│  app_main.c │  <- Entry point, orchestrates everything
└──────┬──────┘
       │
       ├─── can_handler.c ────┐
       │                      │
       ├─── button_handler.c ─┼─── Callbacks ───> app_main.c
       │                      │
       ├─── ui_widgets.c      │
       │                      │
       ├─── notifications.c   │
       │                      │
       └─── can_db.c ─────────┘
```

## Files

| File | Purpose |
|------|---------|
| `app_main.c` | Main loop, initialization, callback orchestration |
| `app_config.h` | All #defines (CAN IDs, colors, timeouts) |
| `can_handler.c/h` | CAN socket management, RX/TX |
| `can_db.c/h` | CAN decode/encode (signal extraction) |
| `button_handler.c/h` | Button polling, press detection |
| `ui_widgets.c/h` | LVGL widgets (speed, battery, etc) |
| `notifications.c/h` | Notification queue (permanent + timeout) |

## Features

### CAN Bus
- **10 Message Placeholders**: `0x100 - 0x109`
  - `0x100`: Speed
  - `0x101`: Battery (voltage, current, direction)
  - `0x102`: Temperature
  - `0x103-0x109`: Reserved

- **10 Signal Placeholders**: Easy to add more signals
  - Speed (km/h)
  - Battery voltage (16-bit, 0.1V units)
  - Battery current (16-bit signed, 0.1A units)
  - Direction (0=Fwd, 1=Rev, 2=Neutral)
  - Temperature (°C)
  - 5x Reserved

- **Button TX Message**: `0x200`
  - All 10 buttons mapped to single message
  - Each button sends unique action code

### Display Widgets
- Speed (center, large)
- Battery voltage (left top)
- Battery current (left bottom)
- Power (computed: V*I, right top)
- Temperature (right bottom)
- Direction indicator (bottom center)
- Notification area (bottom bar)

### Buttons (10 placeholders)
- Button 0: Forward
- Button 1: Reverse
- Button 2: Neutral
- Button 3: Horn
- Button 4: Lights
- Button 5-9: Reserved

### Notifications
- **Permanent**: Stays until cleared
- **Temporary**: Auto-clears after 5 seconds (configurable)
- Up to 5 notifications displayed at once

## How to Customize

### Add New CAN Message

1. In `app_config.h`:
```c
#define CAN_MSG_MY_NEW_MSG  0x10A
```

2. In `can_db.h`:
```c
struct vehicle_data_s {
    // ... existing fields
    uint16_t my_new_signal;
};

void can_db_decode_my_new_msg(const uint8_t *data, 
                               struct vehicle_data_s *vdata);
```

3. In `can_db.c`:
```c
void can_db_decode_my_new_msg(const uint8_t *data,
                               struct vehicle_data_s *vdata)
{
    vdata->my_new_signal = data[0];
}
```

4. In `can_handler.c` (in `can_handler_poll()`):
```c
case CAN_MSG_MY_NEW_MSG:
    can_db_decode_my_new_msg(frame.data, &g_vehicle_data);
    data_updated = true;
    break;
```

### Add New Display Widget

In `ui_widgets.c` (`ui_widgets_init()`):
```c
g_my_widget = lv_label_create(screen);
lv_label_set_text(g_my_widget, "...");
// ... style and position
```

In `ui_widgets.c` (`ui_widgets_update()`):
```c
snprintf(buf, sizeof(buf), "%d", vdata->my_new_signal);
lv_label_set_text(g_my_widget, buf);
```

### Change Button Action

In `app_main.c` (`on_button_press()`):
```c
case 5:  /* Button 5 */
    can_handler_send_button(button_id, MY_NEW_ACTION);
    notifications_add("My action!", NOTIF_TEMPORARY);
    break;
```

## Configuration

All configuration in `app_config.h`:

```c
#define DISPLAY_WIDTH           480
#define DISPLAY_HEIGHT          320
#define DISPLAY_REFRESH_MS      5

#define CAN_INTERFACE           "can0"
#define CAN_POLL_INTERVAL_MS    10

#define MAX_BUTTONS             10
#define BUTTON_POLL_INTERVAL_MS 50

#define NOTIFICATION_TIMEOUT_MS 5000  /* 5 seconds */
#define MAX_NOTIFICATIONS       5

#define COLOR_BACKGROUND        0x000000
#define COLOR_SPEED_TEXT        0x00ff00
// ...
```

## Testing

### Simulate CAN Messages (from Linux)

```bash
# Send speed = 65 km/h
cansend can0 100#4100000000000000

# Send battery: V=400.5V (4005 = 0x0FA5), I=25.3A (253 = 0x00FD), Dir=0
cansend can0 101#0FA500FD00000000

# Send temperature = 23°C
cansend can0 102#1700000000000000
```

### Press Buttons

If you have `/dev/buttons`, just press physical buttons.

If not, you can inject button events via ioctl from another app.

## Dependencies

- NuttX RTOS
- LVGL graphics library
- CAN driver (SocketCAN)
- Framebuffer driver
- Button driver (optional)

## Compilation

```bash
cd nuttx/apps/examples/bev_cluster
make
```

## Running

```bash
nsh> bev_cluster
```

## Notes

- All CAN decoding is in `can_db.c` (single point of truth)
- Main loop is simple: poll inputs, update LVGL
- No threads, no interrupts (except optionally for buttons)
- Notification system is FIFO with timeout
- Easy to extend: just add cases in switch statements
