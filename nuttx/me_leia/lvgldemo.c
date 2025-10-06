i/****************************************************************************
 * apps/examples/lvgldemo/lvgldemo.c
 *
 * ST7796 Compatible Version - Uses Framebuffer Interface
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <unistd.h>
#include <sys/boardctl.h>

#include <lvgl/lvgl.h>
#include <lvgl/demos/lv_demos.h>
#include <port/nuttx/lv_nuttx_entry.h>

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
#include <uv.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef NEED_BOARDINIT

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

/* ST7796 uses framebuffer, not LCD direct */

#ifndef CONFIG_EXAMPLES_LVGLDEMO_FB_DEVPATH
#  define CONFIG_EXAMPLES_LVGLDEMO_FB_DEVPATH "/dev/fb0"
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
static void lv_nuttx_uv_loop(uv_loop_t *loop, lv_nuttx_result_t *result)
{
  lv_nuttx_uv_t uv_info;
  void *data;

  uv_loop_init(loop);

  lv_memset(&uv_info, 0, sizeof(uv_info));
  uv_info.loop = loop;
  uv_info.disp = result->disp;
  uv_info.indev = result->indev;
#ifdef CONFIG_UINPUT_TOUCH
  uv_info.uindev = result->utouch_indev;
#endif

  data = lv_nuttx_uv_init(&uv_info);
  uv_run(loop, UV_RUN_DEFAULT);
  lv_nuttx_uv_deinit(&data);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t ui_loop;
  lv_memzero(&ui_loop, sizeof(ui_loop));
#endif

  if (lv_is_initialized())
    {
      LV_LOG_ERROR("LVGL already initialized! Aborting.");
      return -1;
    }

#ifdef NEED_BOARDINIT
  printf("Performing board initialization...\n");
  boardctl(BOARDIOC_INIT, 0);
#endif

  printf("Initializing LVGL for ST7796...\n");

  lv_init();
  lv_nuttx_dsc_init(&info);

  /* CRITICAL FIX: Use framebuffer for ST7796, not LCD */
  info.fb_path = CONFIG_EXAMPLES_LVGLDEMO_FB_DEVPATH;

  printf("Using framebuffer device: %s\n", info.fb_path);

#ifdef CONFIG_INPUT_TOUCHSCREEN
  info.input_path = CONFIG_EXAMPLES_LVGLDEMO_INPUT_DEVPATH;
  printf("Using touchscreen input: %s\n", info.input_path);
#endif

  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      LV_LOG_ERROR("LVGL initialization failure! Display not available.");
      printf("ERROR: Failed to initialize ST7796 display\n");
      printf("Verify:\n");
      printf("  1. ST7796 driver is enabled (CONFIG_LCD_ST7796=y)\n");
      printf("  2. Display is initialized in board bringup\n");
      printf("  3. Framebuffer device exists: %s\n", info.fb_path);
      return 1;
    }

  printf("ST7796 display initialized successfully!\n");
  printf("Display resolution: %dx%d\n", 
         lv_disp_get_hor_res(result.disp),
         lv_disp_get_ver_res(result.disp));

  if (argc > 1)
    {
      printf("Starting LVGL demo: %s\n", argv[1]);
    }
  else
    {
      printf("Starting default LVGL demo\n");
    }

  if (!lv_demos_create(&argv[1], argc - 1))
    {
      printf("Available LVGL demos:\n");
      lv_demos_show_help();
      printf("\nNo demo selected or demo not available.\n");
      goto demo_end;
    }

  printf("Demo created successfully. Starting event loop...\n");

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  printf("Using libuv event loop\n");
  lv_nuttx_uv_loop(&ui_loop, &result);
#else
  printf("Using simple polling loop\n");
  while (1)
    {
      uint32_t idle = lv_timer_handler();
      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }
#endif

demo_end:
  printf("Cleaning up LVGL...\n");
  lv_nuttx_deinit(&result);
  lv_deinit();

  printf("LVGL demo finished.\n");
  return 0;
}
