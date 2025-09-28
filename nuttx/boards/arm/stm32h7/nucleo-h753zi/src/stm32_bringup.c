/****************************************************************************
 * boards/arm/stm32h7/nucleo-h753zi/src/stm32_bringup.c
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
 * http://www.apache.org/licenses/LICENSE-20.0
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

/* System includes */
#include <nuttx/config.h>
#include <arch/board/board.h>
#include <sys/types.h>
#include <syslog.h>
#include <errno.h>

/* Filesystem includes */
#include <nuttx/fs/fs.h>

/* Communication protocol includes */
#ifdef CONFIG_I2C
#  include <nuttx/i2c/i2c_master.h>
#endif

#ifdef CONFIG_SPI
#  include <nuttx/spi/spi.h>
#endif

/* STM32-specific includes */
#include "stm32_gpio.h"

#ifdef CONFIG_I2C
#  include "stm32_i2c.h"
#endif

#ifdef CONFIG_STM32H7_SPI
#  include "stm32_spi.h" 
#endif

#include "nucleo-h753zi.h"

/* USB-related includes */
#ifdef CONFIG_USBMONITOR
#  include <nuttx/usb/usbmonitor.h>
#endif

#ifdef CONFIG_STM32H7_OTGFS
#  include "stm32_usbhost.h"
#endif

#ifdef CONFIG_RNDIS
#  include <nuttx/usb/rndis.h>
#endif

/* Input device includes */
#ifdef CONFIG_INPUT_BUTTONS
#  include <nuttx/input/buttons.h>
#endif

/* LED includes */
#ifdef CONFIG_USERLED
#  include <nuttx/leds/userled.h>
#endif

/* Timer-related includes */
#ifdef HAVE_RTC_DRIVER
#  include <nuttx/timers/rtc.h>
#  include "stm32_rtc.h"
#endif

#ifdef CONFIG_CAPTURE
#  include <nuttx/timers/capture.h>
#  include "stm32_capture.h"
#endif

#ifdef CONFIG_STM32H7_IWDG
#  include "stm32_wdg.h"
#endif

/* Storage includes */
#ifdef CONFIG_STM32_ROMFS
#  include "drivers/driver_middleware/stm32_romfs.h"
#endif

#ifdef CONFIG_VIDEO_FB
#  include <nuttx/video/fb.h>
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Initialization functions organized by name length (longest to shortest) */

static int nucleo_automotive_initialize(void);
static int nucleo_communication_initialize(void);
static int nucleo_connectivity_initialize(void);
static int nucleo_filesystem_initialize(void);
static int nucleo_watchdog_initialize(void);
static int nucleo_sensors_initialize(void);
static int nucleo_storage_initialize(void);
static int nucleo_timers_initialize(void);
static int nucleo_input_initialize(void);
static int nucleo_gpio_initialize(void);
static int nucleo_led_initialize(void);
static int nucleo_usb_initialize(void);
static int nucleo_rtc_initialize(void);
static int nucleo_adc_initialize(void);

#if defined(CONFIG_I2C) && defined(CONFIG_SYSTEM_I2CTOOL)
  static int nucleo_i2c_tools_initialize(void);
#endif

