/****************************************************************************
 * apps/examples/can_io_lvgl/can_io_lvgl.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>             /* menuconfig definitions            */
#include <stdio.h>                    /* printf                            */
#include <fcntl.h>                    /* open files                        */
#include <unistd.h>                   /* file handler                      */
#include <string.h>                   /* memset and strncpy                */
#include <errno.h>                    /* error feedbakc                    */
#include <signal.h>                   /* signal interrupt hanndler         */
#include <sys/ioctl.h>                /* posix ioctrl (swit. knife)        */
#include <sys/mman.h>                 /* memmory map -> /dev/fb0           */
#include <sys/socket.h>               /* network socket -> /dev/can0       */
#include <sys/select.h>               /* I/O mux with timeout              */
#include <net/if.h>                   /* network interface (ifreq, index)  */
#include <nuttx/can.h>                /* nuttx fdcan_socket driver lib     */
#include <nuttx/video/fb.h>           /* nuttx frame_buffer driver lib     */
#include <nuttx/input/buttons.h>      /* nuttx api driver irq button       */
#include <netutils/netlib.h>          /* ifup can0. Thanks Cap. Smilley    */
#include <lvgl/lvgl.h>                /* HMI, basically!                   */

/****************************************************************************
 * App Configuration
 ****************************************************************************/

/* I did not read the DBC yet ^^'
 * So Im creating a place holder for messages and signals
 * For example, need to check indeed if we just have one message for speed.
 * It seems to 'waste' for me. We could arrange more sinals in one msg, but,
 * as said before, using place-holder it will be easy to undestand this app
 * and easy to edit it as well. So, lets get started!
 *
 * To be implemented:
 *
 *    #define ECU_SDTM    0x87     --> Smart Driver Telematic Module
 *    #define ECU_SBMS    0x70     --> Smart Battery Management System
 *
 * So it will be easy to handler the CAN Filters via switch/case using
 * the 'sender_node'. BITs 7-0 Source Address
 *
 * */

/* === CAN IDs (Extended 29-bit) === */

#define CAN_ID_SPEED         0x18FEF100      /*  Place-holder (PH): speed   */
#define CAN_ID_BATTERY       0x18FEF200      /*  PH: voltage & current      */
#define CAN_ID_DIRECTION     0x18FEF300      /*  PH: direction              */

/* === Byte positions in CAN messages === */

#define SPEED_BYTE           0               /* Speed. 0 - 255              */
#define DIRECTION_BYTE       0               /* Direction in byte 0         */

/* guess: 0.1V of resolution ---> 16 bits of resolution
 * 
 * Big endian format:
 *
 * Byte:   [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]
 * Data:   V_HI  V_LO  I_HI  I_LO  ---   ---   ---   ---
 */

#define VOLTAGE_BYTE_HI      0               /* battery_voltage upper half  */
#define VOLTAGE_BYTE_LO      1               /* battery_voltage lower half  */
#define CURRENT_BYTE_HI      2               /* battery_current upper half  */
#define CURRENT_BYTE_LO      3               /* battery_current lower half  */

/* === Direction values === */

#define DIR_FORWARD          0
#define DIR_REVERSE          1               
#define DIR_NEUTRAL          2               /* default 'gear' state!       */

/* === Button configuration === */

#define BTN0_CAN_ID          CAN_ID_DIRECTION
#define BTN0_ACTION          DIR_NEUTRAL

#define BTN1_CAN_ID          CAN_ID_DIRECTION
#define BTN1_ACTION          DIR_FORWARD

#define BTN2_CAN_ID          CAN_ID_DIRECTION
#define BTN2_ACTION          DIR_REVERSE

/* Place holders
 * #define BTN3_CAN_ID          CAN_ID_HAZARD
 * #define BNT3_ACTION          HAZARD_ON     --->  how to disable it later? 
 * #define BTN4_CAN_ID          CAN_ID_BLINK_ARROW
 * #define BTN4_ACTION          BLINK_ARROW_LEFT
 * #define BTN5_CAN_ID          CAN_ID_BLINK_ARROW
 * #define BTN5_ACTION          BLINK_ARROW_RIGHT
 */

