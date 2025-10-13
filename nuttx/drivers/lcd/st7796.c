/****************************************************************************
 * drivers/lcd/st7796.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>
#include <nuttx/video/fb.h>
#include <nuttx/kmalloc.h>
#include <nuttx/clock.h>
#include <nuttx/signal.h>
#include <nuttx/lcd/st7796.h>

#ifdef CONFIG_LCD_ST7796

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Force SPI MODE 0 (CPOL=0, CPHA=0) - Standard for ST7796 */

#define CONFIG_LCD_ST7796_SPIMODE SPIDEV_MODE0

/* Use configured frequency or default to 40MHz */

#ifndef CONFIG_LCD_ST7796_FREQUENCY
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_FREQUENCY
#    define CONFIG_LCD_ST7796_FREQUENCY CONFIG_NUCLEO_H753ZI_ST7796_FREQUENCY
#  else
#    define CONFIG_LCD_ST7796_FREQUENCY 40000000
#  endif
#endif

/* Display dimensions */

#define ST7796_XRES_RAW    320
#define ST7796_YRES_RAW    480

/* Determine orientation based on configuration */

#if defined(CONFIG_NUCLEO_H753ZI_ST7796_LANDSCAPE) || \
    defined(CONFIG_NUCLEO_H753ZI_ST7796_RLANDSCAPE) || \
    defined(CONFIG_LCD_LANDSCAPE) || defined(CONFIG_LCD_RLANDSCAPE)
#  define ST7796_XRES       ST7796_YRES_RAW
#  define ST7796_YRES       ST7796_XRES_RAW
#else
#  define ST7796_XRES       ST7796_XRES_RAW
#  define ST7796_YRES       ST7796_YRES_RAW
#endif

/* Color format configuration */

#ifdef CONFIG_LCD_ST7796_BPP
#  if (CONFIG_LCD_ST7796_BPP == 16)
#    define ST7796_BPP           16
#    define ST7796_COLORFMT      FB_FMT_RGB16_565
#    define ST7796_BYTESPP       2
#  elif (CONFIG_LCD_ST7796_BPP == 18)
#    define ST7796_BPP           18
#    define ST7796_COLORFMT      FB_FMT_RGB24
#    define ST7796_BYTESPP       3
#  else
#    define ST7796_BPP           16
#    define ST7796_COLORFMT      FB_FMT_RGB16_565
#    define ST7796_BYTESPP       2
#  endif
#else
#  define ST7796_BPP           16
#  define ST7796_COLORFMT      FB_FMT_RGB16_565
#  define ST7796_BYTESPP       2
#endif

#define ST7796_FBSIZE  (ST7796_XRES * ST7796_YRES * ST7796_BYTESPP)

/* DMA threshold - if transfer is larger, consider using DMA */

#ifndef CONFIG_LCD_ST7796_DMA_THRESHOLD
#  define CONFIG_LCD_ST7796_DMA_THRESHOLD 64
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct st7796_dev_s
{
  struct fb_vtable_s vtable;
  FAR struct spi_dev_s *spi;
  FAR uint8_t *fbmem;
  bool power;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int st7796_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                FAR struct fb_videoinfo_s *vinfo);
static int st7796_getplaneinfo(FAR struct fb_vtable_s *vtable, int planeno,
                                FAR struct fb_planeinfo_s *pinfo);
static int st7796_updatearea(FAR struct fb_vtable_s *vtable,
                              FAR const struct fb_area_s *area);
static void st7796_select(FAR struct spi_dev_s *spi);
static void st7796_deselect(FAR struct spi_dev_s *spi);
static void st7796_sendcmd(FAR struct st7796_dev_s *dev, uint8_t cmd);
static void st7796_senddata(FAR struct st7796_dev_s *dev,
                            FAR const uint8_t *data, size_t len);
static void st7796_send_sequence(FAR struct st7796_dev_s *dev,
                                  FAR const struct st7796_cmd_s *seq,
                                  size_t count);
static void st7796_setarea(FAR struct st7796_dev_s *dev,
                           uint16_t x0, uint16_t y0,
                           uint16_t x1, uint16_t y1);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct st7796_dev_s g_st7796dev;

/* Determine MADCTL value based on orientation and color order */

