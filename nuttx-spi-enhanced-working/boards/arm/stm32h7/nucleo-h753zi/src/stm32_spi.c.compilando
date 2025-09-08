/****************************************************************************
 * boards/arm/stm32h7/nucleo-h753zi/src/stm32_spi.c
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
#include <debug.h>
#include <nuttx/spi/spi.h>
#include <arch/board/board.h>
#include "stm32_gpio.h"
#include "stm32_spi.h"
#include "nucleo-h753zi.h"

#ifdef CONFIG_STM32H7_SPI

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_CS_PINS_PER_SPI 8

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* SPI CS pin configurations for each SPI */
static uint32_t g_spi1_cs_pins[MAX_CS_PINS_PER_SPI];
static int g_spi1_cs_count = 0;

static uint32_t g_spi2_cs_pins[MAX_CS_PINS_PER_SPI];
static int g_spi2_cs_count = 0;

static uint32_t g_spi3_cs_pins[MAX_CS_PINS_PER_SPI];
static int g_spi3_cs_count = 0;

static uint32_t g_spi4_cs_pins[MAX_CS_PINS_PER_SPI];
static int g_spi4_cs_count = 0;

static uint32_t g_spi5_cs_pins[MAX_CS_PINS_PER_SPI];
static int g_spi5_cs_count = 0;

static uint32_t g_spi6_cs_pins[MAX_CS_PINS_PER_SPI];
static int g_spi6_cs_count = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: parse_gpio_pin
 *
 * Description:
 *   Parse GPIO pin string like "PA0" into STM32 GPIO configuration.
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
  if (port < 'A' || port > 'H') /* STM32H753ZI only has ports A-H */
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
      case 'A': port_base = GPIO_PORTA; break;
      case 'B': port_base = GPIO_PORTB; break;
      case 'C': port_base = GPIO_PORTC; break;
      case 'D': port_base = GPIO_PORTD; break;
      case 'E': port_base = GPIO_PORTE; break;
      case 'F': port_base = GPIO_PORTF; break;
      case 'G': port_base = GPIO_PORTG; break;
      case 'H': port_base = GPIO_PORTH; break;
      // case 'I': port_base = GPIO_PORTI; break;
      // case 'J': port_base = GPIO_PORTJ; break;
      // case 'K': port_base = GPIO_PORTK; break;
      default:
        *error = -EINVAL;
        return 0;
    }

  /* Use correct STM32 GPIO pin macros */
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

  return (GPIO_OUTPUT | GPIO_OUTPUT_SET | port_base | gpio_pin);
}

/****************************************************************************
 * Name: parse_cs_pins
 *
 * Description:
 *   Parse CS pin configuration string and store in array.
 *
 * Input Parameters:
 *   cs_pins_str - Comma-separated CS pin string
 *   cs_pins     - Array to store parsed pins
 *   max_pins    - Maximum pins in array
 *   cs_count    - Pointer to store actual count
 *
 * Returned Value:
 *   OK on success, negative errno on error
 *
 ****************************************************************************/

static int parse_cs_pins(FAR const char *cs_pins_str,
                         FAR uint32_t *cs_pins,
                         int max_pins,
                         FAR int *cs_count)
{
  char pins_str[256];
  FAR char *token;
  int pin_count = 0;
  int error;
  uint32_t gpio_config;

  *cs_count = 0;

  if (cs_pins_str == NULL || strlen(cs_pins_str) == 0)
    {
      return OK; /* No CS pins configured */
    }

  /* Make a copy for parsing */
  strncpy(pins_str, cs_pins_str, sizeof(pins_str) - 1);
  pins_str[sizeof(pins_str) - 1] = '\0';

  token = strtok(pins_str, ", \t\n\r");
  while (token != NULL && pin_count < max_pins)
    {
      gpio_config = parse_gpio_pin(token, &error);
      if (error != 0)
        {
          spierr("ERROR: Invalid CS pin: %s\n", token);
          return error;
        }

      cs_pins[pin_count] = gpio_config;
      pin_count++;

      spiinfo("Parsed CS pin %d: %s -> 0x%08lx\n",
              pin_count - 1, token, (unsigned long)gpio_config);

      token = strtok(NULL, ", \t\n\r");
    }

  *cs_count = pin_count;
  return OK;
}

