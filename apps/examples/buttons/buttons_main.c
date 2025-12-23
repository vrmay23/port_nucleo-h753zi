/****************************************************************************
 * apps/examples/buttons/buttons_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>

#include <nuttx/input/buttons.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_INPUT_BUTTONS
#  error "CONFIG_INPUT_BUTTONS is not defined in the configuration"
#endif

#ifndef CONFIG_INPUT_BUTTONS_NPOLLWAITERS
#  define CONFIG_INPUT_BUTTONS_NPOLLWAITERS 2
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_SIGNO
#  define CONFIG_EXAMPLES_BUTTONS_SIGNO 32
#endif

#ifndef CONFIG_INPUT_BUTTONS_POLL_DELAY
#  define CONFIG_INPUT_BUTTONS_POLL_DELAY 1000
#endif

/* Button name definitions for buttons 0-31 */

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME0
#  define CONFIG_EXAMPLES_BUTTONS_NAME0 "BUTTON0"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME1
#  define CONFIG_EXAMPLES_BUTTONS_NAME1 "BUTTON1"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME2
#  define CONFIG_EXAMPLES_BUTTONS_NAME2 "BUTTON2"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME3
#  define CONFIG_EXAMPLES_BUTTONS_NAME3 "BUTTON3"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME4
#  define CONFIG_EXAMPLES_BUTTONS_NAME4 "BUTTON4"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME5
#  define CONFIG_EXAMPLES_BUTTONS_NAME5 "BUTTON5"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME6
#  define CONFIG_EXAMPLES_BUTTONS_NAME6 "BUTTON6"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME7
#  define CONFIG_EXAMPLES_BUTTONS_NAME7 "BUTTON7"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME8
#  define CONFIG_EXAMPLES_BUTTONS_NAME8 "BUTTON8"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME9
#  define CONFIG_EXAMPLES_BUTTONS_NAME9 "BUTTON9"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME10
#  define CONFIG_EXAMPLES_BUTTONS_NAME10 "BUTTON10"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME11
#  define CONFIG_EXAMPLES_BUTTONS_NAME11 "BUTTON11"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME12
#  define CONFIG_EXAMPLES_BUTTONS_NAME12 "BUTTON12"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME13
#  define CONFIG_EXAMPLES_BUTTONS_NAME13 "BUTTON13"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME14
#  define CONFIG_EXAMPLES_BUTTONS_NAME14 "BUTTON14"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME15
#  define CONFIG_EXAMPLES_BUTTONS_NAME15 "BUTTON15"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME16
#  define CONFIG_EXAMPLES_BUTTONS_NAME16 "BUTTON16"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME17
#  define CONFIG_EXAMPLES_BUTTONS_NAME17 "BUTTON17"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME18
#  define CONFIG_EXAMPLES_BUTTONS_NAME18 "BUTTON18"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME19
#  define CONFIG_EXAMPLES_BUTTONS_NAME19 "BUTTON19"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME20
#  define CONFIG_EXAMPLES_BUTTONS_NAME20 "BUTTON20"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME21
#  define CONFIG_EXAMPLES_BUTTONS_NAME21 "BUTTON21"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME22
#  define CONFIG_EXAMPLES_BUTTONS_NAME22 "BUTTON22"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME23
#  define CONFIG_EXAMPLES_BUTTONS_NAME23 "BUTTON23"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME24
#  define CONFIG_EXAMPLES_BUTTONS_NAME24 "BUTTON24"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME25
#  define CONFIG_EXAMPLES_BUTTONS_NAME25 "BUTTON25"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME26
#  define CONFIG_EXAMPLES_BUTTONS_NAME26 "BUTTON26"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME27
#  define CONFIG_EXAMPLES_BUTTONS_NAME27 "BUTTON27"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME28
#  define CONFIG_EXAMPLES_BUTTONS_NAME28 "BUTTON28"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME29
#  define CONFIG_EXAMPLES_BUTTONS_NAME29 "BUTTON29"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME30
#  define CONFIG_EXAMPLES_BUTTONS_NAME30 "BUTTON30"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME31
#  define CONFIG_EXAMPLES_BUTTONS_NAME31 "BUTTON31"
#endif

/* Maximum number of buttons supported (increased from 8 to 32) */

#define BUTTON_MAX 32

#ifndef CONFIG_EXAMPLES_BUTTONS_QTD
#  define CONFIG_EXAMPLES_BUTTONS_QTD BUTTON_MAX
#endif