#ifdef CONFIG_CAPTURE
  static int nucleo_capture_initialize(void);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nucleo_led_initialize
 *
 * Description:
 * Initialize LED subsystem based on configuration
 * Priority: HIGH (provides early visual feedback)
 *
 * Dependencies: None
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_led_initialize(void)
{
  int ret = OK;

/* User-controlled LEDs via /dev/userleds */

#ifdef CONFIG_NUCLEO_H753ZI_LEDS_USER

  ret = userled_lower_initialize("/dev/userleds");
  if (ret < 0)
    {
      syslog(LOG_ERR, "[ERROR: BRINGUP] - userled_lower_initialize() failed: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "[INFO: BRINGUP] - User LEDs initialized at /dev/userleds\n");
    }

#elif defined(CONFIG_NUCLEO_H753ZI_LEDS_AUTO)

  /* Automatic LEDs - initialized by kernel via board_autoled_initialize() */

  syslog(LOG_INFO, "[INFO: BRINGUP] - Auto LEDs enabled for system status indication\n");

#elif defined(CONFIG_NUCLEO_H753ZI_LEDS_DISABLED)

  /* LEDs completely disabled */

  syslog(LOG_INFO, "[INFO: BRINGUP] - LEDs disabled by configuration\n");

#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_filesystem_initialize
 *
 * Description:
 * Initialize filesystem support (PROCFS, ROMFS)
 * Priority: HIGH (required for logging and configuration)
 *
 * Dependencies: None
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_filesystem_initialize(void)
{
  int ret = OK;

/* Mount the procfs file system */

#ifdef CONFIG_FS_PROCFS

  int local_ret = nx_mount(NULL, STM32_PROCFS_MOUNTPOINT, "procfs", 0, NULL);
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount PROCFS: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "PROCFS mounted at %s\n", STM32_PROCFS_MOUNTPOINT);
    }

#endif /* CONFIG_FS_PROCFS */

/* Mount the ROMFS partition */

#ifdef CONFIG_STM32_ROMFS

  int local_ret = stm32_romfs_initialize();
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount ROMFS at %s: %d\n",
             CONFIG_STM32_ROMFS_MOUNTPOINT, local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "ROMFS mounted at %s\n", CONFIG_STM32_ROMFS_MOUNTPOINT);
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_rtc_initialize
 *
 * Description:
 * Initialize Real-Time Clock driver
 * Priority: HIGH (time services for other subsystems)
 *
 * Dependencies: None
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_rtc_initialize(void)
{
  int ret = OK;

/* Instantiate the STM32 lower-half RTC driver */

#ifdef HAVE_RTC_DRIVER
  struct rtc_lowerhalf_s *lower;

  lower = stm32_rtc_lowerhalf();
  if (!lower)
    {
      syslog(LOG_ERR, "ERROR: Failed to instantiate RTC lower-half driver\n");
      return -ENOMEM;
    }

  /* Bind and register the RTC driver as /dev/rtc0 */
  ret = rtc_initialize(0, lower);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to bind/register RTC driver: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "RTC driver registered as /dev/rtc0\n");
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_gpio_initialize
 *
 * Description:
 * Initialize GPIO driver for user applications
 * Priority: HIGH (required by many other drivers)
 *
 * Dependencies: None
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_gpio_initialize(void)
{
  int ret = OK;

#ifdef CONFIG_DEV_GPIO
  /* Register the GPIO driver */
  ret = stm32_gpio_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize GPIO driver: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "GPIO driver initialized\n");
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_automotive_initialize
 *
 * Description:
 * Initialize automotive/industrial communication protocols (CAN, LIN, etc.)
 * Priority: MEDIUM
 *
 * Dependencies: GPIO, Clock configuration
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_automotive_initialize(void)
{
  int ret = OK;

/* Initialize CAN subsystem */

/*
#ifdef CONFIG_STM32H7_CAN
  ret = stm32_can_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_can_initialize failed: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "CAN bus initialized\n");
    }
#endif
*/

  /* Future: Add LIN, FlexRay, or other automotive protocols here */

  return ret;
}

/****************************************************************************
 * Name: nucleo_communication_initialize
 *
 * Description:
 * Initialize general-purpose communication bus drivers (SPI, I2C)
 * Priority: HIGH (required by sensors and other peripherals)
 *
 * Dependencies: GPIO
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_communication_initialize(void)
{
  int ret = OK;
  int local_ret;

  /* ========================================================================
   * SPI Bus Initialization  
   * ======================================================================== */
#ifdef CONFIG_STM32H7_SPI
  local_ret = stm32_spi_initialize();
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_spi_initialize failed: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "SPI buses initialized\n");
    }
#endif

/* Register SPI devices */
#ifdef CONFIG_SPI_DRIVER
  local_ret = stm32_spidev_register_all();
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_spidev_register_all failed: %d\n", 
             local_ret);
      /* Don't update ret - this is not critical */
    }
  else
    {
      syslog(LOG_INFO, "SPI devices registered\n");
    }
