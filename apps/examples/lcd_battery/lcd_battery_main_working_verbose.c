/****************************************************************************
 * apps/examples/lcd_battery/lcd_battery_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Battery animation demo for SSD1306 OLED display.
 * Display size determined by Kconfig (128x32 or 128x64).
 *
 * This application demonstrates how to draw graphics on a monochrome
 * OLED display using the NuttX LCD character device interface (/dev/lcd0).
 *
 * Key concepts:
 *   - Framebuffer: A memory array that mirrors the display pixels
 *   - 1bpp format: 1 bit per pixel (on/off only, no colors)
 *   - PUTRUN ioctl: Sends one horizontal line of pixels to the driver
 *   - Static allocation: Size determined at compile time via Kconfig
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
 * To run in background (free console):
 *   lcd_battery &
 *
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

/****************************************************************************
 * Display Dimensions (from Kconfig)
 *
 * WHY USE KCONFIG?
 *
 * The SSD1306 comes in two common sizes:
 *   - 128x32 pixels (0.91" displays)
 *   - 128x64 pixels (0.96" displays)
 *
 * Instead of hardcoding or using malloc, we let Kconfig decide at compile
 * time. This gives us:
 *   - Zero memory waste (exact size allocated)
 *   - Deterministic behavior (no runtime allocation)
 *   - Compile-time error if misconfigured
 *
 * The CONFIG_LCD_SSD1306_128x64 symbol is set in menuconfig when you
 * select the 128x64 display variant.
 *
 ****************************************************************************/

#define LCD_WIDTH 128

#ifdef CONFIG_LCD_SSD1306_128x64
#  define LCD_HEIGHT 64
#else
#  define LCD_HEIGHT 32
#endif

/****************************************************************************
 * Framebuffer Row Size
 *
 * WHY LCD_WIDTH / 8?
 *
 * Each pixel needs only 1 bit (on or off). We have 128 pixels per row.
 * Since 1 byte = 8 bits: 128 pixels / 8 bits = 16 bytes per row.
 *
 * Memory layout for one row:
 *
 *   Pixel:  0-7      8-15     16-23    ...    120-127
 *          +--------+--------+--------+------+--------+
 *          | Byte 0 | Byte 1 | Byte 2 | .... | Byte 15|
 *          +--------+--------+--------+------+--------+
 *
 ****************************************************************************/

#define ROW_BUFFER_SIZE (LCD_WIDTH / 8)

/****************************************************************************
 * Battery Icon Layout
 *
 * Visual representation on display:
 *
 *    (BATTERY_X, BATTERY_Y) = (4, 8)
 *         |
 *         v
 *         +----------------------------------+----+
 *         |                                  |    |
 *         |   +-------------------------+    |    | <- tip (positive terminal)
 *         |   |//////// FILL ///////////|    |    |    BATTERY_TIP_W x TIP_H
 *         |   +-------------------------+    |    |    (3 x 8 pixels)
 *         |        ^                         |    |
 *         |        |                         |    |
 *         |     BORDER (2px each side)       |    |
 *         +----------------------------------+----+
 *         |<-------- BATTERY_W = 40 -------->|
 *         |<----------- + TIP_W = 3 ------------>|
 *
 *         Total battery height: BATTERY_H = 16 pixels
 *
 ****************************************************************************/

#define BATTERY_X       4
#define BATTERY_Y       8
#define BATTERY_W       40
#define BATTERY_H       16
#define BATTERY_TIP_W   3
#define BATTERY_TIP_H   8
#define BATTERY_BORDER  2

/****************************************************************************
 * Text Positions
 *
 * Screen layout:
 *
 *   (0,0)
 *     +--------------------------------------------------+
 *     | BATTERY                        <- TITLE (4,0)    |
 *     |                                                  |
 *     | [████████████]█   75%          <- PERCENT (52,12)|
 *     |                                                  |
 *     +--------------------------------------------------+
 *                                                   (127,31)
 *
 ****************************************************************************/

#define PERCENT_X       52
#define PERCENT_Y       12
#define TITLE_X         4
#define TITLE_Y         0

/****************************************************************************
 * Animation Timing
 *
 * 1,000,000 microseconds = 1 second between each percentage update.
 * Full animation (100% to 0%) takes ~101 seconds.
 *
 ****************************************************************************/

