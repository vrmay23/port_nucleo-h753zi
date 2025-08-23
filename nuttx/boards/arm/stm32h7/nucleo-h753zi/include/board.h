/****************************************************************************
 * boards/arm/stm32h7/nucleo-h753zi/include/board.h
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

/* GPIO PINOUT MAPPING FOR NUCLEO-H753ZI (MB1364)
 *
 * Pin usage table for the STM32H753ZI on the Nucleo board's on-board peripherals.
 * Pins not listed here are, by default, free for general use.
 *
 * Based on the following documents:
 * - Schematics: 19250.pdf
 * - User Manual: UM2407
 *
 * |----------------------|---------------|-----------|-----------------------|----------------------------------------------|
 * |Function / Peripheral | Logical Name  | STM32 Pin | A.F                   | Notes                                        |
 * |----------------------|---------------|-----------|-----------------------|----------------------------------------------|
 * | User Button          | B1_USER       | PC13      | -                     |                                              |
 * | LED1 (Green)         | LD1           | PB0       | -                     |                                              |
 * | LED2 (Orange)        | LD2           | PE1       | -                     |                                              |
 * | LED3 (Red)           | LD3           | PB14      | -                     |                                              |
 * | Ethernet             | RMII_MDIO     | PA2       | AF11                  |                                              |
 * |                      | RMII_MDC      | PC1       | AF11                  |                                              |
 * |                      | RMII_TX_EN    | PG11      | AF11                  |                                              |
 * |                      | RMII_TXD0     | PG13      | AF11                  |                                              |
 * |                      | RMII_TXD1     | PG12      | AF11                  |                                              |
 * |                      | RMII_RXD0     | PC4       | AF11                  |                                              |
 * |                      | RMII_RXD1     | PC5       | AF11                  |                                              |
 * |                      | RMII_CRS_DV   | PA7       | AF11                  |                                              |
 * |                      | RMII_REF_CLK  | PA1       | AF11                  |                                              |
 * | USB VCP              | VCP_TX        | PD8       | AF7 (USART3_TX)       |                                              |
 * |                      | VCP_RX        | PD9       | AF7 (USART3_RX)       |                                              |
 * | USB OTG_FS           | USB_FS_VBUS   | PA9       | AF10 (USB_OTG_FS_VBUS)|                                              |
 * |                      | USB_FS_ID     | PA10      | AF10 (USB_OTG_FS_ID)  |                                              |
 * |                      | USB_FS_N      | PA11      | AF10 (USB_OTG_FS_N)   |                                              |
 * |                      | USB_FS_P      | PA12      | AF10 (USB_OTG_FS_P)   |                                              |
 * | Debug (ST-LINK)      | SWCLK         | PA14      | AF0                   |                                              |
 * |                      | SWDIO         | PA13      | AF0                   |                                              |
 * | Zio Connector (I2C)  | A4            | PB9       | AF4 (I2C1_SDA)        | Need SB55 and SB62 to connect to Zio header, |
 * |                      | A5            | PB8       | AF4 (I2C1_SCL)        | otherwise, no connection from Zio connector. |
 * | External Clock HSE   | HSE_IN        | PH0       | -                     | 8 MHz clock provided by ST-LINK V3.          |
 * |                      | HSE_OUT       | PH1       | -                     | Not in used (but reserved for X3)            |
 * |-------------------------------------------------------------------------------------------------------------------------|
 *
 */

#ifndef __BOARDS_ARM_STM32H7_NUCLEO_H753ZI_INCLUDE_BOARD_H
#define __BOARDS_ARM_STM32H7_NUCLEO_H753ZI_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
#  include <stdint.h>
#endif

/* Do not include STM32 H7 header files here */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Board Clock Configuration ************************************************/

/*  ----------------- HSE clock source configuration -------------------
 *
 * The Nucleo-144 board provides the following clock sources:
 *
 *    HSI: 16 MHz RC factory-trimmed
 *    LSI: 32 KHz RC
 *    HSE: configurable via Kconfig (8 MHz MCO or external crystal)
 *    LSE: 32.768 kHz
 *
 *    Ps.:
 *    MCO: 8 MHz from MCO output of ST-LINK is used as input clock (default)
 *    It is fixed at 8 MHz and connected to the PF0/PH0-OSC_IN.
 *    Configuration must be ajusted via Kconfig and via SB on hardware
 * 
 *         HSE Clock Source Solder Bridge Configurations:
 * 
 *         ST-LINK MCO (8 MHz - Default):
 *         - SB45=ON, SB44=OFF, SB46=OFF, SB3=OFF, SB4=OFF
 *
 *         External Crystal X3:
 *         - SB3=ON, SB4=ON, SB45=OFF, SB44=OFF, SB46=OFF
 *
 *         Additional HSE solder bridges (for both configurations):
 *         - SB148, SB8 and SB9 OFF
 *         - SB112 and SB149 ON
*/

/* ST-LINK MCO 8 MHz */
#ifdef CONFIG_BOARD_HSE_SOURCE_STLINK
#  define STM32_BOARD_XTAL        8000000ul
#  define STM32_BOARD_USEHSE
#  define STM32_HSEBYP_ENABLE               /* MCO is bypassed clock source */