#endif

  /* ========================================================================
   * I2C Tools Initialization (for debugging)
   * ======================================================================== */
#if defined(CONFIG_I2C) && defined(CONFIG_SYSTEM_I2CTOOL)
  local_ret = nucleo_i2c_tools_initialize();
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: I2C tools initialization failed: %d\n", 
             local_ret);
      /* Don't update ret - this is not critical */
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_input_initialize
 *
 * Description:
 * Initialize input devices (buttons, etc.)
 * Priority: MEDIUM
 *
 * Dependencies: GPIO
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_input_initialize(void)
{
  int ret = OK;

/* Register the BUTTON driver */

#ifdef CONFIG_INPUT_BUTTONS
  ret = btn_lower_initialize("/dev/buttons");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: btn_lower_initialize() failed: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "Buttons driver registered as /dev/buttons\n");
    }
#endif /* CONFIG_INPUT_BUTTONS */

  return ret;
}

/****************************************************************************
 * Name: nucleo_usb_initialize
 *
 * Description:
 * Initialize USB subsystem (host, device, monitoring)
 * Priority: MEDIUM
 *
 * Dependencies: None
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_usb_initialize(void)
{
  int ret = OK;

#ifdef HAVE_USBHOST
  /* Initialize USB host operation */
  int local_ret = stm32_usbhost_initialize();
  if (local_ret != OK)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize USB host: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "USB host initialized\n");
    }
#endif

#ifdef HAVE_USBMONITOR
  /* Start the USB Monitor */
  int local_ret = usbmonitor_start();
  if (local_ret != OK)
    {
      syslog(LOG_ERR, "ERROR: Failed to start USB monitor: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "USB monitor started\n");
    }
#endif

#if defined(CONFIG_CDCACM) && !defined(CONFIG_CDCACM_CONSOLE) && \
    !defined(CONFIG_CDCACM_COMPOSITE)
  /* Initialize CDC/ACM USB device */
  syslog(LOG_INFO, "Initializing CDC/ACM device\n");
  int local_ret = cdcacm_initialize(0, NULL);
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: cdcacm_initialize failed: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "CDC/ACM device initialized\n");
    }
#endif /* CONFIG_CDCACM */

#if defined(CONFIG_RNDIS) && !defined(CONFIG_RNDIS_COMPOSITE)
  /* Initialize RNDIS USB device */
  uint8_t mac[6];
  mac[0] = 0xa0;
  mac[1] = (CONFIG_NETINIT_MACADDR_2 >> (8 * 0)) & 0xff;
  mac[2] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 3)) & 0xff;
  mac[3] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 2)) & 0xff;
  mac[4] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 1)) & 0xff;
  mac[5] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 0)) & 0xff;

  int local_ret = usbdev_rndis_initialize(mac);
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: RNDIS initialization failed: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "RNDIS USB device initialized\n");
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_adc_initialize
 *
 * Description:
 * Initialize Analog-to-Digital Converter
 * Priority: MEDIUM
 *
 * Dependencies: GPIO
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_adc_initialize(void)
{
  int ret = OK;

#ifdef CONFIG_ADC
  /* Initialize ADC and register the ADC driver */
  ret = stm32_adc_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_adc_setup failed: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "ADC driver initialized\n");
    }
#endif /* CONFIG_ADC */

  return ret;
}

/****************************************************************************
 * Name: nucleo_sensors_initialize
 *
 * Description:
 * Initialize sensor drivers (IMU, magnetometer, RFID, etc.)
 * Priority: LOW (application-specific)
 *
 * Dependencies: I2C, SPI, GPIO
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_sensors_initialize(void)
{
  int ret = OK;

#ifdef CONFIG_SENSORS_LSM6DSL
  int local_ret = stm32_lsm6dsl_initialize("/dev/lsm6dsl0");
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize LSM6DSL driver: %d\n",
             local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "LSM6DSL sensor initialized as /dev/lsm6dsl0\n");
    }
#endif /* CONFIG_SENSORS_LSM6DSL */

