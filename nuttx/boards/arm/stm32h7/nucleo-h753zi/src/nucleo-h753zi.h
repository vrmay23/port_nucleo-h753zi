/****************************************************************************
 * boards/arm/stm32h7/nucleo-h753zi/src/nucleo-h753zi.h
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
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32H7_NUCLEO_H753ZI_SRC_NUCLEO_H753ZI_H
#define __BOARDS_ARM_STM32H7_NUCLEO_H753ZI_SRC_NUCLEO_H753ZI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* ==========================================================================
 * FEATURE CONFIGURATION
 * ==========================================================================
 *
 * This section defines which features are available based on the
 * configuration. Features are automatically disabled if their dependencies
 * are not met.
 */

/* Core System Features */

#define HAVE_PROC            1
#define HAVE_USBDEV          1
#define HAVE_USBHOST         1
#define HAVE_USBMONITOR      1
#define HAVE_MTDCONFIG       1
#define HAVE_PROGMEM_CHARDEV 1
#define HAVE_RTC_DRIVER      1

/* USB Feature Dependencies */

#ifndef CONFIG_STM32H7_OTGFS
#  undef HAVE_USBDEV
#  undef HAVE_USBHOST
#endif

#ifndef CONFIG_USBDEV
#  undef HAVE_USBDEV
#endif

#ifndef CONFIG_USBHOST
#  undef HAVE_USBHOST
#endif

#ifndef CONFIG_USBMONITOR
#  undef HAVE_USBMONITOR
#endif

#if !defined(HAVE_USBDEV)
#  undef CONFIG_USBDEV_TRACE
#endif

#if !defined(HAVE_USBHOST)
#  undef CONFIG_USBHOST_TRACE
#endif

#if !defined(CONFIG_USBDEV_TRACE) && !defined(CONFIG_USBHOST_TRACE)
#  undef HAVE_USBMONITOR
#endif

/* MTD Feature Dependencies */

#if !defined(CONFIG_STM32H7_PROGMEM) || !defined(CONFIG_MTD_PROGMEM)
#  undef HAVE_PROGMEM_CHARDEV
#endif

/* RTC Feature Dependencies */

#if !defined(CONFIG_RTC) || !defined(CONFIG_RTC_DRIVER)
#  undef HAVE_RTC_DRIVER
#endif

/* Flash-based Parameters */

#if defined(CONFIG_MMCSD)
#  define FLASH_BASED_PARAMS
#endif

/* ==========================================================================
 * DEVICE DRIVER PATHS
 * ==========================================================================
 *
 * This section centralizes the file system paths for all device drivers.
 */

#define LED_DRIVER_PATH      "/dev/userleds"
#define BUTTONS_DRIVER_PATH  "/dev/buttons"
#define RTC_DRIVER_PATH      "/dev/rtc0"

#ifdef CONFIG_FS_PROCFS
#  ifdef CONFIG_NSH_PROC_MOUNTPOINT
#    define STM32_PROCFS_MOUNTPOINT CONFIG_NSH_PROC_MOUNTPOINT
#  else
#    define STM32_PROCFS_MOUNTPOINT "/proc"
#  endif
#endif

#define PROGMEM_MTD_MINOR    0

/* ==========================================================================
 * GPIO PIN DEFINITIONS
 * ==========================================================================
 *
 * This section groups all GPIO pin configurations.
 */

/* LED GPIO Definitions */

#define GPIO_LD1         (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | \
                          GPIO_OUTPUT_CLEAR | GPIO_PORTB | GPIO_PIN0)

#define GPIO_LD2         (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | \
                          GPIO_OUTPUT_CLEAR | GPIO_PORTE | GPIO_PIN1)  

#define GPIO_LD3         (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | \
                          GPIO_OUTPUT_CLEAR | GPIO_PORTB | GPIO_PIN14)

/* LED Logical Name Aliases */

#define GPIO_LED_GREEN       GPIO_LD1
#define GPIO_LED_ORANGE      GPIO_LD2
#define GPIO_LED_RED         GPIO_LD3

/* Button GPIO Definitions */

/* Button GPIO Definitions */

#if defined(CONFIG_NUCLEO_H753ZI_BUTTON_SUPPORT) || defined(CONFIG_NUCLEO_H753ZI_GPIO_DRIVER)
#  define GPIO_BTN_BUILT_IN    (GPIO_INPUT | GPIO_FLOAT | GPIO_EXTI | \
                                GPIO_PORTC | GPIO_PIN13)
#endif

/* USB OTG FS GPIO Definitions */

#define GPIO_OTGFS_VBUS      (GPIO_INPUT | GPIO_FLOAT | GPIO_SPEED_100MHz | \
                              GPIO_OPENDRAIN | GPIO_PORTA | GPIO_PIN9)

#define GPIO_OTGFS_PWRON     (GPIO_OUTPUT | GPIO_FLOAT | GPIO_SPEED_100MHz | \
                              GPIO_PUSHPULL | GPIO_PORTG | GPIO_PIN6)
#ifdef CONFIG_USBHOST
#  define GPIO_OTGFS_OVER    (GPIO_INPUT | GPIO_EXTI | GPIO_FLOAT | \
                              GPIO_SPEED_100MHz | GPIO_PUSHPULL |   \
                              GPIO_PORTG | GPIO_PIN7)
