/****************************************************************************
 * apps/examples/lcd_battery/lcd_battery_main.c
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
 *
 * Battery animation demo for SSD1306 OLED display.
 * Supports both 128x32 and 128x64 displays (auto-detected).
 *
 * Usage:
 *   lcd_battery [options]
 *
 * Options:
 *   --flip-x      Mirror display horizontally
 *   --flip-y      Mirror display vertically
 *   --rotate 180  Rotate display 180 degrees (same as --flip-x --flip-y)
 *   --help        Show this help message
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <nuttx/lcd/lcd_dev.h>
#include <nuttx/video/fb.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maximum supported display size (for static font data) */

#define LCD_MAX_WIDTH   128
#define LCD_MAX_HEIGHT  64

/* Battery icon position and dimensions (relative to display) */

#define BATTERY_X       4
#define BATTERY_Y       8
#define BATTERY_W       40
#define BATTERY_H       16
#define BATTERY_TIP_W   3
#define BATTERY_TIP_H   8
#define BATTERY_BORDER  2

/* Text positions */

#define PERCENT_X       52
#define PERCENT_Y       12
#define TITLE_X         4
#define TITLE_Y         0

/* Animation speed: 500 milisecond per percentage point */

#define ANIMATION_DELAY_US  1000000/2

/* Font table boundaries */

#define FONT5X7_FIRST_CHAR  0x20
#define FONT5X7_LAST_CHAR   0x7e
#define FONT5X7_CHAR_COUNT  (FONT5X7_LAST_CHAR - FONT5X7_FIRST_CHAR + 1)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Display dimensions - detected at runtime */

static uint16_t g_lcd_width = 0;
static uint16_t g_lcd_height = 0;
static uint16_t g_row_bytes = 0;

/* Framebuffer - dynamically allocated */

static uint8_t *g_framebuffer = NULL;

/* Orientation flags - default: both flips enabled (current working config) */

static bool g_flip_x = true;
static bool g_flip_y = true;

/****************************************************************************
 * 5x7 Bitmap Font - Full ASCII Printable Characters (0x20 - 0x7E)
 *
 * Each character is 5 bytes wide, 7 bits tall.
 * Each byte represents one vertical column, LSB at top.
 * Index = character - 0x20 (space)
 *
 ****************************************************************************/

