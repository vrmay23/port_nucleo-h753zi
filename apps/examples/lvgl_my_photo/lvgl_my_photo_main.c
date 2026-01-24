#include <lvgl/lvgl.h>
#include <unistd.h>
#include <stdio.h>

extern const lv_image_dsc_t vo;

int main(void) {
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  
  printf("Init LVGL\n");
  lv_init();
  
  printf("Init display\n");
  lv_nuttx_dsc_init(&info);
  lv_nuttx_init(&info, &result);
  
  if (result.disp == NULL) {
    printf("Display NULL\n");
    return 1;
  }
  
  printf("Create image\n");
  lv_obj_t *img = lv_image_create(lv_screen_active());
  lv_image_set_src(img, &vo);
  lv_obj_center(img);
  
  printf("Loop\n");
  while(1) {
    lv_timer_handler();
    usleep(5000);
  }
}