#if CONFIG_EXAMPLES_BUTTONS_QTD > 32
#  error "CONFIG_EXAMPLES_BUTTONS_QTD > 32"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_BUTTONS_NAMES
static const char button_name[CONFIG_EXAMPLES_BUTTONS_QTD][16] =
{
  CONFIG_EXAMPLES_BUTTONS_NAME0
#if CONFIG_EXAMPLES_BUTTONS_QTD > 1
  , CONFIG_EXAMPLES_BUTTONS_NAME1
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 2
  , CONFIG_EXAMPLES_BUTTONS_NAME2
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 3
  , CONFIG_EXAMPLES_BUTTONS_NAME3
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 4
  , CONFIG_EXAMPLES_BUTTONS_NAME4
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 5
  , CONFIG_EXAMPLES_BUTTONS_NAME5
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 6
  , CONFIG_EXAMPLES_BUTTONS_NAME6
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 7
  , CONFIG_EXAMPLES_BUTTONS_NAME7
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 8
  , CONFIG_EXAMPLES_BUTTONS_NAME8
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 9
  , CONFIG_EXAMPLES_BUTTONS_NAME9
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 10
  , CONFIG_EXAMPLES_BUTTONS_NAME10
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 11
  , CONFIG_EXAMPLES_BUTTONS_NAME11
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 12
  , CONFIG_EXAMPLES_BUTTONS_NAME12
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 13
  , CONFIG_EXAMPLES_BUTTONS_NAME13
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 14
  , CONFIG_EXAMPLES_BUTTONS_NAME14
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 15
  , CONFIG_EXAMPLES_BUTTONS_NAME15
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 16
  , CONFIG_EXAMPLES_BUTTONS_NAME16
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 17
  , CONFIG_EXAMPLES_BUTTONS_NAME17
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 18
  , CONFIG_EXAMPLES_BUTTONS_NAME18
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 19
  , CONFIG_EXAMPLES_BUTTONS_NAME19
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 20
  , CONFIG_EXAMPLES_BUTTONS_NAME20
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 21
  , CONFIG_EXAMPLES_BUTTONS_NAME21
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 22
  , CONFIG_EXAMPLES_BUTTONS_NAME22
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 23
  , CONFIG_EXAMPLES_BUTTONS_NAME23
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 24
  , CONFIG_EXAMPLES_BUTTONS_NAME24
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 25
  , CONFIG_EXAMPLES_BUTTONS_NAME25
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 26
  , CONFIG_EXAMPLES_BUTTONS_NAME26
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 27
  , CONFIG_EXAMPLES_BUTTONS_NAME27
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 28
  , CONFIG_EXAMPLES_BUTTONS_NAME28
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 29
  , CONFIG_EXAMPLES_BUTTONS_NAME29
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 30
  , CONFIG_EXAMPLES_BUTTONS_NAME30
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 31
  , CONFIG_EXAMPLES_BUTTONS_NAME31
#endif
};
#endif

static bool g_button_daemon_started;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: button_daemon
 ****************************************************************************/