/* === Signal for button interrupt === */

#define BUTTON_SIGNAL        SIGUSR1   /* app -> userspace; driver -> kernel*/

/* Plaxe holders
 *
 * #define KILL_CURR_APP        SIGNINT
 * #define KILL_PROCESS         SIGKILL
 * #define GENTLE_KILL_PROC     SIGTERM
 * */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* File descriptors */

static int g_fb_fd =  -1;     /* if (g_fb_fd < 0) { do_this(); } Poxis std  */
static int g_can_fd = -1;
static int g_btn_fd = -1;

/* Framebuffer */

static void *g_fb_mem = NULL; /* nuttx reference --> similar to -1 for fd   */

/* LVGL labels
 * we need a pointers to LVGL widgets 
 * basic type is 'lv_obj_t'.
 *
 * Use it to avoid parsing 'text'.
 *
 * use it to update the display later on, e.g.:
 *      lv_label_set_text(g_speed_label, "80 km/h");
 */

static lv_obj_t *g_speed_label;
static lv_obj_t *g_voltage_label;
static lv_obj_t *g_current_label;
static lv_obj_t *g_direction_label;

/* Vehicle data 
 *
 * real values via socket_can
 * */

static int g_speed     = 0;
static int g_voltage   = 0;
static int g_current   = 0;
static int g_direction = DIR_NEUTRAL;

/* Button flag (set by interrupt) */

/* volatile for avoid optimziation. Maybe it is not needed but,
 * since we are literraly handling with the hardware here (from kernel),
 * such as in a microcontroller, it is might needed...
 * */
static volatile int g_button_pressed = -1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fb_flush
 *
 * Description:
 *   LVGL callback to flush display buffer.
 ****************************************************************************/

/* Private function, no return, LVGL callback 
 *
 *   When you change a label, e.g.:
 *   lv_label_set_text(g_speed_label, "80");
 *
 * LVGL marks the affected area as "dirty" (needs redraw).
 * On the next call to lv_timer_handler(), LVGL renders the new pixels into 
 * the draw buffer.
 *
 * LVGL then calls fb_flush(), saying: "area X,Y to X2,Y2 has been updated".
 * We notify the display driver: "update this region on the physical screen".
 * Finally, we call lv_display_flush_ready() to say: "done, you can continue".
 */

static void fb_flush(lv_display_t *disp,      /* LVGL display */
                     const lv_area_t *area,   /* area to update */
                     uint8_t *px_map)         /* pixel buffer */
{
  /* Struct that NuttX framebuffer driver understands */

  struct fb_area_s fb_area;

  /* Copy X position of top-left corner */
  
  fb_area.x = area->x1;

  /* Copy Y position of top-left corner */
  
  fb_area.y = area->y1;

  /* Calculate width (inclusive limits, hence +1) */
  
  fb_area.w = area->x2 - area->x1 + 1;

  /* Calculate height */
  
  fb_area.h = area->y2 - area->y1 + 1;

  /* Tell driver: "update this region on physical screen" 
   * we just need to update the 'changed' aread. Actually, we are sending
   * 300k every time we update any area, since 320x480, RGB565 --> 300Kbytes
   * */
  
  ioctl(g_fb_fd, FBIO_UPDATE, (unsigned long)&fb_area);

  /* Tell LVGL: "done, you can continue rendering" */
  
  lv_display_flush_ready(disp);
}

/****************************************************************************
 * Name: button_handler
 *
 * Description:
 *   Signal handler for button interrupt.
 *   Called when button is pressed.
 *   we are ignorning the signal number, since we just use 1 signal
 *   and it is for the buttons.
 ****************************************************************************/

