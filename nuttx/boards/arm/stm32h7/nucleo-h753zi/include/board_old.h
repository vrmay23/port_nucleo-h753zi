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
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/* GPIO PINOUT MAPPING FOR NUCLEO-H753ZI (MB1364)
 *
 * Pin usage table for STM32H753ZI on Nucleo board's on-board peripherals.
 * Pins not listed here are, by default, free for general use.
 *
 * Based on the following documents:
 * - Schematics: 19250.pdf
 *
 * |----------------------------------------------------------------------|
 * |Function/Peripheral | Logical Name | STM32 Pin | A.F   | Notes       |
 * |----------------------------------------------------------------------|
 * | User Button        | B1_USER      | PC13      | -     |             |
 * | LED1 (Green)       | LD1          | PB0       | -     |             |
 * | LED2 (Orange)      | LD2          | PE1       | -     |             |
 * | LED3 (Red)         | LD3          | PB14      | -     |             |
 * | Ethernet           | RMII_MDIO    | PA2       | AF11  |             |
 * |                    | RMII_MDC     | PC1       | AF11  |             |
 * |                    | RMII_TX_EN   | PG11      | AF11  |             |
 * |                    | RMII_TXD0    | PG13      | AF11  |             |
 * |                    | RMII_TXD1    | PG12      | AF11  |             |
 * |                    | RMII_RXD0    | PC4       | AF11  |             |
 * |                    | RMII_RXD1    | PC5       | AF11  |             |
 * |                    | RMII_CRS_DV  | PA7       | AF11  |             |
 * |                    | RMII_REF_CLK | PA1       | AF11  |             |
 * | USB VCP            | VCP_TX       | PD8       | AF7   | USART3_TX   |
 * |                    | VCP_RX       | PD9       | AF7   | USART3_RX   |
 * | USB OTG_FS         | USB_FS_VBUS  | PA9       | AF10  |             |
 * |                    | USB_FS_ID    | PA10      | AF10  |             |
 * |                    | USB_FS_N     | PA11      | AF10  |             |
 * |                    | USB_FS_P     | PA12      | AF10  |             |
 * | Debug (ST-LINK)    | SWCLK        | PA14      | AF0   |             |
 * |                    | SWDIO        | PA13      | AF0   |             |
 * | Zio Connector(I2C) | A4           | PB9       | AF4   | I2C1_SDA    |
 * |                    | A5           | PB8       | AF4   | I2C1_SCL    |
 * | External Clock HSE | HSE_IN       | PH0       | -     | 8 MHz MCO   |
 * |                    | HSE_OUT      | PH1       | -     | Reserved X3 |
 * |----------------------------------------------------------------------|
 *
 */

#ifndef __BOARDS_ARM_STM32H7_NUCLEO_H753ZI_INCLUDE_BOARD_H
#define __BOARDS_ARM_STM32H7_NUCLEO_H753ZI_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>

#ifndef __ASSEMBLY__
#  include <stdint.h>
#endif

/* Do not include STM32 H7 header files here */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Clocking *****************************************************************/

/* The NUCLEO-H753ZI board provides the following clock sources:
 *
 *   MCO: 8 MHz from MCO output of ST-LINK (default)
 *   X2:  32.768 kHz crystal for LSE
 *   X3:  HSE crystal oscillator (not provided by default)
 *
 * So we have these clock sources available within the STM32H753:
 *
 *   HSI:   64 MHz RC factory-trimmed
 *   CSI:   4 MHz RC oscillator
 *   LSI:   32 kHz RC
 *   HSE:   8 MHz from ST-LINK MCO (default) or 25 MHz external crystal
 *   LSE:   32.768 kHz
 *   HSI48: 48 MHz RC (dedicated for USB)
 */

/* Clock Tree Configuration Strategy
 *
 * This board supports two HSE clock sources selectable via Kconfig:
 *
 * 1. CONFIG_NUCLEO_H753ZI_HSE_8MHZ (default):
 *    - Uses ST-LINK MCO output (8 MHz)
 *    - No hardware modification required
 *    - Solder bridges: SB45=ON (factory default)
 *
 * 2. CONFIG_NUCLEO_H753ZI_HSE_25MHZ:
 *    - Uses external 25 MHz crystal at X3 footprint
 *    - Requires soldering crystal and configuring solder bridges
 *    - Solder bridges: SB3=ON, SB4=ON, SB45=OFF, SB44=OFF, SB46=OFF
 *
 * Both configurations achieve identical system performance:
 *   SYSCLK = 400 MHz (VOS1 voltage scaling)
 *   HCLK   = 200 MHz
 *   PCLK   = 100 MHz (all APB buses)
 *
 * The 400 MHz SYSCLK target ensures compatibility with NuttX default
 * voltage scaling (VOS1), which limits SYSCLK to 400 MHz maximum.
 * Higher frequencies would require VOS0 configuration.
 */

/* PLL Configuration Overview
 *
 * STM32H753 provides three PLLs for flexible clock generation:
 *
 * PLL1: System clock generation
 *   - Source: HSE (8 MHz or 25 MHz)
 *   - Output: 400 MHz SYSCLK (identical for both HSE sources)
 *   - PLL1P: SYSCLK
 *   - PLL1Q: SPI1/2/3, SDMMC (200 MHz)
 *   - PLL1R: Reserved (200 MHz)
 *
 * PLL2: Peripheral clock generation
 *   - Source: HSE (8 MHz or 25 MHz)
 *   - Output: Multiple peripheral clocks
 *   - PLL2P: ADC, SPI4/5 (75 MHz)
 *   - PLL2Q: FDCAN for 8 MHz config (25 MHz)
 *   - PLL2R: Reserved (200 MHz)
 *
 * PLL3: Additional peripheral clocks
 *   - Source: HSE (8 MHz or 25 MHz)
 *   - Output: Reserved for future use
 *   - Required by NuttX RCC driver (must be defined)
 *
 * PLL Constraints (RM0433 Section 7.7.7):
 *   - Input frequency (after /M):  1-16 MHz (4 ranges available)
 *   - VCO frequency:               192-836 MHz (VCOH mode)
 *   - Output frequencies:          Configurable via dividers
 *   - SYSCLK maximum:              400 MHz @ VOS1, 480 MHz @ VOS0
 */

/* 8 MHz HSE Clock Tree (Default Configuration)
 *
 * Input: 8 MHz from ST-LINK MCO
 *
 * PLL1 Configuration:
 *   Input  = HSE / DIVM1    = 8 MHz / 1   = 8 MHz
 *   Range  = RGE_4_8_MHZ    (4-8 MHz input range)
 *   VCO    = Input * DIVN1  = 8 MHz * 100 = 800 MHz
 *   PLL1P  = VCO / DIVP1    = 800 MHz / 2 = 400 MHz → SYSCLK
 *   PLL1Q  = VCO / DIVQ1    = 800 MHz / 4 = 200 MHz → SPI123, SDMMC
 *   PLL1R  = VCO / DIVR1    = 800 MHz / 4 = 200 MHz
 *
 * PLL2 Configuration:
 *   Input  = HSE / DIVM2    = 8 MHz / 1   = 8 MHz
 *   Range  = RGE_4_8_MHZ    (4-8 MHz input range)
 *   VCO    = Input * DIVN2  = 8 MHz * 75  = 600 MHz
 *   PLL2P  = VCO / DIVP2    = 600 MHz / 8 = 75 MHz  → ADC, SPI45
 *   PLL2Q  = VCO / DIVQ2    = 600 MHz / 24 = 25 MHz → FDCAN
 *   PLL2R  = VCO / DIVR2    = 600 MHz / 3 = 200 MHz
 *
 * PLL3 Configuration:
 *   Input  = HSE / DIVM3    = 8 MHz / 2   = 4 MHz
 *   Range  = RGE_2_4_MHZ    (2-4 MHz input range)
 *   VCO    = Input * DIVN3  = 4 MHz * 100 = 400 MHz
 *   PLL3P  = VCO / DIVP3    = 400 MHz / 2 = 200 MHz
 *   PLL3Q  = VCO / DIVQ3    = 400 MHz / 16 = 25 MHz
 *   PLL3R  = VCO / DIVR3    = 400 MHz / 10 = 40 MHz
 *
 * System Clock Derivation:
 *   SYSCLK = PLL1P                        = 400 MHz
 *   CPUCLK = SYSCLK / D1CPRE              = 400 MHz (no prescaler)
 *   HCLK   = SYSCLK / HPRE                = 200 MHz (AHB buses)
 *   PCLK1  = HCLK / D2PPRE1               = 100 MHz (APB1 peripherals)
 *   PCLK2  = HCLK / D2PPRE2               = 100 MHz (APB2 peripherals)
 *   PCLK3  = HCLK / D1PPRE                = 100 MHz (APB3 peripherals)
 *   PCLK4  = HCLK / D3PPRE                = 100 MHz (APB4 peripherals)
 *
 * Peripheral Clock Sources (RM0433 Table 54):
 *   I2C1/2/3/4:  HSI        = 64 MHz
 *   SPI1/2/3:    PLL1Q      = 200 MHz (kernel clock, prescaled for SCK)
 *   SPI4/5:      PLL2P      = 75 MHz
 *   SPI6:        PCLK4      = 100 MHz
 *   ADC1/2/3:    PLL2P      = 75 MHz
 *   FDCAN1/2:    PLL2Q      = 25 MHz (CAN compliance)
 *   SDMMC1/2:    PLL1Q      = 200 MHz
 *   USB1/2:      HSI48      = 48 MHz (dedicated USB oscillator)
 *   Timers:      2 × PCLK   = 200 MHz (when APB prescaler ≠ 1)
 *
 * Hardware Configuration:
 *   No modifications required - factory default configuration
 */

