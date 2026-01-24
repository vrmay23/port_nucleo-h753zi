/****************************************************************************
 * apps/examples/canflood/canflood_main.c
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
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <nuttx/can.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_IFACE   "can0"
#define DEFAULT_DELAY   1          /* 1 ms delay between frames */
#define DEFAULT_COUNT   0          /* 0 = infinite */
#define CAN_DATA_LEN    8

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct canflood_config_s
{
  const char *iface;        /* CAN interface name */
  uint32_t    delay_ms;     /* Delay between frames in ms */
  uint32_t    count;        /* Number of frames (0=infinite) */
  bool        extended;     /* Use extended (29-bit) IDs */
  bool        verbose;      /* Verbose output */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_running = true;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: canflood_sighandler
 *
 * Description:
 *   Handle SIGINT to stop the flood gracefully.
 *
 ****************************************************************************/

static void canflood_sighandler(int signo)
{
  if (signo == SIGINT)
    {
      g_running = false;
    }
}

/****************************************************************************
 * Name: canflood_random32
 *
 * Description:
 *   Generate a 32-bit random number using /dev/urandom.
 *
 ****************************************************************************/

static uint32_t canflood_random32(int urandom_fd)
{
  uint32_t val;
  ssize_t ret;

  ret = read(urandom_fd, &val, sizeof(val));
  if (ret != sizeof(val))
    {
      /* Fallback to simple PRNG if urandom fails */

      val = (uint32_t)rand();
    }

  return val;
}

/****************************************************************************
 * Name: canflood_fill_random
 *
 * Description:
 *   Fill buffer with random bytes.
 *
 ****************************************************************************/

static void canflood_fill_random(int urandom_fd, uint8_t *buf, size_t len)
{
  ssize_t ret;

  ret = read(urandom_fd, buf, len);
  if (ret != (ssize_t)len)
    {
      /* Fallback to simple PRNG */

      size_t i;
      for (i = 0; i < len; i++)
        {
          buf[i] = (uint8_t)(rand() & 0xff);
        }
    }
}

/****************************************************************************
 * Name: canflood_show_usage
 *
 * Description:
 *   Show usage information.
 *
 ****************************************************************************/

static void canflood_show_usage(const char *progname)
{
  printf("Usage: %s [options]\n", progname);
  printf("Options:\n");
  printf("  -i <iface>   CAN interface (default: %s)\n", DEFAULT_IFACE);
  printf("  -d <ms>      Delay between frames in ms (default: %d)\n",
         DEFAULT_DELAY);
  printf("  -c <count>   Number of frames, 0=infinite (default: %d)\n",
         DEFAULT_COUNT);
  printf("  -e           Use extended (29-bit) IDs\n");
  printf("  -s           Use standard (11-bit) IDs (default)\n");
  printf("  -v           Verbose output\n");
  printf("  -h           Show this help\n");
  printf("\nPress Ctrl+C to stop.\n");
}

/****************************************************************************
 * Name: canflood_parse_args
 *
 * Description:
 *   Parse command line arguments.
 *
 ****************************************************************************/

static int canflood_parse_args(int argc, char *argv[],
                               struct canflood_config_s *config)
{
  int opt;

  /* Set defaults */

  config->iface    = DEFAULT_IFACE;
  config->delay_ms = DEFAULT_DELAY;
  config->count    = DEFAULT_COUNT;
  config->extended = false;
  config->verbose  = false;

  while ((opt = getopt(argc, argv, "i:d:c:esvh")) != -1)
    {
      switch (opt)
        {
          case 'i':
            config->iface = optarg;
            break;

          case 'd':
            config->delay_ms = (uint32_t)atoi(optarg);
            break;

          case 'c':
            config->count = (uint32_t)atoi(optarg);
            break;

          case 'e':
            config->extended = true;
            break;

          case 's':
            config->extended = false;
            break;

          case 'v':
            config->verbose = true;
            break;

          case 'h':
            canflood_show_usage(argv[0]);
            return -1;

          default:
            canflood_show_usage(argv[0]);
            return -1;
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   canflood entry point - floods CAN bus with random extended ID frames.
 *
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct canflood_config_s config;
  struct sockaddr_can addr;
  struct can_frame frame;
  struct ifreq ifr;
  int sock;
  int urandom_fd;
  int ret;
  uint32_t sent = 0;
  uint32_t id_mask;
  struct timespec ts;

  /* Parse arguments */

  if (canflood_parse_args(argc, argv, &config) < 0)
    {
      return EXIT_FAILURE;
    }

  /* Setup signal handler for graceful exit */

  signal(SIGINT, canflood_sighandler);

  /* Open urandom for random data */

  urandom_fd = open("/dev/urandom", O_RDONLY);
  if (urandom_fd < 0)
    {
      printf("Warning: /dev/urandom not available, using rand()\n");
      srand((unsigned int)time(NULL));
      urandom_fd = -1;
    }

  /* Create CAN socket */

  sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (sock < 0)
    {
      printf("Error: socket() failed: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_urandom;
    }

  /* Get interface index */

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, config.iface, IFNAMSIZ - 1);

  ret = ioctl(sock, SIOCGIFINDEX, &ifr);
  if (ret < 0)
    {
      printf("Error: ioctl(SIOCGIFINDEX) failed: %d\n", errno);
      printf("Is interface '%s' up?\n", config.iface);
      ret = EXIT_FAILURE;
      goto errout_sock;
    }

  /* Bind socket to CAN interface */

  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    {
      printf("Error: bind() failed: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_sock;
    }

  /* Set ID mask based on mode */

  if (config.extended)
    {
      id_mask = CAN_EFF_MASK;  /* 29-bit mask */
      printf("CAN Flood: Extended ID (29-bit) mode\n");
    }
  else
    {
      id_mask = CAN_SFF_MASK;  /* 11-bit mask */
      printf("CAN Flood: Standard ID (11-bit) mode\n");
    }

  printf("Interface: %s, Delay: %lu ms, Count: %s\n",
         config.iface, (unsigned long)config.delay_ms,
         config.count == 0 ? "infinite" : "limited");
  printf("Press Ctrl+C to stop...\n\n");

  /* Main flood loop */

  while (g_running)
    {
      /* Check if we reached the count limit */

      if (config.count > 0 && sent >= config.count)
        {
          break;
        }

      /* Generate random CAN ID */

      frame.can_id = canflood_random32(urandom_fd) & id_mask;

      /* Set extended frame flag if using 29-bit IDs */

      if (config.extended)
        {
          frame.can_id |= CAN_EFF_FLAG;
        }

      /* Set DLC to 8 bytes */

      frame.can_dlc = CAN_DATA_LEN;

      /* Fill data with random bytes */

      canflood_fill_random(urandom_fd, frame.data, CAN_DATA_LEN);

      /* Send frame */

      ret = write(sock, &frame, sizeof(frame));
      if (ret != sizeof(frame))
        {
          if (errno == EINTR)
            {
              continue;  /* Interrupted, try again */
            }

          printf("Error: write() failed: %d\n", errno);
          break;
        }

      sent++;

      /* Verbose output */

      if (config.verbose)
        {
          if (config.extended)
            {
              printf("TX: %08lX#", (unsigned long)(frame.can_id & id_mask));
            }
          else
            {
              printf("TX: %03lX#", (unsigned long)(frame.can_id & id_mask));
            }

          int i;
          for (i = 0; i < frame.can_dlc; i++)
            {
              printf("%02X", frame.data[i]);
            }

          printf(" [%lu]\n", (unsigned long)sent);
        }

      /* Delay between frames */

      if (config.delay_ms > 0)
        {
          ts.tv_sec = config.delay_ms / 1000;
          ts.tv_nsec = (config.delay_ms % 1000) * 1000000L;
          nanosleep(&ts, NULL);
        }
    }

  printf("\nSent %lu frames.\n", (unsigned long)sent);
  ret = EXIT_SUCCESS;

errout_sock:
  close(sock);

errout_urandom:
  if (urandom_fd >= 0)
    {
      close(urandom_fd);
    }

  return ret;
}
