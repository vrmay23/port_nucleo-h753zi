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
 *   PLL3P  = VCO / DIVP3    = 400 MHz / 2  = 200 MHz
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
 *   I2C1/2/3/4:  HSI        = 16 MHz (workaround for NuttX I2C driver)
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
 *   I2C:     16 MHz  = 16 MHz  (identical)
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
 * I2C (HSI 16 MHz):
 *   - NuttX I2C driver requires HSI at 16 MHz (workaround for driver bug)
 *   - Recommended: PCLK, but not supported by current NuttX driver
 *   - I2C timing programmable via TIMINGR register
 *   - 16 MHz provides adequate resolution for all I2C speeds
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
 * Why HSI for I2C instead of PCLK?
 *   - NuttX I2C driver currently requires HSI at 16 MHz
 *   - This is a known limitation/bug in the NuttX driver
 *   - Future NuttX versions may support PCLK for I2C
 *   - I2C timing is programmable, not dependent on exact frequency
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
 *   HSI (16 MHz):  ±1% (factory trimmed, prescaled from 64 MHz)
 *   LSE (32768 Hz): ±20 ppm typical (crystal dependent)
 *   HSI48 (48 MHz): ±0.25% (USB compliant)
 */
