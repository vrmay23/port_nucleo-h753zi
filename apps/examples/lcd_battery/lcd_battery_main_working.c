/****************************************************************************
 * apps/examples/lcd_battery/lcd_battery_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <nuttx/lcd/lcd_dev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LCD_WIDTH       128
#define LCD_HEIGHT      32
#define ROW_BUFFER_SIZE (LCD_WIDTH / 8)

#define BATTERY_X       4
#define BATTERY_Y       8
#define BATTERY_W       40
#define BATTERY_H       16
#define BATTERY_TIP_W   3
#define BATTERY_TIP_H   8
#define BATTERY_BORDER  2

#define PERCENT_X       52
#define PERCENT_Y       12
#define TITLE_X         4
#define TITLE_Y         0

#define ANIMATION_DELAY_US  1000000

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_framebuffer[LCD_HEIGHT][ROW_BUFFER_SIZE];

static const uint8_t g_font5x7[][5] =
{
  {0x3e, 0x51, 0x49, 0x45, 0x3e},  /* 0 */
  {0x00, 0x42, 0x7f, 0x40, 0x00},  /* 1 */
  {0x42, 0x61, 0x51, 0x49, 0x46},  /* 2 */
  {0x21, 0x41, 0x45, 0x4b, 0x31},  /* 3 */
  {0x18, 0x14, 0x12, 0x7f, 0x10},  /* 4 */
  {0x27, 0x45, 0x45, 0x45, 0x39},  /* 5 */
  {0x3c, 0x4a, 0x49, 0x49, 0x30},  /* 6 */
  {0x01, 0x71, 0x09, 0x05, 0x03},  /* 7 */
  {0x36, 0x49, 0x49, 0x49, 0x36},  /* 8 */
  {0x06, 0x49, 0x49, 0x29, 0x1e},  /* 9 */
  {0x7e, 0x11, 0x11, 0x11, 0x7e},  /* A */
  {0x7f, 0x49, 0x49, 0x49, 0x36},  /* B */
  {0x3e, 0x41, 0x41, 0x41, 0x22},  /* C */
  {0x7f, 0x41, 0x41, 0x22, 0x1c},  /* D */
  {0x7f, 0x49, 0x49, 0x49, 0x41},  /* E */
  {0x7f, 0x09, 0x09, 0x09, 0x01},  /* F */
  {0x3e, 0x41, 0x49, 0x49, 0x7a},  /* G */
  {0x7f, 0x08, 0x08, 0x08, 0x7f},  /* H */
  {0x00, 0x41, 0x7f, 0x41, 0x00},  /* I */
  {0x20, 0x40, 0x41, 0x3f, 0x01},  /* J */
  {0x7f, 0x08, 0x14, 0x22, 0x41},  /* K */
  {0x7f, 0x40, 0x40, 0x40, 0x40},  /* L */
  {0x7f, 0x02, 0x0c, 0x02, 0x7f},  /* M */
  {0x7f, 0x04, 0x08, 0x10, 0x7f},  /* N */
  {0x3e, 0x41, 0x41, 0x41, 0x3e},  /* O */
  {0x7f, 0x09, 0x09, 0x09, 0x06},  /* P */
  {0x3e, 0x41, 0x51, 0x21, 0x5e},  /* Q */
  {0x7f, 0x09, 0x19, 0x29, 0x46},  /* R */
  {0x46, 0x49, 0x49, 0x49, 0x31},  /* S */
  {0x01, 0x01, 0x7f, 0x01, 0x01},  /* T */
  {0x3f, 0x40, 0x40, 0x40, 0x3f},  /* U */
  {0x1f, 0x20, 0x40, 0x20, 0x1f},  /* V */
  {0x3f, 0x40, 0x38, 0x40, 0x3f},  /* W */
  {0x63, 0x14, 0x08, 0x14, 0x63},  /* X */
  {0x07, 0x08, 0x70, 0x08, 0x07},  /* Y */
  {0x61, 0x51, 0x49, 0x45, 0x43},  /* Z */
  {0x00, 0x00, 0x00, 0x00, 0x00},  /* Space */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void fb_clear(void)
{
  memset(g_framebuffer, 0, sizeof(g_framebuffer));
}

static void fb_set_pixel(int x, int y, int color)
{
  int byte_idx;
  int bit_idx;

  if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT)
    {
      return;
    }

  /* Invert X (mirror horizontally) */

  x = (LCD_WIDTH - 1) - x;

  /* Invert Y (top to bottom) */

  y = (LCD_HEIGHT - 1) - y;

  /* MSB first: bit 7 = leftmost pixel in byte */

  byte_idx = x / 8;
  bit_idx = 7 - (x % 8);

  if (color)
    {
      g_framebuffer[y][byte_idx] |= (1 << bit_idx);
    }
  else
    {
      g_framebuffer[y][byte_idx] &= ~(1 << bit_idx);
    }
}

static void fb_draw_hline(int x, int y, int w, int color)
{
  int i;
  for (i = 0; i < w; i++)
    {
      fb_set_pixel(x + i, y, color);
    }
}

