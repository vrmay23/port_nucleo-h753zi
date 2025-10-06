/****************************************************************************
 * boards/arm/stm32h7/nucleo-h753zi/src/drivers/driver_modules/stm32_st7796.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <nuttx/board.h>
#include <nuttx/spi/spi.h>
#include <nuttx/video/fb.h>
#include <nuttx/lcd/st7796.h>
#include <nuttx/clock.h>
#include <nuttx/signal.h>

#include "../../nucleo-h753zi.h"
#include "stm32_gpio.h"
#include "stm32_spi.h"

#if defined(CONFIG_LCD_ST7796) && defined(CONFIG_NUCLEO_H753ZI_ST7796_ENABLE)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Determine which SPI port to use based on Kconfig selection */

#ifdef CONFIG_NUCLEO_H753ZI_ST7796_SPI1
#  define ST7796_SPI_PORTNO 1
#  ifndef CONFIG_STM32H7_SPI1
#    error "ST7796 configured for SPI1 but CONFIG_STM32H7_SPI1 not enabled"
#  endif
#  ifndef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
#    error "ST7796 configured for SPI1 but CONFIG_NUCLEO_H753ZI_SPI1_ENABLE not enabled"
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_SPI2)
#  define ST7796_SPI_PORTNO 2
#  ifndef CONFIG_STM32H7_SPI2
#    error "ST7796 configured for SPI2 but CONFIG_STM32H7_SPI2 not enabled"
#  endif
#  ifndef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
#    error "ST7796 configured for SPI2 but CONFIG_NUCLEO_H753ZI_SPI2_ENABLE not enabled"
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_SPI3)
#  define ST7796_SPI_PORTNO 3
#  ifndef CONFIG_STM32H7_SPI3
#    error "ST7796 configured for SPI3 but CONFIG_STM32H7_SPI3 not enabled"
#  endif
#  ifndef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE
#    error "ST7796 configured for SPI3 but CONFIG_NUCLEO_H753ZI_SPI3_ENABLE not enabled"
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_SPI4)
#  define ST7796_SPI_PORTNO 4
#  ifndef CONFIG_STM32H7_SPI4
#    error "ST7796 configured for SPI4 but CONFIG_STM32H7_SPI4 not enabled"
#  endif
#  ifndef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
#    error "ST7796 configured for SPI4 but CONFIG_NUCLEO_H753ZI_SPI4_ENABLE not enabled"
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_SPI5)
#  define ST7796_SPI_PORTNO 5
#  ifndef CONFIG_STM32H7_SPI5
#    error "ST7796 configured for SPI5 but CONFIG_STM32H7_SPI5 not enabled"
#  endif
#  ifndef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
#    error "ST7796 configured for SPI5 but CONFIG_NUCLEO_H753ZI_SPI5_ENABLE not enabled"
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_SPI6)
#  define ST7796_SPI_PORTNO 6
#  ifndef CONFIG_STM32H7_SPI6
#    error "ST7796 configured for SPI6 but CONFIG_STM32H7_SPI6 not enabled"
#  endif
#  ifndef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
#    error "ST7796 configured for SPI6 but CONFIG_NUCLEO_H753ZI_SPI6_ENABLE not enabled"
#  endif
#else
#  error "No SPI port selected for ST7796. Please select one in menuconfig."
#endif

/* Default CS pin configuration if not specified */

#ifndef CONFIG_NUCLEO_H753ZI_ST7796_CS_PIN
#  define CONFIG_NUCLEO_H753ZI_ST7796_CS_PIN "PA4"
#endif

/* Default DC pin configuration if not specified */

#ifndef CONFIG_NUCLEO_H753ZI_ST7796_DC_PIN
#  define CONFIG_NUCLEO_H753ZI_ST7796_DC_PIN "PA3"
#endif

/* Default RESET pin configuration if not specified */

#ifndef CONFIG_NUCLEO_H753ZI_ST7796_RESET_PIN
#  define CONFIG_NUCLEO_H753ZI_ST7796_RESET_PIN "PA2"
#endif

/* Default LED/Backlight pin configuration if not specified */

#ifndef CONFIG_NUCLEO_H753ZI_ST7796_LED_PIN
#  define CONFIG_NUCLEO_H753ZI_ST7796_LED_PIN "PA1"
#endif

/* Default active level for CS (most displays are active low) */

#ifndef CONFIG_NUCLEO_H753ZI_ST7796_CS_ACTIVE_LOW
#  define CONFIG_NUCLEO_H753ZI_ST7796_CS_ACTIVE_LOW true
#endif

