/****************************************************************************
 * apps/examples/bev_cluster/app_main.c
 *
 * BEV Cluster Application - Main Entry Point
 *
 * This application demonstrates a complete BEV (Battery Electric Vehicle)
 * instrument cluster with:
 *   - CAN bus integration (10 message placeholders, 10 signal placeholders)
 *   - LVGL graphical display (speed, battery, power, temperature, direction)
 *   - Button handling (10 button placeholders)
 *   - Notification system (permanent + temporary with 5s timeout)
 *
 * Architecture:
 *   app_main.c       -> Main loop and initialization
 *   can_handler.c    -> CAN communication
 *   can_db.c         -> CAN message decode/encode
 *   button_handler.c -> Button input
 *   ui_widgets.c     -> LVGL display
 *   notifications.c  -> Notification management
 *
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <nuttx/video/fb.h>
#include <lvgl/lvgl.h>

#include "app_config.h"
#include "can_handler.h"
#include "can_db.h"
#include "button_handler.h"
#include "ui_widgets.h"
#include "notifications.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_fb_fd;
static void *g_fb_mem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fb_flush
 *
 * Description:
 *   LVGL flush callback - transfers dirty area to display hardware
 ****************************************************************************/

static void fb_flush(lv_display_t *disp, const lv_area_t *area,
                     uint8_t *px_map)
{
  struct fb_area_s fb_area;

  fb_area.x = area->x1;
  fb_area.y = area->y1;
  fb_area.w = area->x2 - area->x1 + 1;
  fb_area.h = area->y2 - area->y1 + 1;

  ioctl(g_fb_fd, FBIO_UPDATE, (unsigned long)&fb_area);
  lv_display_flush_ready(disp);
}

/****************************************************************************
 * Name: on_can_data_update
 *
 * Description:
 *   Callback when CAN data is received - updates UI widgets
 ****************************************************************************/

static void on_can_data_update(struct vehicle_data_s *vdata)
{
  ui_widgets_update(vdata);
}

/****************************************************************************
 * Name: on_button_press
 *
 * Description:
 *   Callback when button is pressed - handles button actions
 ****************************************************************************/

static void on_button_press(uint8_t button_id)
{
  char msg[64];

  /* Button action mapping */

  switch (button_id)
    {
      case 0:  /* Button 0: Forward */
        can_handler_send_button(button_id, BTN_ACTION_FORWARD);
        notifications_add("FORWARD selected", NOTIF_TEMPORARY);
        break;

      case 1:  /* Button 1: Reverse */
        can_handler_send_button(button_id, BTN_ACTION_REVERSE);
        notifications_add("REVERSE selected", NOTIF_TEMPORARY);
        break;

      case 2:  /* Button 2: Neutral */
        can_handler_send_button(button_id, BTN_ACTION_NEUTRAL);
        notifications_add("NEUTRAL selected", NOTIF_TEMPORARY);
        break;

      case 3:  /* Button 3: Horn */
        can_handler_send_button(button_id, BTN_ACTION_HORN);
        notifications_add("HORN activated", NOTIF_TEMPORARY);
        break;

      case 4:  /* Button 4: Lights */
        can_handler_send_button(button_id, BTN_ACTION_LIGHTS);
        notifications_add("LIGHTS toggled", NOTIF_TEMPORARY);
        break;

      case 5:  /* Button 5: Reserved */
        can_handler_send_button(button_id, BTN_ACTION_RESERVED_5);
        snprintf(msg, sizeof(msg), "Button %d pressed", button_id);
        notifications_add(msg, NOTIF_TEMPORARY);
        break;

      case 6:  /* Button 6: Reserved */
        can_handler_send_button(button_id, BTN_ACTION_RESERVED_6);
        snprintf(msg, sizeof(msg), "Button %d pressed", button_id);
        notifications_add(msg, NOTIF_TEMPORARY);
        break;

      case 7:  /* Button 7: Reserved */
        can_handler_send_button(button_id, BTN_ACTION_RESERVED_7);
        snprintf(msg, sizeof(msg), "Button %d pressed", button_id);
        notifications_add(msg, NOTIF_TEMPORARY);
        break;

      case 8:  /* Button 8: Reserved */
        can_handler_send_button(button_id, BTN_ACTION_RESERVED_8);
        snprintf(msg, sizeof(msg), "Button %d pressed", button_id);
        notifications_add(msg, NOTIF_TEMPORARY);
        break;

      case 9:  /* Button 9: Reserved */
        can_handler_send_button(button_id, BTN_ACTION_RESERVED_9);
        snprintf(msg, sizeof(msg), "Button %d pressed", button_id);
        notifications_add(msg, NOTIF_TEMPORARY);
        break;

      default:
        syslog(LOG_INFO, "Unknown button: %d\n", button_id);
        break;
    }

  /* Update notification display */

  ui_widgets_update_notifications();
}

