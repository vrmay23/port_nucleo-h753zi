/****************************************************************************
 * apps/examples/bev_cluster/ui_widgets.h
 *
 * UI Widgets - LVGL display components
 ****************************************************************************/

#ifndef __UI_WIDGETS_H
#define __UI_WIDGETS_H

#include <stdint.h>
#include "can_db.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: ui_widgets_init
 *
 * Description:
 *   Initialize all UI widgets (must be called after LVGL init)
 ****************************************************************************/

void ui_widgets_init(void);

/****************************************************************************
 * Name: ui_widgets_update
 *
 * Description:
 *   Update all widgets with new vehicle data
 *
 * Input Parameters:
 *   vdata - Vehicle data structure
 ****************************************************************************/

void ui_widgets_update(const struct vehicle_data_s *vdata);

/****************************************************************************
 * Name: ui_widgets_update_notifications
 *
 * Description:
 *   Update notification display area
 *   Should be called after notification system changes
 ****************************************************************************/

void ui_widgets_update_notifications(void);

#endif /* __UI_WIDGETS_H */
