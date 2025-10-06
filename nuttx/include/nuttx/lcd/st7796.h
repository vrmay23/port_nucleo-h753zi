/****************************************************************************
 * include/nuttx/lcd/st7796.h
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

#ifndef __INCLUDE_NUTTX_LCD_ST7796_H
#define __INCLUDE_NUTTX_LCD_ST7796_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* ST7796 Commands - Standard */

#define ST7796_NOP              0x00  /* NOP */
#define ST7796_SWRESET          0x01  /* Software Reset */
#define ST7796_RDDID            0x04  /* Read Display ID */
#define ST7796_RDDST            0x09  /* Read Display Status */
#define ST7796_SLPIN            0x10  /* Sleep In */
#define ST7796_SLPOUT           0x11  /* Sleep Out */
#define ST7796_PTLON            0x12  /* Partial Display Mode On */
#define ST7796_NORON            0x13  /* Normal Display Mode On */
#define ST7796_RDIMGFMT         0x0A  /* Read Display Image Format */
#define ST7796_RDSELFDIAG       0x0F  /* Read Display Self-Diagnostic Result */
#define ST7796_INVOFF           0x20  /* Display Inversion Off */
#define ST7796_INVON            0x21  /* Display Inversion On */
#define ST7796_GAMMASET         0x26  /* Gamma Set */
#define ST7796_DISPOFF          0x28  /* Display Off */
#define ST7796_DISPON           0x29  /* Display On */
#define ST7796_CASET            0x2A  /* Column Address Set */
#define ST7796_RASET            0x2B  /* Row Address Set */
#define ST7796_RAMWR            0x2C  /* Memory Write */
#define ST7796_RAMRD            0x2E  /* Memory Read */
#define ST7796_PTLAR            0x30  /* Partial Area */
#define ST7796_VSCRDEF          0x33  /* Vertical Scrolling Definition */
#define ST7796_TEOFF            0x34  /* Tearing Effect Line Off */
#define ST7796_TEON             0x35  /* Tearing Effect Line On */
#define ST7796_MADCTL           0x36  /* Memory Access Control */
#define ST7796_VSCRSADD         0x37  /* Vertical Scrolling Start Address */
#define ST7796_PIXFMT           0x3A  /* Pixel Format Set */
#define ST7796_WRDISPBRIGHT     0x51  /* Write Display Brightness */
#define ST7796_RDDISPBRIGHT     0x52  /* Read Display Brightness */
#define ST7796_WRCTRLD        0x53  /* Write Control Display */
#define ST7796_RDCTRLD        0x54  /* Read Control Display */
#define ST7796_WRCABC           0x55  /* Write Content Adaptive Brightness Control */
#define ST7796_RDCABC           0x56  /* Read Content Adaptive Brightness Control */
#define ST7796_WRCABCMIN        0x5E  /* Write CABC Minimum Brightness */
#define ST7796_RDCABCMIN        0x5F  /* Read CABC Minimum Brightness */

/* ST7796 Commands - Missing Register Definitions (0xB4-0xC6) */

#define ST7796_INVCTR           0xB4  /* Display Inversion Control (INVCTR) */
#define ST7796_DFC              0xB6  /* Display Function Control (DFC) */
#define ST7796_PWCTRL4          0xC3  /* Power Control 4 */
#define ST7796_PWCTRL5          0xC4  /* Power Control 5 */
#define ST7796_VCOM             0xC5  /* VCOM Control (VCOM) */
#define ST7796_PWCTRL6          0xC6  /* Power Control 6 */
#define ST7796_GAMMAPOS         0xE0  /* Positive Gamma Correction */
#define ST7796_GAMMANEG         0xE1  /* Negative Gamma Correction */
#define ST7796_DOCA             0xE9  /* Set DDB Write Address */

/* ST7796 Initialization sequence structure */

#ifndef __ASSEMBLY__

struct st7796_cmd_s
{
  uint8_t cmd;              /* Command byte */
  FAR const uint8_t *data;  /* Parameter data (NULL if no params) */
  uint8_t len;              /* Number of parameter bytes */
  uint16_t delay_ms;        /* Delay after command in milliseconds */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: st7796_fbinitialize
 *
 * Description:
 * Initialize the ST7796 LCD driver as a framebuffer device.
 *
 * Input Parameters:
 * spi    - SPI device instance
 * set_dc - Function pointer to control DC pin (true=data, false=command)
 *
 * Returned Value:
 * Pointer to framebuffer vtable on success; NULL on failure.
 *
 ****************************************************************************/

FAR struct fb_vtable_s *st7796_fbinitialize(FAR struct spi_dev_s *spi,
                                            CODE void (*set_dc)(bool));

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __INCLUDE_NUTTX_LCD_ST7796_H */
