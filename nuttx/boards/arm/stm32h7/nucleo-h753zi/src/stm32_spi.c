/**
 * boards/arm/stm32h7/nucleo-h753zi/src/stm32_spi.c
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
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 **/

/**
 * Included Files
 **/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <nuttx/spi/spi.h>

#include "arm_internal.h"
#include "chip.h"
#include "stm32_gpio.h"
#include "stm32_spi.h"
#include "nucleo-h753zi.h"

#ifdef CONFIG_STM32H7_SPI

/**
 * Pre-processor Definitions
 **/

/* Maximum number of CS pins per SPI */
#define MAX_CS_PINS_PER_SPI  8

/* SPI Device ID mapping:
 * SPI1: Device IDs 0-7  (SPIDEV_USER_DEFINED(0) to SPIDEV_USER_DEFINED(7))
 * SPI2: Device IDs 8-15 (SPIDEV_USER_DEFINED(8) to SPIDEV_USER_DEFINED(15))
 * SPI3: Device IDs 16-23 (SPIDEV_USER_DEFINED(16) to SPIDEV_USER_DEFINED(23))
 * SPI4: Device IDs 24-31 (SPIDEV_USER_DEFINED(24) to SPIDEV_USER_DEFINED(31))
 * SPI5: Device IDs 32-39 (SPIDEV_USER_DEFINED(32) to SPIDEV_USER_DEFINED(39))
 * SPI6: Device IDs 40-47 (SPIDEV_USER_DEFINED(40) to SPIDEV_USER_DEFINED(47))
 */
#define SPI1_DEVID_BASE  0
#define SPI2_DEVID_BASE  8
#define SPI3_DEVID_BASE  16
#define SPI4_DEVID_BASE  24
#define SPI5_DEVID_BASE  32
#define SPI6_DEVID_BASE  40

/**
 * Private Types
 **/

struct spi_cs_config_s
{
  uint32_t gpio_pins[MAX_CS_PINS_PER_SPI];  /* GPIO configurations for CS pins */
  uint8_t num_cs;                           /* Number of configured CS pins */
};

/**
 * Private Data
 **/

#ifdef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
static struct spi_cs_config_s g_spi1_cs_config = {0};
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
static struct spi_cs_config_s g_spi2_cs_config = {0};
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE
static struct spi_cs_config_s g_spi3_cs_config = {0};
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
static struct spi_cs_config_s g_spi4_cs_config = {0};
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
static struct spi_cs_config_s g_spi5_cs_config = {0};
#endif
#ifdef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
static struct spi_cs_config_s g_spi6_cs_config = {0};
#endif

/**
 * Private Functions
 **/

/**
 * Name: parse_gpio_pin
 *
 * Description:
 * Parse a GPIO pin string (e.g., "PE4", "PA15") and return the GPIO
 * configuration.
 *
 * Input Parameters:
 * pin_str - String representation of the pin (e.g., "PE4")
 *
 * Returned Value:
 * GPIO configuration value, or 0 if parsing fails
 *
 **/
static uint32_t parse_gpio_pin(const char *pin_str)
{
  if (pin_str == NULL || strlen(pin_str) < 3)
    {
      return 0;
    }

  char port_char = toupper(pin_str[1]); /* Corrected index for 'E' in "PE4" */
  uint8_t pin_num = 0;
  uint32_t port_base;

  /* Parse pin number */
  if (sscanf(&pin_str[2], "%hhu", &pin_num) != 1 || pin_num > 15)
    {
      spierr("ERROR: Invalid pin number in %s\n", pin_str);
      return 0;
    }

  /* Determine port base */
  switch (port_char)
    {
      case 'A': port_base = GPIO_PORTA; break;
      case 'B': port_base = GPIO_PORTB; break;
      case 'C': port_base = GPIO_PORTC; break;
      case 'D': port_base = GPIO_PORTD; break;
      case 'E': port_base = GPIO_PORTE; break;
      case 'F': port_base = GPIO_PORTF; break;
      case 'G': port_base = GPIO_PORTG; break;
      case 'H': port_base = GPIO_PORTH; break;
      case 'I': port_base = GPIO_PORTI; break;
      case 'J': port_base = GPIO_PORTJ; break;
      case 'K': port_base = GPIO_PORTK; break;
      default:
        spierr("ERROR: Invalid port %c in %s\n", port_char, pin_str);
        return 0;
    }

  /* Build GPIO configuration for CS pin (output, push-pull, high speed) */
  return (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | 
          GPIO_OUTPUT_SET | port_base | GPIO_PIN(pin_num));
}