/* 25 MHz HSE Clock Tree (Optional Configuration)
 *
 * Input: 25 MHz from external crystal X3
 *
 * PLL1 Configuration:
 *   Input  = HSE / DIVM1    = 25 MHz / 5   = 5 MHz
 *   Range  = RGE_4_8_MHZ    (4-8 MHz input range)
 *   VCO    = Input * DIVN1  = 5 MHz * 160  = 800 MHz
 *   PLL1P  = VCO / DIVP1    = 800 MHz / 2  = 400 MHz → SYSCLK
 *   PLL1Q  = VCO / DIVQ1    = 800 MHz / 4  = 200 MHz → SPI123, SDMMC
 *   PLL1R  = VCO / DIVR1    = 800 MHz / 4  = 200 MHz
 *
 * PLL2 Configuration:
 *   Input  = HSE / DIVM2    = 25 MHz / 2   = 12.5 MHz
 *   Range  = RGE_8_16_MHZ   (8-16 MHz input range)
 *   VCO    = Input * DIVN2  = 12.5 MHz * 48 = 600 MHz
 *   PLL2P  = VCO / DIVP2    = 600 MHz / 8  = 75 MHz  → ADC, SPI45
 *   PLL2Q  = VCO / DIVQ2    = 600 MHz / 24 = 25 MHz  (configured but unused)
 *   PLL2R  = VCO / DIVR2    = 600 MHz / 3  = 200 MHz
 *
 * PLL3 Configuration:
 *   Input  = HSE / DIVM3    = 25 MHz / 2   = 12.5 MHz
 *   Range  = RGE_8_16_MHZ   (8-16 MHz input range)
 *   VCO    = Input * DIVN3  = 12.5 MHz * 64 = 800 MHz
 *   PLL3P  = VCO / DIVP3    = 800 MHz / 2  = 400 MHz
 *   PLL3Q  = VCO / DIVQ3    = 800 MHz / 32 = 25 MHz
 *   PLL3R  = VCO / DIVR3    = 800 MHz / 20 = 40 MHz
 *
 * System Clock Derivation:
 *   Identical to 8 MHz HSE configuration (see above)
 *
 * Peripheral Clock Sources:
 *   Same as 8 MHz configuration, except:
 *   FDCAN1/2: HSE direct = 25 MHz (instead of PLL2Q)
 *
 * Note: This configuration uses HSE directly for FDCAN to minimize
 *       clock jitter and provide optimal CAN bus timing.
 *
 * Hardware Configuration:
 *   - Solder 25 MHz crystal at X3 footprint
 *   - Configure solder bridges:
 *     SB3=ON, SB4=ON    (enable X3 crystal)
 *     SB45=OFF          (disable ST-LINK MCO)
 *     SB44=OFF, SB46=OFF (disconnect ST-LINK MCO routing)
 */

/* Performance Comparison: 8 MHz vs 25 MHz HSE
 *
 * System Performance:
 *   SYSCLK:  400 MHz = 400 MHz (identical)
 *   HCLK:    200 MHz = 200 MHz (identical)
 *   PCLK:    100 MHz = 100 MHz (identical)
 *
 * Peripheral Clocks:
 *   I2C:     64 MHz  = 64 MHz  (identical)
 *   SPI123:  200 MHz = 200 MHz (identical, kernel clock)
 *   SPI45:   75 MHz  = 75 MHz  (identical)
 *   SPI6:    100 MHz = 100 MHz (identical)
 *   ADC:     75 MHz  = 75 MHz  (identical)
 *   SDMMC:   200 MHz = 200 MHz (identical)
 *   USB:     48 MHz  = 48 MHz  (identical)
 *   Timers:  200 MHz = 200 MHz (identical)
 *
 * FDCAN Clock:
 *   8 MHz:  PLL2Q = 25 MHz
 *   25 MHz: HSE direct = 25 MHz
 *   Both provide 25 MHz for CAN compliance
 *
 * Conclusion:
 *   Both configurations achieve identical system performance.
 *   Choose 8 MHz for simplicity (no hardware modification).
 *   Choose 25 MHz if crystal is already installed or if direct
 *   HSE routing to FDCAN is preferred for minimal jitter.
 */

/* Peripheral Clock Selection Rationale
 *
 * I2C (HSI 64 MHz):
 *   - Independent from HSE, continues operation if HSE fails
 *   - I2C timing programmable via TIMINGR register
 *   - 64 MHz provides adequate resolution for all I2C speeds
 *   - Supports: Standard (100 kHz), Fast (400 kHz), Fast Plus (1 MHz)
 *
 * SPI1/2/3 (PLL1Q 200 MHz):
 *   - High-frequency kernel clock for fast SPI transfers
 *   - Real SCK frequency = kernel / prescaler (min prescaler /2)
 *   - Maximum SCK: 200 MHz / 2 = 100 MHz (within 133 MHz limit)
 *   - NuttX driver automatically calculates prescaler
 *
 * SPI4/5 (PLL2P 75 MHz):
 *   - Separate PLL prevents interference with SPI1/2/3
 *   - Maximum SCK: 75 MHz / 2 = 37.5 MHz
 *   - Adequate for most SPI peripherals
 *
 * SPI6 (PCLK4 100 MHz):
 *   - Uses APB4 clock directly (simplifies configuration)
 *   - Maximum SCK: 100 MHz / 2 = 50 MHz
 *   - APB4 is low-power domain, suitable for slower devices
 *
 * ADC (PLL2P 75 MHz):
 *   - Within ADC kernel clock limit (80 MHz per DS12110)
 *   - Provides good sampling rates without exceeding specs
 *   - Shared with SPI4/5 to conserve PLL resources
 *
 * FDCAN (PLL2Q 25 MHz or HSE 25 MHz):
 *   - 25 MHz is standard CAN kernel clock frequency
 *   - Enables clean generation of standard CAN bitrates:
 *     125 kbps, 250 kbps, 500 kbps, 1 Mbps
 *   - 8 MHz config: Uses PLL2Q (generated from HSE)
 *   - 25 MHz config: Uses HSE direct (lower jitter)
 *
 * SDMMC (PLL1Q 200 MHz):
 *   - High frequency enables fast SD card transfers
 *   - Transfer mode: 200 MHz / 4 = 50 MHz
 *   - Supports high-speed SD cards (25-50 MB/s)
 *
 * USB (HSI48 48 MHz):
 *   - Dedicated 48 MHz RC oscillator for USB
 *   - Meets USB 2.0 specification (48 MHz ±0.25%)
 *   - Independent from other system clocks
 *
 * Timers (2 × PCLK = 200 MHz):
 *   - Standard STM32 behavior: timer clock = 2 × PCLK when APB prescaler ≠ 1
 *   - Provides high resolution for PWM and timing applications
 *   - All timers operate at 200 MHz for consistent timing
 */