/* Device ID */

#ifndef CONFIG_NUCLEO_H753ZI_ST7796_DEVID
#  define CONFIG_NUCLEO_H753ZI_ST7796_DEVID 0
#endif

/* Define a mask to clear the mode/pull/speed configuration bits,
 * preserving the Port/Pin part. In NuttX STM32 GPIO, mode/pull/speed
 * are typically in the lower 16 bits of the configuration word.
 */

#define ST7796_GPIO_CONFIG_MASK 0xFFFF0000

/* Define a safe state for uninitialized/cleaned-up pins: Input Floating */

#define ST7796_GPIO_IN_FLOAT \
  (GPIO_INPUT | GPIO_FLOAT | GPIO_SPEED_50MHz)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint32_t g_dc_pin;
static uint32_t g_reset_pin;
static uint32_t g_led_pin;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: parse_gpio_pin
 *
 * Description:
 * Parse GPIO pin string like "PA0" into STM32 GPIO configuration.
 *
 * Input Parameters:
 * pin_str - GPIO pin string (e.g., "PA0", "PF15", "PC13")
 * error   - Pointer to error code storage
 *
 * Returned Value:
 * STM32 GPIO configuration value on success, 0 on error
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
      case 'A': port_base = GPIO_PORTA; break;
      case 'B': port_base = GPIO_PORTB; break;
      case 'C': port_base = GPIO_PORTC; break;
      case 'D': port_base = GPIO_PORTD; break;
      case 'E': port_base = GPIO_PORTE; break;
      case 'F': port_base = GPIO_PORTF; break;
      case 'G': port_base = GPIO_PORTG; break;
      case 'H': port_base = GPIO_PORTH; break;
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

  /* Corrected: Added GPIO_FLOAT for explicit pull configuration,
   * following good embedded practice for output pins.
   */

  return (GPIO_OUTPUT | GPIO_OUTPUT_SET | GPIO_SPEED_50MHz | GPIO_FLOAT |
          port_base | gpio_pin);
}

/****************************************************************************
 * Name: stm32_st7796_set_dc
 *
 * Description:
 * Control DC (Data/Command) pin.
 *
 * Input Parameters:
 * data - true for data mode, false for command mode
 *
 ****************************************************************************/

static void stm32_st7796_set_dc(bool data)
{
  stm32_gpiowrite(g_dc_pin, data);
}

/****************************************************************************
 * Name: stm32_st7796_gpio_initialize
 *
 * Description:
 * Initialize GPIO pins for ST7796 (DC, RESET, LED).
 *
 * Returned Value:
 * OK on success, negative errno on error
 *
 ****************************************************************************/

static int stm32_st7796_gpio_initialize(void)
{
  int ret;
  int error;

  syslog(LOG_INFO, "ST7796: Configuring GPIO pins...\n");

  /* Parse and configure DC pin */

  g_dc_pin = parse_gpio_pin(CONFIG_NUCLEO_H753ZI_ST7796_DC_PIN, &error);
  if (error != 0)
    {
      syslog(LOG_ERR, "ERROR: Invalid DC pin '%s': %d\n",
             CONFIG_NUCLEO_H753ZI_ST7796_DC_PIN, error);
      return error;
    }

  ret = stm32_configgpio(g_dc_pin);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to configure DC pin: %d\n", ret);
      return ret;
    }

  /* Parse and configure RESET pin */

  g_reset_pin = parse_gpio_pin(CONFIG_NUCLEO_H753ZI_ST7796_RESET_PIN,
                               &error);
  if (error != 0)
    {
      syslog(LOG_ERR, "ERROR: Invalid RESET pin '%s': %d\n",
             CONFIG_NUCLEO_H753ZI_ST7796_RESET_PIN, error);
      return error;
    }

  ret = stm32_configgpio(g_reset_pin);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to configure RESET pin: %d\n", ret);
      return ret;
    }

  /* Parse and configure LED/Backlight pin */

  g_led_pin = parse_gpio_pin(CONFIG_NUCLEO_H753ZI_ST7796_LED_PIN, &error);
  if (error != 0)
    {
      syslog(LOG_ERR, "ERROR: Invalid LED pin '%s': %d\n",
             CONFIG_NUCLEO_H753ZI_ST7796_LED_PIN, error);
      return error;
    }

  ret = stm32_configgpio(g_led_pin);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to configure LED pin: %d\n", ret);
      return ret;
    }

  /* Set initial states: RESET high, LED on, DC low */

  stm32_gpiowrite(g_reset_pin, true);
  stm32_gpiowrite(g_led_pin, true);    /* Turn on backlight */
  stm32_gpiowrite(g_dc_pin, false);

  syslog(LOG_INFO, "ST7796 GPIO pins initialized:\n");
  syslog(LOG_INFO, "  DC: %s\n", CONFIG_NUCLEO_H753ZI_ST7796_DC_PIN);
  syslog(LOG_INFO, "  RESET: %s\n", CONFIG_NUCLEO_H753ZI_ST7796_RESET_PIN);
  syslog(LOG_INFO, "  LED: %s\n", CONFIG_NUCLEO_H753ZI_ST7796_LED_PIN);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_st7796initialize
 *
 * Description:
 * Initialize and register the ST7796 LCD driver.
 *
 ****************************************************************************/