static int button_daemon(int argc, char *argv[])
{
#ifdef CONFIG_EXAMPLES_BUTTONS_POLL
  struct pollfd fds[1];
#endif

#ifdef CONFIG_EXAMPLES_BUTTONS_SIGNAL
  struct btn_notify_s btnevents;
#endif

  btn_buttonset_t supported;
  btn_buttonset_t sample = 0;

#ifdef CONFIG_EXAMPLES_BUTTONS_NAMES
  btn_buttonset_t oldsample = 0;
#endif

  int ret;
  int fd;
  int i;

  UNUSED(i);

  /* Indicate that we are running */

  g_button_daemon_started = true;
  printf("button_daemon: Running\n");

  /* Open the BUTTON driver */

  printf("button_daemon: Opening %s\n", CONFIG_EXAMPLES_BUTTONS_DEVPATH);
  fd = open(CONFIG_EXAMPLES_BUTTONS_DEVPATH, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      int errcode = errno;
      printf("button_daemon: ERROR: Failed to open %s: %d\n",
             CONFIG_EXAMPLES_BUTTONS_DEVPATH, errcode);
      goto errout;
    }

  /* Get the set of BUTTONs supported */

  ret = ioctl(fd, BTNIOC_SUPPORTED,
              (unsigned long)((uintptr_t)&supported));
  if (ret < 0)
    {
      int errcode = errno;
      printf("button_daemon: ERROR: ioctl(BTNIOC_SUPPORTED) failed: %d\n",
             errcode);
      goto errout_with_fd;
    }

  printf("button_daemon: Supported BUTTONs 0x%08lx\n",
         (unsigned long)supported);

#ifdef CONFIG_EXAMPLES_BUTTONS_SIGNAL
  /* Define the notifications events */

  btnevents.bn_press   = supported;
  btnevents.bn_release = supported;

  btnevents.bn_event.sigev_notify = SIGEV_SIGNAL;
  btnevents.bn_event.sigev_signo  = CONFIG_EXAMPLES_BUTTONS_SIGNO;

  /* Register to receive a signal when buttons are pressed/released */

  ret = ioctl(fd, BTNIOC_REGISTER,
              (unsigned long)((uintptr_t)&btnevents));
  if (ret < 0)
    {
      int errcode = errno;
      printf("button_daemon: ERROR: ioctl(BTNIOC_REGISTER) failed: %d\n",
             errcode);
      goto errout_with_fd;
    }

  /* Ignore the default signal action */

  signal(CONFIG_EXAMPLES_BUTTONS_SIGNO, SIG_IGN);
#endif

  /* Now loop forever, waiting BUTTONs events */

  for (; ; )
    {
#ifdef CONFIG_EXAMPLES_BUTTONS_SIGNAL
      struct siginfo value;
      sigset_t set;
#endif

#ifdef CONFIG_EXAMPLES_BUTTONS_POLL
      bool timeout;
      int nbytes;
#endif

#ifdef CONFIG_EXAMPLES_BUTTONS_SIGNAL
      /* Wait for a signal */

      sigemptyset(&set);
      sigaddset(&set, CONFIG_EXAMPLES_BUTTONS_SIGNO);
      ret = sigwaitinfo(&set, &value);
      if (ret < 0)
        {
          int errcode = errno;
          printf("button_daemon: ERROR: sigwaitinfo() failed: %d\n",
                 errcode);
          goto errout_with_fd;
        }

      sample = (btn_buttonset_t)value.si_value.sival_int;
#endif

#ifdef CONFIG_EXAMPLES_BUTTONS_POLL
      /* Prepare the File Descriptor for poll */

      memset(fds, 0, sizeof(fds));

      fds[0].fd      = fd;
      fds[0].events  = POLLIN;

      timeout        = false;

      ret = poll(fds, 1, CONFIG_INPUT_BUTTONS_POLL_DELAY);

      printf("\nbutton_daemon: poll returned: %d\n", ret);
      if (ret < 0)
        {
          int errcode = errno;
          printf("button_daemon: ERROR poll failed: %d\n", errcode);
        }
      else if (ret == 0)
        {
          printf("button_daemon: Timeout\n");
          timeout = true;
        }
      else if (ret > CONFIG_INPUT_BUTTONS_NPOLLWAITERS)
        {
          printf("button_daemon: ERROR poll reported: %d\n", errno);
        }

      /* In any event, read until the pipe is empty */

      do
        {
          nbytes = read(fds[0].fd, (void *)&sample, sizeof(btn_buttonset_t));

          if (nbytes <= 0)
            {
              if (nbytes == 0 || errno == EAGAIN)
                {
                  if ((fds[0].revents & POLLIN) != 0)
                    {
                      printf("button_daemon: ERROR no read data\n");
                    }
                }
              else if (errno != EINTR)
                {
                  printf("button_daemon: read failed: %d\n", errno);
                }

              nbytes = 0;
            }
          else
            {
              if (timeout)
                {
                  printf("button_daemon: ERROR? Poll timeout, "
                         "but data read\n");
                  printf("               (might just be a race "
                         "condition)\n");
                }
            }

          /* Suppress error report if no read data on the next time
           * through
           */

          fds[0].revents = 0;
        }
      while (nbytes > 0);
#endif

#ifdef CONFIG_EXAMPLES_BUTTONS_NAMES
      /* Print name of all pressed/released buttons */

      for (i = 0; i < CONFIG_EXAMPLES_BUTTONS_QTD; i++)
        {
          btn_buttonset_t button_bit = (btn_buttonset_t)(1 << i);

          /* Check if button was pressed */

          if ((sample & button_bit) && !(oldsample & button_bit))
            {
              printf("%s was pressed\n", button_name[i]);
            }

          /* Check if button was released */

          if (!(sample & button_bit) && (oldsample & button_bit))
            {
              printf("%s was released\n", button_name[i]);
            }
        }

      oldsample = sample;
#else
      printf("Sample = 0x%08lx\n", (unsigned long)sample);
#endif

      /* Make sure that everything is displayed */

      fflush(stdout);

      usleep(1000);
    }

errout_with_fd:
  close(fd);

errout:
  g_button_daemon_started = false;

  printf("button_daemon: Terminating\n");
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * buttons_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  printf("buttons_main: Starting the button_daemon\n");
  if (g_button_daemon_started)
    {
      printf("buttons_main: button_daemon already running\n");
      return EXIT_SUCCESS;
    }

  ret = task_create("button_daemon", CONFIG_EXAMPLES_BUTTONS_PRIORITY,
                    CONFIG_EXAMPLES_BUTTONS_STACKSIZE, button_daemon,
                    NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("buttons_main: ERROR: Failed to start button_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  printf("buttons_main: button_daemon started\n");
  return EXIT_SUCCESS;
}
