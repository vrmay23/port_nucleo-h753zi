/****************************************************************************
 * boards/arm/stm32h7/nucleo-h753zi/src/stm32_buttons.c
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

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>

#include <nuttx/irq.h>
#include <nuttx/board.h>
#include <arch/board/board.h>

#include "stm32_gpio.h"
#include "nucleo-h753zi.h"

#ifdef CONFIG_NUCLEO_H753ZI_BUTTON_SUPPORT

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_INPUT_BUTTONS) && !defined(CONFIG_ARCH_IRQBUTTONS)
#  error "The NuttX Buttons Driver depends on IRQ support to work!"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Dynamic button configuration array */

static uint32_t g_buttons[CONFIG_NUCLEO_H753ZI_BUTTON_COUNT];
static int g_button_count = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: parse_gpio_pin
 *
 * Description:
 *   Parse GPIO pin string like "PF15" into STM32 GPIO configuration.
 *
 * Input Parameters:
 *   pin_str - GPIO pin string (e.g., "PA0", "PF15", "PC13")
 *   error   - Pointer to error code storage
 *
 * Returned Value:
 *   STM32 GPIO configuration value on success, 0 on error
 *
 ****************************************************************************/

static uint32_t parse_gpio_pin(FAR const char *pin_str, FAR int *error)
{
  size_t len;
  char port;
  FAR const char *pin_num_str;
  FAR char *endptr;
  long pin_num;
  uint32_t port_base;
  uint32_t gpio_pin;

  *error = 0;

  if (pin_str == NULL)
    {
      *error = -EINVAL;
      return 0;
    }

  /* Remove leading/trailing spaces */

  while (*pin_str == ' ' || *pin_str == '\t')
    {
      pin_str++;
    }

  len = strlen(pin_str);
  if (len < 3 || len > 4)
    {
      *error = -EINVAL;
      return 0;
    }

  if (pin_str[0] != 'P')
    {
      *error = -EINVAL;
      return 0;
    }

  port = pin_str[1];
  if (port < 'A' || port > 'H')
    {
      *error = -EINVAL;
      return 0;
    }

  pin_num_str = &pin_str[2];
  pin_num = strtol(pin_num_str, &endptr, 10);
  if (*endptr != '\0' || pin_num < 0 || pin_num > 15)
    {
      *error = -EINVAL;
      return 0;
    }

  /* Map port letter to STM32 port base */

  switch (port)
    {
      case 'A':
        port_base = GPIO_PORTA;
        break;
      case 'B':
        port_base = GPIO_PORTB;
        break;
      case 'C':
        port_base = GPIO_PORTC;
        break;
      case 'D':
        port_base = GPIO_PORTD;
        break;
      case 'E':
        port_base = GPIO_PORTE;
        break;
      case 'F':
        port_base = GPIO_PORTF;
        break;
      case 'G':
        port_base = GPIO_PORTG;
        break;
      case 'H':
        port_base = GPIO_PORTH;
      default:
        *error = -EINVAL;
        return 0;
    }

  /* Use correct STM32 GPIO pin macros instead of bit shifting */

  switch (pin_num)
    {
      case 0:  gpio_pin = GPIO_PIN0;  break;
      case 1:  gpio_pin = GPIO_PIN1;  break;
      case 2:  gpio_pin = GPIO_PIN2;  break;
      case 3:  gpio_pin = GPIO_PIN3;  break;
      case 4:  gpio_pin = GPIO_PIN4;  break;
      case 5:  gpio_pin = GPIO_PIN5;  break;
      case 6:  gpio_pin = GPIO_PIN6;  break;
      case 7:  gpio_pin = GPIO_PIN7;  break;
      case 8:  gpio_pin = GPIO_PIN8;  break;
      case 9:  gpio_pin = GPIO_PIN9;  break;
      case 10: gpio_pin = GPIO_PIN10; break;
      case 11: gpio_pin = GPIO_PIN11; break;
      case 12: gpio_pin = GPIO_PIN12; break;
      case 13: gpio_pin = GPIO_PIN13; break;
      case 14: gpio_pin = GPIO_PIN14; break;
      case 15: gpio_pin = GPIO_PIN15; break;
      default:
        *error = -EINVAL;
        return 0;
    }

  return (GPIO_INPUT | GPIO_FLOAT | GPIO_EXTI | port_base | gpio_pin);
}

