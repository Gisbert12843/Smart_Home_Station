#include "ui/ui_elements_button.h"
#include "ui/ui.h"
#include "display/display.h"
#include "esp_log.h"
#include "mqtt/mqtt.h"

static void UI_Button_event_handler(lv_event_t *e)
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

UI_Button::UI_Button(lv_obj_t *parent, const std::string &topic, bool state) : topic(topic), state(state), id(std::string("UI_Button_") + topic)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

    button_obj = lv_btn_create(parent);
    lv_obj_set_style_bg_color(button_obj, lv_color_hex(COLOR_MIDGREY), 0);
    lv_obj_set_style_pad_hor(button_obj, 10, 0);
    lv_obj_set_size(button_obj, 86, 86);
    if (state)
    {
        lv_obj_set_style_border_color(button_obj, lv_color_hex(COLOR_MID_GREEN), 0);
    }
    else
    {
        lv_obj_set_style_border_color(button_obj, lv_color_hex(COLOR_RED), 0);
    }
    lv_obj_set_style_border_width(button_obj, 2, 0);

    status_circle = lv_obj_create(button_obj);
    lv_obj_set_scrollbar_mode(status_circle, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(status_circle, 15, 15);
    lv_obj_set_pos(status_circle, 55, 4);
    if (state)
    {
        lv_obj_set_style_bg_color(status_circle, lv_color_hex(COLOR_MID_GREEN), 0);
    }
    else
    {
        lv_obj_set_style_bg_color(status_circle, lv_color_hex(COLOR_RED), 0);
    }
    lv_obj_set_style_border_width(status_circle, 0, 0);
    lv_obj_set_style_radius(status_circle, LV_RADIUS_CIRCLE, 0);

    label_obj = lv_label_create(button_obj);
    lv_label_set_text(label_obj, topic.c_str());
    lv_obj_center(label_obj);
    lv_obj_set_style_text_font(label_obj, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(COLOR_BLACK), 0);
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    if (topic.length() > 9)
        lv_obj_set_style_anim_duration(label_obj, (topic.length()) * 200, 0);

    lv_obj_set_width(label_obj, 75);

    lv_obj_add_event_cb(button_obj, UI_Button_event_handler, LV_EVENT_CLICKED, this);
}

UI_Button::~UI_Button()
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_obj_del(button_obj);
}

UI_Element_Type UI_Button::get_type() const
{
    return UI_Element_Type::UI_Button;
}

lv_obj_t *UI_Button::get_button_obj() const
{
    return button_obj;
}

lv_obj_t *UI_Button::get_status_circle_obj() const
{
    return status_circle;
}

lv_obj_t *UI_Button::get_label_obj() const
{
    return label_obj;
}

std::string UI_Button::get_topic() const
{
    return topic;
}

bool UI_Button::get_state() const
{
    return state;
}

void UI_Button::set_state(bool new_state)
{
    state = new_state;
    if (state)
    {
        lv_obj_set_style_bg_color(this->get_status_circle_obj(), lv_color_hex(COLOR_MID_GREEN), 0);
        lv_obj_set_style_border_color((this->get_button_obj()), lv_color_hex(COLOR_MID_GREEN), 0);
    }
    else
    {
        lv_obj_set_style_bg_color(this->get_status_circle_obj(), lv_color_hex(COLOR_RED), 0);
        lv_obj_set_style_border_color(this->get_button_obj(), lv_color_hex(COLOR_RED), 0);
    }
}

std::shared_ptr<UI_Button> UI_Button::create_button(lv_obj_t *parent, const std::string &topic, bool state)
{
    return std::shared_ptr<UI_Button>(std::make_shared<UI_Button>(parent, topic, state));
}
