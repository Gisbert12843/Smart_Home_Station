#pragma once
#include "ui_elements.h"
#include "display/display.h"

//
class UI_Popup : public UI_Element
{
private:
    lv_obj_t *popup_obj;
    std::string id;

    UI_Popup(lv_obj_t *popup, std::string &id)
        : popup_obj(popup), id(std::string("UI_Popup_") + id) {} // Assuming topic is unique enough to serve as an ID

public:
    UI_Element_Type getType() const override;

    std::string getId() const override;

    lv_obj_t *get_popup_obj() const;

    ~UI_Popup();

    void hide_popup();

    void show_popup();

    static std::shared_ptr<UI_Popup> create_popup(std::string id, int left_top_pos_x = 0, int left_top_pos_y = 0, int width = LCD_H_RES, int height = LCD_V_RES);
};