#define ANIMATION_DELAY_US  1000000

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * g_framebuffer - Static shadow copy of display memory
 *
 * WHY STATIC AND NOT MALLOC?
 *
 * 1. Deterministic: Size known at compile time, no runtime failures
 * 2. No fragmentation: Memory reserved in .bss section
 * 3. Faster: No malloc overhead
 * 4. Simpler: No need for free() or NULL checks
 *
 * WHY 2D ARRAY [HEIGHT][ROW_BUFFER_SIZE]?
 *
 * This organization matches how we send data to the display:
 *   - Each g_framebuffer[row] is exactly one horizontal line
 *   - We can pass g_framebuffer[row] directly to PUTRUN ioctl
 *
 * MEMORY CALCULATION:
 *
 *   128x32 display: 32 rows × 16 bytes = 512 bytes
 *   128x64 display: 64 rows × 16 bytes = 1024 bytes
 *
 *   Visual representation:
 *
 *   g_framebuffer[0][0..15]   -> Row 0:   128 pixels (16 bytes)
 *   g_framebuffer[1][0..15]   -> Row 1:   128 pixels (16 bytes)
 *   g_framebuffer[2][0..15]   -> Row 2:   128 pixels (16 bytes)
 *   ...
 *   g_framebuffer[31][0..15]  -> Row 31:  128 pixels (16 bytes)
 *                                         ─────────────────────
 *                                Total:   512 bytes (for 128x32)
 *
 ****************************************************************************/

static uint8_t g_framebuffer[LCD_HEIGHT][ROW_BUFFER_SIZE];

/****************************************************************************
 * g_flip_x, g_flip_y - Orientation control flags
 *
 * WHY DO WE NEED THESE?
 *
 * The SSD1306 can be physically mounted in different orientations on a PCB.
 * If mounted "upside down", the image appears flipped. These flags let the
 * user correct the orientation without resoldering the display.
 *
 * DEFAULT VALUES:
 *
 * Both are TRUE by default because our specific hardware configuration
 * requires both X and Y to be flipped to display correctly. This was
 * determined experimentally during development.
 *
 * COMMAND LINE CONTROL:
 *
 *   --flip-x toggles g_flip_x (TRUE->FALSE or FALSE->TRUE)
 *   --flip-y toggles g_flip_y
 *   --rotate 180 toggles both (equivalent to 180° rotation)
 *
 ****************************************************************************/

static bool g_flip_x = true;
static bool g_flip_y = true;

