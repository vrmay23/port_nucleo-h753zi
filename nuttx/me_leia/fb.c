/****************************************************************************
 * apps/examples/fb/fb_main.c
 *
 * ST7796 Compatible Version
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define NCOLORS 6

/* Default framebuffer device for ST7796 */
#ifndef CONFIG_EXAMPLES_FB_DEFAULTFB
#  define CONFIG_EXAMPLES_FB_DEFAULTFB "/dev/fb0"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fb_state_s
{
  int fd;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
#ifdef CONFIG_FB_OVERLAY
  struct fb_overlayinfo_s oinfo;
#endif
  FAR void *fbmem;
  FAR void *fbmem2;
  FAR void *act_fbmem;
  uint32_t mem2_yoffset;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_default_fbdev[] = CONFIG_EXAMPLES_FB_DEFAULTFB;

/* Violet-Blue-Green-Yellow-Orange-Red */

static const uint32_t g_rgb24[NCOLORS] =
{
  RGB24_VIOLET, RGB24_BLUE, RGB24_GREEN,
  RGB24_YELLOW, RGB24_ORANGE, RGB24_RED
};

static const uint16_t g_rgb16[NCOLORS] =
{
  RGB16_VIOLET, RGB16_BLUE, RGB16_GREEN,
  RGB16_YELLOW, RGB16_ORANGE, RGB16_RED
};

static const uint8_t g_rgb8[NCOLORS] =
{
  RGB8_VIOLET, RGB8_BLUE, RGB8_GREEN,
  RGB8_YELLOW, RGB8_ORANGE, RGB8_RED
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * sync_area
 ****************************************************************************/

static void sync_area(FAR struct fb_state_s *state)
{
  if (state->fbmem2 == NULL)
    {
      return;
    }

  if (state->act_fbmem == state->fbmem)
    {
      memcpy(state->fbmem, state->fbmem2,
             state->vinfo.yres * state->pinfo.stride);
    }
  else
    {
      memcpy(state->fbmem2, state->fbmem,
             state->vinfo.yres * state->pinfo.stride);
    }
}

/****************************************************************************
 * pan_display
 ****************************************************************************/

static void pan_display(FAR struct fb_state_s *state)
{
  struct pollfd pfd;
  int ret;
  pfd.fd = state->fd;
  pfd.events = POLLOUT;

  ret = poll(&pfd, 1, 0);

  if (ret > 0)
    {
      if (state->fbmem2 == NULL)
        {
          return;
        }

      if (state->act_fbmem == state->fbmem)
        {
          state->pinfo.yoffset = 0;
        }
      else
        {
          state->pinfo.yoffset = state->mem2_yoffset;
        }

      ioctl(state->fd, FBIOPAN_DISPLAY,
            (unsigned long)(uintptr_t)&state->pinfo);

      state->act_fbmem = state->act_fbmem == state->fbmem ?
                         state->fbmem2 : state->fbmem;
    }
}

/****************************************************************************
 * draw_rect16 - Optimized for ST7796 RGB565
 ****************************************************************************/

static void draw_rect16(FAR struct fb_state_s *state,
                        FAR struct fb_area_s *area, int color)
{
  FAR uint16_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint16_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb16[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect32(FAR struct fb_state_s *state,
                        FAR struct fb_area_s *area, int color)
{
  FAR uint32_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint32_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb24[color] | 0xff000000;
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect24(FAR struct fb_state_s *state,
                        FAR struct fb_area_s *area, int color)
{
  FAR uint8_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint8_t *)row) + area->x * 3;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb24[color] & 0xff;
          *dest++ = (g_rgb24[color] >> 8) & 0xff;
          *dest++ = (g_rgb24[color] >> 16) & 0xff;
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect8(FAR struct fb_state_s *state,
                       FAR struct fb_area_s *area, int color)
{
  FAR uint8_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = row + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb8[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect1(FAR struct fb_state_s *state,
                       FAR struct fb_area_s *area, int color)
{
  FAR uint8_t *pixel;
  FAR uint8_t *row;
  uint8_t color8 = (color & 1) == 0 ? 0 : 0xff;

  int start_full_x;
  int end_full_x;
  int start_bit_shift;
  int last_bits;
  uint8_t lmask;
  uint8_t rmask;
  int y;

  row    = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;

  start_full_x = ((area->x + 7) >> 3);
  end_full_x   = ((area->x + area->w) >> 3);

  start_bit_shift = 8 + area->x - (start_full_x << 3);
  lmask = 0xff >> start_bit_shift;

  last_bits = area->x + area->w - (end_full_x << 3);
  rmask = 0xff << (8 - last_bits);

  for (y = 0; y < area->h; y++)
    {
      if (start_full_x > end_full_x)
        {
          pixel = row + start_full_x - 1;
          *pixel = (*pixel & (~lmask | ~rmask)) | (lmask & rmask & color8);
          continue;
        }

      if (start_bit_shift != 0)
        {
          pixel = row + start_full_x - 1;
          *pixel = (*pixel & ~lmask) | (lmask & color8);
        }

      if (end_full_x > start_full_x)
        {
          pixel = row + start_full_x;
          memset(pixel, color8, end_full_x - start_full_x);
        }

      if (last_bits != 0)
        {
          pixel = row + end_full_x;
          *pixel = (*pixel & ~rmask) | (rmask & color8);
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect(FAR struct fb_state_s *state,
                      FAR struct fb_area_s *area, int color)
{
#ifdef CONFIG_FB_UPDATE
  int ret;
#endif

  switch (state->pinfo.bpp)
    {
      case 32:
        draw_rect32(state, area, color);
        break;

      case 24:
        draw_rect24(state, area, color);
        break;

      case 16:
        draw_rect16(state, area, color);
        break;

      case 8:
        draw_rect8(state, area, color);
        break;

      case 1:
        draw_rect1(state, area, color);
        break;

      default:
        fprintf(stderr, "ERROR: Unsupported BPP: %d\n", state->pinfo.bpp);
        return;
    }

#ifdef CONFIG_FB_UPDATE
  int yoffset = state->act_fbmem == state->fbmem ?
                0 : state->mem2_yoffset;
  area->y += yoffset;

  ret = ioctl(state->fd, FBIO_UPDATE,
              (unsigned long)((uintptr_t)area));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_UPDATE) failed: %d\n",
              errcode);
    }
#endif

  if (state->pinfo.yres_virtual == (state->vinfo.yres * 2))
    {
      pan_display(state);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * fb_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *fbdev = g_default_fbdev;
  struct fb_state_s state;
  struct fb_area_s area;
  int nsteps;
  int xstep;
  int ystep;
  int width;
  int height;
  int color;
  int x;
  int y;
  int ret;

  if (argc == 2)
    {
      fbdev = argv[1];
    }
  else if (argc != 1)
    {
      fprintf(stderr, "ERROR: Single argument required\n");
      fprintf(stderr, "USAGE: %s [<fb-driver-path>]\n", argv[0]);
      return EXIT_FAILURE;
    }

  printf("Opening framebuffer: %s\n", fbdev);

  /* Open the framebuffer driver */

  state.fd = open(fbdev, O_RDWR);
  if (state.fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", fbdev, errcode);
      return EXIT_FAILURE;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(state.fd, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&state.vinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_VIDEOINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("ST7796 VideoInfo:\n");
  printf("      fmt: %u\n", state.vinfo.fmt);
  printf("     xres: %u\n", state.vinfo.xres);
  printf("     yres: %u\n", state.vinfo.yres);
  printf("  nplanes: %u\n", state.vinfo.nplanes);

#ifdef CONFIG_FB_OVERLAY
  printf("noverlays: %u\n", state.vinfo.noverlays);

  ret = ioctl(state.fd, FBIO_SELECT_OVERLAY, 0);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_SELECT_OVERLAY) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  state.oinfo.overlay = 0;
  ret = ioctl(state.fd, FBIOGET_OVERLAYINFO,
                        (unsigned long)((uintptr_t)&state.oinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_OVERLAYINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("OverlayInfo (overlay 0):\n");
  printf("    fbmem: %p\n", state.oinfo.fbmem);
  printf("    fblen: %zu\n", state.oinfo.fblen);
  printf("   stride: %u\n", state.oinfo.stride);
  printf("  overlay: %u\n", state.oinfo.overlay);
  printf("      bpp: %u\n", state.oinfo.bpp);

  ret = ioctl(state.fd, FBIO_SELECT_OVERLAY, FB_NO_OVERLAY);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_SELECT_OVERLAY) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }
#endif

  /* Get plane information */

  memset(&state.pinfo, 0, sizeof(state.pinfo));
  ret = ioctl(state.fd, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&state.pinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("ST7796 PlaneInfo:\n");
  printf("    fbmem: %p\n", state.pinfo.fbmem);
  printf("    fblen: %zu\n", state.pinfo.fblen);
  printf("   stride: %u\n", state.pinfo.stride);
  printf("      bpp: %u\n", state.pinfo.bpp);

  if (state.pinfo.bpp != 16 && state.pinfo.bpp != 24 && state.pinfo.bpp != 32)
    {
      fprintf(stderr, "WARNING: BPP %u may not be fully supported\n",
              state.pinfo.bpp);
    }

  /* mmap() the framebuffer */

  state.fbmem = mmap(NULL, state.pinfo.fblen, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_FILE, state.fd, 0);
  if (state.fbmem == MAP_FAILED)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: mmap() failed: %d\n", errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("Mapped ST7796 FB: %p\n", state.fbmem);

  /* Initialize double buffer support */

  state.fbmem2 = NULL;
  state.mem2_yoffset = 0;

  /* Draw colorful rectangles */

  state.act_fbmem = state.fbmem;
  nsteps = 2 * (NCOLORS - 1) + 1;
  xstep  = state.vinfo.xres / nsteps;
  ystep  = state.vinfo.yres / nsteps;
  width  = state.vinfo.xres;
  height = state.vinfo.yres;

  printf("\nDrawing %d colored rectangles on ST7796...\n", NCOLORS);

  for (x = 0, y = 0, color = 0;
       color < NCOLORS;
       x += xstep, y += ystep, color++)
    {
      area.x = x;
      area.y = y;
      area.w = width;
      area.h = height;

      printf("%2d: (%3d,%3d) (%3d,%3d)\n",
             color, area.x, area.y, area.w, area.h);

      draw_rect(&state, &area, color);
      usleep(500 * 1000);

      width  -= (2 * xstep);
      height -= (2 * ystep);
    }

  printf("ST7796 framebuffer test finished!\n");
  ret = EXIT_SUCCESS;

  munmap(state.fbmem, state.pinfo.fblen);
  close(state.fd);
  return ret;
}
