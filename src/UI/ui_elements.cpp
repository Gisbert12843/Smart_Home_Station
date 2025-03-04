#include "ui/ui_elements.h"
#include "ui/color_palette.h"
#include "ui/ui.h"
#include "mqtt/mqtt.h"
#include <string>
#include <memory>
#include "display/display.h"

#include "esp_log.h"

bool lv_color_compare(lv_color_t color1, lv_color_t color2)
{
    // Extract RGB components from each color

    uint8_t red1 = (color1).red;
    uint8_t green1 = (color1).green;
    uint8_t blue1 = (color1).blue;

    uint8_t red2 = (color2).red;
    uint8_t green2 = (color2).green;
    uint8_t blue2 = (color2).blue;

    ESP_LOGD("lv_color_compare()", "Color1: R: %d, G: %d, B: %d", red1, green1, blue1);
    ESP_LOGD("lv_color_compare()", "Color2: R: %d, G: %d, B: %d", red2, green2, blue2);

    // Compare each component
    if (red1 == red2 && green1 == green2 && blue1 == blue2)
    {
        return true; // Colors are the same
    }
    else
    {
        return false; // Colors are different
    }
}