/* External crystal X3 */
#elif defined(CONFIG_BOARD_HSE_SOURCE_X3)
#  if !defined(CONFIG_BOARD_HSE_X3_FREQ)
#    error "X3 frequency not defined! Set CONFIG_BOARD_HSE_X3_FREQ."
#  endif
#  if (CONFIG_BOARD_HSE_X3_FREQ < 8000000) || \
      (CONFIG_BOARD_HSE_X3_FREQ > 25000000)
#    error "X3 frequency out of supported range (8-25 MHz)."
#  endif
#  define STM32_BOARD_XTAL CONFIG_BOARD_HSE_X3_FREQ
#  define STM32_BOARD_USEHSE

/* No HSEBYP_ENABLE for crystal oscillator */
#else
#  error "HSE clock source not configured! Please select it via menuconfig."
#endif

/* Base Clock Frequencies ***************************************************/

#define STM32_HSE_FREQUENCY       STM32_BOARD_XTAL       /* from menuconfig */
#define STM32_HSI_FREQUENCY       16000000ul
#define STM32_LSI_FREQUENCY       32000
#define STM32_LSE_FREQUENCY       32768

/* PLL Configuration ********************************************************/

/* Main PLL Configuration - Auto-configured based on Kconfig HSE selection
 *
 *             |------------|  |-----------------|   |-----|
 *  input >----| phase comp.|--| low pass filter |---| vco |---|---> PLL
 *         |-->|------------|  |-----------------|   |-----|   |
 *         |                                                   | 
 *         |---------------------------------------------------|
 *
 * PLL source is HSE with frequency determined by Kconfig choice:
 *   - CONFIG_BOARD_HSE_SOURCE_STLINK: 8 MHz from ST-LINK MCO
 *   - CONFIG_BOARD_HSE_SOURCE_X3: User-defined crystal frequency
 *
 * PLL Strategy:
 * This configuration automatically adjusts PLLM and PLLN values to maintain
 * consistent output frequencies regardless of HSE input frequency:
 *   - Target VCO frequency: 800 MHz
 *   - Target SYSCLK: 400 MHz (VCO/2)
 *   - Target PLL1Q: 200 MHz (VCO/4) 
 *   - Target PLL1R: 100 MHz (VCO/8)
 *
 * PLL Calculation Method:
 * PLL_VCO = (HSE_FREQ / PLLM) * PLLN = 800 MHz (target)
 * 
 * For each supported HSE frequency, PLLM is chosen to get 4-5 MHz reference:
 *   HSE  8 MHz --> PLLM=2, PLLN=200 --> ( 8/2)*200 = 800 MHz
 *   HSE 12 MHz --> PLLM=3, PLLN=200 --> (12/3)*200 = 800 MHz  
 *   HSE 16 MHz --> PLLM=4, PLLN=200 --> (16/4)*200 = 800 MHz
 *   HSE 20 MHz --> PLLM=5, PLLN=200 --> (20/5)*200 = 800 MHz
 *   HSE 24 MHz --> PLLM=6, PLLN=200 --> (24/6)*200 = 800 MHz
 *   HSE 25 MHz --> PLLM=5, PLLN=160 --> (25/5)*160 = 800 MHz
 *
 * PLL Constraints (all configurations meet these):
 *   1 <= PLLM <= 63
 *   4 <= PLLN <= 512
 *   4 MHz <= (HSE_FREQ/PLLM) <= 8 MHz (PLL1RGE_4_8_MHZ)
 *   192 MHz <= PLL_VCO <= 836MHz (VCOH range)
 *   SYSCLK = PLL_VCO/PLLP ≤ 400 MHz
 *
 * Output Frequencies (consistent across all HSE frequencies):
 *   SYSCLK = 800MHz/2 = 400 MHz
 *   PLL1Q  = 800MHz/4 = 200 MHz  
 *   PLL1R  = 800MHz/8 = 100 MHz
 */

#define STM32_PLLCFG_PLLSRC       RCC_PLLCKSELR_PLLSRC_HSE

/* PLL1 Configuration - Automatically calculated based on HSE frequency
 * Target: VCO = 800 MHz; SYSCLK = 400 MHz
 */

/* HSE = 8 MHz: PLL_VCO = (8,000,000 / 2) * 200 = 800 MHz */
#if STM32_HSE_FREQUENCY == 8000000
#  define STM32_PLLCFG_PLL1M      RCC_PLLCKSELR_DIVM1(2)
#  define STM32_PLLCFG_PLL1N      RCC_PLL1DIVR_N1(200)
#  define STM32_VCO1_FREQUENCY    ((STM32_HSE_FREQUENCY / 2) * 200)
  
/* HSE = 12 MHz: PLL_VCO = (12,000,000 / 3) * 200 = 800 MHz */
#elif STM32_HSE_FREQUENCY == 12000000
#  define STM32_PLLCFG_PLL1M      RCC_PLLCKSELR_DIVM1(3)
#  define STM32_PLLCFG_PLL1N      RCC_PLL1DIVR_N1(200)
#  define STM32_VCO1_FREQUENCY    ((STM32_HSE_FREQUENCY / 3) * 200)