/**
 * Name: parse_cs_pins
 *
 * Description:
 * Parse comma-separated CS pin configuration string.
 *
 * Input Parameters:
 * cs_pins_str - String with comma-separated pin names (e.g., "PE0,PE1,PE3")
 * config      - Pointer to CS configuration structure to fill
 *
 * Returned Value:
 * Number of pins parsed, or negative error code
 *
 **/
static int parse_cs_pins(const char *cs_pins_str, struct spi_cs_config_s *config)
{
  char *pins_copy;
  char *token;
  char *saveptr;
  int pin_count = 0;

  if (cs_pins_str == NULL || strlen(cs_pins_str) == 0)
    {
      config->num_cs = 0;
      return 0;
    }

  /* Make a copy of the string for tokenization */
  pins_copy = strdup(cs_pins_str);
  if (pins_copy == NULL)
    {
      spierr("ERROR: Failed to allocate memory for pin parsing\n");
      return -ENOMEM;
    }

  /* Parse each pin */
  token = strtok_r(pins_copy, ",", &saveptr);
  while (token != NULL && pin_count < MAX_CS_PINS_PER_SPI)
    {
      /* Remove leading whitespace */
      while (isspace(*token)) token++;
      
      /* Remove trailing whitespace */
      char *end = token + strlen(token) - 1;
      while (end > token && isspace(*end)) *end-- = '\0';
      
      if (strlen(token) > 0)
        {
          uint32_t gpio_config = parse_gpio_pin(token);
          if (gpio_config == 0)
            {
              spierr("ERROR: Failed to parse GPIO pin: %s\n", token);
              free(pins_copy);
              return -EINVAL;
            }

          config->gpio_pins[pin_count] = gpio_config;
          pin_count++;
          spiinfo("Parsed CS pin %d: %s -> 0x%08x\n",
                  pin_count - 1, token, gpio_config);
        }

      token = strtok_r(NULL, ",", &saveptr);
    }

  free(pins_copy);
  config->num_cs = pin_count;
  spiinfo("Parsed %d CS pins total\n", pin_count);
  return pin_count;
}

/**
 * Name: spi_cs_select
 *
 * Description:
 * Generic CS selection function for any SPI.
 *
 * Input Parameters:
 * config   - CS configuration for this SPI
 * devid    - Device ID 
 * selected - true: assert CS, false: deassert CS
 * spi_base - Base device ID for this SPI (for calculating CS index)
 *
 **/
static void spi_cs_select(struct spi_cs_config_s *config, uint32_t devid, 
                          bool selected, uint32_t spi_base)
{
  int cs_index;

  if (config == NULL || config->num_cs == 0)
    {
      spiwarn("WARNING: No CS pins configured\n");
      return;
    }

  /* Calculate CS index from device ID */
  cs_index = devid - spi_base;

  if (cs_index < 0 || cs_index >= config->num_cs)
    {
      spierr("ERROR: Invalid device ID %d for SPI base %d (CS index %d)\n", 
             devid, spi_base, cs_index);
      return;
    }

  /* Control the CS pin (active low) */
  stm32_gpiowrite(config->gpio_pins[cs_index], !selected);

  spiinfo("SPI CS%d (devid=%d): %s\n",
          cs_index, devid, selected ? "ASSERT" : "DEASSERT");
}

/**
 * Public Functions
 **/

/**
 * Name: stm32_spidev_initialize
 *
 * Description:
 * Called to configure SPI chip select GPIO pins for the Nucleo-H753ZI.
 *
 **/