/****************************************************************************
 * g_font5x7 - Bitmap font for text rendering
 *
 * FONT FORMAT:
 *
 * Each character is 5 pixels wide × 7 pixels tall.
 * Stored as 5 bytes per character, where each byte is one COLUMN.
 *
 * WHY COLUMNS INSTEAD OF ROWS?
 *
 * Historical convention in embedded systems. Also matches how some
 * displays (including SSD1306 in page mode) organize memory vertically.
 *
 * HOW TO DECODE A CHARACTER:
 *
 * Example: Letter 'A' = {0x7e, 0x11, 0x11, 0x11, 0x7e}
 *
 *   Column 0: 0x7e = 0b01111110
 *   Column 1: 0x11 = 0b00010001
 *   Column 2: 0x11 = 0b00010001
 *   Column 3: 0x11 = 0b00010001
 *   Column 4: 0x7e = 0b01111110
 *
 *   Bit 0 = top row, Bit 6 = bottom row:
 *
 *          Col: 0 1 2 3 4
 *   Bit 0 (top): . ■ ■ ■ .
 *   Bit 1:       ■ . . . ■
 *   Bit 2:       ■ . . . ■
 *   Bit 3:       ■ ■ ■ ■ ■    <- horizontal bar of 'A'
 *   Bit 4:       ■ . . . ■
 *   Bit 5:       ■ . . . ■
 *   Bit 6 (bot): . . . . .
 *
 * INDEX MAPPING:
 *
 *   Characters '0'-'9' -> indices 0-9
 *   Characters 'A'-'Z' -> indices 10-35
 *   Character  ' '     -> index 36
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: show_usage
 *
 * Description:
 *   Prints help message with all available command line options.
 *
 * Input Parameters:
 *   progname - Program name (argv[0]) for usage line
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void show_usage(const char *progname)
{
  printf("Usage: %s [options]\n", progname);
  printf("\n");
  printf("Options:\n");
  printf("  --flip-x       Mirror display horizontally\n");
  printf("  --flip-y       Mirror display vertically\n");
  printf("  --rotate 180   Rotate 180 degrees (same as --flip-x --flip-y)\n");
  printf("  --help         Show this help message\n");
  printf("\n");
  printf("Display: %dx%d pixels\n", LCD_WIDTH, LCD_HEIGHT);
  printf("\n");
  printf("To run in background: %s &\n", progname);
}

/****************************************************************************
 * Name: parse_args
 *
 * Description:
 *   Parses command line arguments and updates orientation flags.
 *
 * How toggle works:
 *
 *   Default state: g_flip_x = true, g_flip_y = true
 *
 *   --flip-x       -> g_flip_x becomes false
 *   --flip-x again -> g_flip_x becomes true
 *
 *   This lets users experiment to find the correct orientation.
 *
 * Input Parameters:
 *   argc - Argument count
 *   argv - Argument vector
 *
 * Returned Value:
 *    0 = Success, continue program
 *    1 = Help shown, exit successfully
 *   -1 = Error, exit with failure
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
              i++;  /* Skip "180" argument */
            }
          else
            {
              printf("Error: --rotate requires '180' as argument\n");
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
          printf("Unknown option: %s\n", argv[i]);
          show_usage(argv[0]);
          return -1;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: fb_clear
 *
 * Description:
 *   Clears the entire framebuffer (all pixels off).
 *
 * How it works:
 *   memset fills all bytes with 0. Since each bit represents a pixel,
 *   and 0 = pixel off, this turns off all pixels.
 *
 * Note:
 *   This only modifies RAM. Call lcd_write_framebuffer() to update display.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fb_clear(void)
{
  memset(g_framebuffer, 0, sizeof(g_framebuffer));
}

/****************************************************************************
 * Name: fb_set_pixel
 *
 * Description:
 *   Sets or clears a single pixel in the framebuffer.
 *
 * THIS IS THE MOST CRITICAL FUNCTION:
 *   Every drawing operation (lines, rectangles, text) ultimately calls
 *   this function. If this is wrong, everything is wrong.
 *
 * COORDINATE SYSTEM:
 *
 *   Application coordinates (what we use):
 *
 *     (0,0) ─────────────────────► X (127,0)
 *       │
 *       │
 *       │
 *       ▼
 *     Y (0,31)                      (127,31)
 *
 *   But the hardware may be mounted differently, requiring flips.
 *
 * STEP-BY-STEP FOR PIXEL (50, 10):
 *
 *   1. Bounds check: 0 <= 50 < 128 ✓, 0 <= 10 < 32 ✓
 *
 *   2. Apply flips (if g_flip_x and g_flip_y are true):
 *      x = 127 - 50 = 77
 *      y = 31 - 10 = 21
 *
 *   3. Find which byte contains this pixel:
 *      byte_idx = 77 / 8 = 9  (10th byte in the row)
 *
 *   4. Find which bit within that byte (MSB first):
 *      bit_idx = 7 - (77 % 8) = 7 - 5 = 2
 *
 *   5. Set the bit:
 *      g_framebuffer[21][9] |= (1 << 2)
 *      g_framebuffer[21][9] |= 0b00000100
 *
 * WHY MSB FIRST (bit 7 = leftmost)?
 *
 *   The NuttX SSD1306 driver expects this format. Bit 7 of each byte
 *   corresponds to the leftmost pixel in that group of 8.
 *
 *   Byte layout:
 *     Bit:   7   6   5   4   3   2   1   0
 *     Pixel: 0   1   2   3   4   5   6   7  (within byte group)
 *
 * BIT MANIPULATION EXPLAINED:
 *
 *   To SET bit n:   byte |= (1 << n)
 *
 *     Example: Set bit 2 in byte 0b10000000
 *       (1 << 2)     = 0b00000100  (mask with only bit 2 set)
 *       0b10000000 | 0b00000100 = 0b10000100  (bit 2 now set)
 *
 *   To CLEAR bit n: byte &= ~(1 << n)
 *
 *     Example: Clear bit 2 in byte 0b10000100
 *       (1 << 2)     = 0b00000100
 *       ~(1 << 2)    = 0b11111011  (inverted: all 1s except bit 2)
 *       0b10000100 & 0b11111011 = 0b10000000  (bit 2 now clear)
 *
 * Input Parameters:
 *   x     - Horizontal position (0 = left, LCD_WIDTH-1 = right)
 *   y     - Vertical position (0 = top, LCD_HEIGHT-1 = bottom)
 *   color - 0 = pixel off, non-zero = pixel on
 *
 * Returned Value:
 *   None (silently ignores out-of-bounds coordinates)
 *
 ****************************************************************************/

static void fb_set_pixel(int x, int y, int color)
{
  int byte_idx;
  int bit_idx;

  /* Bounds check - prevent memory corruption */

  if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT)
    {
      return;
    }

  /* Apply X flip if enabled (mirror horizontally) */

  if (g_flip_x)
    {
      x = (LCD_WIDTH - 1) - x;
    }

  /* Apply Y flip if enabled (mirror vertically) */

  if (g_flip_y)
    {
      y = (LCD_HEIGHT - 1) - y;
    }

  /* Calculate byte index (which byte in the row) */

  byte_idx = x / 8;

  /* Calculate bit index (MSB first: bit 7 = leftmost pixel) */

  bit_idx = 7 - (x % 8);

  /* Set or clear the bit */

  if (color)
    {
      g_framebuffer[y][byte_idx] |= (1 << bit_idx);
    }
  else
    {
      g_framebuffer[y][byte_idx] &= ~(1 << bit_idx);
    }
}

