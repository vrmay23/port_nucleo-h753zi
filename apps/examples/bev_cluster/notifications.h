/****************************************************************************
 * apps/examples/bev_cluster/notifications.h
 *
 * Notification System - Manages temporary and permanent messages
 ****************************************************************************/

#ifndef __NOTIFICATIONS_H
#define __NOTIFICATIONS_H

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum notification_type_e
{
  NOTIF_PERMANENT = 0,  /* Stays until cleared */
  NOTIF_TEMPORARY       /* Auto-clears after timeout */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: notifications_init
 *
 * Description:
 *   Initialize notification system
 ****************************************************************************/

void notifications_init(void);

/****************************************************************************
 * Name: notifications_add
 *
 * Description:
 *   Add a new notification
 *
 * Input Parameters:
 *   message - Text to display (max 64 chars)
 *   type    - NOTIF_PERMANENT or NOTIF_TEMPORARY
 *
 * Returned Value:
 *   Notification ID (>= 0) on success, -1 if queue full
 ****************************************************************************/

int notifications_add(const char *message, enum notification_type_e type);

/****************************************************************************
 * Name: notifications_clear
 *
 * Description:
 *   Clear a specific notification by ID
 *
 * Input Parameters:
 *   id - Notification ID returned by notifications_add()
 ****************************************************************************/

void notifications_clear(int id);

/****************************************************************************
 * Name: notifications_clear_all
 *
 * Description:
 *   Clear all notifications
 ****************************************************************************/

void notifications_clear_all(void);

/****************************************************************************
 * Name: notifications_update
 *
 * Description:
 *   Update notification system (handle timeouts)
 *   Should be called periodically from main loop
 ****************************************************************************/

void notifications_update(void);

/****************************************************************************
 * Name: notifications_get_count
 *
 * Description:
 *   Get number of active notifications
 *
 * Returned Value:
 *   Number of notifications currently displayed
 ****************************************************************************/

int notifications_get_count(void);

/****************************************************************************
 * Name: notifications_get_text
 *
 * Description:
 *   Get notification text by index
 *
 * Input Parameters:
 *   index - Index (0 to count-1)
 *
 * Returned Value:
 *   Notification text, or NULL if index invalid
 ****************************************************************************/

const char *notifications_get_text(int index);

#endif /* __NOTIFICATIONS_H */