#if defined(CONFIG_NUCLEO_H753ZI_ST7796_LANDSCAPE) || \
    defined(CONFIG_LCD_LANDSCAPE)
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_BGR
#    define ST7796_MADCTL_VALUE 0x28  /* Landscape: MV=1, BGR=1 */
#  else
#    define ST7796_MADCTL_VALUE 0x20  /* Landscape: MV=1, RGB=1 */
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_RPORTRAIT) || \
      defined(CONFIG_LCD_RPORTRAIT)
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_BGR
#    define ST7796_MADCTL_VALUE 0x88  /* Reverse Portrait: MY=1, BGR=1 */
#  else
#    define ST7796_MADCTL_VALUE 0x80  /* Reverse Portrait: MY=1, RGB=1 */
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_RLANDSCAPE) || \
      defined(CONFIG_LCD_RLANDSCAPE)
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_BGR
#    define ST7796_MADCTL_VALUE 0xE8  /* Reverse Landscape: MY=1, MX=1, MV=1, BGR=1 */
#  else
#    define ST7796_MADCTL_VALUE 0xE0  /* Reverse Landscape: MY=1, MX=1, MV=1, RGB=1 */
#  endif
#else
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_BGR
#    define ST7796_MADCTL_VALUE 0x48  /* Portrait: MX=1, BGR=1 */
#  else
#    define ST7796_MADCTL_VALUE 0x40  /* Portrait: MX=1, RGB=1 */
#  endif
#endif

/* Initialization sequence optimized for stability */

static const struct st7796_cmd_s st7796_init_sequence[] =
{
  /* Software Reset */
  {ST7796_SWRESET, NULL, 0, 150},

  /* Sleep Out */
  {ST7796_SLPOUT, NULL, 0, 150},

  /* Command Set Control - Enable extended commands */
  {ST7796_CSCON, (const uint8_t[]){0xC3}, 1, 0},
  {ST7796_CSCON, (const uint8_t[]){0x96}, 1, 0},

  /* Memory Access Control - Set orientation and color order */
  {ST7796_MADCTL, (const uint8_t[]){ST7796_MADCTL_VALUE}, 1, 0},

  /* Pixel Format - 16-bit RGB565 */
  {ST7796_PIXFMT, (const uint8_t[]){0x55}, 1, 0},

  /* Display Inversion Control */
  {ST7796_INVCTR, (const uint8_t[]){0x01}, 1, 0},

  /* Display Function Control */
  {ST7796_DFC, (const uint8_t[]){0x80, 0x02, 0x3B}, 3, 0},

  /* Display Output Ctrl - DPI, RGB */
  {0xE8, (const uint8_t[]){0x40, 0x8A, 0x00, 0x00, 
                           0x29, 0x19, 0xA5, 0x33}, 8, 0},

  /* Power Control 2 */
  {ST7796_PWCTRL2, (const uint8_t[]){0x06}, 1, 0},

  /* Power Control 3 */
  {ST7796_PWCTRL3, (const uint8_t[]){0xA7}, 1, 0},

  /* VCOM Control */
  {ST7796_VCOM, (const uint8_t[]){0x18}, 1, 0},

  /* Positive Gamma Correction */
  {ST7796_GAMMAPOS, (const uint8_t[]){0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15,
                                      0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14,
                                      0x18, 0x1B}, 14, 0},

  /* Negative Gamma Correction */
  {ST7796_GAMMANEG, (const uint8_t[]){0xF0, 0x09, 0x0B, 0x06, 0x04, 0x03,
                                      0x2D, 0x43, 0x42, 0x3B, 0x16, 0x14,
                                      0x17, 0x1B}, 14, 0},

  /* Command Set Control - Lock extended commands */
  {ST7796_CSCON, (const uint8_t[]){0x3C}, 1, 0},
  {ST7796_CSCON, (const uint8_t[]){0x69}, 1, 0},

  /* Inversion On (recommended for better color) */
  {ST7796_INVON, NULL, 0, 10},

  /* Normal Display Mode On */
  {ST7796_NORON, NULL, 0, 10},

  /* Display On */
  {ST7796_DISPON, NULL, 0, 120},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: st7796_select
 *
 * Description:
 *   Select SPI device and configure for ST7796.
 *
 ****************************************************************************/

static void st7796_select(FAR struct spi_dev_s *spi)
{
  SPI_LOCK(spi, true);
  SPI_SETMODE(spi, CONFIG_LCD_ST7796_SPIMODE);
  SPI_SETBITS(spi, 8);
  SPI_SETFREQUENCY(spi, CONFIG_LCD_ST7796_FREQUENCY);
  SPI_SELECT(spi, SPIDEV_DISPLAY(0), true);
}

/****************************************************************************
 * Name: st7796_deselect
 *
 * Description:
 *   Deselect SPI device.
 *
 ****************************************************************************/

static void st7796_deselect(FAR struct spi_dev_s *spi)
{
  SPI_SELECT(spi, SPIDEV_DISPLAY(0), false);
  SPI_LOCK(spi, false);
}

/****************************************************************************
 * Name: st7796_sendcmd
 *
 * Description:
 *   Send command byte to ST7796.
 *
 ****************************************************************************/

static void st7796_sendcmd(FAR struct st7796_dev_s *dev, uint8_t cmd)
{
#ifdef CONFIG_SPI_CMDDATA
  /* Use hardware CMD/DATA support */

  SPI_CMDDATA(dev->spi, SPIDEV_DISPLAY(0), true);
  SPI_SEND(dev->spi, cmd);
#else
  /* This path should not be used with current configuration */

  #error "CONFIG_SPI_CMDDATA must be enabled for ST7796"
#endif
}

/****************************************************************************
 * Name: st7796_senddata
 *
 * Description:
 *   Send data bytes to ST7796.
 *
 ****************************************************************************/

static void st7796_senddata(FAR struct st7796_dev_s *dev,
                            FAR const uint8_t *data, size_t len)
{
  if (len > 0 && data != NULL)
    {
#ifdef CONFIG_SPI_CMDDATA
      /* Use hardware CMD/DATA support - DC pin controlled automatically */

      SPI_CMDDATA(dev->spi, SPIDEV_DISPLAY(0), false);
      SPI_SNDBLOCK(dev->spi, data, len);
#else
      #error "CONFIG_SPI_CMDDATA must be enabled for ST7796"
#endif
    }
}

/****************************************************************************
 * Name: st7796_send_sequence
 *
 * Description:
 *   Send initialization sequence to ST7796.
 *
 ****************************************************************************/

static void st7796_send_sequence(FAR struct st7796_dev_s *dev,
                                  FAR const struct st7796_cmd_s *seq,
                                  size_t count)
{
  size_t i;

  lcdinfo("ST7796: Sending initialization sequence (%zu commands)\n", count);

  for (i = 0; i < count; i++)
    {
      st7796_sendcmd(dev, seq[i].cmd);
      
      if (seq[i].data != NULL && seq[i].len > 0)
        {
          st7796_senddata(dev, seq[i].data, seq[i].len);
        }
      
      if (seq[i].delay_ms > 0)
        {
          nxsig_usleep(seq[i].delay_ms * 1000);
        }
    }

  lcdinfo("ST7796: Initialization sequence complete\n");
}

/****************************************************************************
 * Name: st7796_setarea
 *
 * Description:
 *   Set drawing area window.
 *
 ****************************************************************************/

static void st7796_setarea(FAR struct st7796_dev_s *dev,
                           uint16_t x0, uint16_t y0,
                           uint16_t x1, uint16_t y1)
{
  uint8_t data[4];

  /* Column Address Set */

  st7796_sendcmd(dev, ST7796_CASET);
  data[0] = (x0 >> 8) & 0xFF;
  data[1] = x0 & 0xFF;
  data[2] = (x1 >> 8) & 0xFF;
  data[3] = x1 & 0xFF;
  st7796_senddata(dev, data, 4);

  /* Row Address Set */

  st7796_sendcmd(dev, ST7796_RASET);
  data[0] = (y0 >> 8) & 0xFF;
  data[1] = y0 & 0xFF;
  data[2] = (y1 >> 8) & 0xFF;
  data[3] = y1 & 0xFF;
  st7796_senddata(dev, data, 4);
}

/****************************************************************************
 * Name: st7796_getvideoinfo
 *
 * Description:
 *   Get video information.
 *
 ****************************************************************************/

static int st7796_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                FAR struct fb_videoinfo_s *vinfo)
{
  DEBUGASSERT(vtable && vinfo);

  lcdinfo("ST7796: getvideoinfo\n");

  vinfo->fmt     = ST7796_COLORFMT;
  vinfo->xres    = ST7796_XRES;
  vinfo->yres    = ST7796_YRES;
  vinfo->nplanes = 1;

  return OK;
}