/* Design Decisions and Trade-offs
 *
 * Why 400 MHz SYSCLK instead of 480 MHz?
 *   - NuttX uses VOS1 voltage scaling by default
 *   - VOS1 limits SYSCLK to 400 MHz maximum (DS12110 Table 35)
 *   - 480 MHz would require VOS0 configuration (not in NuttX default)
 *   - 400 MHz provides excellent performance for most applications
 *   - Ensures maximum compatibility across all NuttX platforms
 *
 * Why identical performance for 8 MHz and 25 MHz?
 *   - Simplifies board support (same performance, different HSE)
 *   - Users can choose HSE based on hardware availability
 *   - No software changes needed when switching HSE sources
 *   - Both configurations validated against same specifications
 *
 * Why HSI for I2C instead of HSE?
 *   - Provides clock source independence (HSE failure tolerance)
 *   - I2C timing is programmable, not dependent on exact frequency
 *   - Simplifies clock tree (one less HSE dependency)
 *
 * Why separate PLLs for SPI123 and SPI45?
 *   - Prevents clock domain interference
 *   - Different frequency requirements (200 MHz vs 75 MHz)
 *   - Allows independent optimization of each SPI group
 */

/* Flash Wait States Configuration
 *
 * FLASH wait states must be configured based on:
 *   - Core voltage (VOS level)
 *   - HCLK frequency
 *
 * For VOS1 (1.15-1.26V) and HCLK = 200 MHz:
 *   Reference: DS12110 Table 13
 *   0 WS: up to  70 MHz
 *   1 WS: up to 140 MHz
 *   2 WS: up to 210 MHz ← Minimum required for 200 MHz
 *   3 WS: up to 275 MHz
 *   4 WS: up to 480 MHz ← Selected (conservative, provides margin)
 *
 * Selected: 4 wait states
 *   - Provides safety margin above minimum (2 WS)
 *   - Ensures reliable operation across all conditions
 *   - Could be optimized to 2 WS for slightly better performance
 */

/* Validation Summary
 *
 * This clock configuration has been validated against:
 *   ✓ UM2407 - Nucleo-H753ZI User Manual
 *   ✓ DS12110 Rev 9 - STM32H753ZI Datasheet
 *   ✓ RM0433 - STM32H7x3 Reference Manual
 *   ✓ NuttX 12.11+ STM32H7 drivers (arch/arm/src/stm32h7/)
 *
 * All configurations respect:
 *   ✓ PLL input frequency ranges (1-16 MHz, device-specific)
 *   ✓ VCO frequency limits (192-836 MHz)
 *   ✓ SYSCLK maximum for VOS1 (400 MHz)
 *   ✓ Individual peripheral clock limits
 *   ✓ CAN bitrate requirements (25 MHz kernel clock)
 *   ✓ USB timing requirements (48 MHz ±0.25%)
 *
 * Clock accuracy:
 *   HSE (8 MHz):   ±50 ppm typical (ST-LINK MCO)
 *   HSE (25 MHz):  ±30 ppm typical (crystal dependent)
 *   HSI (64 MHz):  ±1% (factory trimmed)
 *   LSE (32768 Hz): ±20 ppm typical (crystal dependent)
 *   HSI48 (48 MHz): ±0.25% (USB compliant)
 */

#define STM32_HSI_FREQUENCY     64000000ul
#define STM32_CSI_FREQUENCY     4000000ul
#define STM32_LSI_FREQUENCY     32000ul
#define STM32_LSE_FREQUENCY     32768ul

#ifdef CONFIG_NUCLEO_H753ZI_HSE_25MHZ   
#  define STM32_HSE_FREQUENCY   25000000ul
#else
#  define STM32_HSE_FREQUENCY   8000000ul
#endif

/* Main PLL Configuration.
 *
 * PLL source is HSE
 *
 * When STM32_HSE_FREQUENCY / PLLM <= 2MHz VCOL must be selected.
 * VCOH otherwise.
 *
 * PLL_VCOx = (STM32_HSE_FREQUENCY / PLLM) * PLLN
 * Subject to:
 *
 *     1 <= PLLM <= 63
 *     4 <= PLLN <= 512
 *   150 MHz <= PLL_VCOL <= 420MHz
 *   192 MHz <= PLL_VCOH <= 836MHz
 *
 * SYSCLK  = PLL_VCO / PLLP
 * CPUCLK  = SYSCLK / D1CPRE
 * Subject to
 *
 *   PLLP1   = {2, 4, 6, 8, ..., 128}
 *   PLLP2,3 = {2, 3, 4, ..., 128}
 *   CPUCLK <= 480 MHz
 */

#define STM32_BOARD_USEHSE
#define STM32_PLLCFG_PLLSRC      RCC_PLLCKSELR_PLLSRC_HSE

#ifdef CONFIG_NUCLEO_H753ZI_HSE_25MHZ

/* PLL1 - 25 MHz HSE input, enable DIVP, DIVQ, DIVR
 *
 * CORRECTED: VCO limited to 836 MHz, SYSCLK limited to 400 MHz (VOS1)
 *
 *   PLL1_input = 25 MHz / 5 = 5 MHz (within 4-8 MHz range)
 *   PLL1_VCO   = 5 MHz * 160 = 800 MHz (within 192-836 MHz)
 *
 *   PLL1P = PLL1_VCO/2  = 800 MHz / 2  = 400 MHz (SYSCLK)
 *   PLL1Q = PLL1_VCO/4  = 800 MHz / 4  = 200 MHz (SPI123, SDMMC)
 *   PLL1R = PLL1_VCO/4  = 800 MHz / 4  = 200 MHz
 *
 * Note: Same 400 MHz SYSCLK as 8 MHz config (VOS1 safe, NuttX default)
 */

#define STM32_PLLCFG_PLL1CFG     (RCC_PLLCFGR_PLL1VCOSEL_WIDE| \
                                  RCC_PLLCFGR_PLL1RGE_4_8_MHZ| \
                                  RCC_PLLCFGR_DIVP1EN| \
                                  RCC_PLLCFGR_DIVQ1EN| \
                                  RCC_PLLCFGR_DIVR1EN)
#define STM32_PLLCFG_PLL1M       RCC_PLLCKSELR_DIVM1(5)
#define STM32_PLLCFG_PLL1N       RCC_PLL1DIVR_N1(160)
#define STM32_PLLCFG_PLL1P       RCC_PLL1DIVR_P1(2)
#define STM32_PLLCFG_PLL1Q       RCC_PLL1DIVR_Q1(4)
#define STM32_PLLCFG_PLL1R       RCC_PLL1DIVR_R1(4)

#define STM32_VCO1_FREQUENCY     ((STM32_HSE_FREQUENCY / 5) * 160)
#define STM32_PLL1P_FREQUENCY    (STM32_VCO1_FREQUENCY / 2)
#define STM32_PLL1Q_FREQUENCY    (STM32_VCO1_FREQUENCY / 4)
#define STM32_PLL1R_FREQUENCY    (STM32_VCO1_FREQUENCY / 4)

/* PLL2 - 25 MHz HSE input, enable DIVP, DIVQ, DIVR
 *
 * CORRECTED: Input range (was 4-8 MHz, needs 8-16 MHz) and FDCAN clock
 *
 *   PLL2_input = 25 MHz / 2 = 12.5 MHz (within 8-16 MHz range)
 *   PLL2_VCO   = 12.5 MHz * 48 = 600 MHz (within 192-836 MHz)
 *
 *   PLL2P = PLL2_VCO/8  = 600 MHz / 8  = 75 MHz (ADC, SPI45)
 *   PLL2Q = PLL2_VCO/24 = 600 MHz / 24 = 25 MHz (FDCAN - not used, HSE direct)
 *   PLL2R = PLL2_VCO/3  = 600 MHz / 3  = 200 MHz
 *
 * Note: FDCAN uses HSE directly (25 MHz) for this configuration,
 *       but PLL2Q is configured to 25 MHz for consistency
 */

#define STM32_PLLCFG_PLL2CFG (RCC_PLLCFGR_PLL2VCOSEL_WIDE| \
                              RCC_PLLCFGR_PLL2RGE_8_16_MHZ| \
                              RCC_PLLCFGR_DIVP2EN| \
                              RCC_PLLCFGR_DIVQ2EN| \
                              RCC_PLLCFGR_DIVR2EN)