#ifdef CONFIG_SENSORS_LSM9DS1
  int local_ret = stm32_lsm9ds1_initialize();
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize LSM9DS1 driver: %d\n",
             local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "LSM9DS1 sensor initialized\n");
    }
#endif /* CONFIG_SENSORS_LSM9DS1 */

#ifdef CONFIG_SENSORS_LSM303AGR
  int local_ret = stm32_lsm303agr_initialize("/dev/lsm303mag0");
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize LSM303AGR driver: %d\n",
             local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "LSM303AGR magnetometer initialized as /dev/lsm303mag0\n");
    }
#endif /* CONFIG_SENSORS_LSM303AGR */

#ifdef CONFIG_NUCLEO_H753ZI_MFRC522_ENABLE
  /* Initialize MFRC522 RFID reader using the path defined in header */
  int local_ret = stm32_mfrc522initialize(MFRC522_DEVPATH);
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_mfrc522initialize() failed: %d\n", 
             local_ret);
      
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "MFRC522 RFID reader initialized successfully at %s\n", 
             MFRC522_DEVPATH);
    }
#endif /* CONFIG_NUCLEO_H753ZI_MFRC522_ENABLE */

  return ret;
}

/****************************************************************************
 * Name: nucleo_connectivity_initialize
 *
 * Description:
 * Initialize wireless connectivity modules (Wi-Fi, Bluetooth, etc.)
 * Priority: LOW (application-specific)
 *
 * Dependencies: SPI, I2C, GPIO
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_connectivity_initialize(void)
{
  int ret = OK;

#ifdef CONFIG_PCA9635PW
  /* Initialize the PCA9635 LED controller chip */
  ret = stm32_pca9635_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_pca9635_initialize failed: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "PCA9635 LED controller initialized\n");
    }
#endif

#ifdef CONFIG_WL_NRF24L01
  ret = stm32_wlinitialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize wireless driver: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "NRF24L01 wireless driver initialized\n");
    }
#endif /* CONFIG_WL_NRF24L01 */

#if defined(CONFIG_LCD_SSD1306) && defined(CONFIG_LCD_CHARACTER_DEVICE)
  /* Initialize SSD1306 OLED display using board driver */
  int local_ret = board_lcd_initialize();
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: board_lcd_initialize failed: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      struct lcd_dev_s *lcd = board_lcd_getdev(0);
      if (lcd != NULL)
        {
          syslog(LOG_INFO, "SSD1306 OLED initialized successfully");
        }
      else
        {
          syslog(LOG_ERR, "ERROR: Failed to get SSD1306 LCD device\n");
        }
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_storage_initialize
 *
 * Description:
 * Initialize storage devices (SD card, flash, etc.)
 * Priority: MEDIUM
 *
 * Dependencies: SPI (for SD cards), filesystem
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_storage_initialize(void)
{
  int ret = OK;

#ifdef CONFIG_MMCSD_SPI
  /* Initialize the MMC/SD SPI driver (SPI3 is used) */
  ret = stm32_mmcsd_initialize(CONFIG_NSH_MMCSDMINOR);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize SD slot %d: %d\n",
             CONFIG_NSH_MMCSDMINOR, ret);
    }
  else
    {
      syslog(LOG_INFO, "MMC/SD SPI driver initialized (slot %d)\n",
             CONFIG_NSH_MMCSDMINOR);
    }
#endif

#ifdef CONFIG_MTD
#ifdef HAVE_PROGMEM_CHARDEV
  ret = stm32_progmem_init();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize MTD progmem: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "MTD program memory initialized\n");
    }
#endif /* HAVE_PROGMEM_CHARDEV */
#endif /* CONFIG_MTD */

  return ret;
}