#else
#  define GPIO_OTGFS_OVER    (GPIO_INPUT | GPIO_FLOAT | GPIO_SPEED_100MHz | \
                              GPIO_PUSHPULL | GPIO_PORTG | GPIO_PIN7)
#endif

/* GPIO Subsystem Definitions */

#define BOARD_NGPIOIN        1
#define BOARD_NGPIOOUT       3
#define BOARD_NGPIOINT       1

/* pplace holder - example for in, out and interrupt */

#define GPIO_IN1             (GPIO_INPUT | GPIO_FLOAT | GPIO_PORTE | GPIO_PIN2)

#define GPIO_OUT1            (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | \
                              GPIO_OUTPUT_SET | GPIO_PORTE | GPIO_PIN4)

#define GPIO_INT1            (GPIO_INPUT | GPIO_FLOAT | GPIO_PORTE | GPIO_PIN5)

/* Sensor GPIO Definitions */

#define GPIO_LPS22HB_INT1    (GPIO_INPUT | GPIO_FLOAT | GPIO_PORTB | GPIO_PIN10)
#define GPIO_LSM6DSL_INT1    (GPIO_INPUT | GPIO_FLOAT | GPIO_PORTB | GPIO_PIN4)
#define GPIO_LSM6DSL_INT2    (GPIO_INPUT | GPIO_FLOAT | GPIO_PORTB | GPIO_PIN5)

/* Wireless GPIO Definitions */

#define GPIO_NRF24L01_CS     (GPIO_OUTPUT | GPIO_SPEED_50MHz | \
                              GPIO_OUTPUT_SET | GPIO_PORTA | GPIO_PIN4)
#define GPIO_NRF24L01_CE     (GPIO_OUTPUT | GPIO_SPEED_50MHz | \
                              GPIO_OUTPUT_CLEAR | GPIO_PORTF | GPIO_PIN12)
#define GPIO_NRF24L01_IRQ    (GPIO_INPUT | GPIO_FLOAT | GPIO_PORTD | GPIO_PIN15)

/* Storage GPIO Definitions */

#define GPIO_MMCSD_CS        (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | \
                              GPIO_OUTPUT_SET | GPIO_PORTD | GPIO_PIN15)
#define GPIO_MMCSD_NCD       (GPIO_INPUT | GPIO_PULLUP | GPIO_EXTI | \
                              GPIO_PORTF | GPIO_PIN12)

/* ==========================================================================
 * PERIPHERAL DEVICE DEFINITIONS
 * ==========================================================================
 *
 * This section contains peripheral-specific configurations that do not
 * fall under other categories.
 */

/* Sensor Configuration */

#define LMS9DS1_I2CBUS 1

/* PCA9635 LED Controller Configuration */

#define PCA9635_I2CBUS       1
#define PCA9635_I2CADDR      0x40

/* OLED Display Configuration */

#define OLED_I2C_PORT        2

/* PWM Timer Configuration */

#define NUCLEOH753ZI_PWMTIMER 1

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 * Perform architecture-specific initialization
 *
 * CONFIG_BOARD_LATE_INITIALIZE=y :
 * Called from board_late_initialize().
 *
 * CONFIG_BOARD_LATE_INITIALIZE=n && CONFIG_BOARDCTL=y &&
 * CONFIG_NSH_ARCHINIT:
 * Called from the NSH library
 *
 ****************************************************************************/

int stm32_bringup(void);

/* ==========================================================================
 * DRIVER PROTOTYPES
 * ==========================================================================
 *
 * This section centralizes all function prototypes for driver initializations.
 */

#ifdef CONFIG_STM32H7_SPI
void stm32_spidev_initialize(void);
#endif

#ifdef CONFIG_ADC
int stm32_adc_setup(void);
#endif

#if defined(CONFIG_DEV_GPIO) && !defined(CONFIG_GPIO_LOWER_HALF)
int stm32_gpio_initialize(void);
#endif

#ifdef CONFIG_STM32H7_OTGFS
void weak_function stm32_usbinitialize(void);
#endif

#if defined(CONFIG_STM32H7_OTGFS) && defined(CONFIG_USBHOST)
int stm32_usbhost_initialize(void);
#endif

#ifdef CONFIG_SENSORS_LSM6DSL
int stm32_lsm6dsl_initialize(char *devpath);
#endif

#ifdef CONFIG_SENSORS_LSM303AGR
int stm32_lsm303agr_initialize(char *devpath);
#endif

#ifdef CONFIG_SENSORS_LSM9DS1
int stm32_lsm9ds1_initialize(char *devpath);
#endif

#ifdef CONFIG_WL_NRF24L01
int stm32_wlinitialize(void);
#endif

#ifdef CONFIG_PCA9635PW
int stm32_pca9635_initialize(void);
#endif

#ifdef CONFIG_PWM
int stm32_pwm_setup(void);
#endif

#ifdef CONFIG_MTD

#ifdef HAVE_PROGMEM_CHARDEV
int stm32_progmem_init(void);
#endif /* HAVE_PROGMEM_CHARDEV */

#endif /* CONFIG_MTD */

#ifdef CONFIG_MMCSD_SPI
int stm32_mmcsd_initialize(int minor);
#endif

#endif /* __BOARDS_ARM_STM32H7_NUCLEO_H753ZI_SRC_NUCLEO_H753ZI_H */