/* Signal handles
 * 
 * 1. press the button
 * 2. GPIO pin changes state
 * 3. hardware generates an interrupt
 * 4. the button driver (in the kernel) is triggered
 * 5. The driver 'sees' that evend and trigger the signal handling
 * 6. The driver sends SIGUSR1 to your process
 * 7. The kernel pauses your main loop
 * 8. The kernel calls `button_handler()`
 * 9. The handler executes (at userspace layer)
 * 10. The kernel returns to the main loop where it stopped
 *
 * Place holder:
 * static void multi_handler(int signo, siginfo_t *info, void *context)
 * {
 * if (signo == SIGUSR1)
 * {
 *   trata_botao();
 *  }
 *
 * else if (signo == SIGUSR2)
 * {
 *  trata_sensor()
 *  }
 * }
 */

static void button_handler(int signo, siginfo_t *info, void *context)
{
  btn_buttonset_t buttons;

  /* Read which button was pressed */


 /* POSIX read signature
  * read(g_btn_fd, &buttons, sizeof(buttons))
  *     |          |         |
  *     |          |         | quantos bytes? tamanho de 
  *     |          |         | btn_buttonset_t (4 bytes)
  *     |          |-- onde guardar? no endereço da variável buttons
  *     |-- de onde ler? do device de botões
  */

  if (read(g_btn_fd, &buttons, sizeof(buttons)) == sizeof(buttons))
    {
      if (buttons & (1 << 0))
        {
          g_button_pressed = 0;
        }
      else if (buttons & (1 << 1))
        {
          g_button_pressed = 1;
        }
    }
}

/****************************************************************************
 * Name: send_can_direction
 *
 * Description:
 *   Send direction command via CAN.
 ****************************************************************************/

/* standard posix socketCAN:
 * 16 bytes: 4 do ID + 1 do DLC + 3 padding + 8 de dados)
 * struct can_frame {
 *   canid_t can_id;   --> 32 bits: ID + flags
 *   uint8_t can_dlc;  --> Data Length Code: 0-8
 *   uint8_t data[8];  --> payload
 * };
 */

/* just a recap:                          0x18FEF300   DIR_NEUTRAL
 * */
static void send_can_direction(uint32_t can_id, uint8_t direction)
{
  struct can_frame frame;
                                         /* 0x1FFFFFFF */
  frame.can_id = can_id | CAN_EFF_FLAG;  /* 0x80000000 --> extend_id        */
                                         /* this is why we see 98 rather 18 */
  frame.can_dlc = 8;
  frame.data[0] = direction;
  frame.data[1] = 0;
  frame.data[2] = 0;
  frame.data[3] = 0;
  frame.data[4] = 0;
  frame.data[5] = 0;
  frame.data[6] = 0;
  frame.data[7] = 0;

  if (write(g_can_fd, &frame, sizeof(frame)) == sizeof(frame))
    {
      printf("TX: ID=0x%08lX DIR=%d\n", (unsigned long)can_id, direction);
    }
}

/****************************************************************************
 * Name: process_button
 *
 * Description:
 *   Process button press and send CAN message.
 ****************************************************************************/

static void process_button(int button)
{
  if (button == 0)
    {
      send_can_direction(BTN0_CAN_ID, BTN0_ACTION);
    }
  else if (button == 1)
    {
      send_can_direction(BTN1_CAN_ID, BTN1_ACTION);
    }
  else if (button == 2)
   {
      send_can_direction(BTN1_CAN_ID, BTN2_ACTION);
   }
}

/****************************************************************************
 * Name: process_can_message
 *
 * Description:
 *   Process received CAN message and update vehicle data.
 ****************************************************************************/

