#include "ui/ui_elements_popup.h"

#include "ui/ui.h"
#include "display/display.h"
#include "esp_log.h"
#include "mqtt/mqtt.h"

UI_Popup::UI_Popup(std::string id, int left_top_pos_x, int left_top_pos_y, int width, int height) : id(std::string("UI_Button_") + id)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

    popup_obj = lv_obj_create(lv_disp_get_scr_act(NULL)); // Create a full screen cover to display the QR code
    lv_obj_set_size(popup_obj, width, height);
    lv_obj_add_flag(popup_obj, LV_OBJ_FLAG_HIDDEN); // Hide the object for now
    lv_obj_set_pos(popup_obj, 0, 0);
    lv_obj_set_style_bg_color(popup_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(popup_obj, LV_OPA_70, 0); // Adjust opacity as needed
    lv_obj_add_flag(popup_obj, LV_OBJ_FLAG_CLICKABLE);
}

UI_Popup::~UI_Popup()
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_obj_del(popup_obj);
}

UI_Element_Type UI_Popup::get_type() const
{
    return UI_Element_Type::UI_Popup;
}

lv_obj_t *UI_Popup::get_popup_obj() const
{
    return popup_obj;
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
    return std::shared_ptr<UI_Popup>(std::make_shared<UI_Popup>(id, left_top_pos_x, left_top_pos_y, width, height));
}