void stm32_spidev_initialize(void)
{
  int ret;
  int total_pins = 0;

  spiinfo("Initializing SPI CS pins for Nucleo-H753ZI\n");

#ifdef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI1_CS_PINS, &g_spi1_cs_config);
  if (ret > 0)
    {
      spiinfo("SPI1: Configuring %d CS pins\n", ret);
      for (int i = 0; i < g_spi1_cs_config.num_cs; i++)
        {
          stm32_configgpio(g_spi1_cs_config.gpio_pins[i]);
          stm32_gpiowrite(g_spi1_cs_config.gpio_pins[i], true); /* Deasserted */
          total_pins++;
        }
    }
  else if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI1 CS pins: %d\n", ret);
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI2_CS_PINS, &g_spi2_cs_config);
  if (ret > 0)
    {
      spiinfo("SPI2: Configuring %d CS pins\n", ret);
      for (int i = 0; i < g_spi2_cs_config.num_cs; i++)
        {
          stm32_configgpio(g_spi2_cs_config.gpio_pins[i]);
          stm32_gpiowrite(g_spi2_cs_config.gpio_pins[i], true); /* Deasserted */
          total_pins++;
        }
    }
  else if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI2 CS pins: %d\n", ret);
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI3_CS_PINS, &g_spi3_cs_config);
  if (ret > 0)
    {
      spiinfo("SPI3: Configuring %d CS pins\n", ret);
      for (int i = 0; i < g_spi3_cs_config.num_cs; i++)
        {
          stm32_configgpio(g_spi3_cs_config.gpio_pins[i]);
          stm32_gpiowrite(g_spi3_cs_config.gpio_pins[i], true); /* Deasserted */
          total_pins++;
        }
    }
  else if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI3 CS pins: %d\n", ret);
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI4_CS_PINS, &g_spi4_cs_config);
  if (ret > 0)
    {
      spiinfo("SPI4: Configuring %d CS pins\n", ret);
      for (int i = 0; i < g_spi4_cs_config.num_cs; i++)
        {
          stm32_configgpio(g_spi4_cs_config.gpio_pins[i]);
          stm32_gpiowrite(g_spi4_cs_config.gpio_pins[i], true); /* Deasserted */
          total_pins++;
        }
    }
  else if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI4 CS pins: %d\n", ret);
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI5_CS_PINS, &g_spi5_cs_config);
  if (ret > 0)
    {
      spiinfo("SPI5: Configuring %d CS pins\n", ret);
      for (int i = 0; i < g_spi5_cs_config.num_cs; i++)
        {
          stm32_configgpio(g_spi5_cs_config.gpio_pins[i]);
          stm32_gpiowrite(g_spi5_cs_config.gpio_pins[i], true); /* Deasserted */
          total_pins++;
        }
    }
  else if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI5 CS pins: %d\n", ret);
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
  ret = parse_cs_pins(CONFIG_NUCLEO_H753ZI_SPI6_CS_PINS, &g_spi6_cs_config);
  if (ret > 0)
    {
      spiinfo("SPI6: Configuring %d CS pins\n", ret);
      for (int i = 0; i < g_spi6_cs_config.num_cs; i++)
        {
          stm32_configgpio(g_spi6_cs_config.gpio_pins[i]);
          stm32_gpiowrite(g_spi6_cs_config.gpio_pins[i], true); /* Deasserted */
          total_pins++;
        }
    }
  else if (ret < 0)
    {
      spierr("ERROR: Failed to parse SPI6 CS pins: %d\n", ret);
    }
#endif

  spiinfo("SPI CS initialization complete: %d total pins configured\n", total_pins);
}

/**
 * Name: stm32_spi_initialize
 *
 * Description:
 * Initialize SPI buses and bind them to the SPI driver.
 *
 **/
