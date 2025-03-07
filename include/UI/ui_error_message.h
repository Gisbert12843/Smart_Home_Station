#pragma once
// #include "ui_elements.h"

// // represents a toggle button for each MQTT topic
// class UI_Error_Message : public UI_Element
// {
// private:
//     lv_obj_t *button_obj;
//     lv_obj_t *status_circle;
//     lv_obj_t *label_obj;
//     const std::string topic;
//     bool state = false;
//     std::string id;

//     UI_Error_Message(lv_obj_t *button, lv_obj_t *circle, lv_obj_t *label, const std::string &topic, bool p_state)
//         : button_obj(button), status_circle(circle), label_obj(label), topic(topic), state(p_state), id(std::string("UI_Error_Message_") + topic) {} // Assuming topic is unique enough to serve as an ID
// public:
//     ~UI_Error_Message();
//     UI_Element_Type getType() const override;

//     std::string getId() const override;

//     lv_obj_t *get_button_obj() const;

//     lv_obj_t *get_status_circle_obj() const;

//     lv_obj_t *get_label_obj() const;

//     std::string get_topic() const;

//     bool get_state() const;

//     void set_state(bool new_state);

//     static std::shared_ptr<UI_Error_Message> create_button(lv_obj_t *parent, const std::string &topic, bool state = false);
// };