static void fb_draw_vline(int x, int y, int h, int color)
{
  int i;
  for (i = 0; i < h; i++)
    {
      fb_set_pixel(x, y + i, color);
    }
}

static void fb_draw_rect(int x, int y, int w, int h, int color)
{
  fb_draw_hline(x, y, w, color);
  fb_draw_hline(x, y + h - 1, w, color);
  fb_draw_vline(x, y, h, color);
  fb_draw_vline(x + w - 1, y, h, color);
}

static void fb_fill_rect(int x, int y, int w, int h, int color)
{
  int i, j;
  for (j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
        {
          fb_set_pixel(x + i, y + j, color);
        }
    }
}

static void fb_draw_char(int x, int y, char c, int size)
{
  int font_idx;
  int col, row, sx, sy;
  uint8_t line;

  if (c >= '0' && c <= '9')
    font_idx = c - '0';
  else if (c >= 'A' && c <= 'Z')
    font_idx = c - 'A' + 10;
  else if (c >= 'a' && c <= 'z')
    font_idx = c - 'a' + 10;
  else if (c == ' ')
    font_idx = 36;
  else if (c == '%')
    {
      fb_set_pixel(x, y, 1);
      fb_set_pixel(x + 1, y, 1);
      fb_set_pixel(x, y + 1, 1);
      fb_set_pixel(x + 1, y + 1, 1);
      fb_set_pixel(x + 4, y + 5, 1);
      fb_set_pixel(x + 5, y + 5, 1);
      fb_set_pixel(x + 4, y + 6, 1);
      fb_set_pixel(x + 5, y + 6, 1);
      fb_set_pixel(x + 4, y + 1, 1);
      fb_set_pixel(x + 3, y + 2, 1);
      fb_set_pixel(x + 2, y + 3, 1);
      fb_set_pixel(x + 1, y + 4, 1);
      fb_set_pixel(x, y + 5, 1);
      return;
    }
  else
    return;

  for (col = 0; col < 5; col++)
    {
      line = g_font5x7[font_idx][col];
      for (row = 0; row < 7; row++)
        {
          if (line & (1 << row))
            {
              for (sy = 0; sy < size; sy++)
                for (sx = 0; sx < size; sx++)
                  fb_set_pixel(x + col * size + sx, y + row * size + sy, 1);
            }
        }
    }
}

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

static void draw_battery(int percent)
{
  int fill_width, inner_x, inner_y, inner_w, inner_h;

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
    fb_fill_rect(inner_x, inner_y, fill_width, inner_h, 1);
}

static void draw_battery_screen(int percent)
{
  char percent_str[8];
  fb_clear();
  fb_draw_string(TITLE_X, TITLE_Y, "BATTERY", 1);
  draw_battery(percent);
  snprintf(percent_str, sizeof(percent_str), "%d%%", percent);
  fb_draw_string(PERCENT_X, PERCENT_Y, percent_str, 2);
}

static void draw_nuttx_screen(void)
{
  fb_clear();
  fb_draw_string(TITLE_X, TITLE_Y, "BATTERY", 1);
  fb_draw_rect(BATTERY_X, BATTERY_Y, BATTERY_W, BATTERY_H, 1);
  fb_fill_rect(BATTERY_X + BATTERY_W,
               BATTERY_Y + (BATTERY_H - BATTERY_TIP_H) / 2,
               BATTERY_TIP_W, BATTERY_TIP_H, 1);
  fb_draw_string(BATTERY_X + 3, BATTERY_Y + 5, "NUTTX", 1);
  fb_draw_string(PERCENT_X, PERCENT_Y, "0%", 2);
}

static int lcd_write_framebuffer(int fd)
{
  struct lcddev_run_s run;
  int row, ret;

  for (row = 0; row < LCD_HEIGHT; row++)
    {
      run.row = row;
      run.col = 0;
      run.npixels = LCD_WIDTH;
      run.data = g_framebuffer[row];

      ret = ioctl(fd, LCDDEVIO_PUTRUN, (unsigned long)&run);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: PUTRUN row %d: %d\n", row, errno);
          return ret;
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int fd, percent, ret;

  printf("LCD Battery Demo for SSD1306\n");
  printf("Opening /dev/lcd0...\n");

  fd = open("/dev/lcd0", O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open /dev/lcd0: %d\n", errno);
      return EXIT_FAILURE;
    }

  printf("Starting animation (100%% -> 0%%)\n");

  for (percent = 100; percent >= 0; percent--)
    {
      draw_battery_screen(percent);
      ret = lcd_write_framebuffer(fd);
      if (ret < 0)
        {
          close(fd);
          return EXIT_FAILURE;
        }
      usleep(ANIMATION_DELAY_US);
    }

  printf("Showing NuttX screen...\n");
  draw_nuttx_screen();
  lcd_write_framebuffer(fd);
  sleep(3);

  printf("Demo complete!\n");
  close(fd);
  return EXIT_SUCCESS;
}