static const uint8_t g_font5x7[][5] =
{
  {0x00, 0x00, 0x00, 0x00, 0x00},  /* 0x20 Space */
  {0x00, 0x00, 0x5f, 0x00, 0x00},  /* 0x21 ! */
  {0x00, 0x07, 0x00, 0x07, 0x00},  /* 0x22 " */
  {0x14, 0x7f, 0x14, 0x7f, 0x14},  /* 0x23 # */
  {0x24, 0x2a, 0x7f, 0x2a, 0x12},  /* 0x24 $ */
  {0x23, 0x13, 0x08, 0x64, 0x62},  /* 0x25 % */
  {0x36, 0x49, 0x55, 0x22, 0x50},  /* 0x26 & */
  {0x00, 0x05, 0x03, 0x00, 0x00},  /* 0x27 ' */
  {0x00, 0x1c, 0x22, 0x41, 0x00},  /* 0x28 ( */
  {0x00, 0x41, 0x22, 0x1c, 0x00},  /* 0x29 ) */
  {0x14, 0x08, 0x3e, 0x08, 0x14},  /* 0x2A * */
  {0x08, 0x08, 0x3e, 0x08, 0x08},  /* 0x2B + */
  {0x00, 0x50, 0x30, 0x00, 0x00},  /* 0x2C , */
  {0x08, 0x08, 0x08, 0x08, 0x08},  /* 0x2D - */
  {0x00, 0x60, 0x60, 0x00, 0x00},  /* 0x2E . */
  {0x20, 0x10, 0x08, 0x04, 0x02},  /* 0x2F / */
  {0x3e, 0x51, 0x49, 0x45, 0x3e},  /* 0x30 0 */
  {0x00, 0x42, 0x7f, 0x40, 0x00},  /* 0x31 1 */
  {0x42, 0x61, 0x51, 0x49, 0x46},  /* 0x32 2 */
  {0x21, 0x41, 0x45, 0x4b, 0x31},  /* 0x33 3 */
  {0x18, 0x14, 0x12, 0x7f, 0x10},  /* 0x34 4 */
  {0x27, 0x45, 0x45, 0x45, 0x39},  /* 0x35 5 */
  {0x3c, 0x4a, 0x49, 0x49, 0x30},  /* 0x36 6 */
  {0x01, 0x71, 0x09, 0x05, 0x03},  /* 0x37 7 */
  {0x36, 0x49, 0x49, 0x49, 0x36},  /* 0x38 8 */
  {0x06, 0x49, 0x49, 0x29, 0x1e},  /* 0x39 9 */
  {0x00, 0x36, 0x36, 0x00, 0x00},  /* 0x3A : */
  {0x00, 0x56, 0x36, 0x00, 0x00},  /* 0x3B ; */
  {0x08, 0x14, 0x22, 0x41, 0x00},  /* 0x3C < */
  {0x14, 0x14, 0x14, 0x14, 0x14},  /* 0x3D = */
  {0x00, 0x41, 0x22, 0x14, 0x08},  /* 0x3E > */
  {0x02, 0x01, 0x51, 0x09, 0x06},  /* 0x3F ? */
  {0x32, 0x49, 0x79, 0x41, 0x3e},  /* 0x40 @ */
  {0x7e, 0x11, 0x11, 0x11, 0x7e},  /* 0x41 A */
  {0x7f, 0x49, 0x49, 0x49, 0x36},  /* 0x42 B */
  {0x3e, 0x41, 0x41, 0x41, 0x22},  /* 0x43 C */
  {0x7f, 0x41, 0x41, 0x22, 0x1c},  /* 0x44 D */
  {0x7f, 0x49, 0x49, 0x49, 0x41},  /* 0x45 E */
  {0x7f, 0x09, 0x09, 0x09, 0x01},  /* 0x46 F */
  {0x3e, 0x41, 0x49, 0x49, 0x7a},  /* 0x47 G */
  {0x7f, 0x08, 0x08, 0x08, 0x7f},  /* 0x48 H */
  {0x00, 0x41, 0x7f, 0x41, 0x00},  /* 0x49 I */
  {0x20, 0x40, 0x41, 0x3f, 0x01},  /* 0x4A J */
  {0x7f, 0x08, 0x14, 0x22, 0x41},  /* 0x4B K */
  {0x7f, 0x40, 0x40, 0x40, 0x40},  /* 0x4C L */
  {0x7f, 0x02, 0x0c, 0x02, 0x7f},  /* 0x4D M */
  {0x7f, 0x04, 0x08, 0x10, 0x7f},  /* 0x4E N */
  {0x3e, 0x41, 0x41, 0x41, 0x3e},  /* 0x4F O */
  {0x7f, 0x09, 0x09, 0x09, 0x06},  /* 0x50 P */
  {0x3e, 0x41, 0x51, 0x21, 0x5e},  /* 0x51 Q */
  {0x7f, 0x09, 0x19, 0x29, 0x46},  /* 0x52 R */
  {0x46, 0x49, 0x49, 0x49, 0x31},  /* 0x53 S */
  {0x01, 0x01, 0x7f, 0x01, 0x01},  /* 0x54 T */
  {0x3f, 0x40, 0x40, 0x40, 0x3f},  /* 0x55 U */
  {0x1f, 0x20, 0x40, 0x20, 0x1f},  /* 0x56 V */
  {0x3f, 0x40, 0x38, 0x40, 0x3f},  /* 0x57 W */
  {0x63, 0x14, 0x08, 0x14, 0x63},  /* 0x58 X */
  {0x07, 0x08, 0x70, 0x08, 0x07},  /* 0x59 Y */
  {0x61, 0x51, 0x49, 0x45, 0x43},  /* 0x5A Z */
  {0x00, 0x7f, 0x41, 0x41, 0x00},  /* 0x5B [ */
  {0x02, 0x04, 0x08, 0x10, 0x20},  /* 0x5C \ */
  {0x00, 0x41, 0x41, 0x7f, 0x00},  /* 0x5D ] */
  {0x04, 0x02, 0x01, 0x02, 0x04},  /* 0x5E ^ */
  {0x40, 0x40, 0x40, 0x40, 0x40},  /* 0x5F _ */
  {0x00, 0x01, 0x02, 0x04, 0x00},  /* 0x60 ` */
  {0x20, 0x54, 0x54, 0x54, 0x78},  /* 0x61 a */
  {0x7f, 0x48, 0x44, 0x44, 0x38},  /* 0x62 b */
  {0x38, 0x44, 0x44, 0x44, 0x20},  /* 0x63 c */
  {0x38, 0x44, 0x44, 0x48, 0x7f},  /* 0x64 d */
  {0x38, 0x54, 0x54, 0x54, 0x18},  /* 0x65 e */
  {0x08, 0x7e, 0x09, 0x01, 0x02},  /* 0x66 f */
  {0x0c, 0x52, 0x52, 0x52, 0x3e},  /* 0x67 g */
  {0x7f, 0x08, 0x04, 0x04, 0x78},  /* 0x68 h */
  {0x00, 0x44, 0x7d, 0x40, 0x00},  /* 0x69 i */
  {0x20, 0x40, 0x44, 0x3d, 0x00},  /* 0x6A j */
  {0x7f, 0x10, 0x28, 0x44, 0x00},  /* 0x6B k */
  {0x00, 0x41, 0x7f, 0x40, 0x00},  /* 0x6C l */
  {0x7c, 0x04, 0x18, 0x04, 0x78},  /* 0x6D m */
  {0x7c, 0x08, 0x04, 0x04, 0x78},  /* 0x6E n */
  {0x38, 0x44, 0x44, 0x44, 0x38},  /* 0x6F o */
  {0x7c, 0x14, 0x14, 0x14, 0x08},  /* 0x70 p */
  {0x08, 0x14, 0x14, 0x18, 0x7c},  /* 0x71 q */
  {0x7c, 0x08, 0x04, 0x04, 0x08},  /* 0x72 r */
  {0x48, 0x54, 0x54, 0x54, 0x20},  /* 0x73 s */
  {0x04, 0x3f, 0x44, 0x40, 0x20},  /* 0x74 t */
  {0x3c, 0x40, 0x40, 0x20, 0x7c},  /* 0x75 u */
  {0x1c, 0x20, 0x40, 0x20, 0x1c},  /* 0x76 v */
  {0x3c, 0x40, 0x30, 0x40, 0x3c},  /* 0x77 w */
  {0x44, 0x28, 0x10, 0x28, 0x44},  /* 0x78 x */
  {0x0c, 0x50, 0x50, 0x50, 0x3c},  /* 0x79 y */
  {0x44, 0x64, 0x54, 0x4c, 0x44},  /* 0x7A z */
  {0x00, 0x08, 0x36, 0x41, 0x00},  /* 0x7B { */
  {0x00, 0x00, 0x7f, 0x00, 0x00},  /* 0x7C | */
  {0x00, 0x41, 0x36, 0x08, 0x00},  /* 0x7D } */
  {0x10, 0x08, 0x08, 0x10, 0x08},  /* 0x7E ~ */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 *
 * Description:
 *   Print command line usage information to syslog.
 *
 * Input Parameters:
 *   progname - Program name string from argv[0]
 *
 ****************************************************************************/

static void show_usage(const char *progname)
{
  syslog(LOG_INFO, "Usage: %s [options]\n", progname);
  syslog(LOG_INFO, "Options:\n");
  syslog(LOG_INFO, "  --flip-x       Mirror display horizontally\n");
  syslog(LOG_INFO, "  --flip-y       Mirror display vertically\n");
  syslog(LOG_INFO, "  --rotate 180   Rotate 180 degrees\n");
  syslog(LOG_INFO, "  --help         Show this help message\n");
  syslog(LOG_INFO, "Default: both flip-x and flip-y enabled\n");
}

/****************************************************************************
 * Name: parse_args
 *
 * Description:
 *   Parse command line arguments and update global orientation flags.
 *
 * Input Parameters:
 *   argc - Argument count
 *   argv - Argument vector
 *
 * Returned Value:
 *   0 on success, 1 if help was requested, -1 on error.
 *
 ****************************************************************************/

static int parse_args(int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "--flip-x") == 0)
        {
          g_flip_x = !g_flip_x;
        }
      else if (strcmp(argv[i], "--flip-y") == 0)
        {
          g_flip_y = !g_flip_y;
        }
      else if (strcmp(argv[i], "--rotate") == 0)
        {
          if (i + 1 < argc && strcmp(argv[i + 1], "180") == 0)
            {
              g_flip_x = !g_flip_x;
              g_flip_y = !g_flip_y;
              i++;
            }
          else
            {
              syslog(LOG_ERR,
                     "Error: --rotate requires '180' as argument\n");
              return -1;
            }
        }
      else if (strcmp(argv[i], "--help") == 0)
        {
          show_usage(argv[0]);
          return 1;
        }
      else
        {
          syslog(LOG_ERR, "Unknown option: %s\n", argv[i]);
          show_usage(argv[0]);
          return -1;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: lcd_get_info
 *
 * Description:
 *   Query display dimensions from driver using ioctl and allocate the
 *   framebuffer based on actual display size.
 *
 * Input Parameters:
 *   fd - File descriptor for the LCD device
 *
 * Returned Value:
 *   0 on success, negative errno on failure.
 *
 ****************************************************************************/

static int lcd_get_info(int fd)
{
  struct fb_videoinfo_s vinfo;
  size_t fb_size;
  int ret;

  /* Get display info from driver */

  ret = ioctl(fd, LCDDEVIO_GETVIDEOINFO, (unsigned long)&vinfo);
  if (ret < 0)
    {
      syslog(LOG_ERR, "LCDDEVIO_GETVIDEOINFO failed: %d\n", errno);
      return ret;
    }

  /* Store dimensions */

  g_lcd_width = vinfo.xres;
  g_lcd_height = vinfo.yres;
  g_row_bytes = g_lcd_width / 8;

  syslog(LOG_INFO, "Display detected: %dx%d pixels\n",
         g_lcd_width, g_lcd_height);

  /* Validate dimensions */

  if (g_lcd_width > LCD_MAX_WIDTH || g_lcd_height > LCD_MAX_HEIGHT)
    {
      syslog(LOG_ERR, "Display too large (%dx%d > %dx%d)\n",
             g_lcd_width, g_lcd_height, LCD_MAX_WIDTH, LCD_MAX_HEIGHT);
      return -EINVAL;
    }

  if (g_lcd_width == 0 || g_lcd_height == 0)
    {
      syslog(LOG_ERR, "Invalid display dimensions\n");
      return -EINVAL;
    }

  /* Allocate framebuffer: height rows * (width/8) bytes per row */

  fb_size = g_lcd_height * g_row_bytes;
  g_framebuffer = (uint8_t *)malloc(fb_size);
  if (g_framebuffer == NULL)
    {
      syslog(LOG_ERR, "Failed to allocate framebuffer (%zu bytes)\n",
             fb_size);
      return -ENOMEM;
    }

  syslog(LOG_INFO, "Framebuffer allocated: %zu bytes\n", fb_size);

  return 0;
}

/****************************************************************************
 * Name: fb_clear
 *
 * Description:
 *   Clear the entire framebuffer by setting all pixels to zero (off).
 *
 ****************************************************************************/

static void fb_clear(void)
{
  memset(g_framebuffer, 0, g_lcd_height * g_row_bytes);
}

/****************************************************************************
 * Name: fb_set_pixel
 *
 * Description:
 *   Set or clear a single pixel in the framebuffer. Applies flip
 *   transformations based on global orientation flags.
 *
 * Input Parameters:
 *   x     - Horizontal pixel coordinate (0 = left)
 *   y     - Vertical pixel coordinate (0 = top)
 *   color - Pixel state: 1 = on (white), 0 = off (black)
 *
 ****************************************************************************/

static void fb_set_pixel(int x, int y, int color)
{
  int byte_idx;
  int bit_idx;
  int row_offset;

  if (x < 0 || x >= g_lcd_width || y < 0 || y >= g_lcd_height)
    {
      return;
    }

  if (g_flip_x)
    {
      x = (g_lcd_width - 1) - x;
    }

  if (g_flip_y)
    {
      y = (g_lcd_height - 1) - y;
    }

  byte_idx = x / 8;
  bit_idx = 7 - (x % 8);
  row_offset = y * g_row_bytes;

  if (color)
    {
      g_framebuffer[row_offset + byte_idx] |= (1 << bit_idx);
    }
  else
    {
      g_framebuffer[row_offset + byte_idx] &= ~(1 << bit_idx);
    }
}

/****************************************************************************
 * Name: fb_draw_hline
 *
 * Description:
 *   Draw a horizontal line in the framebuffer.
 *
 * Input Parameters:
 *   x     - Starting X coordinate
 *   y     - Y coordinate
 *   w     - Line width in pixels
 *   color - Pixel state: 1 = on, 0 = off
 *
 ****************************************************************************/

static void fb_draw_hline(int x, int y, int w, int color)
{
  int i;

  for (i = 0; i < w; i++)
    {
      fb_set_pixel(x + i, y, color);
    }
}

/****************************************************************************
 * Name: fb_draw_vline
 *
 * Description:
 *   Draw a vertical line in the framebuffer.
 *
 * Input Parameters:
 *   x     - X coordinate
 *   y     - Starting Y coordinate
 *   h     - Line height in pixels
 *   color - Pixel state: 1 = on, 0 = off
 *
 ****************************************************************************/

static void fb_draw_vline(int x, int y, int h, int color)
{
  int i;

  for (i = 0; i < h; i++)
    {
      fb_set_pixel(x, y + i, color);
    }
}

/****************************************************************************
 * Name: fb_draw_rect
 *
 * Description:
 *   Draw a hollow rectangle (border only) in the framebuffer.
 *
 * Input Parameters:
 *   x     - Top-left X coordinate
 *   y     - Top-left Y coordinate
 *   w     - Rectangle width
 *   h     - Rectangle height
 *   color - Pixel state: 1 = on, 0 = off
 *
 ****************************************************************************/

static void fb_draw_rect(int x, int y, int w, int h, int color)
{
  fb_draw_hline(x, y, w, color);
  fb_draw_hline(x, y + h - 1, w, color);
  fb_draw_vline(x, y, h, color);
  fb_draw_vline(x + w - 1, y, h, color);
}

/****************************************************************************
 * Name: fb_fill_rect
 *
 * Description:
 *   Draw a filled rectangle in the framebuffer.
 *
 * Input Parameters:
 *   x     - Top-left X coordinate
 *   y     - Top-left Y coordinate
 *   w     - Rectangle width
 *   h     - Rectangle height
 *   color - Pixel state: 1 = on, 0 = off
 *
 ****************************************************************************/

static void fb_fill_rect(int x, int y, int w, int h, int color)
{
  int i;
  int j;

  for (j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
        {
          fb_set_pixel(x + i, y + j, color);
        }
    }
}

/****************************************************************************
 * Name: fb_draw_char
 *
 * Description:
 *   Draw a single ASCII character using the 5x7 bitmap font. Supports
 *   scaling via the size parameter. Characters outside the printable
 *   ASCII range (0x20-0x7E) are silently ignored.
 *
 * Input Parameters:
 *   x    - Top-left X coordinate for character
 *   y    - Top-left Y coordinate for character
 *   c    - ASCII character to draw
 *   size - Scale factor (1 = 5x7, 2 = 10x14, etc.)
 *
 ****************************************************************************/

static void fb_draw_char(int x, int y, char c, int size)
{
  int font_idx;
  int col;
  int row;
  int sx;
  int sy;
  uint8_t line;

  /* Check printable range */

  if (c < FONT5X7_FIRST_CHAR || c > FONT5X7_LAST_CHAR)
    {
      return;
    }

  font_idx = c - FONT5X7_FIRST_CHAR;

  for (col = 0; col < 5; col++)
    {
      line = g_font5x7[font_idx][col];
      for (row = 0; row < 7; row++)
        {
          if (line & (1 << row))
            {
              for (sy = 0; sy < size; sy++)
                {
                  for (sx = 0; sx < size; sx++)
                    {
                      fb_set_pixel(x + col * size + sx,
                                   y + row * size + sy, 1);
                    }
                }
            }
        }
    }
}

/****************************************************************************
 * Name: fb_draw_string
 *
 * Description:
 *   Draw a null-terminated string using the 5x7 bitmap font.
 *
 * Input Parameters:
 *   x    - Starting X coordinate
 *   y    - Starting Y coordinate
 *   str  - Null-terminated string to draw
 *   size - Scale factor for characters
 *
 ****************************************************************************/

static void fb_draw_string(int x, int y, const char *str, int size)
{
  int spacing = 6 * size;

  while (*str)
    {
      fb_draw_char(x, y, *str, size);
      x += spacing;
      str++;
    }
}

/****************************************************************************
 * Name: draw_battery
 *
 * Description:
 *   Draw a battery icon with fill level representing charge percentage.
 *   The battery consists of a rectangular body with a tip on the right
 *   side, and an inner fill area that grows from left to right.
 *
 * Input Parameters:
 *   percent - Battery charge level (0-100)
 *
 ****************************************************************************/

static void draw_battery(int percent)
{
  int fill_width;
  int inner_x;
  int inner_y;
  int inner_w;
  int inner_h;

  fb_draw_rect(BATTERY_X, BATTERY_Y, BATTERY_W, BATTERY_H, 1);
  fb_fill_rect(BATTERY_X + BATTERY_W,
               BATTERY_Y + (BATTERY_H - BATTERY_TIP_H) / 2,
               BATTERY_TIP_W, BATTERY_TIP_H, 1);

  inner_x = BATTERY_X + BATTERY_BORDER;
  inner_y = BATTERY_Y + BATTERY_BORDER;
  inner_w = BATTERY_W - (BATTERY_BORDER * 2);
  inner_h = BATTERY_H - (BATTERY_BORDER * 2);

  fill_width = (inner_w * percent) / 100;
  if (fill_width > 0)
    {
      fb_fill_rect(inner_x, inner_y, fill_width, inner_h, 1);
    }
}

/****************************************************************************
 * Name: draw_battery_screen
 *
 * Description:
 *   Draw complete battery status screen with title, battery icon,
 *   and percentage text.
 *
 * Input Parameters:
 *   percent - Battery charge level (0-100)
 *
 ****************************************************************************/

static void draw_battery_screen(int percent)
{
  char percent_str[8];

  fb_clear();
  fb_draw_string(TITLE_X, TITLE_Y, "BATTERY", 1);
  draw_battery(percent);

  snprintf(percent_str, sizeof(percent_str), "%d%%", percent);
  fb_draw_string(PERCENT_X, PERCENT_Y, percent_str, 2);
}

/****************************************************************************
 * Name: draw_nuttx_screen
 *
 * Description:
 *   Draw a special screen showing "NuttX" text inside the battery icon.
 *   Used as the final frame of the animation.
 *
 ****************************************************************************/

static void draw_nuttx_screen(void)
{
  fb_clear();
  fb_draw_string(TITLE_X, TITLE_Y, "BATTERY", 1);

  fb_draw_rect(BATTERY_X, BATTERY_Y, BATTERY_W, BATTERY_H, 1);
  fb_fill_rect(BATTERY_X + BATTERY_W,
               BATTERY_Y + (BATTERY_H - BATTERY_TIP_H) / 2,
               BATTERY_TIP_W, BATTERY_TIP_H, 1);

  fb_draw_string(BATTERY_X + 3, BATTERY_Y + 5, "NuttX", 1);
  fb_draw_string(PERCENT_X, PERCENT_Y, "0%", 2);
}

/****************************************************************************
 * Name: lcd_write_framebuffer
 *
 * Description:
 *   Transfer the entire framebuffer to the LCD display using PUTRUN ioctl.
 *   Each row is sent as a separate run operation.
 *
 * Input Parameters:
 *   fd - File descriptor for the LCD device
 *
 * Returned Value:
 *   0 on success, negative errno on failure.
 *
 ****************************************************************************/

static int lcd_write_framebuffer(int fd)
{
  struct lcddev_run_s run;
  int row;
  int ret;

  for (row = 0; row < g_lcd_height; row++)
    {
      run.row = row;
      run.col = 0;
      run.npixels = g_lcd_width;
      run.data = &g_framebuffer[row * g_row_bytes];

      ret = ioctl(fd, LCDDEVIO_PUTRUN, (unsigned long)&run);
      if (ret < 0)
        {
          syslog(LOG_ERR, "PUTRUN row %d: %d\n", row, errno);
          return ret;
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Application entry point. Initializes display, runs battery discharge
 *   animation from 100% to 0%, then shows NuttX branding screen.
 *
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int fd;
  int percent;
  int ret;

  /* Parse command line arguments */

  ret = parse_args(argc, argv);
  if (ret != 0)
    {
      return (ret < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
    }

  syslog(LOG_INFO, "LCD Battery Demo\n");
  syslog(LOG_INFO, "Orientation: flip_x=%s, flip_y=%s\n",
         g_flip_x ? "ON" : "OFF",
         g_flip_y ? "ON" : "OFF");

  /* Open LCD device */

  fd = open("/dev/lcd0", O_RDWR);
  if (fd < 0)
    {
      syslog(LOG_ERR, "Failed to open /dev/lcd0: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Get display info and allocate framebuffer */

  ret = lcd_get_info(fd);
  if (ret < 0)
    {
      close(fd);
      return EXIT_FAILURE;
    }

  syslog(LOG_INFO, "Starting animation (100%% -> 0%%)\n");

  fb_clear();
  fb_draw_string(25, 8, "Clarice", 2);
  lcd_write_framebuffer(fd);
  sleep(5);

  for (percent = 100; percent >= 0; percent--)
    {
      draw_battery_screen(percent);
      ret = lcd_write_framebuffer(fd);
      if (ret < 0)
        {
          free(g_framebuffer);
          close(fd);
          return EXIT_FAILURE;
        }

      usleep(ANIMATION_DELAY_US);
    }

  syslog(LOG_INFO, "Showing NuttX screen...\n");

  draw_nuttx_screen();
  lcd_write_framebuffer(fd);
  sleep(3);

  syslog(LOG_INFO, "Demo complete!\n");

  /* Cleanup */

  free(g_framebuffer);
  close(fd);

  return EXIT_SUCCESS;
}
