#include "ui/ui_elements_popup.h"
#include "ui/ui.h"
#include "tft/tft.h"
#include "esp_log.h"
#include "mqtt/mqtt.h"

UI_Element_Type UI_Popup::getType() const
{
    return UI_Element_Type::UI_Popup;
}

std::string UI_Popup::getId() const
{
    return id;
}

lv_obj_t *UI_Popup::get_popup_obj() const
{
    return popup_obj;
}
UI_Popup::~UI_Popup()
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_obj_del(popup_obj);
}

void UI_Popup::hide_popup()
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_obj_add_flag(popup_obj, LV_OBJ_FLAG_HIDDEN);
}

void UI_Popup::show_popup()
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_obj_clear_flag(popup_obj, LV_OBJ_FLAG_HIDDEN);
}

std::shared_ptr<UI_Popup> UI_Popup::create_popup(std::string id, int left_top_pos_x, int left_top_pos_y, int width, int height)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

    lv_obj_t *popup_obj = lv_obj_create(lv_disp_get_scr_act(NULL)); // Create a full screen cover to display the QR code
    lv_obj_set_size(popup_obj, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_add_flag(popup_obj, LV_OBJ_FLAG_HIDDEN); // Hide the object for now
    lv_obj_set_pos(popup_obj, 0, 0);
    lv_obj_set_style_bg_color(popup_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(popup_obj, LV_OPA_70, 0); // Adjust opacity as needed
    lv_obj_add_flag(popup_obj, LV_OBJ_FLAG_CLICKABLE);

    return std::shared_ptr<UI_Popup>(new UI_Popup(popup_obj, id));
}
