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

/* SPI Frequency Resolution: Board configuration takes precedence */

#ifdef CONFIG_LCD_ST7796_FREQUENCY
#  undef CONFIG_LCD_ST7796_FREQUENCY
#endif

#ifdef CONFIG_NUCLEO_H753ZI_ST7796_FREQUENCY
#  define CONFIG_LCD_ST7796_FREQUENCY CONFIG_NUCLEO_H753ZI_ST7796_FREQUENCY
#else
#  define CONFIG_LCD_ST7796_FREQUENCY 40000000
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

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct st7796_dev_s
{
  struct fb_vtable_s vtable;
  FAR struct spi_dev_s *spi;
  FAR uint8_t *fbmem;
  FAR uint16_t *swap_buf; /* Persistent buffer for byte-swapping */
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

#if defined(CONFIG_NUCLEO_H753ZI_ST7796_LANDSCAPE) || \
    defined(CONFIG_LCD_LANDSCAPE)
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_BGR
#    define ST7796_MADCTL_VALUE 0x28
#  else
#    define ST7796_MADCTL_VALUE 0x20
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_RPORTRAIT) || \
      defined(CONFIG_LCD_RPORTRAIT)
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_BGR
#    define ST7796_MADCTL_VALUE 0x88
#  else
#    define ST7796_MADCTL_VALUE 0x80
#  endif
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_RLANDSCAPE) || \
      defined(CONFIG_LCD_RLANDSCAPE)
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_BGR
#    define ST7796_MADCTL_VALUE 0xe8
#  else
#    define ST7796_MADCTL_VALUE 0xE0
#  endif
#else
#  ifdef CONFIG_NUCLEO_H753ZI_ST7796_BGR
#    define ST7796_MADCTL_VALUE 0x48
#  else
#    define ST7796_MADCTL_VALUE 0x40
#  endif
#endif

/* Initialization sequence data arrays
 *
 * ST7796S Register Configuration Reference:
 *
 * CSCON (Command Set Control):
 *   0xc3, 0x96  - Enable extended command access (unlock)
 *   0x3c, 0x69  - Disable extended command access (lock)
 *
 * PIXFMT (Pixel Format):
 *   0x55 - RGB565 16bpp (0101 0101b: 16-bit RGB + 16-bit MCU)
 *
 * INVCTR (Display Inversion):
 *   0x01 - Column inversion mode
 *
 * DFC (Display Function Control):
 *   0x80 - Source output scan direction
 *   0x02 - Gate output scan direction
 *   0x3b - Number of lines (59 * 8 = 472 lines)
 *
 * CMD 0xE8 (Undocumented):
 *   Internal timing adjustment, from manufacturer reference
 *
 * PWCTRL2/3 (Power Control):
 *   0x06 - VGH/VGL voltage setting
 *   0xa7 - VCOM voltage adjustment
 *
 * VCOM:
 *   0x18 - VCOM voltage for contrast tuning
 *
 * GAMMAPOS/GAMMANEG:
 *   14-byte gamma correction curves for color linearity
 *
 * MADCTL values defined by orientation config (see ST7796_MADCTL_VALUE)
 */

static const uint8_t g_cscon1_data[] =
{
  0xc3
};

static const uint8_t g_cscon2_data[] =
{
  0x96
};

static const uint8_t g_madctl_data[] =
{
  ST7796_MADCTL_VALUE
};

static const uint8_t g_pixfmt_data[] =
{
  0x55
};

static const uint8_t g_invctr_data[] =
{
  0x01
};

static const uint8_t g_dfc_data[] =
{
  0x80, 0x02, 0x3b
};

static const uint8_t g_cmd_e8_data[] =
{
  0x40, 0x8a, 0x00, 0x00, 0x29, 0x19, 0xa5, 0x33
};

static const uint8_t g_pwctrl2_data[] =
{
  0x06
};

static const uint8_t g_pwctrl3_data[] =
{
  0xa7
};

static const uint8_t g_vcom_data[] =
{
  0x18
};

static const uint8_t g_gammapos_data[] =
{
  0xf0, 0x09, 0x0b, 0x06, 0x04, 0x15,
  0x2f, 0x54, 0x42, 0x3c, 0x17, 0x14,
  0x18, 0x1b
};

static const uint8_t g_gammaneg_data[] =
{
  0xf0, 0x09, 0x0b, 0x06, 0x04, 0x03,
  0x2d, 0x43, 0x42, 0x3b, 0x16, 0x14,
  0x17, 0x1b
};

static const uint8_t g_cscon3_data[] =
{
  0x3c
};

static const uint8_t g_cscon4_data[] =
{
  0x69
};

/* Initialization sequence
 *
 * Timing requirements from ST7796S datasheet section 9.16:
 *
 *   SWRESET: 120ms minimum recovery time (using 150ms for margin)
 *   SLPOUT:  120ms minimum for DC-DC and oscillator stabilization
 *   INVON:   No minimum specified, 10ms for safety
 *   NORON:   No minimum specified, 10ms for safety
 *   DISPON:  Frame memory must be written first, 120ms for stabilization
 *
 * Sequence follows recommended power-on flow:
 *   1. Software reset
 *   2. Exit sleep mode
 *   3. Unlock extended registers (CSCON)
 *   4. Configure display parameters
 *   5. Lock extended registers (CSCON)
 *   6. Enable inversion and normal mode
 *   7. Turn on display
 */

