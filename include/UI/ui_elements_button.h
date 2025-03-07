#pragma once
#include "ui_elements.h"

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
    UI_Button(lv_obj_t *parent, const std::string &topic, bool state = false);

    ~UI_Button();
    UI_Element_Type get_type() const override;

    lv_obj_t *get_button_obj() const;

    lv_obj_t *get_status_circle_obj() const;

    lv_obj_t *get_label_obj() const;

    std::string get_topic() const;

    bool get_state() const;

    void set_state(bool new_state);

    static std::shared_ptr<UI_Button> create_button(lv_obj_t *parent, const std::string &topic, bool state = false);
};
