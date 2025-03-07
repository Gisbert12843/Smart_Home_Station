#include "display/display_driver.h"

esp_err_t init_display_driver(esp_lcd_panel_io_handle_t io_handle, const esp_lcd_panel_dev_config_t *panel_dev_config, size_t buffer_size, esp_lcd_panel_handle_t *panel_handle)
{
    return esp_lcd_new_panel_ili9488(io_handle, panel_dev_config, buffer_size, panel_handle);
}

esp_err_t init_touch_driver(const esp_lcd_panel_io_handle_t io_handle, const esp_lcd_touch_config_t *touch_config, esp_lcd_touch_handle_t *touch_handle)
{
    return esp_lcd_touch_new_i2c_gt911(io_handle, touch_config, touch_handle);
}