int stm32_spi_initialize(void)
{
  struct spi_dev_s *spi_dev;
  int ret = OK;

  spiinfo("Initializing SPI buses\n");

  /* Initialize SPI CS pins first */
  stm32_spidev_initialize();

#ifdef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
  spi_dev = stm32_spibus_initialize(1);
  if (spi_dev == NULL)
    {
      spierr("ERROR: Failed to initialize SPI1\n");
      ret = -ENODEV;
    }
  else
    {
      spiinfo("SPI1 initialized successfully\n");
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
  spi_dev = stm32_spibus_initialize(2);
  if (spi_dev == NULL)
    {
      spierr("ERROR: Failed to initialize SPI2\n");
      ret = -ENODEV;
    }
  else
    {
      spiinfo("SPI2 initialized successfully\n");
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE
  spi_dev = stm32_spibus_initialize(3);
  if (spi_dev == NULL)
    {
      spierr("ERROR: Failed to initialize SPI3\n");
      ret = -ENODEV;
    }
  else
    {
      spiinfo("SPI3 initialized successfully\n");
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
  spi_dev = stm32_spibus_initialize(4);
  if (spi_dev == NULL)
    {
      spierr("ERROR: Failed to initialize SPI4\n");
      ret = -ENODEV;
    }
  else
    {
      spiinfo("SPI4 initialized successfully\n");
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
  spi_dev = stm32_spibus_initialize(5);
  if (spi_dev == NULL)
    {
      spierr("ERROR: Failed to initialize SPI5\n");
      ret = -ENODEV;
    }
  else
    {
      spiinfo("SPI5 initialized successfully\n");
    }
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
  spi_dev = stm32_spibus_initialize(6);
  if (spi_dev == NULL)
    {
      spierr("ERROR: Failed to initialize SPI6\n");
      ret = -ENODEV;
    }
  else
    {
      spiinfo("SPI6 initialized successfully\n");
    }
#endif

  return ret;
}

/**
 * Name: stm32_spiX_select and stm32_spiX_status
 *
 * Description:
 * SPI select and status functions for each SPI bus.
 *
 **/
#ifdef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
void stm32_spi1select(struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(&g_spi1_cs_config, devid, selected, SPI1_DEVID_BASE);
}

uint8_t stm32_spi1status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0; /* Device is always present */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
void stm32_spi2select(struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(&g_spi2_cs_config, devid, selected, SPI2_DEVID_BASE);
}

uint8_t stm32_spi2status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0; /* Device is always present */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE
void stm32_spi3select(struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(&g_spi3_cs_config, devid, selected, SPI3_DEVID_BASE);
}

uint8_t stm32_spi3status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0; /* Device is always present */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
void stm32_spi4select(struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(&g_spi4_cs_config, devid, selected, SPI4_DEVID_BASE);
}

uint8_t stm32_spi4status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0; /* Device is always present */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
void stm32_spi5select(struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(&g_spi5_cs_config, devid, selected, SPI5_DEVID_BASE);
}

uint8_t stm32_spi5status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0; /* Device is always present */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
void stm32_spi6select(struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spi_cs_select(&g_spi6_cs_config, devid, selected, SPI6_DEVID_BASE);
}

uint8_t stm32_spi6status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0; /* Device is always present */
}
#endif

/**
 * Name: stm32_spiX_cmddata
 *
 * Description:
 * Command/Data selection for SPI displays (if needed).
 *
 **/
#ifdef CONFIG_SPI_CMDDATA
#ifdef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
int stm32_spi1cmddata(struct spi_dev_s *dev, uint32_t devid, bool cmd)
{
  return -ENODEV; /* Not implemented */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
int stm32_spi2cmddata(struct spi_dev_s *dev, uint32_t devid, bool cmd)
{
  return -ENODEV; /* Not implemented */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE
int stm32_spi3cmddata(struct spi_dev_s *dev, uint32_t devid, bool cmd)
{
  return -ENODEV; /* Not implemented */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
int stm32_spi4cmddata(struct spi_dev_s *dev, uint32_t devid, bool cmd)
{
  return -ENODEV; /* Not implemented */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
int stm32_spi5cmddata(struct spi_dev_s *dev, uint32_t devid, bool cmd)
{
  return -ENODEV; /* Not implemented */
}
#endif

#ifdef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
int stm32_spi6cmddata(struct spi_dev_s *dev, uint32_t devid, bool cmd)
{
  return -ENODEV; /* Not implemented */
}
#endif
#endif /* CONFIG_SPI_CMDDATA */
#endif /* CONFIG_STM32H7_SPI */
