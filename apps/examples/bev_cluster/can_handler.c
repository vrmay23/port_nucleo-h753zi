/****************************************************************************
 * apps/examples/bev_cluster/can_handler.c
 ****************************************************************************/

#include "can_handler.h"
#include "app_config.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <nuttx/can.h>
#include <stdio.h>
#include <syslog.h>
#include <netutils/netlib.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_can_socket = -1;
static struct vehicle_data_s g_vehicle_data;
static can_update_callback_t g_callback = NULL;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int can_handler_init(const char *interface)
{
  struct sockaddr_can addr;
  struct ifreq ifr;
  int flags;
  int ret;

  /* Bring CAN interface up */

  ret = netlib_ifup(interface);
  if (ret < 0)
    {
      syslog(LOG_INFO, "CAN Handler: Failed to bring up %s\n", interface);
      return -1;
    }

  /* Create socket */

  g_can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (g_can_socket < 0)
    {
      syslog(LOG_INFO, "CAN Handler: socket() failed\n");
      return -1;
    }

  /* Get interface index */

  strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
  if (ifr.ifr_ifindex == 0)
    {
      syslog(LOG_INFO, "CAN Handler: if_nametoindex() failed\n");
      close(g_can_socket);
      return -1;
    }

  /* Bind to interface */

  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(g_can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      syslog(LOG_INFO, "CAN Handler: bind() failed\n");
      close(g_can_socket);
      return -1;
    }

  /* Set non-blocking mode */

  flags = fcntl(g_can_socket, F_GETFL, 0);
  fcntl(g_can_socket, F_SETFL, flags | O_NONBLOCK);

  /* Initialize vehicle data to safe defaults */

  memset(&g_vehicle_data, 0, sizeof(g_vehicle_data));
  g_vehicle_data.direction = 2;  /* Neutral */

  syslog(LOG_INFO, "CAN Handler: Initialized on %s\n", interface);
  return 0;
}

void can_handler_register_callback(can_update_callback_t callback)
{
  g_callback = callback;
}

void can_handler_poll(void)
{
  struct can_frame frame;
  int nbytes;
  bool data_updated = false;
  uint32_t msg_id;

  /* Read all available frames (non-blocking) */

  while ((nbytes = read(g_can_socket, &frame, sizeof(frame))) > 0)
    {
      if (nbytes != sizeof(frame))
        {
          continue;
        }

      /* Extract message ID (mask extended ID flag) */

      msg_id = frame.can_id & CAN_EFF_MASK;

      /* Decode based on message ID */

      if (msg_id == SIGNAL_SPEED_MSG)
        {
          can_db_decode_speed(frame.data, &g_vehicle_data);
          data_updated = true;
        }
      else if (msg_id == SIGNAL_BATTERY_V_MSG)
        {
          can_db_decode_battery(frame.data, &g_vehicle_data);
          data_updated = true;
        }
      else if (msg_id == SIGNAL_TEMP_MSG)
        {
          can_db_decode_temperature(frame.data, &g_vehicle_data);
          data_updated = true;
        }
      else if (msg_id == SIGNAL_RESERVED_5_MSG ||
               msg_id == SIGNAL_RESERVED_6_MSG ||
               msg_id == SIGNAL_RESERVED_7_MSG ||
               msg_id == SIGNAL_RESERVED_8_MSG ||
               msg_id == SIGNAL_RESERVED_9_MSG)
        {
          /* TODO: Implement reserved message handlers */
        }
    }

  /* Notify callback if data changed */

  if (data_updated && g_callback != NULL)
    {
      g_callback(&g_vehicle_data);
    }
}

int can_handler_send_button(uint8_t button_id, uint8_t action)
{
  struct can_frame frame;

  /* Build frame */

  frame.can_id = CAN_TX_BUTTON_MSG | CAN_EFF_FLAG;
  frame.can_dlc = 8;
  can_db_encode_button(button_id, action, frame.data);

  /* Send */

  if (write(g_can_socket, &frame, sizeof(frame)) != sizeof(frame))
    {
      return -1;
    }

  syslog(LOG_INFO, "CAN TX: Button %d, Action 0x%02X\n", button_id, action);
  return 0;
}

const struct vehicle_data_s *can_handler_get_data(void)
{
  return &g_vehicle_data;
}