/****************************************************************************
 * Name: fb_draw_hline
 *
 * Description:
 *   Draws a horizontal line.
 *
 * Visual:
 *   (x,y)
 *     ████████████████  <- w pixels
 *
 * Why separate from vline?
 *   Could be optimized differently. Horizontal lines touch consecutive
 *   bits (potentially multiple bits per byte), while vertical lines
 *   touch one bit per byte across multiple rows.
 *
 * Input Parameters:
 *   x     - Starting X position
 *   y     - Y position (row)
 *   w     - Width in pixels
 *   color - 0 = off, non-zero = on
 *
 * Returned Value:
 *   None
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
 *   Draws a vertical line.
 *
 * Visual:
 *   (x,y)
 *     █
 *     █
 *     █   <- h pixels tall
 *     █
 *     █
 *
 * Input Parameters:
 *   x     - X position (column)
 *   y     - Starting Y position
 *   h     - Height in pixels
 *   color - 0 = off, non-zero = on
 *
 * Returned Value:
 *   None
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
 *   Draws a hollow rectangle (outline only).
 *
 * Visual:
 *   (x,y)
 *     ████████████████  <- top edge (hline)
 *     █              █
 *     █              █  <- left & right edges (vlines)
 *     █              █
 *     ████████████████  <- bottom edge (hline)
 *                (x+w-1, y+h-1)
 *
 * Input Parameters:
 *   x, y  - Top-left corner
 *   w     - Width in pixels
 *   h     - Height in pixels
 *   color - 0 = off, non-zero = on
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fb_draw_rect(int x, int y, int w, int h, int color)
{
  fb_draw_hline(x, y, w, color);          /* Top */
  fb_draw_hline(x, y + h - 1, w, color);  /* Bottom */
  fb_draw_vline(x, y, h, color);          /* Left */
  fb_draw_vline(x + w - 1, y, h, color);  /* Right */
}

/****************************************************************************
 * Name: fb_fill_rect
 *
 * Description:
 *   Draws a filled rectangle (solid block).
 *
 * Visual:
 *   (x,y)
 *     ████████████████
 *     ████████████████
 *     ████████████████  <- every pixel inside is set
 *     ████████████████
 *     ████████████████
 *                (x+w-1, y+h-1)
 *
 * Performance note:
 *   For a w×h rectangle, this calls fb_set_pixel w×h times.
 *   A 40×16 battery fill = 640 function calls.
 *   Could be optimized by setting multiple bits per operation.
 *
 * Input Parameters:
 *   x, y  - Top-left corner
 *   w     - Width in pixels
 *   h     - Height in pixels
 *   color - 0 = off, non-zero = on
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: fb_draw_char
 *
 * Description:
 *   Draws a single character with optional scaling.
 *
 * RENDERING ALGORITHM:
 *
 *   1. Map ASCII to font index:
 *      '0'-'9' -> 0-9
 *      'A'-'Z' -> 10-35
 *      'a'-'z' -> 10-35 (same as uppercase)
 *      ' '     -> 36
 *      '%'     -> special case (drawn manually)
 *
 *   2. For each column (0-4):
 *      a. Get column byte from font table
 *      b. For each row (0-6):
 *         - Test if bit 'row' is set in column byte
 *         - If set, draw a size×size block at position
 *
 * SCALING:
 *
 *   size=1: Each font pixel = 1 display pixel (5×7 total)
 *   size=2: Each font pixel = 2×2 display pixels (10×14 total)
 *   size=3: Each font pixel = 3×3 display pixels (15×21 total)
 *
 *   Example with size=2:
 *
 *     Font pixel        Display pixels
 *         █       ->       ██
 *                          ██
 *
 * WHY HANDLE '%' SPECIALLY?
 *
 *   Our font table only has 0-9, A-Z, and space. Rather than expand
 *   the table for one character, we draw '%' manually with individual
 *   pixels. It's a simple pattern: two small circles and a diagonal.
 *
 * Input Parameters:
 *   x    - Left edge position
 *   y    - Top edge position
 *   c    - ASCII character to draw
 *   size - Scale factor (1 = normal, 2 = double, etc.)
 *
 * Returned Value:
 *   None (unknown characters silently ignored)
 *
 ****************************************************************************/