#define STM32_PLLCFG_PLL2M       RCC_PLLCKSELR_DIVM2(2)
#define STM32_PLLCFG_PLL2N       RCC_PLL2DIVR_N2(48)
#define STM32_PLLCFG_PLL2P       RCC_PLL2DIVR_P2(8)
#define STM32_PLLCFG_PLL2Q       RCC_PLL2DIVR_Q2(24)
#define STM32_PLLCFG_PLL2R       RCC_PLL2DIVR_R2(3)

#define STM32_VCO2_FREQUENCY     ((STM32_HSE_FREQUENCY / 2) * 48)
#define STM32_PLL2P_FREQUENCY    (STM32_VCO2_FREQUENCY / 8)
#define STM32_PLL2Q_FREQUENCY    (STM32_VCO2_FREQUENCY / 24)
#define STM32_PLL2R_FREQUENCY    (STM32_VCO2_FREQUENCY / 3)

#define STM32_VCO2_FREQUENCY     ((STM32_HSE_FREQUENCY / 2) * 48)
#define STM32_PLL2P_FREQUENCY    (STM32_VCO2_FREQUENCY / 8)
#define STM32_PLL2Q_FREQUENCY    (STM32_VCO2_FREQUENCY / 40)
#define STM32_PLL2R_FREQUENCY    (STM32_VCO2_FREQUENCY / 3)

/* PLL3 - 25 MHz HSE input, enable DIVP, DIVQ, DIVR
 *
 * CORRECTED: Input frequency limited to 8-16 MHz range
 *
 *   PLL3_input = 25 MHz / 2 = 12.5 MHz (within 8-16 MHz range)
 *   PLL3_VCO   = 12.5 MHz * 64 = 800 MHz (within 192-836 MHz)
 *
 *   PLL3P = PLL3_VCO/2  = 800 MHz / 2  = 400 MHz
 *   PLL3Q = PLL3_VCO/32 = 800 MHz / 32 = 25 MHz
 *   PLL3R = PLL3_VCO/20 = 800 MHz / 20 = 40 MHz
 */

#define STM32_PLLCFG_PLL3CFG (RCC_PLLCFGR_PLL3VCOSEL_WIDE| \
                              RCC_PLLCFGR_PLL3RGE_8_16_MHZ| \
                              RCC_PLLCFGR_DIVP3EN| \
                              RCC_PLLCFGR_DIVQ3EN| \
                              RCC_PLLCFGR_DIVR3EN)
#define STM32_PLLCFG_PLL3M       RCC_PLLCKSELR_DIVM3(2)
#define STM32_PLLCFG_PLL3N       RCC_PLL3DIVR_N3(64)
#define STM32_PLLCFG_PLL3P       RCC_PLL3DIVR_P3(2)
#define STM32_PLLCFG_PLL3Q       RCC_PLL3DIVR_Q3(32)
#define STM32_PLLCFG_PLL3R       RCC_PLL3DIVR_R3(20)

#define STM32_VCO3_FREQUENCY     ((STM32_HSE_FREQUENCY / 2) * 64)
#define STM32_PLL3P_FREQUENCY    (STM32_VCO3_FREQUENCY / 2)
#define STM32_PLL3Q_FREQUENCY    (STM32_VCO3_FREQUENCY / 32)
#define STM32_PLL3R_FREQUENCY    (STM32_VCO3_FREQUENCY / 20)

#else /* CONFIG_NUCLEO_H753ZI_HSE_8MHZ (default) */

/* PLL1 - 8 MHz HSE input, enable DIVP, DIVQ, DIVR
 *
 * Calculation for 8 MHz HSE:
 *   Step 1: PLL1 input = HSE / DIVM1 = 8 MHz / 1 = 8 MHz
 *           Range: 8 MHz is within 4-16 MHz (RCC_PLLCFGR_PLL1RGE_4_8_MHZ)
 *
 *   Step 2: VCO = PLL1_input * DIVN1 = 8 MHz * 100 = 800 MHz
 *           Range: 800 MHz is within 192-836 MHz (VCOH)
 *
 *   PLL1P = VCO / DIVP1 = 800 MHz / 2 = 400 MHz (SYSCLK)
 *   PLL1Q = VCO / DIVQ1 = 800 MHz / 4 = 200 MHz
 *   PLL1R = VCO / DIVR1 = 800 MHz / 4 = 200 MHz
 */

#define STM32_PLLCFG_PLL1CFG     (RCC_PLLCFGR_PLL1VCOSEL_WIDE| \
                                  RCC_PLLCFGR_PLL1RGE_4_8_MHZ| \
                                  RCC_PLLCFGR_DIVP1EN| \
                                  RCC_PLLCFGR_DIVQ1EN| \
                                  RCC_PLLCFGR_DIVR1EN)
#define STM32_PLLCFG_PLL1M       RCC_PLLCKSELR_DIVM1(1)
#define STM32_PLLCFG_PLL1N       RCC_PLL1DIVR_N1(100)
#define STM32_PLLCFG_PLL1P       RCC_PLL1DIVR_P1(2)
#define STM32_PLLCFG_PLL1Q       RCC_PLL1DIVR_Q1(4)
#define STM32_PLLCFG_PLL1R       RCC_PLL1DIVR_R1(4)

#define STM32_VCO1_FREQUENCY     ((STM32_HSE_FREQUENCY / 1) * 100)
#define STM32_PLL1P_FREQUENCY    (STM32_VCO1_FREQUENCY / 2)
#define STM32_PLL1Q_FREQUENCY    (STM32_VCO1_FREQUENCY / 4)
#define STM32_PLL1R_FREQUENCY    (STM32_VCO1_FREQUENCY / 4)

/* PLL2 - 8 MHz HSE input, enable DIVP, DIVQ, DIVR
 *
 * CRITICAL: PLL2Q must output 25 MHz for FDCAN compatibility
 *
 * Calculation for 8 MHz HSE:
 *   Step 1: PLL2 input = HSE / DIVM2 = 8 MHz / 1 = 8 MHz
 *           Range: 8 MHz is within 4-16 MHz (RCC_PLLCFGR_PLL2RGE_4_8_MHZ)
 *
 *   Step 2: VCO = PLL2_input * DIVN2 = 8 MHz * 75 = 600 MHz
 *           Range: 600 MHz is within 192-836 MHz (VCOH)
 *
 *   PLL2P = VCO / DIVP2 = 600 MHz / 8  = 75 MHz (ADC, SPI45)
 *   PLL2Q = VCO / DIVQ2 = 600 MHz / 24 = 25 MHz (FDCAN kernel clock)
 *   PLL2R = VCO / DIVR2 = 600 MHz / 3  = 200 MHz
 */

#define STM32_PLLCFG_PLL2CFG (RCC_PLLCFGR_PLL2VCOSEL_WIDE| \
                              RCC_PLLCFGR_PLL2RGE_4_8_MHZ| \
                              RCC_PLLCFGR_DIVP2EN| \
                              RCC_PLLCFGR_DIVQ2EN| \
                              RCC_PLLCFGR_DIVR2EN)
#define STM32_PLLCFG_PLL2M       RCC_PLLCKSELR_DIVM2(1)
#define STM32_PLLCFG_PLL2N       RCC_PLL2DIVR_N2(75)
#define STM32_PLLCFG_PLL2P       RCC_PLL2DIVR_P2(8)
#define STM32_PLLCFG_PLL2Q       RCC_PLL2DIVR_Q2(24)
#define STM32_PLLCFG_PLL2R       RCC_PLL2DIVR_R2(3)

#define STM32_VCO2_FREQUENCY     ((STM32_HSE_FREQUENCY / 1) * 75)
#define STM32_PLL2P_FREQUENCY    (STM32_VCO2_FREQUENCY / 8)
#define STM32_PLL2Q_FREQUENCY    (STM32_VCO2_FREQUENCY / 24)
#define STM32_PLL2R_FREQUENCY    (STM32_VCO2_FREQUENCY / 3)

