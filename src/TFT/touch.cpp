#include "tft/touch.h"
#include "driver/i2c_master.h"
#include "espressif_esp_lcd_touch_gt911_1.1.0/include/esp_lcd_touch_gt911.h"
#include "tft/tft.h"

void touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev_drv);
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    assert(tp);

    /* Read data from touch controller into memory */
    esp_lcd_touch_read_data(tp);

    /* Read data from touch controller */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0)
    {
        data->point.x = -1*((touchpad_x[0] - EXAMPLE_LCD_H_RES)); //This is to turn the 0/0 point from the top/right into the top/left corner (landscape mode)
        data->point.y = touchpad_y[0];

        // ESP_LOGD("touchpad_read()", "Touchpad pressed at x: %d, y: %d", data->point.x, data->point.y);
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}