static void fb_draw_char(int x, int y, char c, int size)
{
  int font_idx;
  int col, row, sx, sy;
  uint8_t line;

  /* Map character to font array index */

  if (c >= '0' && c <= '9')
    {
      font_idx = c - '0';
    }
  else if (c >= 'A' && c <= 'Z')
    {
      font_idx = c - 'A' + 10;
    }
  else if (c >= 'a' && c <= 'z')
    {
      font_idx = c - 'a' + 10;  /* Lowercase = uppercase */
    }
  else if (c == ' ')
    {
      font_idx = 36;
    }
  else if (c == '%')
    {
      /* Draw '%' manually:
       *
       *   ██        <- top circle
       *      █
       *     █       <- diagonal line
       *    █
       *   █
       *      ██     <- bottom circle
       */

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
    {
      return;  /* Unknown character - ignore */
    }

  /* Render character from font data */

  for (col = 0; col < 5; col++)
    {
      line = g_font5x7[font_idx][col];

      for (row = 0; row < 7; row++)
        {
          if (line & (1 << row))  /* Test if pixel should be on */
            {
              /* Draw size×size block */

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
 *   Draws a null-terminated string.
 *
 * CHARACTER SPACING:
 *
 *   Each character cell is 6 pixels wide (5 for char + 1 for spacing).
 *   With scaling, it's 6 × size pixels per character.
 *
 *   size=1: |H|E|L|L|O|  -> 5×6 = 30 pixels wide
 *            └─ 6px ─┘
 *
 *   size=2: |H |E |L |L |O |  -> 5×12 = 60 pixels wide
 *            └── 12px ──┘
 *
 * Input Parameters:
 *   x    - Starting X position
 *   y    - Y position
 *   str  - Null-terminated string
 *   size - Scale factor for all characters
 *
 * Returned Value:
 *   None
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
 *   Draws battery icon with fill level.
 *
 * STRUCTURE:
 *
 *   +----------------------------------+----+
 *   |           BORDER                 |    |
 *   |   +-------------------------+    | T  |
 *   |   |                         |    | I  |
 *   |   |    FILL AREA            |    | P  |
 *   |   |    (proportional to %)  |    |    |
 *   |   +-------------------------+    |    |
 *   |                                  |    |
 *   +----------------------------------+----+
 *
 * FILL CALCULATION:
 *
 *   inner_w = BATTERY_W - 2×BORDER = 40 - 4 = 36 pixels
 *   fill_width = inner_w × percent / 100
 *
 *   Examples:
 *     100% -> 36 pixels filled
 *      50% -> 18 pixels filled
 *       0% ->  0 pixels filled
 *
 * Input Parameters:
 *   percent - Battery level (0-100)
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void draw_battery(int percent)
{
  int fill_width, inner_x, inner_y, inner_w, inner_h;

  /* Draw battery outline */

  fb_draw_rect(BATTERY_X, BATTERY_Y, BATTERY_W, BATTERY_H, 1);

  /* Draw positive terminal (tip) */

  fb_fill_rect(BATTERY_X + BATTERY_W,
               BATTERY_Y + (BATTERY_H - BATTERY_TIP_H) / 2,
               BATTERY_TIP_W, BATTERY_TIP_H, 1);

  /* Calculate inner fill area dimensions */

  inner_x = BATTERY_X + BATTERY_BORDER;
  inner_y = BATTERY_Y + BATTERY_BORDER;
  inner_w = BATTERY_W - (BATTERY_BORDER * 2);
  inner_h = BATTERY_H - (BATTERY_BORDER * 2);

  /* Calculate fill width based on percentage */

  fill_width = (inner_w * percent) / 100;

  /* Draw fill */

  if (fill_width > 0)
    {
      fb_fill_rect(inner_x, inner_y, fill_width, inner_h, 1);
    }
}

/****************************************************************************
 * Name: draw_battery_screen
 *
 * Description:
 *   Draws complete battery status screen.
 *
 * LAYOUT:
 *
 *   +--------------------------------------------------+
 *   | BATTERY                                          |
 *   |                                                  |
 *   | [████████████]█   75%                            |
 *   |                                                  |
 *   +--------------------------------------------------+
 *
 * Input Parameters:
 *   percent - Battery level (0-100)
 *
 * Returned Value:
 *   None
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
 *   Draws final screen with "NUTTX" inside empty battery.
 *
 * LAYOUT:
 *
 *   +--------------------------------------------------+
 *   | BATTERY                                          |
 *   |                                                  |
 *   | [  NUTTX  ]█   0%                                |
 *   |                                                  |
 *   +--------------------------------------------------+
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void draw_nuttx_screen(void)
{
  fb_clear();
  fb_draw_string(TITLE_X, TITLE_Y, "BATTERY", 1);

  /* Empty battery outline */

  fb_draw_rect(BATTERY_X, BATTERY_Y, BATTERY_W, BATTERY_H, 1);
  fb_fill_rect(BATTERY_X + BATTERY_W,
               BATTERY_Y + (BATTERY_H - BATTERY_TIP_H) / 2,
               BATTERY_TIP_W, BATTERY_TIP_H, 1);

  /* "NUTTX" inside battery */

  fb_draw_string(BATTERY_X + 3, BATTERY_Y + 5, "NUTTX", 1);

  fb_draw_string(PERCENT_X, PERCENT_Y, "0%", 2);
}

/****************************************************************************
 * Name: lcd_write_framebuffer
 *
 * Description:
 *   Sends entire framebuffer to display via PUTRUN ioctl.
 *
 * HOW IT WORKS:
 *
 *   1. Loop through all rows (0 to LCD_HEIGHT-1)
 *   2. For each row, fill lcddev_run_s structure:
 *      - row: which horizontal line
 *      - col: starting column (0 = full row)
 *      - npixels: pixels to send (LCD_WIDTH = full row)
 *      - data: pointer to g_framebuffer[row]
 *   3. Call ioctl(LCDDEVIO_PUTRUN) to send row to driver
 *   4. Driver converts data and sends to SSD1306 via I2C
 *
 * DATA FLOW:
 *
 *   g_framebuffer[row]  ->  ioctl(PUTRUN)  ->  ssd1306_putrun()
 *         │                      │                    │
 *         │                      │                    v
 *    16 bytes              NuttX LCD            I2C transfer
 *    (128 pixels)          driver               to SSD1306
 *
 * Input Parameters:
 *   fd - File descriptor for /dev/lcd0
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Application entry point.
 *
 * PROGRAM FLOW:
 *
 *   1. Parse command line arguments
 *   2. Open /dev/lcd0
 *   3. Animation loop: 100% -> 0% (101 frames, ~101 seconds)
 *   4. Show "NUTTX" screen for 3 seconds
 *   5. Cleanup and exit
 *
 * Input Parameters:
 *   argc - Argument count
 *   argv - Argument vector
 *
 * Returned Value:
 *   EXIT_SUCCESS (0) on success
 *   EXIT_FAILURE (1) on error
 *
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int fd, percent, ret;

  /* Parse command line */

  ret = parse_args(argc, argv);
  if (ret != 0)
    {
      return (ret < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
    }

  /* Print startup info */

  printf("LCD Battery Demo\n");
  printf("Display: %dx%d pixels\n", LCD_WIDTH, LCD_HEIGHT);
  printf("Framebuffer: %d bytes\n", LCD_HEIGHT * ROW_BUFFER_SIZE);
  printf("Orientation: flip_x=%s, flip_y=%s\n",
         g_flip_x ? "ON" : "OFF",
         g_flip_y ? "ON" : "OFF");

  /* Open LCD device */

  fd = open("/dev/lcd0", O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open /dev/lcd0: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Main animation loop */

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

  /* Final screen */

  printf("Showing NuttX screen...\n");
  draw_nuttx_screen();
  lcd_write_framebuffer(fd);
  sleep(3);

  /* Cleanup */

  printf("Demo complete!\n");
  close(fd);

  return EXIT_SUCCESS;
}