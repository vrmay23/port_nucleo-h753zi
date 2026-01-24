/****************************************************************************
 * apps/examples/canio/canio_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * apps/examples/canio/canio_main.c
 *
 * SIMPLE EXAMPLE: LED Control via CAN + Button via Interrupt
 *
 * What this program does:
 *   1. Receives CAN messages and turns LEDs on/off
 *   2. When button is pressed, sends a CAN message
 *
 * CAN codes (bytes 4 and 5):
 *   BE 01 = GREEN LED
 *   BE 02 = ORANGE LED
 *   BE 03 = RED LED
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <nuttx/can.h>
#include <nuttx/leds/userled.h>
#include <nuttx/input/buttons.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* LEDs */

#define NUM_LEDS  3

/* Buttons */

#define NUM_BUTTONS  1

/* Signal used for button notification */

#define BUTTON_SIGNAL  SIGUSR1

/* CAN ID for transmission */

#define TX_CAN_ID  0x12345678

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *g_led_names[NUM_LEDS] =
{
  "GREEN",
  "ORANGE",
  "RED"
};

static int g_led_state[NUM_LEDS] =
{
  0, 0, 0
};

static const char *g_btn_names[NUM_BUTTONS] =
{
  "USER"
};

/* We need these global variables for the signal handler */

static int g_can_socket = -1;
static int g_led_fd = -1;
static volatile int g_button_pressed = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: turn_on_led
 *
 * Description:
 *   Turns on the specified LED.
 *
 * Input Parameters:
 *   fd  - File descriptor for the LED device
 *   led - LED number to turn on
 *
 ****************************************************************************/

static void turn_on_led(int fd, int led)
{
  userled_set_t leds;
  ioctl(fd, ULEDIOC_GETALL, &leds);
  leds = leds | (1 << led);
  ioctl(fd, ULEDIOC_SETALL, leds);
}

/****************************************************************************
 * Name: turn_off_led
 *
 * Description:
 *   Turns off the specified LED.
 *
 * Input Parameters:
 *   fd  - File descriptor for the LED device
 *   led - LED number to turn off
 *
 ****************************************************************************/

static void turn_off_led(int fd, int led)
{
  userled_set_t leds;
  ioctl(fd, ULEDIOC_GETALL, &leds);
  leds = leds & ~(1 << led);
  ioctl(fd, ULEDIOC_SETALL, leds);
}

/****************************************************************************
 * Name: toggle_led
 *
 * Description:
 *   Toggles the specified LED on/off.
 *
 * Input Parameters:
 *   fd  - File descriptor for the LED device
 *   led - LED number to toggle
 *
 ****************************************************************************/

static void toggle_led(int fd, int led)
{
  if (g_led_state[led] == 0)
    {
      turn_on_led(fd, led);
      g_led_state[led] = 1;
      printf("  -> LED %s ON\n", g_led_names[led]);
    }
  else
    {
      turn_off_led(fd, led);
      g_led_state[led] = 0;
      printf("  -> LED %s OFF\n", g_led_names[led]);
    }
}

/****************************************************************************
 * Name: verify_message
 *
 * Description:
 *   Verifies if the CAN message contains valid LED control data.
 *
 * Input Parameters:
 *   msg - Pointer to the CAN frame to verify
 *
 * Returned Value:
 *   LED number (0-2) if valid, -1 otherwise
 *
 ****************************************************************************/

static int verify_message(struct can_frame *msg)
{
  if (msg->can_dlc < 6)
    {
      return -1;
    }

  if (msg->data[4] != 0xbe)
    {
      return -1;
    }

  switch (msg->data[5])
    {
      case 0x01:
        return 0;                                                 /*  Green */

      case 0x02:
        return 1;                                                 /* Orange */

      case 0x03:
        return 2;                                                 /*   Red  */

      default:
        return -1;
    }
}