static void process_can_message(struct can_frame *frame)
{
  uint32_t id;

  /* Get ID without flags */

  id = frame->can_id & CAN_EFF_MASK;

  /* Speed message */

  if (id == CAN_ID_SPEED)
    {
      g_speed = frame->data[SPEED_BYTE];
      printf("RX: Speed = %d km/h\n", g_speed);
    }

  /* Battery message */

  else if (id == CAN_ID_BATTERY)
    {
      g_voltage = (frame->data[VOLTAGE_BYTE_HI] << 8) |
                   frame->data[VOLTAGE_BYTE_LO];
      g_current = (frame->data[CURRENT_BYTE_HI] << 8) |
                   frame->data[CURRENT_BYTE_LO];
      printf("RX: Voltage = %d.%d V, Current = %d.%d A\n",
             g_voltage / 10, g_voltage % 10,
             g_current / 10, g_current % 10);
    }

  /* Direction message */

  else if (id == CAN_ID_DIRECTION)
    {
      g_direction = frame->data[DIRECTION_BYTE];
      printf("RX: Direction = %d\n", g_direction);
    }
}

/****************************************************************************
 * Name: update_display
 *
 * Description:
 *   Update LVGL labels with current vehicle data.
 ****************************************************************************/

static void update_display(void)
{
  char buf[32];

  /* Speed */

  snprintf(buf, sizeof(buf), "%d km/h", g_speed);
  lv_label_set_text(g_speed_label, buf);

  /* Voltage (value is in 0.1V units) */

  snprintf(buf, sizeof(buf), "%d.%d V", g_voltage / 10, g_voltage % 10);
  lv_label_set_text(g_voltage_label, buf);

  /* Current (value is in 0.1A units) */

  snprintf(buf, sizeof(buf), "%d.%d A", g_current / 10, g_current % 10);
  lv_label_set_text(g_current_label, buf);

  /* Direction */

  if (g_direction == DIR_FORWARD)
    {
      lv_label_set_text(g_direction_label, "FORWARD");
      lv_obj_set_style_text_color(g_direction_label,
                                   lv_color_hex(0x00FF00), 0);
    }
  else if (g_direction == DIR_REVERSE)
    {
      lv_label_set_text(g_direction_label, "REVERSE");
      lv_obj_set_style_text_color(g_direction_label,
                                   lv_color_hex(0xFF0000), 0);
    }
  else
    {
      lv_label_set_text(g_direction_label, "NEUTRAL");
      lv_obj_set_style_text_color(g_direction_label,
                                   lv_color_hex(0xFFFFFF), 0);
    }
}

/****************************************************************************
 * Name: init_framebuffer
 *
 * Description:
 *   Initialize framebuffer device.
 ****************************************************************************/

static int init_framebuffer(void)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  lv_display_t *disp;

  /* Open framebuffer */

  g_fb_fd = open("/dev/fb0", O_RDWR);
  if (g_fb_fd < 0)
    {
      printf("ERROR: Cannot open /dev/fb0\n");
      return -1;
    }

  /* Get info */

  ioctl(g_fb_fd, FBIOGET_VIDEOINFO, &vinfo);
  ioctl(g_fb_fd, FBIOGET_PLANEINFO, &pinfo);

  printf("Display: %dx%d @ %d bpp\n", vinfo.xres, vinfo.yres, pinfo.bpp);

  /* Map framebuffer memory */

  g_fb_mem = mmap(NULL, pinfo.fblen, PROT_READ | PROT_WRITE,
                  MAP_SHARED, g_fb_fd, 0);
  if (g_fb_mem == MAP_FAILED)
    {
      printf("ERROR: mmap failed\n");
      return -1;
    }

  /* Initialize LVGL */

  lv_init();

  /* Create display */

  disp = lv_display_create(vinfo.xres, vinfo.yres);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, fb_flush);
  lv_display_set_buffers(disp, g_fb_mem, NULL, pinfo.fblen,
                          LV_DISPLAY_RENDER_MODE_DIRECT);

  return 0;
}

/****************************************************************************
 * Name: init_can
 *
 * Description:
 *   Initialize CAN socket.
 ****************************************************************************/

