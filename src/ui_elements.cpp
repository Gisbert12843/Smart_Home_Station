#include "ui_elements.h"
#include "color_palette.h"
#include "ui.h"
#include "mqtt.h"
#include <string>
#include <memory>
#include "tft.h"

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

void UI_Button_event_handler(lv_event_t *e)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

    // lv_obj_t *button_obj = (lv_obj_t *)lv_event_get_target(e);

    UI_Button *button = (UI_Button *)(lv_event_get_user_data(e));

    std::string topic = lv_label_get_text(lv_obj_get_child_by_type(button->get_button_obj(), 0, &lv_label_class));

    ESP_LOGD("button_event_handler()", "Button: \"%s\" was clicked", topic.c_str());

    lv_color_t current_color = lv_obj_get_style_bg_color(button->get_status_circle_obj(), 0);
    if (lv_color_compare(current_color, lv_color_hex(COLOR_MID_GREEN)))
    {
        ESP_LOGD("button_event_handler()", "Button: \"%s\" will be turned off", topic.c_str());
        send_mqtt_message(topic, "LED_OFF");
        topic_object_update(topic, "LED_OFF");
    }
    else if (lv_color_compare(current_color, lv_color_hex(COLOR_RED)))
    {
        ESP_LOGD("button_event_handler()", "Button: \"%s\" will be turned on", topic.c_str());
        send_mqtt_message(topic, "LED_ON");
        topic_object_update(topic, "LED_ON");
    }
    else
    {
        ESP_LOGE("button_event_handler()", "Unknown color/state for button: \"%s\"", topic.c_str());
    }
}

std::shared_ptr<UI_Button> UI_Button::create_button(lv_obj_t *parent, const std::string &topic, bool state)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_MIDGREY), 0);
    lv_obj_set_style_pad_hor(button, 10, 0);
    lv_obj_set_size(button, 86, 86);
    if (state)
    {
        lv_obj_set_style_border_color(button, lv_color_hex(COLOR_MID_GREEN), 0);
    }
    else
    {
        lv_obj_set_style_border_color(button, lv_color_hex(COLOR_RED), 0);
    }
    lv_obj_set_style_border_width(button, 2, 0);

    lv_obj_t *circle = lv_obj_create(button);
    lv_obj_set_scrollbar_mode(circle, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(circle, 15, 15);
    lv_obj_set_pos(circle, 55, 4);
    if (state)
    {
        lv_obj_set_style_bg_color(circle, lv_color_hex(COLOR_MID_GREEN), 0);
    }
    else
    {
        lv_obj_set_style_bg_color(circle, lv_color_hex(COLOR_RED), 0);
    }
    lv_obj_set_style_border_width(circle, 0, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, topic.c_str());
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_BLACK), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    if (topic.length() > 9)
        lv_obj_set_style_anim_duration(label, (topic.length()) * 200, 0);

    lv_obj_set_width(label, 75);

    auto new_button = std::shared_ptr<UI_Button>(std::make_shared<UI_Button>(button, circle, label, topic, state));
    lv_obj_add_event_cb(button, UI_Button_event_handler, LV_EVENT_CLICKED, &(*new_button));
    return new_button;
}


std::shared_ptr<lv_obj_t *> create_new_global_overlay()
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    return std::make_shared<lv_obj_t *>(overlay);
}

