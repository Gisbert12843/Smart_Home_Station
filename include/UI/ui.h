/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "wholeinclude.h"
#include "ui/color_palette.h"
#include <map>
#include "ui/ui_elements.h"

extern esp_event_loop_handle_t ui_event_loop;
extern EventGroupHandle_t ui_event_group;
extern EventBits_t UI_STATUSBAR_UPDATE_BIT;
extern EventBits_t UI_RELOAD_BIT;
extern std::recursive_mutex lvgl_mutex;

// Define an event base
ESP_EVENT_DEFINE_BASE(CUSTOM_EVENT_BASE);

typedef struct
{
    lv_obj_t *label = nullptr; // what label to update
    int remaining_time = 120;  // how much time is left
} timer_data_t;

// spawns and updates the overlay and all elements for the dpp process
// the qr code must be scanned by the users phone which is connected to the target wifi network
// when the timer runs out the dpp process is cancelled and the overlay despawns
class DPP_QR_Code
{
private:
    inline static bool initialized = false;
    inline static std::recursive_mutex DPP_QR_Code_mutex;
    inline static lv_timer_t *qr_code_timer = nullptr;
    inline static lv_obj_t *qr_screen_wrapper = nullptr;
    inline static lv_obj_t *qr_code_timer_label = nullptr;
    inline static int qr_code_seconds_left = 120;

    // private con- and destructor
    DPP_QR_Code();
    ~DPP_QR_Code();

    // Deleted copy constructor and assignment operators
    DPP_QR_Code(const DPP_QR_Code &) = delete;
    DPP_QR_Code &operator=(const DPP_QR_Code &) = delete;
    DPP_QR_Code(DPP_QR_Code &&) = delete;
    DPP_QR_Code &operator=(DPP_QR_Code &&) = delete;

    static void qr_code_timer_cb(lv_timer_t *timer);

public:
    // Constructor, safe to call multiple times
    // called by dpp_enrollee_event_cb() which in turn is invoked by esp_supp_dpp_init()
    static void create_QR_Code_from_url(std::string url);
    static std::recursive_mutex &get_mutex();
    // Destructor, safe to call multiple times
    static void delete_QR_Code_Overlay();

    static void start_timer(lv_obj_t *label = qr_code_timer_label, int seconds_to_run = 120);

    static void show_QR_Code();
    static void hide_QR_Code();
};

// Define event IDs
typedef enum
{
    Event_Connected,
    Event_Disconnected,
    Event_Subbed,
    Event_Unsubbed,
    Undefined
} custom_event_id_t;

// Manages LVGL UI elements WIP
class ui_elements
{
private:
    static inline std::map<std::string, std::shared_ptr<UI_Element>> ui_elements_map = {};

    // Private constructor for Singleton
    ui_elements() {};

    // Prevent copying
    ui_elements(const ui_elements &) = delete;
    ui_elements &operator=(const ui_elements &) = delete;

public:
    static std::recursive_mutex &getMutex();

    static std::map<std::string, std::shared_ptr<UI_Element>> getVectorShallowCopy();

    static std::shared_ptr<UI_Element> get_element(std::string element_id);

    // this does not delete the elements, just removes them from the map
    static void updateMap(std::map<std::string, std::shared_ptr<UI_Element>> &topics);

    static void add_element(std::string element_id, std::shared_ptr<UI_Element> element);

    static void remove_element(std::shared_ptr<UI_Element> element);

    static void clear_elements();
};

void main_view(void *params);
void ui_init(lv_obj_t *scr = nullptr);
// void show_QR_Code(timer_data_t *timer_data, std::string url = "https://lvgl.io");
// void show_add_new_client_overlay();

void lvgl_manager_bg_task(void *arg);

// should be called when an mqtt_message is received on a subscribed topic
// WIP! Currently only works for buttons
void topic_object_update(std::string topic, std::string message);
bool lv_color_compare(lv_color_t color1, lv_color_t color2);

// redraws the buttons/labels for each unique topic in the s_topics map onto the screen
// should be triggered by setting the UI_RELOAD_BIT in the ui_event_group whenever an Sub/Unsub event occurs and a topic is added/removed
// should not be called when a topic is simply incremented/decremented (use topic_object_update in this case)
void topics_reload(lv_obj_t *scr);