/****************************************************************************
 * Name: nucleo_timers_initialize
 *
 * Description:
 * Initialize timer-related drivers (PWM, capture, etc.)
 * Priority: LOW
 *
 * Dependencies: GPIO
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_timers_initialize(void)
{
  int ret = OK;

#ifdef CONFIG_PWM
  /* Initialize PWM and register the PWM device */
  int local_ret = stm32_pwm_setup();
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_pwm_setup() failed: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "PWM drivers initialized\n");
    }
#endif

#ifdef CONFIG_CAPTURE
  /* Initialize the capture driver */
  int local_ret = nucleo_capture_initialize();
  if (local_ret < 0)
    {
      syslog(LOG_ERR, "ERROR: nucleo_capture_initialize() failed: %d\n", local_ret);
      if (ret == OK)
        {
          ret = local_ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "Timer capture drivers initialized\n");
    }
#endif

  return ret;
}


/****************************************************************************
 * Name: nucleo_watchdog_initialize
 *
 * Description:
 * Initialize watchdog timer
 * Priority: LOW (should be last - for system monitoring)
 *
 * Dependencies: None (intentionally independent)
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_streaming_initialize(void)
{
  int ret = OK;

#ifdef CONFIG_VIDEO_FB
  /* Initialize and register the framebuffer driver */

  ret = fb_register(0, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fb_register() failed: %d\n", ret);
    }
    
   else
    {
      syslog(LOG_INFO, "FB initialized as /dev/fb0\n");
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_watchdog_initialize
 *
 * Description:
 * Initialize watchdog timer
 * Priority: LOW (should be last - for system monitoring)
 *
 * Dependencies: None (intentionally independent)
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int nucleo_watchdog_initialize(void)
{
  int ret = OK;

#ifdef CONFIG_STM32H7_IWDG
  /* Initialize the watchdog timer */
  ret = stm32_iwdginitialize("/dev/watchdog0", STM32_LSI_FREQUENCY);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize watchdog: %d\n", ret);
    }
  else
    {
      syslog(LOG_INFO, "Watchdog initialized as /dev/watchdog0\n");
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: nucleo_i2c_tools_initialize
 *
 * Description:
 * Initialize I2C tools for debugging and development
 * Priority: LOW (development/debugging only)
 *
 * Dependencies: I2C
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

#if defined(CONFIG_I2C) && defined(CONFIG_SYSTEM_I2CTOOL)
static void stm32_i2c_register(int bus)
{
  struct i2c_master_s *i2c;
  int ret;

  i2c = stm32_i2cbus_initialize(bus);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to get I2C%d interface\n", bus);
    }
  else
    {
      ret = i2c_register(i2c, bus);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: Failed to register I2C%d driver: %d\n",
                 bus, ret);
          stm32_i2cbus_uninitialize(i2c);
        }
      else
        {
          syslog(LOG_INFO, "I2C%d registered for i2c tools\n", bus);
        }
    }
}

static int nucleo_i2c_tools_initialize(void)
{
#ifdef CONFIG_STM32H7_I2C1
  stm32_i2c_register(1);
#endif
#ifdef CONFIG_STM32H7_I2C2
  stm32_i2c_register(2);
#endif
#ifdef CONFIG_STM32H7_I2C3
  stm32_i2c_register(3);
#endif
#ifdef CONFIG_STM32H7_I2C4
  stm32_i2c_register(4);
#endif

  return OK;
}
#endif