/****************************************************************************
 * Name: print_message
 *
 * Description:
 *   Prints the received CAN message to console.
 *
 * Input Parameters:
 *   msg - Pointer to the CAN frame to print
 *
 ****************************************************************************/

static void print_message(struct can_frame *msg)
{
  int i;
  printf("RX: ID=0x%08lX Data=",
         (unsigned long)(msg->can_id & 0x1fffffff));
  for (i = 0; i < msg->can_dlc; i++)
    {
      printf("%02X ", msg->data[i]);
    }

  printf("\n");
}

/****************************************************************************
 * Name: send_button_message
 *
 * Description:
 *   Sends a CAN message when a button is pressed.
 *
 * Input Parameters:
 *   sock   - CAN socket file descriptor
 *   button - Button number that was pressed
 *
 ****************************************************************************/

static void send_button_message(int sock, int button)
{
  struct can_frame msg;
  int ret;

  /* Build the message */

  msg.can_id = TX_CAN_ID | CAN_EFF_FLAG;           /*       Extended ID      */
  msg.can_dlc = 8;
  msg.data[0] = 0xbb;                              /* Marker: button message */
  msg.data[1] = (uint8_t)button;                   /*       Which button     */
  msg.data[2] = 0x00;
  msg.data[3] = 0x00;
  msg.data[4] = 0xbe;                              /*         Signature      */
  msg.data[5] = 0xef;
  msg.data[6] = 0x00;
  msg.data[7] = 0x00;

  /* Send */

  ret = write(sock, &msg, sizeof(msg));
  if (ret == sizeof(msg))
    {
      printf("TX: Button %s -> ID=0x%08lX\n",
             g_btn_names[button], (unsigned long)TX_CAN_ID);
    }
  else
    {
      printf("ERROR sending: %d\n", errno);
    }
}

/****************************************************************************
 * Name: button_handler
 *
 * Description:
 *   Signal handler for button press events.
 *
 *   IMPORTANT: This handler is called asynchronously!
 *   We cannot do much here, so we just set a flag.
 *
 * Input Parameters:
 *   signo   - Signal number
 *   info    - Signal information
 *   context - Context (unused)
 *
 ****************************************************************************/