/****************************************************************************
 * Name: spi_cs_select
 *
 * Description:
 *   Select/deselect SPI chip select pin.
 *
 * Input Parameters:
 *   devid    - Device ID
 *   selected - Select (true) or deselect (false)
 *
 ****************************************************************************/

static void spi_cs_select(uint32_t devid, bool selected)
{
  uint32_t spi_base = devid / MAX_CS_PINS_PER_SPI;
  uint32_t cs_index = devid % MAX_CS_PINS_PER_SPI;
  uint32_t *cs_pins = NULL;
  int cs_count = 0;

  /* Map device ID to appropriate CS pin array */
  switch (spi_base)
    {
#ifdef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
      case 0: /* SPI1 */
        cs_pins = g_spi1_cs_pins;
        cs_count = g_spi1_cs_count;
        break;
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
      case 1: /* SPI2 */
        cs_pins = g_spi2_cs_pins;
        cs_count = g_spi2_cs_count;
        break;
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE
      case 2: /* SPI3 */
        cs_pins = g_spi3_cs_pins;
        cs_count = g_spi3_cs_count;
        break;
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
      case 3: /* SPI4 */
        cs_pins = g_spi4_cs_pins;
        cs_count = g_spi4_cs_count;
        break;
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
      case 4: /* SPI5 */
        cs_pins = g_spi5_cs_pins;
        cs_count = g_spi5_cs_count;
        break;
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
      case 5: /* SPI6 */
        cs_pins = g_spi6_cs_pins;
        cs_count = g_spi6_cs_count;
        break;
#endif
      default:
        spierr("ERROR: Invalid device ID %lu for SPI base %lu (CS index %lu)\n",
               (unsigned long)devid, (unsigned long)spi_base, (unsigned long)cs_index);
        return;
    }

  if (cs_pins == NULL || cs_index >= cs_count)
    {
      return; /* CS pin not configured */
    }

  spiinfo("SPI CS%lu (devid=%lu): %s\n",
          (unsigned long)cs_index, (unsigned long)devid,
          selected ? "ASSERT" : "DEASSERT");

  /* CS is active low - set low to select, high to deselect */
  stm32_gpiowrite(cs_pins[cs_index], !selected);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_spi_initialize
 *
 * Description:
 *   Initialize SPI interfaces and CS pins.
 *
 * Returned Value:
 *   OK on success, negative errno on error
 *
 ****************************************************************************/

int stm32_spi_initialize(void)
{
  int ret = OK;

  spiinfo("Initializing SPI interfaces\n");

#ifdef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
  /* Parse SPI1 CS pins */
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI1_CS_PINS,
                      g_spi1_cs_pins, MAX_CS_PINS_PER_SPI,
                      &g_spi1_cs_count);
  if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI1 CS pins\n");
      return ret;
    }

  /* Configure SPI1 CS pins */
  for (int i = 0; i < g_spi1_cs_count; i++)
    {
      ret = stm32_configgpio(g_spi1_cs_pins[i]);
      if (ret < 0)
        {
          spierr("ERROR: Failed to configure SPI1 CS pin %d\n", i);
          return ret;
        }
      /* Initialize CS pins as deselected (high) */
      stm32_gpiowrite(g_spi1_cs_pins[i], true);
    }

  spiinfo("SPI1 initialized with %d CS pins\n", g_spi1_cs_count);
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
  /* Parse SPI2 CS pins */
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI2_CS_PINS,
                      g_spi2_cs_pins, MAX_CS_PINS_PER_SPI,
                      &g_spi2_cs_count);
  if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI2 CS pins\n");
      return ret;
    }

  /* Configure SPI2 CS pins */
  for (int i = 0; i < g_spi2_cs_count; i++)
    {
      ret = stm32_configgpio(g_spi2_cs_pins[i]);
      if (ret < 0)
        {
          spierr("ERROR: Failed to configure SPI2 CS pin %d\n", i);
          return ret;
        }
      stm32_gpiowrite(g_spi2_cs_pins[i], true);
    }

  spiinfo("SPI2 initialized with %d CS pins\n", g_spi2_cs_count);
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE
  /* Parse SPI3 CS pins */
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI3_CS_PINS,
                      g_spi3_cs_pins, MAX_CS_PINS_PER_SPI,
                      &g_spi3_cs_count);
  if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI3 CS pins\n");
      return ret;
    }

  /* Configure SPI3 CS pins */
  for (int i = 0; i < g_spi3_cs_count; i++)
    {
      ret = stm32_configgpio(g_spi3_cs_pins[i]);
      if (ret < 0)
        {
          spierr("ERROR: Failed to configure SPI3 CS pin %d\n", i);
          return ret;
        }
      stm32_gpiowrite(g_spi3_cs_pins[i], true);
    }

  spiinfo("SPI3 initialized with %d CS pins\n", g_spi3_cs_count);
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
  /* Parse SPI4 CS pins */
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI4_CS_PINS,
                      g_spi4_cs_pins, MAX_CS_PINS_PER_SPI,
                      &g_spi4_cs_count);
  if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI4 CS pins\n");
      return ret;
    }

  /* Configure SPI4 CS pins */
  for (int i = 0; i < g_spi4_cs_count; i++)
    {
      ret = stm32_configgpio(g_spi4_cs_pins[i]);
      if (ret < 0)
        {
          spierr("ERROR: Failed to configure SPI4 CS pin %d\n", i);
          return ret;
        }
      stm32_gpiowrite(g_spi4_cs_pins[i], true);
    }

  spiinfo("SPI4 initialized with %d CS pins\n", g_spi4_cs_count);
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
  /* Parse SPI5 CS pins */
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI5_CS_PINS,
                      g_spi5_cs_pins, MAX_CS_PINS_PER_SPI,
                      &g_spi5_cs_count);
  if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI5 CS pins\n");
      return ret;
    }

  /* Configure SPI5 CS pins */
  for (int i = 0; i < g_spi5_cs_count; i++)
    {
      ret = stm32_configgpio(g_spi5_cs_pins[i]);
      if (ret < 0)
        {
          spierr("ERROR: Failed to configure SPI5 CS pin %d\n", i);
          return ret;
        }
      stm32_gpiowrite(g_spi5_cs_pins[i], true);
    }

  spiinfo("SPI5 initialized with %d CS pins\n", g_spi5_cs_count);
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
  /* Parse SPI6 CS pins */
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI6_CS_PINS,
                      g_spi6_cs_pins, MAX_CS_PINS_PER_SPI,
                      &g_spi6_cs_count);
  if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI6 CS pins\n");
      return ret;
    }

  /* Configure SPI6 CS pins */
  for (int i = 0; i < g_spi6_cs_count; i++)
    {
      ret = stm32_configgpio(g_spi6_cs_pins[i]);
      if (ret < 0)
        {
          spierr("ERROR: Failed to configure SPI6 CS pin %d\n", i);
          return ret;
        }
      stm32_gpiowrite(g_spi6_cs_pins[i], true);
    }

  spiinfo("SPI6 initialized with %d CS pins\n", g_spi6_cs_count);
#endif

  spiinfo("SPI initialization completed\n");
  return ret;
}

/****************************************************************************
 * Name: stm32_spi1/2/3/4/5/6_select
 *
 * Description:
 *   SPI select functions for each interface.
 *
 ****************************************************************************/

#ifdef CONFIG_STM32H7_SPI1
void stm32_spi1_select(FAR struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(devid, selected);
}
#endif

#ifdef CONFIG_STM32H7_SPI2
void stm32_spi2_select(FAR struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(devid + 8, selected);
}
#endif

#ifdef CONFIG_STM32H7_SPI3
void stm32_spi3_select(FAR struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(devid + 16, selected);
}
#endif

#ifdef CONFIG_STM32H7_SPI4
void stm32_spi4_select(FAR struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(devid + 24, selected);
}
#endif

#ifdef CONFIG_STM32H7_SPI5
void stm32_spi5_select(FAR struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(devid + 32, selected);
}
#endif

#ifdef CONFIG_STM32H7_SPI6
void stm32_spi6_select(FAR struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(devid + 40, selected);
}
#endif

#endif /* CONFIG_STM32H7_SPI */
