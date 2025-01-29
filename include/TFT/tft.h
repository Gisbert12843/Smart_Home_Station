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
static lv_display_t *disp;

// static lv_color16_t *buf1, *buf2;

static esp_lcd_touch_handle_t tp; // LCD touch handle

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp_driver = (lv_display_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void lvgl_flush_cb(lv_display_t *drv, const lv_area_t *area, uint8_t *color_map)
{
    esp_lcd_panel_draw_bitmap((esp_lcd_panel_handle_t)lv_display_get_user_data(drv), area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

static void lvgl_tick_increment_cb(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

void tft_init(void);