/* PLL3 - Not configured for 8 MHz HSE
 * FDCAN clock is provided by PLL2Q (25 MHz) instead
 */
 /* PLL3 - 8 MHz HSE input, enable DIVP, DIVQ, DIVR
 *
 * NOTE: PLL3 is not used for 8 MHz HSE configuration
 *       FDCAN clock is provided by PLL2Q (25 MHz) instead
 *       However, PLL3 must be defined for NuttX RCC code compatibility
 *
 *   Step 1: PLL3 input = HSE / DIVM3 = 8 MHz / 2 = 4 MHz
 *           Range: 4 MHz is within 2-16 MHz (RCC_PLLCFGR_PLL3RGE_2_4_MHZ)
 *
 *   Step 2: VCO = PLL3_input * DIVN3 = 4 MHz * 100 = 400 MHz
 *           Range: 400 MHz is within 192-836 MHz (VCOH)
 *
 *   PLL3P = VCO / DIVP3 = 400 MHz / 2  = 200 MHz
 *   PLL3Q = VCO / DIVQ3 = 400 MHz / 16 = 25 MHz
 *   PLL3R = VCO / DIVR3 = 400 MHz / 10 = 40 MHz
 */

#define STM32_PLLCFG_PLL3CFG (RCC_PLLCFGR_PLL3VCOSEL_WIDE| \
                              RCC_PLLCFGR_PLL3RGE_2_4_MHZ| \
                              RCC_PLLCFGR_DIVP3EN| \
                              RCC_PLLCFGR_DIVQ3EN| \
                              RCC_PLLCFGR_DIVR3EN)
#define STM32_PLLCFG_PLL3M       RCC_PLLCKSELR_DIVM3(2)
#define STM32_PLLCFG_PLL3N       RCC_PLL3DIVR_N3(100)
#define STM32_PLLCFG_PLL3P       RCC_PLL3DIVR_P3(2)
#define STM32_PLLCFG_PLL3Q       RCC_PLL3DIVR_Q3(16)
#define STM32_PLLCFG_PLL3R       RCC_PLL3DIVR_R3(10)

#define STM32_VCO3_FREQUENCY     ((STM32_HSE_FREQUENCY / 2) * 100)
#define STM32_PLL3P_FREQUENCY    (STM32_VCO3_FREQUENCY / 2)
#define STM32_PLL3Q_FREQUENCY    (STM32_VCO3_FREQUENCY / 16)
#define STM32_PLL3R_FREQUENCY    (STM32_VCO3_FREQUENCY / 10)

#endif /* CONFIG_NUCLEO_H753ZI_HSE_25MHZ */

/* SYSCLK = PLL1P
 * CPUCLK = SYSCLK / 1
 */

#define STM32_RCC_D1CFGR_D1CPRE  (RCC_D1CFGR_D1CPRE_SYSCLK)
#define STM32_SYSCLK_FREQUENCY   (STM32_PLL1P_FREQUENCY)
#define STM32_CPUCLK_FREQUENCY   (STM32_SYSCLK_FREQUENCY / 1)

/* Configure Clock Assignments */

/* AHB clock (HCLK) is SYSCLK/2
 * HCLK1 = HCLK2 = HCLK3 = HCLK4
 */

#define STM32_RCC_D1CFGR_HPRE   RCC_D1CFGR_HPRE_SYSCLKd2
#define STM32_ACLK_FREQUENCY    (STM32_SYSCLK_FREQUENCY / 2)
#define STM32_HCLK_FREQUENCY    (STM32_SYSCLK_FREQUENCY / 2)

/* APB1 clock (PCLK1) is HCLK/2 */

#define STM32_RCC_D2CFGR_D2PPRE1  RCC_D2CFGR_D2PPRE1_HCLKd2
#define STM32_PCLK1_FREQUENCY     (STM32_HCLK_FREQUENCY/2)

/* APB2 clock (PCLK2) is HCLK/2 */

#define STM32_RCC_D2CFGR_D2PPRE2  RCC_D2CFGR_D2PPRE2_HCLKd2
#define STM32_PCLK2_FREQUENCY     (STM32_HCLK_FREQUENCY/2)

/* APB3 clock (PCLK3) is HCLK/2 */

#define STM32_RCC_D1CFGR_D1PPRE   RCC_D1CFGR_D1PPRE_HCLKd2
#define STM32_PCLK3_FREQUENCY     (STM32_HCLK_FREQUENCY/2)

/* APB4 clock (PCLK4) is HCLK/2 */

#define STM32_RCC_D3CFGR_D3PPRE   RCC_D3CFGR_D3PPRE_HCLKd2
#define STM32_PCLK4_FREQUENCY     (STM32_HCLK_FREQUENCY/2)

/* Timer clock frequencies */

/* Timers driven from APB1 will be twice PCLK1 */

#define STM32_APB1_TIM2_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM3_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM4_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM5_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM6_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM7_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM12_CLKIN  (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM13_CLKIN  (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM14_CLKIN  (2*STM32_PCLK1_FREQUENCY)

/* Timers driven from APB2 will be twice PCLK2 */

#define STM32_APB2_TIM1_CLKIN   (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM8_CLKIN   (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM15_CLKIN  (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM16_CLKIN  (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM17_CLKIN  (2*STM32_PCLK2_FREQUENCY)

/* Kernel Clock Configuration
 *
 * Note: Refer to Table 54 in STM32H7 Reference Manual (RM0433)
 */

/* I2C123 clock source - HSI (64 MHz) */

#define STM32_RCC_D2CCIP2R_I2C123SRC RCC_D2CCIP2R_I2C123SEL_HSI

/* I2C4 clock source - HSI (64 MHz) */

#define STM32_RCC_D3CCIPR_I2C4SRC    RCC_D3CCIPR_I2C4SEL_HSI

/* SPI123 clock source - PLL1Q */

#define STM32_RCC_D2CCIP1R_SPI123SRC RCC_D2CCIP1R_SPI123SEL_PLL1

/* SPI45 clock source - PLL2P */

#define STM32_RCC_D2CCIP1R_SPI45SRC  RCC_D2CCIP1R_SPI45SEL_PLL2

/* SPI6 clock source - PCLK4 */

#define STM32_RCC_D3CCIPR_SPI6SRC    RCC_D3CCIPR_SPI6SEL_PCLK4

/* USB 1 and 2 clock source - HSI48 */

#define STM32_RCC_D2CCIP2R_USBSRC    RCC_D2CCIP2R_USBSEL_HSI48

/* ADC 1 2 3 clock source - PLL2P */

#define STM32_RCC_D3CCIPR_ADCSRC     RCC_D3CCIPR_ADCSEL_PLL2

#ifdef CONFIG_NUCLEO_H753ZI_HSE_25MHZ
/* FDCAN 1 2 clock source - HSE (25 MHz direct) */
#  define STM32_RCC_D2CCIP1R_FDCANSEL  RCC_D2CCIP1R_FDCANSEL_HSE
#else
/* FDCAN 1 2 clock source - PLL2Q (25 MHz for CAN compliance) */
#  define STM32_RCC_D2CCIP1R_FDCANSEL  RCC_D2CCIP1R_FDCANSEL_PLL2
#endif

/* SDMMC 1 2 clock source - PLL1Q */

#define STM32_RCC_D1CCIPR_SDMMCSEL   RCC_D1CCIPR_SDMMC_PLL1

/* FMC clock source - HCLK */

#define BOARD_FMC_CLK                RCC_D1CCIPR_FMCSEL_HCLK

/* FLASH Configuration ******************************************************/

/* FLASH wait states
 *
 *  ------------ ---------- -----------
 *  Vcore        MAX ACLK   WAIT STATES
 *  ------------ ---------- -----------
 *  1.15-1.26 V     70 MHz    0
 *  (VOS1 level)   140 MHz    1
 *                 210 MHz    2
 *                 275 MHz    3
 *                 480 MHz    4
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
 * because PLL1Q is maintained at a constant 200 MHz regardless of HSE freq.
 *
 * Clock Frequency Verification Table:
 * +----------------+--------+---------+-------------+------------------+
 * | HSE Source     | VCO    | PLL1Q   | SDMMC Init  | SDMMC Transfer   |
 * +----------------+--------+---------+-------------+------------------+
 * | ST-LINK 8MHz   | 800MHz | 200MHz  | 416 kHz     | 50 MHz           |
 * | Crystal 25MHz  | 800MHz | 200MHz  | 416 kHz     | 50 MHz           |
 * +----------------+--------+---------+-------------+------------------+
 *
 * Calculation for 8 MHz HSE:
 *   SDMMC_Init = PLL1Q / (2*240) = 200MHz / 480 = 416 kHz (SD compliant)
 *   SDMMC_Xfer = PLL1Q / (2*2)   = 200MHz / 4   = 50 MHz  (25 MB/s)
 *
 * Calculation for 25 MHz HSE:
 *   SDMMC_Init = PLL1Q / (2*240) = 200MHz / 480 = 416 kHz (SD compliant)
 *   SDMMC_Xfer = PLL1Q / (2*2)   = 200MHz / 4   = 50 MHz  (25 MB/s)
 */

