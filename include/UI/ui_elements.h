#pragma once

#include "lvgl.h"
#include "ui/color_palette.h"
#include <string>
#include <memory>
#include <mutex>

extern std::recursive_mutex lvgl_mutex;

bool lv_color_compare(lv_color_t color1, lv_color_t color2);

enum class UI_Element_Type
{
    Undefined,
    UI_Button,
    UI_Popup,
    UI_PWM_Element,
};

/*!
    virtual class for UI elements
 */
class UI_Element
{
private:
    std::string id;
public:
    virtual ~UI_Element() = default;
    virtual UI_Element_Type get_type() const = 0;
    const std::string &get_id() const { return id; }
};