/****************************************************************************
 * Name: init_button_configs
 *
 * Description:
 *   Initialize button configuration from Kconfig settings with validation.
 *
 * Returned Value:
 *   OK on success, negative errno on error
 *
 ****************************************************************************/

static int init_button_configs(void)
{
  int expected_pins;
  int provided_pins = 0;
  FAR const char *pins_config;
  char pins_str[512];
  char count_str[512];
  FAR char *pin;
  int pin_index;
  int error;
  uint32_t gpio_config;
  int i;

  g_button_count = 0;

  syslog(LOG_INFO, "nucleo-h753zi: Initializing button configuration\n");

  /* Calculate how many external pins we expect */

#ifdef CONFIG_NUCLEO_H753ZI_BUTTON_BUILTIN
  g_buttons[g_button_count++] = GPIO_BTN_BUILT_IN;
  expected_pins = CONFIG_NUCLEO_H753ZI_BUTTON_COUNT - 1;
  syslog(LOG_INFO, "nucleo-h753zi: Built-in enabled, expecting %d "
         "external pins\n", expected_pins);
#else
  expected_pins = CONFIG_NUCLEO_H753ZI_BUTTON_COUNT;
  syslog(LOG_INFO, "nucleo-h753zi: Built-in disabled, expecting %d "
         "total pins\n", expected_pins);
#endif

  /* If no external pins needed, we're done */

  if (expected_pins == 0)
    {
      syslog(LOG_INFO, "nucleo-h753zi: Button configuration complete: "
             "%d buttons\n", g_button_count);
      return OK;
    }

  /* Validate pin string is not empty */

  pins_config = CONFIG_NUCLEO_H753ZI_BUTTON_PINS;
  if (pins_config == NULL || strlen(pins_config) == 0)
    {
      syslog(LOG_ERR, "nucleo-h753zi: ERROR: Button configuration "
             "invalid!\n");
      syslog(LOG_ERR, "nucleo-h753zi: Expected %d GPIO pins but "
             "NUCLEO_H753ZI_BUTTON_PINS is empty.\n", expected_pins);
      syslog(LOG_ERR, "nucleo-h753zi: Please configure GPIO pins in "
             "menuconfig:\n");
      syslog(LOG_ERR, "nucleo-h753zi: Board Selection -> Button "
             "Configuration -> Button GPIO pin list\n");
      return -EINVAL;
    }

  /* Make a copy for parsing (strtok modifies the string) */

  strncpy(pins_str, pins_config, sizeof(pins_str) - 1);
  pins_str[sizeof(pins_str) - 1] = '\0';

  /* First pass: count provided pins */

  strcpy(count_str, pins_str);
  pin = strtok(count_str, ", \t\n\r");
  while (pin != NULL)
    {
      provided_pins++;
      pin = strtok(NULL, ", \t\n\r");
    }

  /* Validate pin count */

  if (provided_pins != expected_pins)
    {
      syslog(LOG_ERR, "nucleo-h753zi: ERROR: Button pin count mismatch!\n");
      syslog(LOG_ERR, "nucleo-h753zi: Configuration: "
             "NUCLEO_H753ZI_BUTTON_COUNT = %d\n",
             CONFIG_NUCLEO_H753ZI_BUTTON_COUNT);
#ifdef CONFIG_NUCLEO_H753ZI_BUTTON_BUILTIN
      syslog(LOG_ERR, "nucleo-h753zi: Built-in button: ENABLED (uses "
             "PC13)\n");
      syslog(LOG_ERR, "nucleo-h753zi: External pins needed: %d\n",
             expected_pins);
#else
      syslog(LOG_ERR, "nucleo-h753zi: Built-in button: DISABLED\n");
      syslog(LOG_ERR, "nucleo-h753zi: Total pins needed: %d\n",
             expected_pins);
#endif
      syslog(LOG_ERR, "nucleo-h753zi: Pins provided: %d\n", provided_pins);
      syslog(LOG_ERR, "nucleo-h753zi: Pin list: \"%s\"\n", pins_config);
      syslog(LOG_ERR, "nucleo-h753zi: SOLUTION:\n");
      if (provided_pins < expected_pins)
        {
          syslog(LOG_ERR, "nucleo-h753zi: Add %d more GPIO pins to the "
                 "pin list, OR\n", expected_pins - provided_pins);
          syslog(LOG_ERR, "nucleo-h753zi: Reduce "
                 "NUCLEO_H753ZI_BUTTON_COUNT to %d\n",
                 provided_pins +
                 (CONFIG_NUCLEO_H753ZI_BUTTON_COUNT - expected_pins));
        }
      else
        {
          syslog(LOG_ERR, "nucleo-h753zi: Remove %d GPIO pins from the "
                 "pin list, OR\n", provided_pins - expected_pins);
          syslog(LOG_ERR, "nucleo-h753zi: Increase "
                 "NUCLEO_H753ZI_BUTTON_COUNT to %d\n",
                 provided_pins +
                 (CONFIG_NUCLEO_H753ZI_BUTTON_COUNT - expected_pins));
        }

      return -EINVAL;
    }

  /* Second pass: parse and validate each pin */

  pin = strtok(pins_str, ", \t\n\r");
  pin_index = 0;
  while (pin != NULL && g_button_count < CONFIG_NUCLEO_H753ZI_BUTTON_COUNT)
    {
      gpio_config = parse_gpio_pin(pin, &error);
      if (error != 0)
        {
          syslog(LOG_ERR, "nucleo-h753zi: ERROR: Invalid GPIO pin at "
                 "position %d\n", pin_index + 1);
          syslog(LOG_ERR, "nucleo-h753zi: Pin string: \"%s\"\n", pin);
          syslog(LOG_ERR, "nucleo-h753zi: Full config: \"%s\"\n",
                 pins_config);
          syslog(LOG_ERR, "nucleo-h753zi: SOLUTION:\n");
          syslog(LOG_ERR, "nucleo-h753zi: Use format: PORT+PIN "
                 "(e.g., \"PA0\", \"PB12\", \"PC13\")\n");
          syslog(LOG_ERR, "nucleo-h753zi: Valid ports: PA, PB, PC, PD, "
                 "PE, PF, PG, PH\n");
          syslog(LOG_ERR, "nucleo-h753zi: Valid pins: 0-15\n");
          syslog(LOG_ERR, "nucleo-h753zi: Examples: PA0, PF15, PG14, "
                 "PE0\n");
          return -EINVAL;
        }

      /* Check for duplicate pins */

      for (i = 0; i < g_button_count; i++)
        {
          if (g_buttons[i] == gpio_config)
            {
              syslog(LOG_ERR, "nucleo-h753zi: ERROR: Duplicate GPIO pin "
                     "detected!\n");
              syslog(LOG_ERR, "nucleo-h753zi: Pin \"%s\" is used "
                     "multiple times\n", pin);
              syslog(LOG_ERR, "nucleo-h753zi: Position: %d\n",
                     pin_index + 1);
              syslog(LOG_ERR, "nucleo-h753zi: Full config: \"%s\"\n",
                     pins_config);
              syslog(LOG_ERR, "nucleo-h753zi: SOLUTION: Remove duplicate "
                     "pins from the configuration\n");
              return -EINVAL;
            }
        }

      g_buttons[g_button_count++] = gpio_config;
      syslog(LOG_INFO, "nucleo-h753zi: Button %d: %s configured "
             "successfully\n", g_button_count - 1, pin);

      pin_index++;
      pin = strtok(NULL, ", \t\n\r");
    }

  syslog(LOG_INFO, "nucleo-h753zi: Button configuration completed "
         "successfully:\n");
  syslog(LOG_INFO, "nucleo-h753zi: Total buttons: %d\n", g_button_count);
#ifdef CONFIG_NUCLEO_H753ZI_BUTTON_BUILTIN
  syslog(LOG_INFO, "nucleo-h753zi: Built-in (PC13): Button 0\n");
  syslog(LOG_INFO, "nucleo-h753zi: External buttons: %d\n",
         g_button_count - 1);
#else
  syslog(LOG_INFO, "nucleo-h753zi: All external buttons: %d\n",
         g_button_count);
#endif

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_button_initialize
 *
 * Description:
 *   board_button_initialize() must be called to initialize button
 *   resources. After that, board_buttons() may be called to collect the
 *   current state of all buttons or board_button_irq() may be called to
 *   register button interrupt handlers.
 *
 ****************************************************************************/

uint32_t board_button_initialize(void)
{
  int ret;
  int i;

  ret = init_button_configs();
  if (ret < 0)
    {
      syslog(LOG_ERR, "nucleo-h753zi: === BUTTON CONFIGURATION FAILED "
             "===\n");
      syslog(LOG_ERR, "nucleo-h753zi: The system cannot start with "
             "invalid button configuration.\n");
      syslog(LOG_ERR, "nucleo-h753zi: Please fix the configuration "
             "errors above and rebuild.\n");
      syslog(LOG_ERR, "nucleo-h753zi: ============================="
             "==========\n");
      return 0;  /* Return 0 to indicate failure */
    }

  /* Configure GPIO pins */

  for (i = 0; i < g_button_count; i++)
    {
      ret = stm32_configgpio(g_buttons[i]);
      if (ret < 0)
        {
          syslog(LOG_ERR, "nucleo-h753zi: ERROR: Failed to configure "
                 "GPIO for button %d (ret=%d)\n", i, ret);
          return 0;
        }
    }

  syslog(LOG_INFO, "nucleo-h753zi: Button driver initialized with "
         "%d buttons\n", g_button_count);

  return g_button_count;
}

/****************************************************************************
 * Name: board_buttons
 *
 * Description:
 *   board_buttons() may be called to collect the current state of all
 *   buttons. board_buttons() returns a 32-bit bit set with each bit
 *   associated with a button.  See the BUTTON_*_BIT definitions in
 *   board.h for the meaning of each bit.
 *
 * Returned Value:
 *   32-bit set of button states. Bit set = button pressed.
 *
 ****************************************************************************/

uint32_t board_buttons(void)
{
  uint32_t ret = 0;
  bool released;
  int i;

  /* Check the state of each button */

  for (i = 0; i < g_button_count; i++)
    {
      /* A LOW value means button is pressed (pull-down config) */

      released = stm32_gpioread(g_buttons[i]);

      /* Accumulate the set of depressed (released) keys */

      if (released)
        {
          ret |= (1 << i);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: board_button_irq
 *
 * Description:
 *   board_button_irq() may be called to register an interrupt handler
 *   that will be called when a button is depressed or released.  The ID
 *   value is a button enumeration value that uniquely identifies a button
 *   resource. See the BUTTON_* definitions in board.h for the meaning of
 *   enumeration value.
 *
 * Input Parameters:
 *   id         - Button ID (0-based index)
 *   irqhandler - IRQ handler function
 *   arg        - Argument passed to IRQ handler
 *
 * Returned Value:
 *   OK on success, negative errno on error
 *
 ****************************************************************************/

#ifdef CONFIG_ARCH_IRQBUTTONS
int board_button_irq(int id, xcpt_t irqhandler, FAR void *arg)
{
  int ret = -EINVAL;

  /* Validate button ID */

  if (id >= 0 && id < g_button_count)
    {
      ret = stm32_gpiosetevent(g_buttons[id], true, true, true,
                               irqhandler, arg);
      if (ret >= 0)
        {
          syslog(LOG_INFO, "nucleo-h753zi: IRQ handler registered for "
                 "button %d\n", id);
        }
      else
        {
          syslog(LOG_ERR, "nucleo-h753zi: Failed to register IRQ for "
                 "button %d (ret=%d)\n", id, ret);
        }
    }
  else
    {
      syslog(LOG_ERR, "nucleo-h753zi: Invalid button ID %d "
             "(valid range: 0-%d)\n", id, g_button_count - 1);
    }

  return ret;
}
#endif

#endif /* CONFIG_NUCLEO_H753ZI_BUTTON_SUPPORT */