/* Init 400kHz, PLL1Q/(2*240) */
#define STM32_SDMMC_INIT_CLKDIV   (240 << STM32_SDMMC_CLKCR_CLKDIV_SHIFT)

/* Transfer at max speed, PLL1Q/(2*2) */
#define STM32_SDMMC_MMCXFR_CLKDIV (2 << STM32_SDMMC_CLKCR_CLKDIV_SHIFT)
#define STM32_SDMMC_SDXFR_CLKDIV  (2 << STM32_SDMMC_CLKCR_CLKDIV_SHIFT)
#define STM32_SDMMC_CLKCR_EDGE    STM32_SDMMC_CLKCR_NEGEDGE

/* SDMMC clock source - PLL1Q */
#define STM32_RCC_D1CCIPR_SDMMCSEL RCC_D1CCIPR_SDMMC_PLL1

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
 * By default nucleo-h753 has Solder Bridges 'ON' (SBXY: ON).
 * MCU pins are already connected to Ethernet connector.
 * Hence, no connection for these pins from ST Zio or Morpho connectors
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

#define GPIO_ETH_RMII_REF_CLK (GPIO_ETH_RMII_REF_CLK_0|GPIO_SPEED_100MHz)/*PA1*/
#define GPIO_ETH_RMII_CRS_DV  (GPIO_ETH_RMII_CRS_DV_0 |GPIO_SPEED_100MHz)/*PA7*/
#define GPIO_ETH_RMII_TX_EN   (GPIO_ETH_RMII_TX_EN_2  |GPIO_SPEED_100MHz)/*PG11*/
#define GPIO_ETH_RMII_TXD0    (GPIO_ETH_RMII_TXD0_2   |GPIO_SPEED_100MHz)/*PG13*/
#define GPIO_ETH_RMII_TXD1    (GPIO_ETH_RMII_TXD1_1   |GPIO_SPEED_100MHz)/*PB13*/
#define GPIO_ETH_RMII_RXD0    (GPIO_ETH_RMII_RXD0_0   |GPIO_SPEED_100MHz)/*PC4*/
#define GPIO_ETH_RMII_RXD1    (GPIO_ETH_RMII_RXD1_0   |GPIO_SPEED_100MHz)/*PC5*/
#define GPIO_ETH_MDIO         (GPIO_ETH_MDIO_0        |GPIO_SPEED_100MHz)/*PA2*/
#define GPIO_ETH_MDC          (GPIO_ETH_MDC_0         |GPIO_SPEED_100MHz)/*PC1*/

/* FDCAN1 GPIO Configuration */
#define GPIO_CAN1_RX     GPIO_CAN1_RX_2      /* PB8 */
#define GPIO_CAN1_TX     GPIO_CAN1_TX_2      /* PB9 */

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
 *   For more information, check the Kconfig file or use menuconfig help.
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

/* If CONFIG_ARCH_LEDS is defined, usage by board port is defined in
 * include/board.h and src/stm32_leds.c.
 * The LEDs are used to encode OS-related events as follows:
 *
 *
 *   SYMBOL                     Meaning                  LED state
 *                                                  Red   Green   Orange
 *   ----------------------  ---------------------- ----- ------ ------
 */

#define LED_STARTED        0 /* NuttX has been started OFF   OFF    OFF  */
#define LED_HEAPALLOCATE   1 /* Heap allocated         OFF   OFF    ON   */
#define LED_IRQSENABLED    2 /* Interrupts enabled     OFF   ON     OFF  */
#define LED_STACKCREATED   3 /* Idle stack created     OFF   ON     ON   */
#define LED_INIRQ          4 /* In an interrupt        N/C   N/C    GLOW */
#define LED_SIGNAL         5 /* In a signal handler    N/C   GLOW   N/C  */
#define LED_ASSERTION      6 /* An assertion failed    GLOW  N/C    GLOW */
#define LED_PANIC          7 /* System has crashed     Blink OFF    N/C  */
#define LED_IDLE           8 /* MCU is in sleep mode   ON    OFF    OFF  */

/* Button Configuration *****************************************************/
/*
 * Dynamic Button Configuration with EXTI Conflict Prevention
 *
 * CRITICAL: STM32 EXTI lines are shared across ports!
 * Only ONE pin NUMBER can be used as interrupt source at a time.
 * Example of CONFLICT:
 * PG3 + PE3 (both use EXTI3 -> only last one works)
 * PG3 + PE4 (EXTI3 and EXTI4 -> both work)
 *
 * Valid configuration for 10 buttons:
 *        PC13,  PB1, PB2, PD0, PD4, PD5, PE6, PE7, PF8, PF9
 *   EXTI:  13,    1,   2,   0,   4,   5,   6,   7,   8,   9
 *
 *                 NO EXTERNAL INTERRUPTION CONFLICTS!
 */

/* Define NUM_BUTTONS FIRST */
#ifdef CONFIG_NUCLEO_H753ZI_BUTTON_SUPPORT
#  define NUM_BUTTONS    CONFIG_NUCLEO_H753ZI_BUTTON_COUNT
#else
#  define NUM_BUTTONS    0
#endif

/* Button definitions - only if enabled */
#ifdef CONFIG_NUCLEO_H753ZI_BUTTON_SUPPORT

/* Built-in button */
#  ifdef CONFIG_NUCLEO_H753ZI_BUTTON_BUILTIN
#    define BUTTON_BUILT_IN         0
#    define BUTTON_BUILT_IN_BIT     (1 << 0)
#  endif

/* External buttons */

#  if NUM_BUTTONS > 1
#    define BUTTON_1                1
#    define BUTTON_1_BIT            (1 << 1)
#  endif

#  if NUM_BUTTONS > 2
#    define BUTTON_2                2
#    define BUTTON_2_BIT            (1 << 2)
#  endif

#  if NUM_BUTTONS > 3
#    define BUTTON_3                3
#    define BUTTON_3_BIT            (1 << 3)
#  endif

#  if NUM_BUTTONS > 4
#    define BUTTON_4                4
#    define BUTTON_4_BIT            (1 << 4)
#  endif

#  if NUM_BUTTONS > 5
#    define BUTTON_5                5
#    define BUTTON_5_BIT            (1 << 5)
#  endif

#  if NUM_BUTTONS > 6
#    define BUTTON_6                6
#    define BUTTON_6_BIT            (1 << 6)
#  endif

#  if NUM_BUTTONS > 7
#    define BUTTON_7                7
#    define BUTTON_7_BIT            (1 << 7)
#  endif

#  if NUM_BUTTONS > 8
#    define BUTTON_8                8
#    define BUTTON_8_BIT            (1 << 8)
#  endif

#  if NUM_BUTTONS > 9
#    define BUTTON_9                9
#    define BUTTON_9_BIT            (1 << 9)
#  endif

#  if NUM_BUTTONS > 10
#    define BUTTON_10                10
#    define BUTTON_10_BIT            (1 << 10)
#  endif

#  if NUM_BUTTONS > 11
#    define BUTTON_11                11
#    define BUTTON_11_BIT            (1 << 11)
#  endif

#  if NUM_BUTTONS > 12
#    define BUTTON_12                12
#    define BUTTON_12_BIT            (1 << 12)
#  endif

#  if NUM_BUTTONS > 13
#    define BUTTON_13                13
#    define BUTTON_13_BIT            (1 << 13)
#  endif

#  if NUM_BUTTONS > 14
#    define BUTTON_14                14
#    define BUTTON_14_BIT            (1 << 14)
#  endif