int stm32_st7796initialize(int devno)
{
  FAR struct spi_dev_s *spi;
  FAR struct fb_vtable_s *fb;
  int ret;

  syslog(LOG_INFO, "Initializing ST7796 on SPI%d, device ID: %d\n",
         ST7796_SPI_PORTNO, CONFIG_NUCLEO_H753ZI_ST7796_DEVID);

  /* Step 1: Initialize GPIO pins (DC, RESET, LED) */

  ret = stm32_st7796_gpio_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize ST7796 GPIO pins: %d\n",
             ret);
      return ret;
    }

  /* ========================================================================
   * CORREÇÃO CRÍTICA: Sequência de reset conforme código do fabricante
   * 
   * Código original (INCORRETO):
   *   RESET = LOW (0V)
   *   delay 100ms
   *   RESET = HIGH (3.3V)
   *   delay 150ms
   * 
   * Código do fabricante (CORRETO - ST7796_IPS_initialization.txt):
   *   LCD_RST_SET;      // RESET = HIGH (3.3V)
   *   delay_ms(1);      // 1ms
   *   LCD_RST_CLR;      // RESET = LOW (0V)
   *   delay_ms(10);     // 10ms
   *   LCD_RST_SET;      // RESET = HIGH (3.3V)
   *   delay_ms(120);    // 120ms
   * ======================================================================== */

  syslog(LOG_INFO, "ST7796: Performing hardware reset (manufacturer sequence)...\n");
  
  /* Passo 1: RESET = HIGH (estado inicial) */
  stm32_gpiowrite(g_reset_pin, true);
  nxsig_usleep(1000);  /* 1ms - estabilização inicial */
  syslog(LOG_INFO, "ST7796: RESET=HIGH (1ms)\n");
  
  /* Passo 2: RESET = LOW (ativar reset) */
  stm32_gpiowrite(g_reset_pin, false);
  nxsig_usleep(10000);  /* 10ms - tempo de reset */
  syslog(LOG_INFO, "ST7796: RESET=LOW (10ms)\n");
  
  /* Passo 3: RESET = HIGH (liberar reset) */
  stm32_gpiowrite(g_reset_pin, true);
  nxsig_usleep(120000);  /* 120ms - tempo de estabilização após reset */
  syslog(LOG_INFO, "ST7796: RESET=HIGH (120ms) - reset complete\n");

  /* Step 3: Register the CS device with the SPI system */

  ret = stm32_spi_register_cs_device(ST7796_SPI_PORTNO,
                                     CONFIG_NUCLEO_H753ZI_ST7796_DEVID,
                                     CONFIG_NUCLEO_H753ZI_ST7796_CS_PIN,
                                     CONFIG_NUCLEO_H753ZI_ST7796_CS_ACTIVE_LOW);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to register CS device for ST7796: %d\n",
             ret);
      return ret;
    }

  syslog(LOG_INFO, "ST7796 CS device registered: SPI%d, ID %d, pin %s (%s)\n",
         ST7796_SPI_PORTNO, CONFIG_NUCLEO_H753ZI_ST7796_DEVID,
         CONFIG_NUCLEO_H753ZI_ST7796_CS_PIN,
         CONFIG_NUCLEO_H753ZI_ST7796_CS_ACTIVE_LOW ? "active_low" :
         "active_high");

  /* Step 4: Initialize the SPI bus */

  spi = stm32_spibus_initialize(ST7796_SPI_PORTNO);
  if (!spi)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize SPI%d\n",
             ST7796_SPI_PORTNO);

      /* Cleanup: unregister the CS device */

      stm32_spi_unregister_cs_device(ST7796_SPI_PORTNO,
                                     CONFIG_NUCLEO_H753ZI_ST7796_DEVID);
      return -ENODEV;
    }

  syslog(LOG_INFO, "SPI%d bus initialized successfully\n", ST7796_SPI_PORTNO);

  /* Step 5: Initialize the ST7796 framebuffer driver */

  syslog(LOG_INFO, "ST7796: Initializing framebuffer driver...\n");
  fb = st7796_fbinitialize(spi, stm32_st7796_set_dc);
  if (!fb)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize ST7796 framebuffer driver\n");

      /* Cleanup: unregister the CS device */

      stm32_spi_unregister_cs_device(ST7796_SPI_PORTNO,
                                     CONFIG_NUCLEO_H753ZI_ST7796_DEVID);
      return -ENODEV;
    }

  syslog(LOG_INFO, "ST7796 framebuffer driver initialized successfully\n");

  /* Step 6: Register the framebuffer device */

  ret = fb_register_device(0, 0, fb);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to register framebuffer device: %d\n",
             ret);

      /* Cleanup: unregister the CS device */

      stm32_spi_unregister_cs_device(ST7796_SPI_PORTNO,
                                     CONFIG_NUCLEO_H753ZI_ST7796_DEVID);
      return ret;
    }

  syslog(LOG_INFO, "ST7796 registered as /dev/fb%d\n", devno);

  return OK;
}