static int init_can(void)
{
  struct sockaddr_can addr;
  struct ifreq ifr;
  int ret;

  /* Bring interface up */

  ret = netlib_ifup("can0");
  if (ret < 0)
    {
      printf("ERROR: netlib_ifup failed\n");
      return -1;
    }

  /* Create socket */

  g_can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (g_can_fd < 0)
    {
      printf("ERROR: Cannot create CAN socket\n");
      return -1;
    }

  /* Get interface index */

  strncpy(ifr.ifr_name, "can0", IFNAMSIZ);
  ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
  if (ifr.ifr_ifindex == 0)
    {
      printf("ERROR: can0 not found\n");
      return -1;
    }

  /* Bind socket */

  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(g_can_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      printf("ERROR: bind failed\n");
      return -1;
    }

  printf("CAN: Initialized on can0\n");
  return 0;
}

/****************************************************************************
 * Name: init_buttons
 *
 * Description:
 *   Initialize buttons with interrupt.
 ****************************************************************************/

static int init_buttons(void)
{
  struct sigaction sa;
  struct btn_notify_s notify;
  btn_buttonset_t supported;

  /* Open buttons */

  g_btn_fd = open("/dev/buttons", O_RDONLY);
  if (g_btn_fd < 0)
    {
      printf("ERROR: Cannot open /dev/buttons\n");
      return -1;
    }

  /* Get supported buttons */

  if (ioctl(g_btn_fd, BTNIOC_SUPPORTED, &supported) < 0)
    {
      printf("ERROR: BTNIOC_SUPPORTED failed\n");
      return -1;
    }

  printf("Buttons: Supported mask = 0x%02X\n", (unsigned)supported);

  /* Setup signal handler */

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = button_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);

  if (sigaction(BUTTON_SIGNAL, &sa, NULL) < 0)
    {
      printf("ERROR: sigaction failed\n");
      return -1;
    }

  /* Register for button press notification */

  notify.bn_press = supported;
  notify.bn_release = 0;
  notify.bn_event.sigev_notify = SIGEV_SIGNAL;
  notify.bn_event.sigev_signo = BUTTON_SIGNAL;

  if (ioctl(g_btn_fd, BTNIOC_REGISTER, &notify) < 0)
    {
      printf("ERROR: BTNIOC_REGISTER failed\n");
      return -1;
    }

  printf("Buttons: Interrupt configured\n");
  return 0;
}

/****************************************************************************
 * Name: create_ui
 *
 * Description:
 *   Create LVGL user interface.
 ****************************************************************************/

