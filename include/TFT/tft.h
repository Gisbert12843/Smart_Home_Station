#pragma once

#include "ui/ui.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_ili9488.h"
#include "tft/touch.h"
#include "utils/helper_functions.h"
#include "driver/i2c_master.h"

#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

#define EXAMPLE_LVGL_TICK_PERIOD_MS 10

#define BUFFER_SIZE EXAMPLE_LCD_H_RES *EXAMPLE_LCD_V_RES / 30

extern void ui_init(lv_obj_t *scr);

void tft_init(void);