/****************************************************************************
 * Name: nucleo_capture_initialize
 *
 * Description:
 * Initialize and register capture drivers
 * Priority: LOW
 *
 * Dependencies: Timer peripherals
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

#ifdef CONFIG_CAPTURE
static int nucleo_capture_initialize(void)
{
  int ret;
  struct cap_lowerhalf_s *lower[] =
    {
#if defined(CONFIG_STM32H7_TIM1_CAP)
      stm32_cap_initialize(1),
#endif
#if defined(CONFIG_STM32H7_TIM2_CAP)
      stm32_cap_initialize(2),
#endif
#if defined(CONFIG_STM32H7_TIM3_CAP)
      stm32_cap_initialize(3),
#endif
#if defined(CONFIG_STM32H7_TIM4_CAP)
      stm32_cap_initialize(4),
#endif
#if defined(CONFIG_STM32H7_TIM5_CAP)
      stm32_cap_initialize(5),
#endif
#if defined(CONFIG_STM32H7_TIM8_CAP)
      stm32_cap_initialize(8),
#endif
#if defined(CONFIG_STM32H7_TIM12_CAP)
      stm32_cap_initialize(12),
#endif
#if defined(CONFIG_STM32H7_TIM13_CAP)
      stm32_cap_initialize(13),
#endif
#if defined(CONFIG_STM32H7_TIM14_CAP)
      stm32_cap_initialize(14),
#endif
#if defined(CONFIG_STM32H7_TIM15_CAP)
      stm32_cap_initialize(15),
#endif
#if defined(CONFIG_STM32H7_TIM16_CAP)
      stm32_cap_initialize(16),
#endif
#if defined(CONFIG_STM32H7_TIM17_CAP)
      stm32_cap_initialize(17),
#endif
    };

  size_t count = sizeof(lower) / sizeof(lower[0]);

  /* Nothing to do if no timers enabled */
  if (count == 0)
    {
      return OK;
    }

  /* Register "/dev/cap0" ... "/dev/cap<count-1>" */
  ret = cap_register_multiple("/dev/cap", lower, count);
  if (ret == EINVAL)
    {
      syslog(LOG_ERR, "ERROR: cap_register_multiple path is invalid\n");
    }
  else if (ret == EEXIST)
    {
      syslog(LOG_ERR, "ERROR: cap_register_multiple inode already exists\n");
    }
  else if (ret == ENOMEM)
    {
      syslog(LOG_ERR, "ERROR: cap_register_multiple not enough memory\n");
    }
  else if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: cap_register_multiple failed: %d\n", ret);
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 * Perform architecture-specific initialization with dependency-aware ordering
 *
 * This function initializes all board-specific drivers and subsystems
 * in a controlled manner, ensuring that dependencies between subsystems
 * are respected and failures in one subsystem do not prevent 
 * initialization of others.
 *
 * INITIALIZATION PHASES WITH DEPENDENCY MANAGEMENT:
 *
 * Phase 1 - Basic System & Visual Feedback: Essential services and early feedback
 * Phase 2 - Hardware Interfaces & Communication Protocols: GPIO, buses (SPI/I2C/CAN)
 * Phase 3 - User Interface & USB Services: Input devices and USB connectivity 
 * Phase 4 - Analog Measurement (ADC): Analog-to-digital conversion capabilities
 * Phase 5 - Sensors & Wireless Connectivity: Application sensors and wireless modules
 * Phase 6 - Storage Devices (SD Card, Flash): File systems and storage hardware
 * Phase 7 - Timers, PWM & Signal Processing: Timer-based features and signal generation
 * Phase 8 - System Monitoring (Watchdog): Final watchdog initialization for monitoring
 *
 * ERROR HANDLING STRATEGY:
 * 1. Individual subsystem failures are logged via syslog.
 * 2. The function continues to initialize other subsystems even if one fails.
 * 3. A single return value (ret) tracks the status of the *first* failure encountered.
 * 4. This ensures that a single, non-critical driver failure does not
 * halt the entire system boot process, while still reporting the
 * root cause of the first problem to the caller.
 *
 * The logic 'if (subsys_ret != OK && ret == OK)' is critical here. It:
 * - Checks if the most recent subsystem initialization failed (subsys_ret != OK).
 * - Checks if this is the first error found so far (ret == OK).
 * - If both are true, it updates 'ret' with the error code, but only once.
 * - This prevents subsequent errors from overwriting the first error code,
 * which is typically the most useful for debugging root causes.
 *
 * Input Parameters:
 * None
 *
 * Returned Value:
 * Zero (OK) on success; a negated errno value on failure.
 * Note: Individual subsystem failures are logged but do not cause
 * overall initialization failure unless critical.
 *
 ****************************************************************************/