/* HSE = 16 MHz: PLL_VCO = (16,000,000 / 4) * 200 = 800 MHz */
#elif STM32_HSE_FREQUENCY == 16000000
#  define STM32_PLLCFG_PLL1M      RCC_PLLCKSELR_DIVM1(4)
#  define STM32_PLLCFG_PLL1N      RCC_PLL1DIVR_N1(200)
#  define STM32_VCO1_FREQUENCY    ((STM32_HSE_FREQUENCY / 4) * 200)
  
/* HSE = 20 MHz: PLL_VCO = (20,000,000 / 5) * 200 = 800 MHz */
#elif STM32_HSE_FREQUENCY == 20000000
#  define STM32_PLLCFG_PLL1M      RCC_PLLCKSELR_DIVM1(5)
#  define STM32_PLLCFG_PLL1N      RCC_PLL1DIVR_N1(200)
#  define STM32_VCO1_FREQUENCY    ((STM32_HSE_FREQUENCY / 5) * 200)

/* HSE = 24 MHz: PLL_VCO = (24,000,000 / 6) * 200 = 800 MHz */
#elif STM32_HSE_FREQUENCY == 24000000
#  define STM32_PLLCFG_PLL1M      RCC_PLLCKSELR_DIVM1(6)
#  define STM32_PLLCFG_PLL1N      RCC_PLL1DIVR_N1(200)
#  define STM32_VCO1_FREQUENCY    ((STM32_HSE_FREQUENCY / 6) * 200)

/* HSE = 25 MHz: PLL_VCO = (25,000,000 / 5) * 160 = 800 MHz */
#elif STM32_HSE_FREQUENCY == 25000000
#  define STM32_PLLCFG_PLL1M      RCC_PLLCKSELR_DIVM1(5)
#  define STM32_PLLCFG_PLL1N      RCC_PLL1DIVR_N1(160)
#  define STM32_VCO1_FREQUENCY    ((STM32_HSE_FREQUENCY / 5) * 160)
  
#else
#  error "Unsupported HSE frequency. Choose among: 8, 12, 16, 20, 24, 25MHz."
#endif

/* PLL1, wide 4 - 8 MHz input, enable DIVP, DIVQ, DIVR
 *
 * PLL1P = PLL1_VCO/2 = 800 MHz / 2 = 400 MHz
 * PLL1Q = PLL1_VCO/4 = 800 MHz / 4 = 200 MHz
 * PLL1R = PLL1_VCO/8 = 800 MHz / 8 = 100 MHz
 */

#define STM32_PLLCFG_PLL1CFG      (RCC_PLLCFGR_PLL1VCOSEL_WIDE | \
                                   RCC_PLLCFGR_PLL1RGE_4_8_MHZ | \
                                   RCC_PLLCFGR_DIVP1EN | \
                                   RCC_PLLCFGR_DIVQ1EN | \
                                   RCC_PLLCFGR_DIVR1EN)

#define STM32_PLLCFG_PLL1P        RCC_PLL1DIVR_P1(2)
#define STM32_PLLCFG_PLL1Q        RCC_PLL1DIVR_Q1(4)
#define STM32_PLLCFG_PLL1R        RCC_PLL1DIVR_R1(8)

#define STM32_PLL1P_FREQUENCY     (STM32_VCO1_FREQUENCY / 2)
#define STM32_PLL1Q_FREQUENCY     (STM32_VCO1_FREQUENCY / 4)
#define STM32_PLL1R_FREQUENCY     (STM32_VCO1_FREQUENCY / 8)

/* PLL2 Configuration - Same configuration pattern as PLL1 */

#if STM32_HSE_FREQUENCY == 8000000
  #define STM32_PLLCFG_PLL2M      RCC_PLLCKSELR_DIVM2(2)
  #define STM32_PLLCFG_PLL2N      RCC_PLL2DIVR_N2(200)
  #define STM32_VCO2_FREQUENCY    ((STM32_HSE_FREQUENCY / 2) * 200)
#elif STM32_HSE_FREQUENCY == 12000000
  #define STM32_PLLCFG_PLL2M      RCC_PLLCKSELR_DIVM2(3)
  #define STM32_PLLCFG_PLL2N      RCC_PLL2DIVR_N2(200)
  #define STM32_VCO2_FREQUENCY    ((STM32_HSE_FREQUENCY / 3) * 200)
#elif STM32_HSE_FREQUENCY == 16000000
  #define STM32_PLLCFG_PLL2M      RCC_PLLCKSELR_DIVM2(4)
  #define STM32_PLLCFG_PLL2N      RCC_PLL2DIVR_N2(200)
  #define STM32_VCO2_FREQUENCY    ((STM32_HSE_FREQUENCY / 4) * 200)
#elif STM32_HSE_FREQUENCY == 20000000
  #define STM32_PLLCFG_PLL2M      RCC_PLLCKSELR_DIVM2(5)
  #define STM32_PLLCFG_PLL2N      RCC_PLL2DIVR_N2(200)
  #define STM32_VCO2_FREQUENCY    ((STM32_HSE_FREQUENCY / 5) * 200)
