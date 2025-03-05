#pragma once

#include "lvgl/lvgl.h"
#include "display/display_driver.h"

#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

#define LVGL_TICK_PERIOD_MS 10



//driver specific 
extern esp_err_t init_display_driver(esp_lcd_panel_io_handle_t io_handle, const esp_lcd_panel_dev_config_t *panel_dev_config, size_t buffer_size, esp_lcd_panel_handle_t *panel_handle);
extern esp_err_t init_touch_driver(const esp_lcd_panel_io_handle_t io_handle, const esp_lcd_touch_config_t *touch_config, esp_lcd_touch_handle_t *touch_handle);

//callback function for a registered touch event
void touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data);

esp_err_t init_i8080();