/****************************************************************************
 * Name: stm32_st7796_backlight
 *
 * Description:
 * Control the ST7796 backlight LED.
 *
 * Input Parameters:
 * on - true to turn on, false to turn off
 *
 * Returned Value:
 * None
 *
 ****************************************************************************/

void stm32_st7796_backlight(bool on)
{
  stm32_gpiowrite(g_led_pin, on);
  syslog(LOG_INFO, "ST7796 backlight: %s\n", on ? "ON" : "OFF");
}

/****************************************************************************
 * Name: stm32_st7796_reset
 *
 * Description:
 * Reset the ST7796 display.
 *
 * Returned Value:
 * None
 *
 ****************************************************************************/

void stm32_st7796_reset(void)
{
  /* Reset sequence: LOW for 100ms, then HIGH for 150ms (OK) */
  syslog(LOG_INFO, "ST7796: Manual hardware reset requested\n");
  stm32_gpiowrite(g_reset_pin, false);
  nxsig_usleep(100000);  /* 100ms */
  stm32_gpiowrite(g_reset_pin, true);
  nxsig_usleep(150000);   /* 150ms */
  syslog(LOG_INFO, "ST7796 hardware reset completed\n");
}


/****************************************************************************
 * Name: stm32_st7796_cleanup
 *
 * Description:
 * Cleanup ST7796 resources. This function can be called during
 * shutdown or error recovery.
 *
 * Returned Value:
 * OK on success, negative errno on error
 *
 ****************************************************************************/

int stm32_st7796_cleanup(void)
{
  int ret;

  /* Turn off backlight */

  stm32_gpiowrite(g_led_pin, false);

  /* Critical Correction: Unconfigure control GPIO pins (DC, RESET, LED)
   * to a safe state (Input Floating) to prevent power leakage or pin
   * conflicts with other drivers.
   *
   * We mask out the configuration bits (mode/pull/speed) and apply the
   * ST7796_GPIO_IN_FLOAT base, preserving the port and pin number.
   */

  syslog(LOG_INFO, "ST7796: Unconfiguring control GPIO pins...\n");

  stm32_configgpio((g_dc_pin & ST7796_GPIO_CONFIG_MASK) | ST7796_GPIO_IN_FLOAT);
  stm32_configgpio((g_reset_pin & ST7796_GPIO_CONFIG_MASK) | ST7796_GPIO_IN_FLOAT);
  stm32_configgpio((g_led_pin & ST7796_GPIO_CONFIG_MASK) | ST7796_GPIO_IN_FLOAT);

  /* Unregister CS device */

  ret = stm32_spi_unregister_cs_device(ST7796_SPI_PORTNO,
                                       CONFIG_NUCLEO_H753ZI_ST7796_DEVID);
  if (ret < 0)
    {
      syslog(LOG_WARNING, "WARNING: Failed to unregister ST7796 CS dev: %d\n",
             ret);
    }
  else
    {
      syslog(LOG_INFO, "ST7796 CS device unregistered successfully\n");
    }

  return ret;
}

#endif /* CONFIG_LCD_ST7796 && CONFIG_NUCLEO_H753ZI_ST7796_ENABLE */