#elif STM32_HSE_FREQUENCY == 24000000
  #define STM32_PLLCFG_PLL2M      RCC_PLLCKSELR_DIVM2(6)
  #define STM32_PLLCFG_PLL2N      RCC_PLL2DIVR_N2(200)
  #define STM32_VCO2_FREQUENCY    ((STM32_HSE_FREQUENCY / 6) * 200)
#elif STM32_HSE_FREQUENCY == 25000000
  #define STM32_PLLCFG_PLL2M      RCC_PLLCKSELR_DIVM2(5)
  #define STM32_PLLCFG_PLL2N      RCC_PLL2DIVR_N2(160)
  #define STM32_VCO2_FREQUENCY    ((STM32_HSE_FREQUENCY / 5) * 160)
#endif

#define STM32_PLLCFG_PLL2CFG      (RCC_PLLCFGR_PLL2VCOSEL_WIDE | \
                                   RCC_PLLCFGR_PLL2RGE_4_8_MHZ | \
                                   RCC_PLLCFGR_DIVP2EN)

#define STM32_PLLCFG_PLL2P        RCC_PLL2DIVR_P2(40)
#define STM32_PLLCFG_PLL2Q        0
#define STM32_PLLCFG_PLL2R        0

#define STM32_PLL2P_FREQUENCY     (STM32_VCO2_FREQUENCY / 40)
#define STM32_PLL2Q_FREQUENCY     0
#define STM32_PLL2R_FREQUENCY     0

/* PLL3 Configuration - Disabled */

#define STM32_PLLCFG_PLL3CFG      0
#define STM32_PLLCFG_PLL3M        0
#define STM32_PLLCFG_PLL3N        0
#define STM32_PLLCFG_PLL3P        0
#define STM32_PLLCFG_PLL3Q        0
#define STM32_PLLCFG_PLL3R        0

#define STM32_VCO3_FREQUENCY      0
#define STM32_PLL3P_FREQUENCY     0
#define STM32_PLL3Q_FREQUENCY     0 
#define STM32_PLL3R_FREQUENCY     0

/* System Clock Configuration ***********************************************/

/* SYSCLK = PLL1P = 400 MHz
 * CPUCLK = SYSCLK / 1 = 400 MHz
 */

#define STM32_RCC_D1CFGR_D1CPRE   (RCC_D1CFGR_D1CPRE_SYSCLK)
#define STM32_SYSCLK_FREQUENCY    (STM32_PLL1P_FREQUENCY)
#define STM32_CPUCLK_FREQUENCY    (STM32_SYSCLK_FREQUENCY / 1)

/* AHB and APB Clock Configuration ******************************************/

/* AHB clock (HCLK) is SYSCLK/2 (200 MHz max)
 * HCLK1 = HCLK2 = HCLK3 = HCLK4
 */

#define STM32_RCC_D1CFGR_HPRE  RCC_D1CFGR_HPRE_SYSCLKd2      /* HCLK = SYSCLK / 2     */
#define STM32_ACLK_FREQUENCY   (STM32_SYSCLK_FREQUENCY / 2)  /* ACLK = D1, HCLK3 = D1 */
#define STM32_HCLK_FREQUENCY   (STM32_SYSCLK_FREQUENCY / 2)  /* HCLK = D2, HCLK4 = D3 */

/* APB1 clock (PCLK1) is HCLK/4 (50 MHz) */

#define STM32_RCC_D2CFGR_D2PPRE1  RCC_D2CFGR_D2PPRE1_HCLKd4 /* PCLK1 = HCLK / 4 */
#define STM32_PCLK1_FREQUENCY     (STM32_HCLK_FREQUENCY/4)

/* APB2 clock (PCLK2) is HCLK/4 (50 MHz) */

#define STM32_RCC_D2CFGR_D2PPRE2  RCC_D2CFGR_D2PPRE2_HCLKd4 /* PCLK2 = HCLK / 4 */
#define STM32_PCLK2_FREQUENCY     (STM32_HCLK_FREQUENCY/4)

/* APB3 clock (PCLK3) is HCLK/4 (50 MHz) */

#define STM32_RCC_D1CFGR_D1PPRE   RCC_D1CFGR_D1PPRE_HCLKd4  /* PCLK3 = HCLK / 4 */
#define STM32_PCLK3_FREQUENCY     (STM32_HCLK_FREQUENCY/4)

/* APB4 clock (PCLK4) is HCLK/4 (50 MHz) */

#define STM32_RCC_D3CFGR_D3PPRE   RCC_D3CFGR_D3PPRE_HCLKd4  /* PCLK4 = HCLK / 4 */
#define STM32_PCLK4_FREQUENCY     (STM32_HCLK_FREQUENCY/4)

/* Timer Clock Configuration ************************************************/

/* Timers driven from APB1 will be twice PCLK1 */

#define STM32_APB1_TIM2_CLKIN     (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM3_CLKIN     (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM4_CLKIN     (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM5_CLKIN     (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM6_CLKIN     (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM7_CLKIN     (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM12_CLKIN    (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM13_CLKIN    (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM14_CLKIN    (2*STM32_PCLK1_FREQUENCY)

