/****************************************************************************
 * apps/examples/bev_cluster/notifications.c
 ****************************************************************************/

#include "notifications.h"
#include "app_config.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <syslog.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct notification_s
{
  char message[64];
  enum notification_type_e type;
  uint32_t expire_time_ms;
  bool active;
  int id;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct notification_s g_notifications[MAX_NOTIFICATIONS];
static int g_next_id = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint32_t get_time_ms(void)
{
  return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void notifications_init(void)
{
  memset(g_notifications, 0, sizeof(g_notifications));
  g_next_id = 0;
}

int notifications_add(const char *message, enum notification_type_e type)
{
  int i;

  /* Find empty slot */

  for (i = 0; i < MAX_NOTIFICATIONS; i++)
    {
      if (!g_notifications[i].active)
        {
          strncpy(g_notifications[i].message, message, 63);
          g_notifications[i].message[63] = '\0';
          g_notifications[i].type = type;
          g_notifications[i].active = true;
          g_notifications[i].id = g_next_id++;

          if (type == NOTIF_TEMPORARY)
            {
              g_notifications[i].expire_time_ms = get_time_ms() +
                                                  NOTIFICATION_TIMEOUT_MS;
            }
          else
            {
              g_notifications[i].expire_time_ms = 0;
            }

          syslog(LOG_INFO, "Notification: [%d] %s\n", g_notifications[i].id,
                 message);
          return g_notifications[i].id;
        }
    }

  syslog(LOG_INFO, "Notification queue full!\n");
  return -1;
}

void notifications_clear(int id)
{
  int i;

  for (i = 0; i < MAX_NOTIFICATIONS; i++)
    {
      if (g_notifications[i].active && g_notifications[i].id == id)
        {
          g_notifications[i].active = false;
          syslog(LOG_INFO, "Notification cleared: [%d]\n", id);
          return;
        }
    }
}

void notifications_clear_all(void)
{
  int i;

  for (i = 0; i < MAX_NOTIFICATIONS; i++)
    {
      g_notifications[i].active = false;
    }

  syslog(LOG_INFO, "All notifications cleared\n");
}

void notifications_update(void)
{
  /* TODO: implement timeout later */
  (void)0;
}

int notifications_get_count(void)
{
  int count = 0;
  int i;

  for (i = 0; i < MAX_NOTIFICATIONS; i++)
    {
      if (g_notifications[i].active)
        {
          count++;
        }
    }

  return count;
}

const char *notifications_get_text(int index)
{
  int count = 0;
  int i;

  for (i = 0; i < MAX_NOTIFICATIONS; i++)
    {
      if (g_notifications[i].active)
        {
          if (count == index)
            {
              return g_notifications[i].message;
            }

          count++;
        }
    }

  return NULL;
}
