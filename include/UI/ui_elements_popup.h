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

    /**
     * @brief Creates and initializes a new UI_Popup instance.
     *
     * This function creates a full-screen LVGL object configured as a clickable overlay,
     * sets its dimensions according to the LCD resolution, applies a semi-transparent black
     * background, and initially hides it. It also ensures thread safety by locking the LVGL mutex.
     *
     * @param id              A unique identifier for the popup.
     * @param left_top_pos_x  The x-coordinate for the top-left position (unused; the position is set to (0, 0)).
     * @param left_top_pos_y  The y-coordinate for the top-left position (unused; the position is set to (0, 0)).
     * @param width           The defined width for the popup (unused; the size is set to LCD_H_RES).
     * @param height          The defined height for the popup (unused; the size is set to LCD_V_RES).
     *
     * @return std::shared_ptr<UI_Popup> A shared pointer to the newly created UI_Popup instance.
     *
     * @note The parameters left_top_pos_x, left_top_pos_y, width, and height are provided for interface
     *       consistency but the actual values are determined by LCD resolution constants.
     */
    static std::shared_ptr<UI_Popup> create_popup(std::string id, int left_top_pos_x = 0, int left_top_pos_y = 0, int width = LCD_H_RES, int height = LCD_V_RES);
};