/* Timers driven from APB2 will be twice PCLK2 */

#define STM32_APB2_TIM1_CLKIN     (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM8_CLKIN     (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM15_CLKIN    (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM16_CLKIN    (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM17_CLKIN    (2*STM32_PCLK2_FREQUENCY)

/* Peripheral Clock Configuration *******************************************/

/* Kernel Clock Configuration - Note: look at Table 54 in ST Manual */

/* I2C4 clock source - HSI */
#define STM32_RCC_D3CCIPR_I2C4SRC RCC_D3CCIPR_I2C4SEL_HSI

/* I2C123 clock source - HSI */
#define STM32_RCC_D2CCIP2R_I2C123SRC RCC_D2CCIP2R_I2C123SEL_HSI

/* SPI45 clock source - APB (PCLK2?) */
#define STM32_RCC_D2CCIP1R_SPI45SRC RCC_D2CCIP1R_SPI45SEL_APB

/* SPI123 clock source - PLL1Q */
#define STM32_RCC_D2CCIP1R_SPI123SRC RCC_D2CCIP1R_SPI123SEL_PLL1

/* SPI6 clock source - APB (PCLK4) */
#define STM32_RCC_D3CCIPR_SPI6SRC RCC_D3CCIPR_SPI6SEL_PCLK4

/* USB 1 and 2 clock source - HSI48 */
#define STM32_RCC_D2CCIP2R_USBSRC RCC_D2CCIP2R_USBSEL_HSI48

/* ADC 1 2 3 clock source - pll2_pclk */
#define STM32_RCC_D3CCIPR_ADCSRC  RCC_D3CCIPR_ADCSEL_PLL2

/* FLASH Configuration ******************************************************/

/* FLASH wait states
 *
 *  ------------ ---------- -----------
 *  Vcore        MAX ACLK   WAIT STATES
 *  ------------ ---------- -----------
 *  1.15-1.26 V     70 MHz    0
 *  (VOS1 level)   140 MHz    1
 *                 210 MHz    2
 *  1.05-1.15 V     55 MHz    0
 *  (VOS2 level)   110 MHz    1
 *                 165 MHz    2
 *                 220 MHz    3
 *  0.95-1.05 V     45 MHz    0
 *  (VOS3 level)    90 MHz    1
 *                 135 MHz    2
 *                 180 MHz    3
 *                 225 MHz    4
 *  ------------ ---------- -----------
 */

#define BOARD_FLASH_WAITSTATES 4

/* SDMMC Configuration ******************************************************/

/* SDMMC Clock Configuration - Frequency Stability Across HSE Sources
 *
 * These SDMMC clock dividers remain valid for ALL HSE source configurations
 * because PLL1Q is maintained at a constant 200 MHz regardless of HSE frequency.
 *
 * Clock Frequency Verification Table:
 * +----------------+--------+---------+-------------+------------------+
 * | HSE Source     | VCO    | PLL1Q   | SDMMC Init  | SDMMC Transfer   |
 * +----------------+--------+---------+-------------+------------------+
 * | ST-LINK 8MHz   | 800MHz | 200MHz  | 400 kHz     | 25 MHz           |
 * | Crystal 12MHz  | 800MHz | 200MHz  | 400 kHz     | 25 MHz           |
 * | Crystal 16MHz  | 800MHz | 200MHz  | 400 kHz     | 25 MHz           |
 * | Crystal 20MHz  | 800MHz | 200MHz  | 400 kHz     | 25 MHz           |
 * | Crystal 24MHz  | 800MHz | 200MHz  | 400 kHz     | 25 MHz           |
 * | Crystal 25MHz  | 800MHz | 200MHz  | 400 kHz     | 25 MHz           |
 * +----------------+--------+---------+-------------+------------------+
 *
 * Calculation:
 *   SDMMC_Init = PLL1Q / (2 * 250) = 200MHz / 500 = 400 kHz  (SD spec compliant)
 *   SDMMC_Xfer = PLL1Q / (2 * 4)   = 200MHz / 8   = 25 MHz   (~12.5 MB/s)
 */

/* Init 400kHz, PLL1Q/(2*250) */
#define STM32_SDMMC_INIT_CLKDIV   (250 << STM32_SDMMC_CLKCR_CLKDIV_SHIFT)

/* Transfer at 25 MHz, PLL1Q/(2*4), for speed ~12.5MB/s */
#define STM32_SDMMC_MMCXFR_CLKDIV (4 << STM32_SDMMC_CLKCR_CLKDIV_SHIFT)
#define STM32_SDMMC_SDXFR_CLKDIV  (4 << STM32_SDMMC_CLKCR_CLKDIV_SHIFT)
#define STM32_SDMMC_CLKCR_EDGE    STM32_SDMMC_CLKCR_NEGEDGE

/* Peripheral Support Configuration *****************************************/

/* PIN CONFLICTS 
 *
 * ---------------------------------------------------------------------------
 * |   PB13   |    RMII      |  JP6: ON   |  I2S_A_CK                       |
 * |          |    TXD1      |            |                                 |
 * ---------------------------------------------------------------------------
 */

