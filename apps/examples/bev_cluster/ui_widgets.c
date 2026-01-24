/****************************************************************************
 * apps/examples/bev_cluster/ui_widgets.c
 ****************************************************************************/

#include "ui_widgets.h"
#include "app_config.h"
#include "notifications.h"
#include <lvgl/lvgl.h>
#include <stdio.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Widget handles */

static lv_obj_t *g_speed_label;
static lv_obj_t *g_voltage_label;
static lv_obj_t *g_current_label;
static lv_obj_t *g_power_label;
static lv_obj_t *g_temp_label;
static lv_obj_t *g_direction_label;
static lv_obj_t *g_notif_area;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void ui_widgets_init(void)
{
  lv_obj_t *screen = lv_screen_active();
  lv_obj_t *container;

  /* Set background */

  lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BACKGROUND), 0);

  /* === Speed Display (top center) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 200, 120);
  lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_SPEED_BOX), 0);
  lv_obj_set_style_border_width(container, 2, 0);
  lv_obj_set_style_border_color(container,
                                 lv_color_hex(COLOR_SPEED_TEXT), 0);

  g_speed_label = lv_label_create(container);
  lv_label_set_text(g_speed_label, "0\nkm/h");
  lv_obj_set_style_text_color(g_speed_label,
                               lv_color_hex(COLOR_SPEED_TEXT), 0);
  lv_obj_set_style_text_font(g_speed_label, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_align(g_speed_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(g_speed_label, LV_ALIGN_CENTER, 0, 0);

  /* === Battery Info (left side) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 150, 80);
  lv_obj_align(container, LV_ALIGN_LEFT_MID, 10, -40);
  lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_SPEED_BOX), 0);

  g_voltage_label = lv_label_create(container);
  lv_label_set_text(g_voltage_label, "0.0V");
  lv_obj_set_style_text_color(g_voltage_label,
                               lv_color_hex(COLOR_BATTERY_HIGH), 0);
  lv_obj_align(g_voltage_label, LV_ALIGN_CENTER, 0, 0);

  /* === Current Info (left side) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 150, 80);
  lv_obj_align(container, LV_ALIGN_LEFT_MID, 10, 40);
  lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_SPEED_BOX), 0);

  g_current_label = lv_label_create(container);
  lv_label_set_text(g_current_label, "0.0A");
  lv_obj_set_style_text_color(g_current_label,
                               lv_color_hex(COLOR_BATTERY_HIGH), 0);
  lv_obj_align(g_current_label, LV_ALIGN_CENTER, 0, 0);

  /* === Power Display (right side) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 150, 80);
  lv_obj_align(container, LV_ALIGN_RIGHT_MID, -10, -40);
  lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_SPEED_BOX), 0);

  g_power_label = lv_label_create(container);
  lv_label_set_text(g_power_label, "0.0kW");
  lv_obj_set_style_text_color(g_power_label,
                               lv_color_hex(COLOR_BATTERY_HIGH), 0);
  lv_obj_align(g_power_label, LV_ALIGN_CENTER, 0, 0);

  /* === Temperature Display (right side) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 150, 80);
  lv_obj_align(container, LV_ALIGN_RIGHT_MID, -10, 40);
  lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_SPEED_BOX), 0);

  g_temp_label = lv_label_create(container);
  lv_label_set_text(g_temp_label, "0°C");
  lv_obj_set_style_text_color(g_temp_label,
                               lv_color_hex(COLOR_TEMP_NORMAL), 0);
  lv_obj_align(g_temp_label, LV_ALIGN_CENTER, 0, 0);

  /* === Direction Display (bottom center) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 200, 50);
  lv_obj_align(container, LV_ALIGN_BOTTOM_MID, 0, -80);
  lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_SPEED_BOX), 0);

  g_direction_label = lv_label_create(container);
  lv_label_set_text(g_direction_label, "NEUTRAL");
  lv_obj_set_style_text_color(g_direction_label,
                               lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(g_direction_label, LV_ALIGN_CENTER, 0, 0);

  /* === Notification Area (bottom) === */

  g_notif_area = lv_obj_create(screen);
  lv_obj_set_size(g_notif_area, 460, 60);
  lv_obj_align(g_notif_area, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_color(g_notif_area,
                             lv_color_hex(COLOR_NOTIF_BG), 0);
  lv_obj_set_flex_flow(g_notif_area, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(g_notif_area, 5, 0);
}

void ui_widgets_update(const struct vehicle_data_s *vdata)
{
  char buf[32];

  /* Update speed */

  snprintf(buf, sizeof(buf), "%d\nkm/h", vdata->speed_kmh);
  lv_label_set_text(g_speed_label, buf);

  /* Update voltage */

  snprintf(buf, sizeof(buf), "%.1fV", vdata->battery_voltage * 0.1f);
  lv_label_set_text(g_voltage_label, buf);

  /* Update current */

  snprintf(buf, sizeof(buf), "%.1fA", vdata->battery_current * 0.1f);
  lv_label_set_text(g_current_label, buf);

  /* Update power */

  snprintf(buf, sizeof(buf), "%.2fkW", vdata->power_kw);
  lv_label_set_text(g_power_label, buf);

  /* Update temperature */

  snprintf(buf, sizeof(buf), "%d°C", vdata->temperature_c);
  lv_label_set_text(g_temp_label, buf);

  /* Color code temperature */

  if (vdata->temperature_c > 50)
    {
      lv_obj_set_style_text_color(g_temp_label,
                                   lv_color_hex(COLOR_TEMP_HIGH), 0);
    }
  else
    {
      lv_obj_set_style_text_color(g_temp_label,
                                   lv_color_hex(COLOR_TEMP_NORMAL), 0);
    }

  /* Update direction */

  switch (vdata->direction)
    {
      case 0:
        lv_label_set_text(g_direction_label, "FORWARD");
        lv_obj_set_style_text_color(g_direction_label,
                                     lv_color_hex(0x00FF00), 0);
        break;

      case 1:
        lv_label_set_text(g_direction_label, "REVERSE");
        lv_obj_set_style_text_color(g_direction_label,
                                     lv_color_hex(0xFFAA00), 0);
        break;

      default:
        lv_label_set_text(g_direction_label, "NEUTRAL");
        lv_obj_set_style_text_color(g_direction_label,
                                     lv_color_hex(0xFFFFFF), 0);
        break;
    }
}

void ui_widgets_update_notifications(void)
{
  int count = notifications_get_count();
  int i;

  /* Clear existing notification labels */

  lv_obj_clean(g_notif_area);

  /* Create label for each notification */

  for (i = 0; i < count; i++)
    {
      const char *text = notifications_get_text(i);
      if (text != NULL)
        {
          lv_obj_t *label = lv_label_create(g_notif_area);
          lv_label_set_text(label, text);
          lv_obj_set_style_text_color(label,
                                       lv_color_hex(COLOR_NOTIF_TEXT), 0);
        }
    }
}