/****************************************************************************
 * Name: init_framebuffer
 *
 * Description:
 *   Initialize framebuffer device
 ****************************************************************************/

static int init_framebuffer(void)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;

  /* Open framebuffer */

  g_fb_fd = open("/dev/fb0", O_RDWR);
  if (g_fb_fd < 0)
    {
      syslog(LOG_INFO, "ERROR: Cannot open /dev/fb0\n");
      return -1;
    }

  /* Get video info */

  ioctl(g_fb_fd, FBIOGET_VIDEOINFO, &vinfo);
  ioctl(g_fb_fd, FBIOGET_PLANEINFO, &pinfo);

  syslog(LOG_INFO, "Display: %dx%d @ %d bpp\n", vinfo.xres, vinfo.yres,
         pinfo.bpp);

  /* Map framebuffer memory */

  g_fb_mem = mmap(NULL, pinfo.fblen, PROT_READ | PROT_WRITE,
                  MAP_SHARED, g_fb_fd, 0);
  if (g_fb_mem == MAP_FAILED)
    {
      syslog(LOG_INFO, "ERROR: mmap failed\n");
      close(g_fb_fd);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: init_lvgl
 *
 * Description:
 *   Initialize LVGL library and display
 ****************************************************************************/

static int init_lvgl(void)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  lv_display_t *disp;

  ioctl(g_fb_fd, FBIOGET_VIDEOINFO, &vinfo);
  ioctl(g_fb_fd, FBIOGET_PLANEINFO, &pinfo);

  /* Initialize LVGL */

  lv_init();

  /* Create display */

  disp = lv_display_create(vinfo.xres, vinfo.yres);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, fb_flush);
  lv_display_set_buffers(disp, g_fb_mem, NULL, pinfo.fblen,
                          LV_DISPLAY_RENDER_MODE_DIRECT);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Application entry point
 ****************************************************************************/

int main(int argc, char *argv[])
{
  syslog(LOG_INFO, "\n");
  syslog(LOG_INFO, "========================================\n");
  syslog(LOG_INFO, "  BEV Cluster Application v1.0\n");
  syslog(LOG_INFO, "========================================\n");
  syslog(LOG_INFO, "\n");

  /* === Initialize subsystems === */

  syslog(LOG_INFO, "Initializing framebuffer...\n");
  if (init_framebuffer() < 0)
    {
      return 1;
    }

  syslog(LOG_INFO, "Initializing LVGL...\n");
  if (init_lvgl() < 0)
    {
      return 1;
    }

  syslog(LOG_INFO, "Initializing UI widgets...\n");
  ui_widgets_init();

  syslog(LOG_INFO, "Initializing notifications...\n");
  notifications_init();

  syslog(LOG_INFO, "Initializing CAN handler...\n");
  if (can_handler_init(CAN_INTERFACE) < 0)
    {
      syslog(LOG_INFO, "WARNING: CAN not available (continuing anyway)\n");
    }
  else
    {
      can_handler_register_callback(on_can_data_update);
    }

  syslog(LOG_INFO, "Initializing button handler...\n");
  if (button_handler_init() < 0)
    {
      syslog(LOG_INFO, "WARNING: Buttons not available (continuing anyway)\n");
    }
  else
    {
      button_handler_register_callback(on_button_press);
      syslog(LOG_INFO, "Button Handler: %d buttons ready\n",
             button_handler_get_count());
    }

  /* Add startup notification */

  notifications_add("System Ready", NOTIF_TEMPORARY);
  ui_widgets_update_notifications();

  syslog(LOG_INFO, "\n");
  syslog(LOG_INFO, "=== BEV Cluster Running ===\n");
  syslog(LOG_INFO, "CAN Messages: 10 placeholders (0x18FEF100-0x18FEFA00)\n");
  syslog(LOG_INFO, "CAN Signals:  10 placeholders\n");
  syslog(LOG_INFO, "Buttons:      Auto-detected from board config\n");
  syslog(LOG_INFO, "Notifications: Permanent + 5s timeout\n");
  syslog(LOG_INFO, "\n");
  syslog(LOG_INFO, "Configure buttons via menuconfig:\n");
  syslog(LOG_INFO, "  Board Selection -> Button Configuration\n");
  syslog(LOG_INFO, "\n");
  syslog(LOG_INFO, "Press Ctrl+C to exit\n");
  syslog(LOG_INFO, "\n");

  /* === Main Loop === */

  while (1)
    {
      /* Advance LVGL tick (5ms per iteration) */

      lv_tick_inc(DISPLAY_REFRESH_MS);

      /* Poll CAN bus */

      can_handler_poll();

      /* Poll buttons */

      button_handler_poll();

      /* Update notifications (handle timeouts) */

      notifications_update();

      /* Process LVGL (redraws if needed) */

      lv_timer_handler();

      /* Sleep until next iteration */

      usleep(DISPLAY_REFRESH_MS * 1000);
    }

  return 0;
}