/* Ethernet GPIO Configuration **********************************************/

/* UM2407 REV 4, page 28/49
 * 
 * By default the nucleo-h753 has the Solder Bridges 'ON' ( SBXY: ON).
 * It means the MCU pins are connected already to the Ethernet connector.
 * Hence, there is no connection for this pins from ST Zio or Morpho connectors 
 * 
 * ---------------------------------------------------------------------------
 * |          |              |            |            |                     |
 * | pin_name |   function   |   Config   |  conflict  |  config when using  |
 * |          |              | ST ZIO CON | ST ZIO CON | ST ZIO or morpho    |
 * ---------------------------------------------------------------------------
 * |   PA1    |   RMII ref.  |  SB57: ON  |     NO     |  SB57: OFF          |
 * |          |    clock     |            |            |                     |
 * ---------------------------------------------------------------------------
 * |   PA2    |     RMII     |  SB72: ON  |     NO     |  SB72: OFF          |
 * |          |     MDIO     |            |            |                     |
 * ---------------------------------------------------------------------------
 * |   PC1    |     RMII     |  SB64: ON  |     NO     |  SB64: OFF          |
 * |          |     MDC      |            |            |                     |
 * ---------------------------------------------------------------------------
 * |   PA7    |   RMII RX    |  SB31: ON  |     NO     |  SB31: OFF          |
 * |          |  data valid  |            |            |                     |
 * ---------------------------------------------------------------------------
 * |   PC4    |    RMII      |  SB36: ON  |     NO     |  SB36: OFF          |
 * |          |    RXD0      |            |            |                     |
 * ---------------------------------------------------------------------------
 * |   PC5    |    RMII      |  SB29: ON  |     NO     |  SB29: OFF          |
 * |          |    RXD1      |            |            |                     |
 * ---------------------------------------------------------------------------
 * |   PG11   |    RMII      |  SB27: ON  |     NO     |  SB27: OFF          |
 * |          |  TX enable   |            |            |                     |
 * ---------------------------------------------------------------------------
 * |   PG13   |    RMII      |  SB30: ON  |     NO     |  SB30: OFF          |
 * |          |    TXD0      |            |            |                     |
 * ---------------------------------------------------------------------------
 * |   PB13   |    RMII      |  JP6: ON   |   I2S_A_CK |  JP6: OFF           |
 * |          |    TXD1      |            |            |                     |
 * ---------------------------------------------------------------------------
 */

#define GPIO_ETH_RMII_REF_CLK (GPIO_ETH_RMII_REF_CLK_0 | GPIO_SPEED_100MHz)    /* PA1  */
#define GPIO_ETH_RMII_CRS_DV  (GPIO_ETH_RMII_CRS_DV_0  | GPIO_SPEED_100MHz)    /* PA7  */
#define GPIO_ETH_RMII_TX_EN   (GPIO_ETH_RMII_TX_EN_2   | GPIO_SPEED_100MHz)    /* PG11 */
#define GPIO_ETH_RMII_TXD0    (GPIO_ETH_RMII_TXD0_2    | GPIO_SPEED_100MHz)    /* PG13 */
#define GPIO_ETH_RMII_TXD1    (GPIO_ETH_RMII_TXD1_1    | GPIO_SPEED_100MHz)    /* PB13 */
#define GPIO_ETH_RMII_RXD0    (GPIO_ETH_RMII_RXD0_0    | GPIO_SPEED_100MHz)    /* PC4  */
#define GPIO_ETH_RMII_RXD1    (GPIO_ETH_RMII_RXD1_0    | GPIO_SPEED_100MHz)    /* PC5  */
#define GPIO_ETH_MDIO         (GPIO_ETH_MDIO_0         | GPIO_SPEED_100MHz)    /* PA2  */
#define GPIO_ETH_MDC          (GPIO_ETH_MDC_0          | GPIO_SPEED_100MHz)    /* PC1  */

/* LED Configuration ********************************************************/

/* The Nucleo-H753ZI board has several LEDs.
 * Only three are user-controllable:
 *
 *   LD1 -> Green
 *   LD2 -> Orange
 *   LD3 -> Red
 *
 * Behavior depends on CONFIG_ARCH_LEDS:
 *
 *   SYMBOL            OWNER     USAGE
 *   ----------------  --------  -------------------------------
 *   CONFIG_ARCH_LEDS=n User     /dev/leds
 *                                boards/.../stm32_userleds.c
 *                                apps/examples/leds
 *
 *   CONFIG_ARCH_LEDS=y NuttX    boards/.../stm32_autoleds.c
 *
 *   For more information, check the Kconfig file or use the menuconfig help.
 */

/* LED index values for use with board_userled() */

#define BOARD_LED1        0
#define BOARD_LED2        1
#define BOARD_LED3        2
#define BOARD_NLEDS       3

#define BOARD_LED_GREEN   BOARD_LED1
#define BOARD_LED_ORANGE  BOARD_LED2
#define BOARD_LED_RED     BOARD_LED3