#  if NUM_BUTTONS > 15
#    define BUTTON_15                15
#    define BUTTON_15_BIT            (1 << 15)
#  endif

#  if NUM_BUTTONS > 16
#    define BUTTON_16                16
#    define BUTTON_16_BIT            (1 << 16)
#  endif

#  if NUM_BUTTONS > 17
#    define BUTTON_17                17
#    define BUTTON_17_BIT            (1 << 17)
#  endif

#  if NUM_BUTTONS > 18
#    define BUTTON_18                18
#    define BUTTON_18_BIT            (1 << 18)
#  endif

#  if NUM_BUTTONS > 19
#    define BUTTON_19                19
#    define BUTTON_19_BIT            (1 << 19)
#  endif

#  if NUM_BUTTONS > 20
#    define BUTTON_20                20
#    define BUTTON_20_BIT            (1 << 20)
#  endif

#  if NUM_BUTTONS > 21
#    define BUTTON_21                21
#    define BUTTON_21_BIT            (1 << 21)
#  endif

#  if NUM_BUTTONS > 22
#    define BUTTON_22                22
#    define BUTTON_22_BIT            (1 << 22)
#  endif

#  if NUM_BUTTONS > 23
#    define BUTTON_23                23
#    define BUTTON_23_BIT            (1 << 23)
#  endif

#  if NUM_BUTTONS > 24
#    define BUTTON_24                24
#    define BUTTON_24_BIT            (1 << 24)
#  endif

#  if NUM_BUTTONS > 25
#    define BUTTON_25                25
#    define BUTTON_25_BIT            (1 << 25)
#  endif

#  if NUM_BUTTONS > 26
#    define BUTTON_26                26
#    define BUTTON_26_BIT            (1 << 26)
#  endif

#  if NUM_BUTTONS > 27
#    define BUTTON_27                27
#    define BUTTON_27_BIT            (1 << 27)
#  endif

#  if NUM_BUTTONS > 28
#    define BUTTON_28                28
#    define BUTTON_28_BIT            (1 << 28)
#  endif

#  if NUM_BUTTONS > 29
#    define BUTTON_29                29
#    define BUTTON_29_BIT            (1 << 29)
#  endif

#  if NUM_BUTTONS > 30
#    define BUTTON_30                30
#    define BUTTON_30_BIT            (1 << 30)
#  endif

#  if NUM_BUTTONS > 31
#    define BUTTON_31                31
#    define BUTTON_31_BIT            (1 << 31)
#  endif

/* IRQ button range */
#  define MIN_IRQBUTTON      0
#  define MAX_IRQBUTTON      (NUM_BUTTONS - 1)
#  define NUM_IRQBUTTONS     NUM_BUTTONS

#else /* !CONFIG_NUCLEO_H753ZI_BUTTON_SUPPORT */

#  define NUM_BUTTONS        0
#  define MIN_IRQBUTTON      0
#  define MAX_IRQBUTTON      0
#  define NUM_IRQBUTTONS     0

#endif /* CONFIG_NUCLEO_H753ZI_BUTTON_SUPPORT */

/* GPIO Pin Alternate Function Selections ***********************************/

/* ADC GPIO Definitions */

#define GPIO_ADC123_INP10 GPIO_ADC123_INP10_0                      /* PC0 */
#define GPIO_ADC123_INP12 GPIO_ADC123_INP12_0                      /* PC2 */
#define GPIO_ADC123_INP11 GPIO_ADC123_INP11_0                      /* PC1 */
#define GPIO_ADC12_INP13  GPIO_ADC12_INP13_0                       /* PC3 */
#define GPIO_ADC12_INP15  GPIO_ADC12_INP15_0                       /* PA3 */
#define GPIO_ADC12_INP18  GPIO_ADC12_INP18_0                       /* PA4 */
#define GPIO_ADC12_INP19  GPIO_ADC12_INP19_0                       /* PA5 */
#define GPIO_ADC12_INP14  GPIO_ADC12_INP14_0                       /* PA2 */
#define GPIO_ADC123_INP7  GPIO_ADC12_INP7_0                        /* PA7 */
#define GPIO_ADC12_INP5   GPIO_ADC12_INP5_0                        /* PB1 */
#define GPIO_ADC12_INP3   GPIO_ADC12_INP3_0                        /* PA6 */
#define GPIO_ADC12_INP4   GPIO_ADC12_INP4_0                        /* PC4 */
#define GPIO_ADC12_INP8   GPIO_ADC12_INP8_0                        /* PC5 */
#define GPIO_ADC2_INP2    GPIO_ADC2_INP2_0                         /* PF13 */

/* UART/USART GPIO Definitions */

/* USART3 (Nucleo Virtual Console) */
#define GPIO_USART3_RX    (GPIO_USART3_RX_3 | GPIO_SPEED_100MHz)   /* PD9 */
#define GPIO_USART3_TX    (GPIO_USART3_TX_3 | GPIO_SPEED_100MHz)   /* PD8 */

/* USART6 (Arduino Serial Shield) */
#define GPIO_USART6_RX    (GPIO_USART6_RX_2 | GPIO_SPEED_100MHz)   /* PG9 */
#define GPIO_USART6_TX    (GPIO_USART6_TX_2 | GPIO_SPEED_100MHz)   /* PG14 */

/* I2C GPIO Definitions */

/* I2C1 Use Nucleo I2C1 pins */
#define GPIO_I2C1_SCL     (GPIO_I2C1_SCL_1 | GPIO_SPEED_50MHz)     /* PB6 */
#define GPIO_I2C1_SDA     (GPIO_I2C1_SDA_1 | GPIO_SPEED_50MHz)     /* PB7 */

/* I2C2 Use Nucleo I2C2 pins */
#define GPIO_I2C2_SCL     (GPIO_I2C2_SCL_2  | GPIO_SPEED_50MHz)    /* PF1 */
#define GPIO_I2C2_SDA     (GPIO_I2C2_SDA_2  | GPIO_SPEED_50MHz)    /* PF0 */
#define GPIO_I2C2_SMBA    (GPIO_I2C2_SMBA_2 | GPIO_SPEED_50MHz)    /* PF2 */

/* SPI GPIO Definitions */

/* SPI GPIO Definitions - Based on Kconfig selections */
#define GPIO_SPI_CS_SPEED     GPIO_SPEED_50MHz  /* CS pin speed */

/* SPI1 Pin Configurations */
#ifdef CONFIG_NUCLEO_H753ZI_SPI1_ENABLE
#  ifdef CONFIG_NUCLEO_H753ZI_SPI1_PINSET_1
#    define GPIO_SPI1_SCK     GPIO_SPI1_SCK_1    /* PA5 */
#    define GPIO_SPI1_MISO    GPIO_SPI1_MISO_1   /* PA6 */
#    define GPIO_SPI1_MOSI    GPIO_SPI1_MOSI_1   /* PA7 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI1_PINSET_2)
#    define GPIO_SPI1_SCK     GPIO_SPI1_SCK_2    /* PB3 */
#    define GPIO_SPI1_MISO    GPIO_SPI1_MISO_2   /* PB4 */
#    define GPIO_SPI1_MOSI    GPIO_SPI1_MOSI_2   /* PB5 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI1_PINSET_3)
#    define GPIO_SPI1_SCK     GPIO_SPI1_SCK_3    /* PG11 */
#    define GPIO_SPI1_MISO    GPIO_SPI1_MISO_3   /* PG9 */
#    define GPIO_SPI1_MOSI    GPIO_SPI1_MOSI_3   /* PD7 */
#  endif
#endif /* CONFIG_NUCLEO_H753ZI_SPI1_ENABLE */

/* SPI2 Pin Configurations */
#ifdef CONFIG_NUCLEO_H753ZI_SPI2_ENABLE
#  ifdef CONFIG_NUCLEO_H753ZI_SPI2_PINSET_1
#    define GPIO_SPI2_SCK     GPIO_SPI2_SCK_4    /* PB13 */
#    define GPIO_SPI2_MISO    GPIO_SPI2_MISO_1   /* PB14 */
#    define GPIO_SPI2_MOSI    GPIO_SPI2_MOSI_1   /* PB15 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI2_PINSET_2)
#    define GPIO_SPI2_SCK     GPIO_SPI2_SCK_1    /* PA12 */
#    define GPIO_SPI2_MISO    GPIO_SPI2_MISO_2   /* PC2 */
#    define GPIO_SPI2_MOSI    GPIO_SPI2_MOSI_2   /* PC1 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI2_PINSET_3)
#    define GPIO_SPI2_SCK     GPIO_SPI2_SCK_5    /* PD3 */
#    define GPIO_SPI2_MISO    GPIO_SPI2_MISO_2   /* PC2 */
#    define GPIO_SPI2_MOSI    GPIO_SPI2_MOSI_3   /* PC3 */
#  endif
#endif /* CONFIG_NUCLEO_H753ZI_SPI2_ENABLE */

