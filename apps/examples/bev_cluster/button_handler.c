/****************************************************************************
 * apps/examples/bev_cluster/button_handler.c
 ****************************************************************************/

#include "button_handler.h"
#include "app_config.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <nuttx/input/buttons.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_button_fd = -1;
static btn_buttonset_t g_last_state = 0;
static btn_buttonset_t g_supported_buttons = 0;
static int g_num_buttons = 0;
static button_callback_t g_callback = NULL;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int button_handler_init(void)
{
  int ret;
  int i;

  g_button_fd = open("/dev/buttons", O_RDONLY | O_NONBLOCK);
  if (g_button_fd < 0)
    {
      syslog(LOG_INFO, "Button Handler: /dev/buttons not found\n");
      return -1;
    }

  /* Query which buttons are supported by the board */

  ret = ioctl(g_button_fd, BTNIOC_SUPPORTED,
              (unsigned long)&g_supported_buttons);
  if (ret < 0)
    {
      syslog(LOG_INFO, "Button Handler: BTNIOC_SUPPORTED failed\n");
      close(g_button_fd);
      return -1;
    }

  /* Count how many buttons are actually available */

  g_num_buttons = 0;
  for (i = 0; i < 32; i++)
    {
      if (g_supported_buttons & (1 << i))
        {
          g_num_buttons++;
        }
    }

  syslog(LOG_INFO, "Button Handler: Initialized - %d buttons detected (mask: 0x%X)\n",
         g_num_buttons, g_supported_buttons);

  return 0;
}

void button_handler_register_callback(button_callback_t callback)
{
  g_callback = callback;
}

void button_handler_poll(void)
{
  btn_buttonset_t state;
  int i;

  /* Read current button state */

  if (read(g_button_fd, &state, sizeof(state)) != sizeof(state))
    {
      return;
    }

  /* Detect rising edges (button just pressed) */

  for (i = 0; i < 32; i++)
    {
      btn_buttonset_t mask = (1 << i);

      /* Skip if button not supported */

      if (!(g_supported_buttons & mask))
        {
          continue;
        }

      /* Check for rising edge */

      if ((state & mask) && !(g_last_state & mask))
        {
          /* Button i was just pressed */

          syslog(LOG_INFO, "Button pressed: %d\n", i);

          if (g_callback != NULL)
            {
              g_callback(i);
            }
        }
    }

  g_last_state = state;
}

int button_handler_get_count(void)
{
  return g_num_buttons;
}