/* LED bits for use with board_userled_all() */

#define BOARD_LED1_BIT    (1 << BOARD_LED1)
#define BOARD_LED2_BIT    (1 << BOARD_LED2)
#define BOARD_LED3_BIT    (1 << BOARD_LED3)

/* If CONFIG_ARCH_LEDS is defined, the usage by the board port is defined in
 * include/board.h and src/stm32_leds.c.
 * The LEDs are used to encode OS-related events as follows:
 *
 *
 *   SYMBOL                     Meaning                        LED state
 *                                                        Red   Green   Orange
 *   ----------------------  --------------------------  ------ ------ ------
 */

#define LED_STARTED        0 /* NuttX has been started   OFF     OFF    OFF  */
#define LED_HEAPALLOCATE   1 /* Heap has been allocated  OFF     OFF    ON   */
#define LED_IRQSENABLED    2 /* Interrupts enabled       OFF     ON     OFF  */
#define LED_STACKCREATED   3 /* Idle stack created       OFF     ON     ON   */
#define LED_INIRQ          4 /* In an interrupt          N/C     N/C    GLOW */
#define LED_SIGNAL         5 /* In a signal handler      N/C     GLOW   N/C  */
#define LED_ASSERTION      6 /* An assertion failed      GLOW    N/C    GLOW */
#define LED_PANIC          7 /* The system has crashed   Blink   OFF    N/C  */
#define LED_IDLE           8 /* MCU is is sleep mode     ON      OFF    OFF  */

/* Button Configuration *****************************************************/

/* The STM32H7 Discovery has just one user_button natively (B1), which one is
 * connected to GPIO PC13. This button, in this context, named as BUILT_IN,
 * is connected in a pulldown resistor. Thus, when it changes from default
 * value (LOW) to HIGH value, it is considered as 'pressed'.
 *
 * Plus, we can use the same strategy like in stm32103-minimun (bluepill) to
 * provide more freedom to the users. Hence, four additional buttons will be
 * available now and, then, five buttons can be directly handled.
 *
 * Please, make sure to also use your external buttons with a pulldown
 * resistor as well, otherwise it will not work as expected.
 *
 * For this example we'll use the available pin as listed below:
 *
 *
 *   -------------------|----------|------------|-----------------
 *      button_name     | pin_name | pin_number |  stm32_gpio_pin
 *   -------------------|----------|------------|-----------------
 *     BUTTON_EXTERN_1  |    D2    |     12     |     PF_15
 *     BUTTON_EXTERN_2  |    D1    |     14     |     PG_14
 *     BUTTON_EXTERN_3  |    D0    |     16     |     PG_9
 *     BUTTON_EXTERN_4  |    D34   |     33     |     PE_0
 *   -------------------------------------------------------------
 *
 *   Ps.: This buttons are handled by IRQ. Hence, make sure you have
 *   enabled it via menuconfig at:
 *
 *     Board Selection
 *                    | [ x ] Board button support
 *                    | [ x ] Button interrupt support <----- IRQ
 */

#define BUTTON_BUILT_IN        0
#define BUTTON_EXTERN_1        1
#define BUTTON_EXTERN_2        2
#define BUTTON_EXTERN_3        3
#define BUTTON_EXTERN_4        4

#define BUTTON_BUILT_IN_BIT    (1 << BUTTON_BUILT_IN)
#define BUTTON_EXTERN_1_BIT    (1 << BUTTON_EXTERN_1)
#define BUTTON_EXTERN_2_BIT    (1 << BUTTON_EXTERN_2)
#define BUTTON_EXTERN_3_BIT    (1 << BUTTON_EXTERN_3)
#define BUTTON_EXTERN_4_BIT    (1 << BUTTON_EXTERN_4)

#define NUM_BUTTONS            5


/* GPIO Pin Alternate Function Selections ***********************************/

/* ADC GPIO Definitions */

#define GPIO_ADC123_INP10 GPIO_ADC123_INP10_0                      /* PC0,  channel 10 */
#define GPIO_ADC123_INP12 GPIO_ADC123_INP12_0                      /* PC2,  channel 12 */
#define GPIO_ADC123_INP11 GPIO_ADC123_INP11_0                      /* PC1,  channel 11 */
#define GPIO_ADC12_INP13  GPIO_ADC12_INP13_0                       /* PC3,  channel 13 */
#define GPIO_ADC12_INP15  GPIO_ADC12_INP15_0                       /* PA3,  channel 15 */
#define GPIO_ADC12_INP18  GPIO_ADC12_INP18_0                       /* PA4,  channel 18 */
#define GPIO_ADC12_INP19  GPIO_ADC12_INP19_0                       /* PA5,  channel 19 */
#define GPIO_ADC12_INP14  GPIO_ADC12_INP14_0                       /* PA2,  channel 14 */
#define GPIO_ADC123_INP7  GPIO_ADC12_INP7_0                        /* PA7,  channel  7 */
#define GPIO_ADC12_INP5   GPIO_ADC12_INP5_0                        /* PB1,  channel  5 */
#define GPIO_ADC12_INP3   GPIO_ADC12_INP3_0                        /* PA6,  channel  3 */
#define GPIO_ADC12_INP4   GPIO_ADC12_INP4_0                        /* PC4,  channel  4 */
#define GPIO_ADC12_INP8   GPIO_ADC12_INP8_0                        /* PC5,  channel  8 */
#define GPIO_ADC2_INP2    GPIO_ADC2_INP2_0                         /* PF13, channel  2 */

