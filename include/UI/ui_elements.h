#pragma once

#include "wholeinclude.h"
#include "lvgl.h"
#include "ui/color_palette.h"

extern std::recursive_mutex lvgl_mutex;

class UI_Element;
class UI_Button;
class UI_PWM_Element;

bool lv_color_compare(lv_color_t color1, lv_color_t color2);

void UI_Button_event_handler(lv_event_t *e);

enum class UI_Element_Type
{
  UI_Button,
  UI_PWM_Element,
  Undefined
};

class UI_Element
{
public:
  virtual ~UI_Element() = default;
  virtual UI_Element_Type getType() const = 0;
  virtual std::string getId() const = 0; // Add a method to get the ID
};

// represents a toggle button for each MQTT topic
class UI_Button : public UI_Element
{
private:
  lv_obj_t *button_obj;
  lv_obj_t *status_circle;
  lv_obj_t *label_obj;
  const std::string topic;
  bool state = false;
  std::string id;

public:
  // Do not use this directly, use create_button instead
  UI_Button(lv_obj_t *button, lv_obj_t *circle, lv_obj_t *label, const std::string &topic, bool p_state)
      : button_obj(button), status_circle(circle), label_obj(label), topic(topic), state(p_state), id(std::string("UI_Button_") + topic) {} // Assuming topic is unique enough to serve as an ID

  UI_Element_Type getType() const override
  {
    return UI_Element_Type::UI_Button;
  }

  std::string getId() const override
  {
    return id;
  }

  lv_obj_t *get_button_obj() const
  {
    return button_obj;
  }

  lv_obj_t *get_status_circle_obj() const
  {
    return status_circle;
  }

  lv_obj_t *get_label_obj() const
  {
    return label_obj;
  }

  std::string get_topic() const
  {
    return topic;
  }

  bool get_state() const
  {
    return state;
  }

  void set_state(bool new_state)
  {
    state = new_state;
    if (state)
    {
      lv_obj_set_style_bg_color(this->get_status_circle_obj(), lv_color_hex(COLOR_MID_GREEN), 0);
      lv_obj_set_style_border_color(this->get_button_obj(), lv_color_hex(COLOR_MID_GREEN), 0);
    }
    else
    {
      lv_obj_set_style_bg_color(this->get_status_circle_obj(), lv_color_hex(COLOR_RED), 0);
      lv_obj_set_style_border_color(this->get_button_obj(), lv_color_hex(COLOR_RED), 0);
    }
  }

  static std::shared_ptr<UI_Button> create_button(lv_obj_t *parent, const std::string &topic, bool state = false);
};