int stm32_bringup(void)
{
  int ret = OK;
  int subsys_ret;

  syslog(LOG_INFO, "\n[INFO: BRINGUP]Nucleo-H753ZI initialization...\n");

  /* ========================================================================
   * PHASE 1: BASIC SYSTEM & VISUAL FEEDBACK
   * Essential services that provide fundamental functionality and early feedback
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 1: Initializing basic system & visual feedback\n");

  /* Initialize LED subsystem first for visual feedback */
  subsys_ret = nucleo_led_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret; /* Remember first failure */
    }

  /* Initialize filesystems early for logging and configuration */
  subsys_ret = nucleo_filesystem_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* Initialize RTC for timekeeping services */
  subsys_ret = nucleo_rtc_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * PHASE 2: HARDWARE INTERFACES & COMMUNICATION PROTOCOLS
   * GPIO and all communication buses required by many other drivers
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 2: Initializing hardware interfaces & communication protocols\n");

  /* Initialize GPIO driver - many other drivers depend on this */
  subsys_ret = nucleo_gpio_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* Initialize communication buses (SPI, I2C) - required by sensors */
  subsys_ret = nucleo_communication_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* Initialize automotive protocols (CAN, etc.) */
  subsys_ret = nucleo_automotive_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * PHASE 3: USER INTERFACE & USB SERVICES
   * Input devices and USB connectivity services
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 3: Initializing user interface & USB services\n");

  /* Initialize input devices */
  subsys_ret = nucleo_input_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* Initialize USB subsystem */
  subsys_ret = nucleo_usb_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * PHASE 4: ANALOG MEASUREMENT (ADC)
   * Analog measurement capabilities
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 4: Initializing analog measurement (ADC)\n");

  /* Initialize ADC for analog measurements */
  subsys_ret = nucleo_adc_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * PHASE 5: SENSORS & WIRELESS CONNECTIVITY
   * Application-specific sensors and wireless communication modules
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 5: Initializing sensors & wireless connectivity\n");

  /* Initialize sensor drivers (depends on I2C/SPI buses) */
  subsys_ret = nucleo_sensors_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* Initialize connectivity modules (depends on SPI/I2C buses) */
  subsys_ret = nucleo_connectivity_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * PHASE 6: STORAGE DEVICES (SD CARD, FLASH)
   * File systems and storage devices
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 6: Initializing storage devices (SD card, flash)\n");

  /* Initialize storage devices (depends on SPI for SD cards) */
  subsys_ret = nucleo_storage_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * PHASE 7: TIMERS, PWM & SIGNAL PROCESSING
   * Timer-based drivers and signal generation/processing
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 7: Initializing timers, PWM & signal processing\n");

  /* Initialize timer-related drivers */
  subsys_ret = nucleo_timers_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * PHASE 8: STREAMING
   * Timer-based drivers and signal generation/processing
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 8: Initializing audio and video features\n");

  /* Initialize timer-related drivers */
  subsys_ret = nucleo_streaming_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * PHASE 9: SYSTEM MONITORING (WATCHDOG)
   * Watchdog should be initialized last for proper system monitoring
   * ======================================================================== */

  syslog(LOG_INFO, "[INFO: BRINGUP]Phase 9: Initializing system monitoring (watchdog)\n");

  /* Initialize watchdog last for system monitoring */
  subsys_ret = nucleo_watchdog_initialize();
  if (subsys_ret != OK && ret == OK)
    {
      ret = subsys_ret;
    }

  /* ========================================================================
   * INITIALIZATION COMPLETE
   * ======================================================================== */

  if (ret == OK)
    {
      syslog(LOG_INFO,
             "[INFO: BRINGUP] - Nucleo-H753ZI board initialization completed successfully\n");
    }
  else
    {
      syslog(LOG_WARNING,
             "[WARNING: BRINGUP] - Nucleo-H753ZI board initialization completed with errors: %d\n",
             ret);
      syslog(LOG_INFO,
             "[INFO: BRINGUP] - System is functional, but some drivers may be unavailable\n");
    }

  return ret;
}