static const struct st7796_cmd_s st7796_init_sequence[] =
{
  {ST7796_SWRESET, NULL, 0, 150},
  {ST7796_SLPOUT, NULL, 0, 150},
  {ST7796_CSCON, g_cscon1_data, 1, 0},
  {ST7796_CSCON, g_cscon2_data, 1, 0},
  {ST7796_MADCTL, g_madctl_data, 1, 0},
  {ST7796_PIXFMT, g_pixfmt_data, 1, 0},
  {ST7796_INVCTR, g_invctr_data, 1, 0},
  {ST7796_DFC, g_dfc_data, 3, 0},
  {0xe8, g_cmd_e8_data, 8, 0},
  {ST7796_PWCTRL2, g_pwctrl2_data, 1, 0},
  {ST7796_PWCTRL3, g_pwctrl3_data, 1, 0},
  {ST7796_VCOM, g_vcom_data, 1, 0},
  {ST7796_GAMMAPOS, g_gammapos_data, 14, 0},
  {ST7796_GAMMANEG, g_gammaneg_data, 14, 0},
  {ST7796_CSCON, g_cscon3_data, 1, 0},
  {ST7796_CSCON, g_cscon4_data, 1, 0},
  {ST7796_INVON, NULL, 0, 10},
  {ST7796_NORON, NULL, 0, 10},
  {ST7796_DISPON, NULL, 0, 120},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: st7796_select
 *
 * Description:
 *   Select the SPI device, locking the bus and configuring SPI parameters.
 *   This function locks the SPI bus, sets the SPI mode, bit width, and
 *   frequency, then asserts the chip select signal.
 *
 * Input Parameters:
 *   spi - Reference to the SPI driver structure
 *
 * Returned Value:
 *   None
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
 *   Deselect the SPI device, releasing the chip select and unlocking the
 *   SPI bus for other devices.
 *
 * Input Parameters:
 *   spi - Reference to the SPI driver structure
 *
 * Returned Value:
 *   None
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
 *   Send a command byte to the ST7796 display controller. The D/C (Data/
 *   Command) line is set to command mode before transmission.
 *
 * Input Parameters:
 *   dev - Reference to the ST7796 device structure
 *   cmd - Command byte to send
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void st7796_sendcmd(FAR struct st7796_dev_s *dev, uint8_t cmd)
{
#ifdef CONFIG_SPI_CMDDATA
  SPI_CMDDATA(dev->spi, SPIDEV_DISPLAY(0), true);
  SPI_SEND(dev->spi, cmd);
#else
  #error "CONFIG_SPI_CMDDATA must be enabled for ST7796"
#endif
}

/****************************************************************************
 * Name: st7796_senddata
 *
 * Description:
 *   Send data bytes to the ST7796 display controller. The D/C (Data/
 *   Command) line is set to data mode before transmission.
 *
 * Input Parameters:
 *   dev  - Reference to the ST7796 device structure
 *   data - Pointer to the data buffer to send
 *   len  - Number of bytes to send
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void st7796_senddata(FAR struct st7796_dev_s *dev,
                            FAR const uint8_t *data, size_t len)
{
  if (len > 0 && data != NULL)
    {
#ifdef CONFIG_SPI_CMDDATA
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
 *   Send a sequence of commands and data to the ST7796 display controller.
 *   Each entry in the sequence contains a command, optional data, and an
 *   optional delay. This function is typically used for display
 *   initialization.
 *
 * Input Parameters:
 *   dev   - Reference to the ST7796 device structure
 *   seq   - Pointer to an array of command/data sequence entries
 *   count - Number of entries in the sequence array
 *
 * Returned Value:
 *   None
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
 *   Set the active drawing area on the ST7796 display. This defines the
 *   rectangular region where subsequent pixel data will be written. The
 *   function sends CASET (Column Address Set) and RASET (Row Address Set)
 *   commands to define the window boundaries.
 *
 * Input Parameters:
 *   dev - Reference to the ST7796 device structure
 *   x0  - Start column (left edge)
 *   y0  - Start row (top edge)
 *   x1  - End column (right edge, inclusive)
 *   y1  - End row (bottom edge, inclusive)
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void st7796_setarea(FAR struct st7796_dev_s *dev,
                           uint16_t x0, uint16_t y0,
                           uint16_t x1, uint16_t y1)
{
  uint8_t data[4];
  st7796_sendcmd(dev, ST7796_CASET);
  data[0] = (x0 >> 8) & 0xff;
  data[1] = x0 & 0xff;
  data[2] = (x1 >> 8) & 0xff;
  data[3] = x1 & 0xff;
  st7796_senddata(dev, data, 4);

  st7796_sendcmd(dev, ST7796_RASET);
  data[0] = (y0 >> 8) & 0xff;
  data[1] = y0 & 0xff;
  data[2] = (y1 >> 8) & 0xff;
  data[3] = y1 & 0xff;
  st7796_senddata(dev, data, 4);
}

/****************************************************************************
 * Name: st7796_getvideoinfo
 *
 * Description:
 *   Get information about the video controller and the configuration of
 *   the video plane. This is part of the framebuffer interface required
 *   by NuttX.
 *
 * Input Parameters:
 *   vtable - Reference to the framebuffer virtual table
 *   vinfo  - Pointer to the video info structure to be filled
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int st7796_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                FAR struct fb_videoinfo_s *vinfo)
{
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
 *   Get information about the framebuffer plane. Returns details about
 *   the framebuffer memory, stride, bits per pixel, and virtual
 *   resolution. This is part of the framebuffer interface required by
 *   NuttX.
 *
 * Input Parameters:
 *   vtable  - Reference to the framebuffer virtual table
 *   planeno - Plane number (must be 0 for this single-plane display)
 *   pinfo   - Pointer to the plane info structure to be filled
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int st7796_getplaneinfo(FAR struct fb_vtable_s *vtable, int planeno,
                                FAR struct fb_planeinfo_s *pinfo)
{
  FAR struct st7796_dev_s *priv = (FAR struct st7796_dev_s *)vtable;
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
 *   Update a rectangular area of the display from the framebuffer. This
 *   function transfers pixel data from the in-memory framebuffer to the
 *   ST7796 display controller via SPI. The pixel data is byte-swapped
 *   from little-endian (CPU native) to big-endian (display native) format
 *   during transfer.
 *
 * Input Parameters:
 *   vtable - Reference to the framebuffer virtual table
 *   area   - Pointer to structure describing the rectangular area to
 *            update (x, y, width, height)
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int st7796_updatearea(FAR struct fb_vtable_s *vtable,
                              FAR const struct fb_area_s *area)
{
  FAR struct st7796_dev_s *priv = (FAR struct st7796_dev_s *)vtable;
  FAR uint16_t *src_fbptr;
  size_t row_size_pixels;
  int row;
  int i;

  lcdinfo("ST7796: updatearea - x=%d y=%d w=%d h=%d\n",
          area->x, area->y, area->w, area->h);

  st7796_select(priv->spi);
  st7796_setarea(priv, area->x, area->y,
                 area->x + area->w - 1,
                 area->y + area->h - 1);

  st7796_sendcmd(priv, ST7796_RAMWR);

#ifdef CONFIG_SPI_CMDDATA
  SPI_CMDDATA(priv->spi, SPIDEV_DISPLAY(0), false);
#endif

  row_size_pixels = area->w;
  src_fbptr = (FAR uint16_t *)
              (priv->fbmem +
               (area->y * ST7796_XRES + area->x) * ST7796_BYTESPP);

  for (row = 0; row < area->h; row++)
    {
      for (i = 0; i < row_size_pixels; i++)
        {
          uint16_t pixel = src_fbptr[i];
          priv->swap_buf[i] = (pixel << 8) | (pixel >> 8);
        }

      SPI_SNDBLOCK(priv->spi, (FAR const uint8_t *)priv->swap_buf,
                   row_size_pixels * ST7796_BYTESPP);

      src_fbptr += ST7796_XRES;
    }

  st7796_deselect(priv->spi);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: st7796_fbinitialize
 *
 * Description:
 *   Initialize the ST7796 LCD controller and framebuffer driver. This
 *   function allocates the framebuffer memory, initializes the driver
 *   structure, sends the display initialization sequence, and returns
 *   a pointer to the framebuffer virtual table for use with the NuttX
 *   framebuffer interface.
 *
 * Input Parameters:
 *   spi - Reference to the SPI driver structure to use for communication
 *
 * Returned Value:
 *   On success, a pointer to the framebuffer virtual table is returned.
 *   On failure, NULL is returned.
 *
 ****************************************************************************/

FAR struct fb_vtable_s *st7796_fbinitialize(FAR struct spi_dev_s *spi)
{
  FAR struct st7796_dev_s *priv = &g_st7796dev;

  lcdinfo("ST7796: Initializing framebuffer driver\n");
  lcdinfo("ST7796: Resolution: %dx%d @ %d bpp\n",
          ST7796_XRES, ST7796_YRES, ST7796_BPP);
  lcdinfo("ST7796: SPI Frequency: %d Hz\n", CONFIG_LCD_ST7796_FREQUENCY);

  /* Allocate framebuffer memory */

  priv->fbmem = (FAR uint8_t *)kmm_zalloc(ST7796_FBSIZE);
  if (!priv->fbmem)
    {
      lcderr("ERROR: Failed to allocate framebuffer (%d bytes)\n",
             ST7796_FBSIZE);
      return NULL;
    }

  /* Allocate persistent row swap buffer to avoid malloc in updatearea */

  priv->swap_buf = (FAR uint16_t *)kmm_malloc(ST7796_XRES * ST7796_BYTESPP);
  if (!priv->swap_buf)
    {
      lcderr("ERROR: Failed to allocate swap buffer\n");
      kmm_free(priv->fbmem);
      return NULL;
    }

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