/* SPI3 Pin Configurations */
#ifdef CONFIG_NUCLEO_H753ZI_SPI3_ENABLE

#  ifdef CONFIG_NUCLEO_H753ZI_SPI3_PINSET_1
#    define GPIO_SPI3_SCK     GPIO_SPI3_SCK_1    /* PB3 */
#    define GPIO_SPI3_MISO    GPIO_SPI3_MISO_1   /* PB4 */
#    define GPIO_SPI3_MOSI    GPIO_SPI3_MOSI_1   /* PB2 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI3_PINSET_2)
#    define GPIO_SPI3_SCK     GPIO_SPI3_SCK_2    /* PC10 */
#    define GPIO_SPI3_MISO    GPIO_SPI3_MISO_2   /* PC11 */
#    define GPIO_SPI3_MOSI    GPIO_SPI3_MOSI_2   /* PC12 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI3_PINSET_3)
#    define GPIO_SPI3_SCK     GPIO_SPI3_SCK_3    /* PB3 */
#    define GPIO_SPI3_MISO    GPIO_SPI3_MISO_3   /* PB4 */
#    define GPIO_SPI3_MOSI    GPIO_SPI3_MOSI_3   /* PB5 */
#  endif
#endif /* CONFIG_NUCLEO_H753ZI_SPI3_ENABLE */

/* SPI4 Pin Configurations */
#ifdef CONFIG_NUCLEO_H753ZI_SPI4_ENABLE
#  ifdef CONFIG_NUCLEO_H753ZI_SPI4_PINSET_1
#    define GPIO_SPI4_SCK     GPIO_SPI4_SCK_1    /* PE12 */
#    define GPIO_SPI4_MISO    GPIO_SPI4_MISO_1   /* PE13 */
#    define GPIO_SPI4_MOSI    GPIO_SPI4_MOSI_1   /* PE14 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI4_PINSET_2)
#    define GPIO_SPI4_SCK     GPIO_SPI4_SCK_2    /* PE2 */
#    define GPIO_SPI4_MISO    GPIO_SPI4_MISO_2   /* PE5 */
#    define GPIO_SPI4_MOSI    GPIO_SPI4_MOSI_2   /* PE6 */
#  endif
#endif /* CONFIG_NUCLEO_H753ZI_SPI4_ENABLE */

/* SPI5 Pin Configurations */
#ifdef CONFIG_NUCLEO_H753ZI_SPI5_ENABLE
#  ifdef CONFIG_NUCLEO_H753ZI_SPI5_PINSET_1
#    define GPIO_SPI5_SCK     GPIO_SPI5_SCK_1    /* PF7 */
#    define GPIO_SPI5_MISO    GPIO_SPI5_MISO_1   /* PF8 */
#    define GPIO_SPI5_MOSI    GPIO_SPI5_MOSI_1   /* PF11 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI5_PINSET_2)
#    define GPIO_SPI5_SCK     GPIO_SPI5_SCK_2    /* PF7 */
#    define GPIO_SPI5_MISO    GPIO_SPI5_MISO_2   /* PF8 */
#    define GPIO_SPI5_MOSI    GPIO_SPI5_MOSI_2   /* PF9 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI5_PINSET_3)
#    define GPIO_SPI5_SCK     GPIO_SPI5_SCK_3    /* PH6 */
#    define GPIO_SPI5_MISO    GPIO_SPI5_MISO_3   /* PH7 */
#    define GPIO_SPI5_MOSI    GPIO_SPI5_MOSI_3   /* PF11 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI5_PINSET_4)
#    define GPIO_SPI5_SCK     GPIO_SPI5_SCK_4    /* PK0 */
#    define GPIO_SPI5_MISO    GPIO_SPI5_MISO_4   /* PJ11 */
#    define GPIO_SPI5_MOSI    GPIO_SPI5_MOSI_4   /* PJ10 */
#  endif
#endif /* CONFIG_NUCLEO_H753ZI_SPI5_ENABLE */

/* SPI6 Pin Configurations */
#ifdef CONFIG_NUCLEO_H753ZI_SPI6_ENABLE
#  ifdef CONFIG_NUCLEO_H753ZI_SPI6_PINSET_1
#    define GPIO_SPI6_SCK     GPIO_SPI6_SCK_1    /* PG13 */
#    define GPIO_SPI6_MISO    GPIO_SPI6_MISO_1   /* PG12 */
#    define GPIO_SPI6_MOSI    GPIO_SPI6_MOSI_1   /* PG14 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI6_PINSET_2)
#    define GPIO_SPI6_SCK     GPIO_SPI6_SCK_2    /* PA5 */
#    define GPIO_SPI6_MISO    GPIO_SPI6_MISO_2   /* PA6 */
#    define GPIO_SPI6_MOSI    GPIO_SPI6_MOSI_2   /* PA7 */
#  elif defined(CONFIG_NUCLEO_H753ZI_SPI6_PINSET_3)
#    define GPIO_SPI6_SCK     GPIO_SPI6_SCK_3    /* PB3 */
#    define GPIO_SPI6_MISO    GPIO_SPI6_MISO_3   /* PB4 */
#    define GPIO_SPI6_MOSI    GPIO_SPI6_MOSI_3   /* PB5 */
#  endif
#endif /* CONFIG_NUCLEO_H753ZI_SPI6_ENABLE */

/* SPI CS REGISTRATION PROTOTYPES */

#ifdef CONFIG_STM32H7_SPI
/**
 * Name: stm32_spi_register_cs_device
 *
 * Description:
 *   Register a CS device for a specific SPI bus and device ID.
 *
 * Input Parameters:
 *   spi_bus     - SPI bus number (1-6)
 *   devid       - Device ID (0-15)
 *   cs_pin      - CS pin string (e.g., "PF1")
 *   active_low  - true if CS is active low, false if active high
 *
 * Returned Value:
 *   OK on success, negative errno on error
 */
int stm32_spi_register_cs_device(int spi_bus, uint32_t devid,
                                  const char *cs_pin, bool active_low);

/**
 * Name: stm32_spi_unregister_cs_device
 *
 * Description:
 *   Unregister a CS device.
 *
 * Input Parameters:
 *   spi_bus - SPI bus number (1-6)
 *   devid   - Device ID
 *
 * Returned Value:
 *   OK on success, negative errno on error
 */
int stm32_spi_unregister_cs_device(int spi_bus, uint32_t devid);
#endif /* CONFIG_STM32H7_SPI */

/* Function prototypes ******************************************************/
int stm32_spi_initialize(void);

/* Timer GPIO Definitions */

/* TIM1 */
#define GPIO_TIM1_CH1OUT  (GPIO_TIM1_CH1OUT_2  | GPIO_SPEED_50MHz) /* PE9 */
#define GPIO_TIM1_CH1NOUT (GPIO_TIM1_CH1NOUT_3 | GPIO_SPEED_50MHz) /* PE8 */
#define GPIO_TIM1_CH2OUT  (GPIO_TIM1_CH2OUT_2  | GPIO_SPEED_50MHz) /* PE11 */
#define GPIO_TIM1_CH2NOUT (GPIO_TIM1_CH2NOUT_3 | GPIO_SPEED_50MHz) /* PE10 */
#define GPIO_TIM1_CH3OUT  (GPIO_TIM1_CH3OUT_2  | GPIO_SPEED_50MHz) /* PE13 */
#define GPIO_TIM1_CH3NOUT (GPIO_TIM1_CH3NOUT_3 | GPIO_SPEED_50MHz) /* PE12 */
#define GPIO_TIM1_CH4OUT  (GPIO_TIM1_CH4OUT_2  | GPIO_SPEED_50MHz) /* PE14 */

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