static void create_ui(void)
{
  lv_obj_t *screen;
  lv_obj_t *container;

  /* Get screen */

  screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

  /* === Speed (top center) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 200, 100);
  lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_bg_color(container, lv_color_hex(0x1a1a1a), 0);
  lv_obj_set_style_border_color(container, lv_color_hex(0x00ff00), 0);
  lv_obj_set_style_border_width(container, 2, 0);

  g_speed_label = lv_label_create(container);
  lv_label_set_text(g_speed_label, "0 km/h");
  lv_obj_set_style_text_color(g_speed_label, lv_color_hex(0x00ff00), 0);
  lv_obj_set_style_text_font(g_speed_label, &lv_font_montserrat_32, 0);
  lv_obj_center(g_speed_label);

  /* === Voltage (left) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 140, 60);
  lv_obj_align(container, LV_ALIGN_LEFT_MID, 20, -30);
  lv_obj_set_style_bg_color(container, lv_color_hex(0x1a1a1a), 0);
  lv_obj_set_style_border_color(container, lv_color_hex(0xffaa00), 0);
  lv_obj_set_style_border_width(container, 2, 0);

  g_voltage_label = lv_label_create(container);
  lv_label_set_text(g_voltage_label, "0.0 V");
  lv_obj_set_style_text_color(g_voltage_label, lv_color_hex(0xffaa00), 0);
  lv_obj_set_style_text_font(g_voltage_label, &lv_font_montserrat_20, 0);
  lv_obj_center(g_voltage_label);

  /* === Current (right) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 140, 60);
  lv_obj_align(container, LV_ALIGN_RIGHT_MID, -20, -30);
  lv_obj_set_style_bg_color(container, lv_color_hex(0x1a1a1a), 0);
  lv_obj_set_style_border_color(container, lv_color_hex(0x00aaff), 0);
  lv_obj_set_style_border_width(container, 2, 0);

  g_current_label = lv_label_create(container);
  lv_label_set_text(g_current_label, "0.0 A");
  lv_obj_set_style_text_color(g_current_label, lv_color_hex(0x00aaff), 0);
  lv_obj_set_style_text_font(g_current_label, &lv_font_montserrat_20, 0);
  lv_obj_center(g_current_label);

  /* === Direction (bottom center) === */

  container = lv_obj_create(screen);
  lv_obj_set_size(container, 200, 60);
  lv_obj_align(container, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_set_style_bg_color(container, lv_color_hex(0x1a1a1a), 0);
  lv_obj_set_style_border_color(container, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_border_width(container, 2, 0);

  g_direction_label = lv_label_create(container);
  lv_label_set_text(g_direction_label, "NEUTRAL");
  lv_obj_set_style_text_color(g_direction_label, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(g_direction_label, &lv_font_montserrat_24, 0);
  lv_obj_center(g_direction_label);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Application entry point.
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct can_frame frame;
  struct timeval tv;
  fd_set readfds;
  int ret;

  /* Welcome message */

  printf("\n");
  printf("================================\n");
  printf("  Simple BEV Cluster\n");
  printf("================================\n");
  printf("\n");

  /* Initialize framebuffer + LVGL */

  printf("Initializing display...\n");
  if (init_framebuffer() < 0)
    {
      return 1;
    }

  /* Initialize CAN */

  printf("Initializing CAN...\n");
  if (init_can() < 0)
    {
      return 1;
    }

  /* Initialize buttons */

  printf("Initializing buttons...\n");
  if (init_buttons() < 0)
    {
      printf("WARNING: Buttons not available\n");
    }

  /* Create UI */

  printf("Creating UI...\n");
  create_ui();

  /* First display update */

  update_display();

  /* Instructions */

  printf("\n");
  printf("Ready!\n");
  printf("\n");
  printf("CAN RX IDs:\n");
  printf("  Speed:     0x%08lX (byte 0 = km/h)\n",
         (unsigned long)CAN_ID_SPEED);
  printf("  Battery:   0x%08lX (bytes 0-1=V, 2-3=A)\n",
         (unsigned long)CAN_ID_BATTERY);
  printf("  Direction: 0x%08lX (byte 0: 0=FWD, 1=REV, 2=NEU)\n",
         (unsigned long)CAN_ID_DIRECTION);
  printf("\n");
  printf("Test commands:\n");
  printf("  cansend can0 18FEF100#5000000000000000  (80 km/h)\n");
  printf("  cansend can0 18FEF200#01F400C8000000    (50.0V, 20.0A)\n");
  printf("  cansend can0 18FEF300#0000000000000000  (FORWARD)\n");
  printf("  cansend can0 18FEF300#0100000000000000  (REVERSE)\n");
  printf("\n");

  /* Main loop */

  while (1)
    {
      /* Check button press (from interrupt) */

      if (g_button_pressed >= 0)
        {
          process_button(g_button_pressed);
          g_button_pressed = -1;
        }

      /* Wait for CAN message with timeout */

      FD_ZERO(&readfds);
      FD_SET(g_can_fd, &readfds);
      tv.tv_sec = 0;
      tv.tv_usec = 10000;  /* 10ms */

      ret = select(g_can_fd + 1, &readfds, NULL, NULL, &tv);
      if (ret > 0 && FD_ISSET(g_can_fd, &readfds))
        {
          /* Read CAN message */

          ret = read(g_can_fd, &frame, sizeof(frame));
          if (ret == sizeof(frame))
            {
              process_can_message(&frame);
              update_display();
            }
        }

      /* Update LVGL */

      lv_tick_inc(10);
      lv_timer_handler();
    }

  /* Cleanup (never reached) */

  close(g_can_fd);
  close(g_btn_fd);
  close(g_fb_fd);

  return 0;
}
