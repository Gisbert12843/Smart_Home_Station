#pragma once
#include "wholeinclude.h"

#include "ui/ui.h"

#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_ili9488.h"
#include "tft/touch.h"
#include "utils/helper_functions.h"
#include "driver/i2c_master.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The pixel number in horizontal and vertical
// #define EXAMPLE_LCD_H_RES 320
// #define EXAMPLE_LCD_V_RES 480
#define EXAMPLE_LCD_H_RES 480
#define EXAMPLE_LCD_V_RES 320
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

#define EXAMPLE_LVGL_TICK_PERIOD_MS 10

#define BUFFER_SIZE EXAMPLE_LCD_H_RES *EXAMPLE_LCD_V_RES / 30

extern void ui_init(lv_obj_t *scr);

// static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
// static lv_disp_drv_t disp_drv;      // contains callback functions

// static lv_color16_t *buf1, *buf2;

void tft_init(void);