static void button_handler(int signo, siginfo_t *info, void *context)
{
  /* Just mark that button was pressed */

  g_button_pressed = 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Main entry point for the CAN + LED + Button example.
 *
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int btn_fd;
  struct sockaddr_can address;
  struct ifreq interface;
  struct can_frame message;
  struct btn_notify_s btnevents;
  struct sigaction sa;
  btn_buttonset_t supported;
  int led_number;
  int ret;

  /* ===== Welcome message ===== */

  printf("\n");
  printf("========================================\n");
  printf("  CAN + LED + Button (with interrupt)\n");
  printf("========================================\n");
  printf("\n");
  printf("CAN commands (send from Linux):\n");
  printf("  cansend can0 12345678#00000000BE010000  -> Green LED\n");
  printf("  cansend can0 12345678#00000000BE020000  -> Orange LED\n");
  printf("  cansend can0 12345678#00000000BE030000  -> Red LED\n");
  printf("\n");
  printf("USER button: sends CAN message\n");
  printf("\n");

  /* ===== Open LEDs ===== */

  printf("Opening LEDs...\n");
  g_led_fd = open("/dev/userleds", O_RDWR);
  if (g_led_fd < 0)
    {
      printf("ERROR: /dev/userleds not found\n");
      return 1;
    }

  ioctl(g_led_fd, ULEDIOC_SETALL, 0);                     /*  Turn off all   */
  printf("  OK!\n");

  /* ===== Open Buttons ===== */

  printf("Opening buttons...\n");
  btn_fd = open("/dev/buttons", O_RDONLY);
  if (btn_fd < 0)
    {
      printf("ERROR: /dev/buttons not found\n");
      close(g_led_fd);
      return 1;
    }

  /* Find out which buttons are available */

  ret = ioctl(btn_fd, BTNIOC_SUPPORTED, &supported);
  if (ret < 0)
    {
      printf("ERROR: BTNIOC_SUPPORTED failed\n");
      close(btn_fd);
      close(g_led_fd);
      return 1;
    }

  printf("  Available buttons: 0x%02X\n", (unsigned)supported);

  /* ===== Configure Signal Handler ===== */

  printf("Configuring button interrupt...\n");

  /* Configure signal action */

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = button_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  ret = sigaction(BUTTON_SIGNAL, &sa, NULL);
  if (ret < 0)
    {
      printf("ERROR: sigaction failed\n");
      close(btn_fd);
      close(g_led_fd);
      return 1;
    }

  /* Register to receive signal when button is pressed */

  btnevents.bn_press   = supported;  /* Notify on press */
  btnevents.bn_release = 0;          /* Don't notify on release */
  btnevents.bn_event.sigev_notify = SIGEV_SIGNAL;
  btnevents.bn_event.sigev_signo  = BUTTON_SIGNAL;
  ret = ioctl(btn_fd, BTNIOC_REGISTER, &btnevents);
  if (ret < 0)
    {
      printf("ERROR: BTNIOC_REGISTER failed: %d\n", errno);
      close(btn_fd);
      close(g_led_fd);
      return 1;
    }

  printf("  OK! Interrupt configured.\n");

  /* ===== Create CAN Socket ===== */

  printf("Creating CAN socket...\n");
  g_can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (g_can_socket < 0)
    {
      printf("ERROR: socket failed\n");
      close(btn_fd);
      close(g_led_fd);
      return 1;
    }

  printf("  OK!\n");

  /* ===== Connect to can0 interface ===== */

  printf("Connecting to can0...\n");
  memset(&interface, 0, sizeof(interface));
  strcpy(interface.ifr_name, "can0");
  interface.ifr_ifindex = if_nametoindex(interface.ifr_name);
  if (interface.ifr_ifindex == 0)
    {
      printf("ERROR: can0 not found\n");
      close(g_can_socket);
      close(btn_fd);
      close(g_led_fd);
      return 1;
    }

  memset(&address, 0, sizeof(address));
  address.can_family = AF_CAN;
  address.can_ifindex = interface.ifr_ifindex;
  ret = bind(g_can_socket, (struct sockaddr *)&address, sizeof(address));
  if (ret < 0)
    {
      printf("ERROR: bind failed\n");
      close(g_can_socket);
      close(btn_fd);
      close(g_led_fd);
      return 1;
    }

  printf("  OK!\n");

  /* ===== Main Loop ===== */

  printf("\n");
  printf("Ready! Waiting for CAN messages and buttons...\n");
  printf("(Ctrl+C to exit)\n");
  printf("\n");

  while (1)
    {
      /* Check if button was pressed (via interrupt) */

      if (g_button_pressed)
        {
          g_button_pressed = 0;
          send_button_message(g_can_socket, 0);
        }

      /* Wait for CAN message (with short timeout to check button) */

      struct timeval tv;
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(g_can_socket, &readfds);
      tv.tv_sec = 0;
      tv.tv_usec = 100000;  /* 100ms */

      ret = select(g_can_socket + 1, &readfds, NULL, NULL, &tv);
      if (ret > 0 && FD_ISSET(g_can_socket, &readfds))
        {
          /* Got CAN message! */

          ret = read(g_can_socket, &message, sizeof(message));
          if (ret == sizeof(message))
            {
              print_message(&message);
              led_number = verify_message(&message);
              if (led_number >= 0 && led_number < NUM_LEDS)
                {
                  toggle_led(g_led_fd, led_number);
                }
            }
        }
    }

  /* Cleanup (never reached) */

  close(g_can_socket);
  close(btn_fd);
  close(g_led_fd);
  return 0;
}