/****************************************************************************
 * Name: st7796_getplaneinfo
 *
 * Description:
 *   Get plane information.
 *
 ****************************************************************************/

static int st7796_getplaneinfo(FAR struct fb_vtable_s *vtable, int planeno,
                                FAR struct fb_planeinfo_s *pinfo)
{
  FAR struct st7796_dev_s *priv = (FAR struct st7796_dev_s *)vtable;

  DEBUGASSERT(vtable && pinfo && planeno == 0);

  lcdinfo("ST7796: getplaneinfo - plane %d\n", planeno);

  pinfo->fbmem   = priv->fbmem;
  pinfo->fblen   = ST7796_FBSIZE;
  pinfo->stride  = ST7796_XRES * ST7796_BYTESPP;
  pinfo->bpp     = ST7796_BPP;
  pinfo->xres_virtual = ST7796_XRES;
  pinfo->yres_virtual = ST7796_YRES;
  pinfo->xoffset = 0;
  pinfo->yoffset = 0;

  return OK;
}

/****************************************************************************
 * Name: st7796_updatearea
 *
 * Description:
 *   Update display area from framebuffer.
 *
 ****************************************************************************/

static int st7796_updatearea(FAR struct fb_vtable_s *vtable,
                              FAR const struct fb_area_s *area)
{
  FAR struct st7796_dev_s *priv = (FAR struct st7796_dev_s *)vtable;
  FAR uint16_t *src_fbptr; 
  size_t row_size_bytes;
  size_t row_size_pixels;
  int row;
  FAR uint16_t *swap_buffer;
  int i;

  DEBUGASSERT(priv && area);

  lcdinfo("ST7796: updatearea - x=%d y=%d w=%d h=%d\n",
          area->x, area->y, area->w, area->h);

  st7796_select(priv->spi);

  /* Set drawing window */

  st7796_setarea(priv, area->x, area->y,
                 area->x + area->w - 1,
                 area->y + area->h - 1);

  /* Memory Write command */

  st7796_sendcmd(priv, ST7796_RAMWR);

  /* Switch to data mode */

#ifdef CONFIG_SPI_CMDDATA
  SPI_CMDDATA(priv->spi, SPIDEV_DISPLAY(0), false);
#endif

  row_size_pixels = area->w;
  row_size_bytes = row_size_pixels * ST7796_BYTESPP;
  
  /* Calculate framebuffer start position */

  src_fbptr = (FAR uint16_t *)
              (priv->fbmem + (area->y * ST7796_XRES + area->x) *
               ST7796_BYTESPP);

  /* Allocate buffer for byte-swapped data */

  swap_buffer = (FAR uint16_t *)kmm_malloc(row_size_bytes);
  if (!swap_buffer)
    {
      lcderr("ERROR: Failed to allocate swap buffer\n");
      st7796_deselect(priv->spi);
      return -ENOMEM;
    }

  /* Transfer framebuffer data row by row with byte swapping */

  for (row = 0; row < area->h; row++)
    {
      /* Byte swap: ST7796 expects MSB first in RGB565 */

      for (i = 0; i < row_size_pixels; i++)
        {
          uint16_t pixel = src_fbptr[i];
          swap_buffer[i] = (pixel << 8) | (pixel >> 8); 
        }
      
      /* Send row data */

      SPI_SNDBLOCK(priv->spi, (FAR const uint8_t *)swap_buffer,
                   row_size_bytes);
      
      /* Move to next row in framebuffer */

      src_fbptr += ST7796_XRES;
    }
    
  kmm_free(swap_buffer);
  st7796_deselect(priv->spi);

  lcdinfo("ST7796: updatearea complete\n");

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: st7796_fbinitialize
 *
 * Description:
 *   Initialize ST7796 LCD as framebuffer device.
 *
 * Input Parameters:
 *   spi - SPI device instance
 *
 * Returned Value:
 *   Pointer to framebuffer vtable on success; NULL on failure.
 *
 ****************************************************************************/

FAR struct fb_vtable_s *st7796_fbinitialize(FAR struct spi_dev_s *spi)
{
  FAR struct st7796_dev_s *priv = &g_st7796dev;

  lcdinfo("ST7796: Initializing framebuffer driver\n");
  lcdinfo("ST7796: Resolution: %dx%d @ %d bpp\n",
          ST7796_XRES, ST7796_YRES, ST7796_BPP);
  lcdinfo("ST7796: Framebuffer size: %d bytes (%d KB)\n",
          ST7796_FBSIZE, ST7796_FBSIZE / 1024);
  lcdinfo("ST7796: SPI Frequency: %d Hz\n", CONFIG_LCD_ST7796_FREQUENCY);

  /* Allocate framebuffer memory */

  priv->fbmem = (FAR uint8_t *)kmm_zalloc(ST7796_FBSIZE);
  if (!priv->fbmem)
    {
      lcderr("ERROR: Failed to allocate framebuffer (%d bytes)\n",
             ST7796_FBSIZE);
      return NULL;
    }

  lcdinfo("ST7796: Framebuffer allocated at %p\n", priv->fbmem);

  /* Initialize driver structure */

  priv->vtable.getvideoinfo  = st7796_getvideoinfo;
  priv->vtable.getplaneinfo  = st7796_getplaneinfo;
  priv->vtable.updatearea    = st7796_updatearea;
  priv->spi                  = spi;
  priv->power                = false;

  /* Send initialization sequence */

  st7796_select(priv->spi);
  st7796_send_sequence(priv, st7796_init_sequence,
                       sizeof(st7796_init_sequence) /
                       sizeof(struct st7796_cmd_s));
  st7796_deselect(priv->spi);

  priv->power = true;
  lcdinfo("ST7796: Display ready\n");

  return &priv->vtable;
}

#endif /* CONFIG_LCD_ST7796 */