/* UART/USART GPIO Definitions */

/* USART3 (Nucleo Virtual Console) */
#define GPIO_USART3_RX    (GPIO_USART3_RX_3 | GPIO_SPEED_100MHz)   /* PD9 */
#define GPIO_USART3_TX    (GPIO_USART3_TX_3 | GPIO_SPEED_100MHz)   /* PD8 */

/* USART6 (Arduino Serial Shield) */
#define GPIO_USART6_RX    (GPIO_USART6_RX_2 | GPIO_SPEED_100MHz)   /* PG9 */
#define GPIO_USART6_TX    (GPIO_USART6_TX_2 | GPIO_SPEED_100MHz)   /* PG14 */

/* I2C GPIO Definitions */

/* I2C1 Use Nucleo I2C1 pins */
#define GPIO_I2C1_SCL     (GPIO_I2C1_SCL_2 | GPIO_SPEED_50MHz)     /* PB8 - D15 */
#define GPIO_I2C1_SDA     (GPIO_I2C1_SDA_2 | GPIO_SPEED_50MHz)     /* PB9 - D14 */

/* I2C2 Use Nucleo I2C2 pins */
#define GPIO_I2C2_SCL     (GPIO_I2C2_SCL_2  | GPIO_SPEED_50MHz)    /* PF1 - D69 */
#define GPIO_I2C2_SDA     (GPIO_I2C2_SDA_2  | GPIO_SPEED_50MHz)    /* PF0 - D68 */
#define GPIO_I2C2_SMBA    (GPIO_I2C2_SMBA_2 | GPIO_SPEED_50MHz)    /* PF2 - D70 */

/* SPI GPIO Definitions */

/* SPI3 */
#define GPIO_SPI3_MISO    (GPIO_SPI3_MISO_1 | GPIO_SPEED_50MHz)    /* PB4 */
#define GPIO_SPI3_MOSI    (GPIO_SPI3_MOSI_4 | GPIO_SPEED_50MHz)    /* PB5 */
#define GPIO_SPI3_SCK     (GPIO_SPI3_SCK_1 | GPIO_SPEED_50MHz)     /* PB3 */
#define GPIO_SPI3_NSS     (GPIO_SPI3_NSS_2 | GPIO_SPEED_50MHz)     /* PA4 */

/* Timer GPIO Definitions */

/* TIM1 */
#define GPIO_TIM1_CH1OUT  (GPIO_TIM1_CH1OUT_2  | GPIO_SPEED_50MHz) /* PE9  - D6 */
#define GPIO_TIM1_CH1NOUT (GPIO_TIM1_CH1NOUT_3 | GPIO_SPEED_50MHz) /* PE8  - D42 */
#define GPIO_TIM1_CH2OUT  (GPIO_TIM1_CH2OUT_2  | GPIO_SPEED_50MHz) /* PE11 - D5 */
#define GPIO_TIM1_CH2NOUT (GPIO_TIM1_CH2NOUT_3 | GPIO_SPEED_50MHz) /* PE10 - D40 */
#define GPIO_TIM1_CH3OUT  (GPIO_TIM1_CH3OUT_2  | GPIO_SPEED_50MHz) /* PE13 - D3 */
#define GPIO_TIM1_CH3NOUT (GPIO_TIM1_CH3NOUT_3 | GPIO_SPEED_50MHz) /* PE12 - D39 */
#define GPIO_TIM1_CH4OUT  (GPIO_TIM1_CH4OUT_2  | GPIO_SPEED_50MHz) /* PE14 - D38 */

/* USB OTG FS GPIO Definitions */

#define GPIO_OTGFS_DM  (GPIO_OTGFS_DM_0  | GPIO_SPEED_100MHz)
#define GPIO_OTGFS_DP  (GPIO_OTGFS_DP_0  | GPIO_SPEED_100MHz)
#define GPIO_OTGFS_ID  (GPIO_OTGFS_ID_0  | GPIO_SPEED_100MHz)

/* DMA Channel Mappings *****************************************************/

/* UART/USART DMA Mappings */
#define DMAMAP_USART3_RX  DMAMAP_DMA12_USART3RX_0
#define DMAMAP_USART3_TX  DMAMAP_DMA12_USART3TX_1
#define DMAMAP_USART6_RX DMAMAP_DMA12_USART6RX_1
#define DMAMAP_USART6_TX DMAMAP_DMA12_USART6TX_0

/* SPI DMA Mappings */
#define DMAMAP_SPI3_RX DMAMAP_DMA12_SPI3RX_0 /* DMA1 */
#define DMAMAP_SPI3_TX DMAMAP_DMA12_SPI3TX_0 /* DMA1 */

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_STM32H7_NUCLEO_H753ZI_INCLUDE_BOARD_H */
