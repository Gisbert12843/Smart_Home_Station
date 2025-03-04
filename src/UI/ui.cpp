#include "ui/ui.h"
#include "ui/ui_elements.h"
#include "UI/ui_elements_button.h"
#include "mqtt/mqtt.h"
#include "utils/helper_functions.h"
#include "WIFI/WiFi_Functions.h"
#include "mqtt/mqtt_server.h"
#include "display/display.h"

#define status_bar_height 37
#define main_view_height EXAMPLE_LCD_V_RES - status_bar_height

extern std::recursive_mutex mqtt_server_mutex; // mqtt_server.cpp

esp_event_loop_handle_t ui_event_loop;
EventGroupHandle_t ui_event_group = xEventGroupCreate();
EventBits_t UI_STATUSBAR_UPDATE_BIT = BIT0;
EventBits_t UI_RELOAD_BIT = BIT1;

std::recursive_mutex lvgl_mutex;

void lvgl_manager_bg_task(void *arg)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
        lv_task_handler();
    }
    vTaskDelete(NULL);
}

void topic_object_update(std::string topic, std::string message)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    std::lock_guard<std::recursive_mutex> lock2(mqtt_server_mutex);

    ESP_LOGI("topic_object_update()", "Topic: \"%s\"", topic.c_str());
    ESP_LOGI("topic_object_update()", "Message: \"%s\"", message.c_str());

    UI_Button *button = (UI_Button *)(ui_elements::getVectorShallowCopy()[topic].get());
    if (button != nullptr)
    {
        // the button obj only has one child, which is the label
        // we then set the text of the label to the message received
        lv_label_set_text(lv_obj_get_child_by_type(button->get_button_obj(), 0, &lv_label_class), topic.c_str());
        if (message == "LED_ON")
        {
            button->set_state(true);
            s_topics[topic].second = message;
            ESP_LOGI("topic_object_update()", "Updated Button: \"%s\" from Value: \"%s\" to Value: \"%s\"", topic.c_str(), s_topics[topic].second.c_str(), message.c_str());
        }
        else if (message == "LED_OFF")
        {
            button->set_state(false);
            s_topics[topic].second = message;

            ESP_LOGI("topic_object_update()", "Updated Button: \"%s\" from Value: \"%s\" to Value: \"%s\"", topic.c_str(), s_topics[topic].second.c_str(), message.c_str());
        }
        else
        {
            ESP_LOGE("topic_object_update()", "Unknown message: \"%s\" for topic: \"%s\"", message.c_str(), topic.c_str());
        }

        for (auto &topic : s_topics)
        {
            ESP_LOGI("topic_object_update()", "Topic: \"%s\" Sub_Count \"%d\" Value: \"%s\"", topic.first.c_str(), topic.second.first, topic.second.second.c_str());
        }
    }
    else
    {
        ESP_LOGE("topic_object_update()", "Object found for topic: \"%s\" was nullptr.", topic.c_str());
    }
}
void topics_reload(lv_obj_t *scr)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

    ESP_LOGI("topics_reload()", "Updating topics");

// std::vector<int32_t> col_dsc = {86, 86, 86, LV_GRID_TEMPLATE_LAST};
// std::vector<int32_t> row_dsc = {86, 86, 86, 86, LV_GRID_TEMPLATE_LAST};
#define ITEM_COUNT_PER_ROW 4
#define ROW_COUNT_PER_COL 3, ,

    static int32_t col_dsc[] = {LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc[] = {LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_set_size(grid, EXAMPLE_LCD_H_RES, main_view_height);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(grid);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_hor(grid, 30, 0);
    lv_obj_set_style_pad_ver(grid, 10, 0);
    // To prevent an uneccessary long lock, we need to copy the s_topics map to a temporary map
    // We have to lock the mutex to prevent the map from being modified by the Brokers Eventhandler while we are copying it,

    std::lock_guard<std::recursive_mutex> lock2(mqtt_server_mutex);
    std::map<std::string, std::pair<int, std::string>> temp_topic_map = s_topics;
    for (auto &topic : s_topics)
    {
        ESP_LOGI("topics_reload()", "Topic: \"%s\" Sub_Count \"%d\" Value: \"%s\"", topic.first.c_str(), topic.second.first, topic.second.second.c_str());
    }
    for (auto &topic : temp_topic_map)
    {
        ESP_LOGI("topics_reload()", "Topic: \"%s\" Sub_Count \"%d\" Value: \"%s\"", topic.first.c_str(), topic.second.first, topic.second.second.c_str());
    }
    lock2.~lock_guard();

    std::map<std::string, std::shared_ptr<UI_Element>> temp_ui_obj_map = {};

    uint32_t i = 0;

    for (auto topic = temp_topic_map.begin(); topic != temp_topic_map.end(); topic++, i++)
    {
        uint8_t col = (i % 4) * 2; // Column indices are 0, 2, 4
        uint8_t row = (i / 4) * 2; // Row indices are 0, 2, 4, 6

        std::shared_ptr<UI_Button> button;

        bool button_option;

        if (topic->second.second == "LED_ON")
        {
            button_option = true;
            ESP_LOGI("topics_reload()", "Button: \"%s\" is ON", (*topic).first.c_str());
        }
        else if (topic->second.second == "LED_OFF")
        {
            button_option = false;
            ESP_LOGI("topics_reload()", "Button: \"%s\" is OFF", (*topic).first.c_str());
        }
        else
        {
            ESP_LOGE("topics_reload()", "Unknown state for Button: \"%s\"", (*topic).first.c_str());
            button_option = false;
        }

        button = UI_Button::create_button(grid, (*topic).first, button_option);
        // button = UI_Button::create_button(grid, (*topic).first, (*topic).second.second == "LED_ON" ? true : (*topic).second.second == "LED_OFF" ? false
        //                                                                                                                                         : false);

        lv_obj_set_grid_cell(button.get()->get_button_obj(), LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);

        ESP_LOGI("topics_reload()", "Created Button: \"%s\"", (*topic).first.c_str());

        temp_ui_obj_map.insert({(*topic).first, button});

        ESP_LOGI("topics_reload()", "Added Button: \"%s\" to temp_ui_obj_map", (*topic).first.c_str());
    }
    ui_elements::updateMap(temp_ui_obj_map);

    ESP_LOGI("topics_reload()", "Finished updating topics");
}
void main_view(void *params)
{
    while (1)
    {
        std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

        static lv_obj_t *main_view_screen = nullptr;
        if (main_view_screen != nullptr)
        {
            lv_obj_clean(main_view_screen);
        }
        else
        {
            main_view_screen = lv_obj_create(lv_disp_get_scr_act(NULL));
            lv_obj_set_size(main_view_screen, EXAMPLE_LCD_H_RES, main_view_height);
            lv_obj_set_pos(main_view_screen, 0, status_bar_height);
            lv_obj_set_style_bg_color(main_view_screen, lv_color_hex(COLOR_BLUE), 0); // RGB for grey is (192, 192, 192)
            lv_obj_set_style_border_width(main_view_screen, 0, 0);
            lv_obj_set_style_outline_width(main_view_screen, 0, 0);
            lv_obj_set_style_radius(main_view_screen, 0, 0);
            lv_obj_clear_flag(main_view_screen, LV_OBJ_FLAG_SCROLLABLE);
        }

        ESP_LOGI("main_view() -> UI_RELOAD_BIT set", "Starting topics_update()");
        topics_reload(main_view_screen);

        lock.~lock_guard();

        xEventGroupWaitBits(ui_event_group, UI_RELOAD_BIT, true, true, portMAX_DELAY);
    }
    vTaskDelete(NULL);
}

// ui_elements

std::recursive_mutex &ui_elements::getMutex()
{
    return lvgl_mutex;
}
std::map<std::string, std::shared_ptr<UI_Element>> ui_elements::getVectorShallowCopy()
{
    std::map<std::string, std::shared_ptr<UI_Element>> copy;

    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    {
        copy = ui_elements_map;
    }
    return copy;
}
void ui_elements::updateMap(std::map<std::string, std::shared_ptr<UI_Element>> &topics)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    ui_elements_map = topics;
    ESP_LOGD("updateMap()", "Updated ui_elements_map");
}
void ui_elements::add_element(std::string element_id, std::shared_ptr<UI_Element> element)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    {
        ui_elements_map.insert({element_id, element});
    }
}
void ui_elements::remove_element(std::shared_ptr<UI_Element> element)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    {
        for (auto &it : ui_elements_map)
        {
            if (it.second == element)
            {
                ui_elements_map.erase(it.first);
                break;
            }
        }
    }
}
void ui_elements::clear_elements()
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    {
        ui_elements_map.clear();
    }
}
std::shared_ptr<UI_Element> ui_elements::get_element(std::string element_id)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    {
        return ui_elements_map[element_id];
    }
}

// DPP
DPP_QR_Code::~DPP_QR_Code()
{
    ESP_LOGI("DPP_QR_Code()", "Despawning QR_Code Overlay");
    delete_timer();
}

std::shared_ptr<DPP_QR_Code> DPP_QR_Code::create_QR_Code_from_url(std::string url)
{
    std::lock_guard<std::recursive_mutex> lock2(lvgl_mutex);

    std::shared_ptr<DPP_QR_Code> qr_code = std::shared_ptr<DPP_QR_Code>(new DPP_QR_Code(UI_Popup::create_popup("QR_Code")));

    constexpr const char *TAG = "update_QR_Code_from_url()";
    constexpr int QR_CODE_SIZE = 150;
    constexpr int MSG_BOX_WIDTH = QR_CODE_SIZE + 120;
    constexpr int MSG_BOX_HEIGHT = QR_CODE_SIZE + 80;

    ESP_LOGI(TAG, "Creating QR Code");

    lv_color_t qr_code_bg_color = lv_color_hex(COLOR_LIGHTGREY);
    lv_color_t qr_code_fg_color = lv_color_hex(COLOR_BLACK);

    // qr_screen_wrapper = lv_obj_create(lv_disp_get_scr_act(NULL)); // Create a full screen cover to display the QR code
    // lv_obj_set_size(qr_screen_wrapper, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    // lv_obj_add_flag(qr_screen_wrapper, LV_OBJ_FLAG_HIDDEN); // Hide the object for now
    // lv_obj_set_pos(qr_screen_wrapper, 0, 0);
    // lv_obj_set_style_bg_color(qr_screen_wrapper, lv_color_black(), 0);
    // lv_obj_set_style_bg_opa(qr_screen_wrapper, LV_OPA_70, 0); // Adjust opacity as needed
    // lv_obj_add_flag(qr_screen_wrapper, LV_OBJ_FLAG_CLICKABLE);

    // Create the wrapper object for the QR code and the close button
    lv_obj_t *qr_msg_box = lv_msgbox_create(qr_code.get()->popup->get_popup_obj());
    lv_obj_set_size(qr_msg_box, MSG_BOX_WIDTH, MSG_BOX_HEIGHT);
    lv_obj_center(qr_msg_box);
    lv_obj_set_pos(qr_msg_box, lv_obj_get_x(qr_msg_box), (EXAMPLE_LCD_V_RES - status_bar_height - MSG_BOX_HEIGHT) / 2);

    lv_obj_update_layout(qr_msg_box);

    // Add title
    lv_obj_t *title = lv_msgbox_add_title(qr_msg_box, "Scan this QR Code with your phone");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0); // Increase font size

    lv_obj_set_height(lv_msgbox_get_header(qr_msg_box), 40); // Increase the title container height (60px in this example)

    lv_obj_t *content = lv_msgbox_get_content(qr_msg_box);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN); // Stack items vertically
    lv_obj_set_flex_align(content,
                          LV_FLEX_ALIGN_START,  // Main axis (y-axis) alignment: start from the top
                          LV_FLEX_ALIGN_CENTER, // Cross axis (x-axis) alignment: center horizontally
                          LV_FLEX_ALIGN_START); // Track cross alignment (not used here)
    // lv_obj_set_style_pad_all(content, 5, 0);    // Add padding around the content
    lv_obj_set_style_border_width(content, 2, 0); // Remove border

    lv_obj_t *qr = lv_qrcode_create(content);
    lv_qrcode_set_size(qr, QR_CODE_SIZE);
    lv_qrcode_set_dark_color(qr, qr_code_fg_color);
    lv_qrcode_set_light_color(qr, qr_code_bg_color);
    lv_obj_set_style_border_width(qr, 2, 0);
    // lv_obj_align(qr, LV_ALIGN_CENTER, 0, 20); // Center the QR code below the title
    // lv_obj_set_align(qr, LV_ALIGN_CENTER);
    ESP_LOGI(TAG, "Updating QR Code with URL: %s", url.c_str());

    // lv_qrcode_update(qr, url.c_str(), strlen(url.c_str()));
    if (lv_qrcode_update(qr, url.c_str(), strlen(url.c_str())) == LV_RESULT_OK)
    {
        ESP_LOGI(TAG, "QR Code updated successfully");
    }
    else
    {
        ESP_LOGE(TAG, "QR Code update failed");
    }

    lv_obj_center(qr);
    qr_code.get()->qr_code_seconds_left = 20-1;

    qr_code.get()->qr_code_timer_label = lv_label_create(content);
    lv_label_set_text(qr_code.get()->qr_code_timer_label, (std::to_string(qr_code.get()->qr_code_seconds_left) + " seconds remaining").c_str());
    lv_obj_set_size(qr_code.get()->qr_code_timer_label, 250, 20);
    // lv_obj_set_align(qr_code_timer_label, LV_ALIGN_CENTER);

    qr_code.get()->start_timer(20);
    qr_code.get()->show_QR_Code();

    return qr_code;
}

// void DPP_QR_Code::despawn_QR_Code()
// {
//     std::lock_guard<std::recursive_mutex> lock(DPP_QR_Code_mutex);
//     ESP_LOGI("DPP_QR_Code()", "Despawning QR_Code Overlay");
//     if(qr_code_timer != nullptr)
//         lv_timer_delete(qr_code_timer);
//     popup.get()->~UI_Popup();
// }

std::recursive_mutex &DPP_QR_Code::get_mutex()
{
    std::lock_guard<std::recursive_mutex> lock(DPP_QR_Code_mutex);
    return DPP_QR_Code_mutex;
}

void DPP_QR_Code::show_QR_Code()
{
    popup.get()->show_popup();
}

void DPP_QR_Code::hide_QR_Code()
{
    popup.get()->hide_popup();
}

void DPP_QR_Code::start_timer(int seconds_to_run)
{
    std::lock_guard<std::recursive_mutex> lock(DPP_QR_Code_mutex);
    std::lock_guard<std::recursive_mutex> lock2(lvgl_mutex);
    if (qr_code_timer != nullptr)
    {
        ESP_LOGI("start_timer()", "Deleting old timer");
        lv_timer_delete(qr_code_timer);
        qr_code_timer = nullptr;
    }
    ESP_LOGI("start_timer()", "Starting new timer");
    qr_code_timer = lv_timer_create(qr_code_timer_cb, 1000, this);
    lv_timer_set_repeat_count(qr_code_timer, seconds_to_run);
    lv_timer_set_auto_delete(qr_code_timer, true);
    // lv_timer_set_repeat_count(qr_code_timer, 1 + seconds_to_run); // 120 seconds + 1 second for the last call at 0 seconds
}
void DPP_QR_Code::delete_timer()
{
    std::lock_guard<std::recursive_mutex> lock(DPP_QR_Code_mutex);
    std::lock_guard<std::recursive_mutex> lock2(lvgl_mutex);
    if (qr_code_timer != NULL)
    {
        ESP_LOGI("delete_timer()", "Deleting timer");
        lv_timer_delete(qr_code_timer);
        qr_code_timer = NULL;
    }
}

void DPP_QR_Code::qr_code_timer_cb(lv_timer_t *timer)
{
    constexpr const char *TAG = "qr_code_timer_cb()";
    std::lock_guard<std::recursive_mutex> lvgl_lock(lvgl_mutex);

    DPP_QR_Code *qr_code = (DPP_QR_Code *)lv_timer_get_user_data(timer);

    std::lock_guard<std::recursive_mutex> dpp_lock(qr_code->DPP_QR_Code_mutex);

    if (qr_code->qr_code_timer == nullptr)
    {
        ESP_LOGE(TAG, "Timer is null. Returning.");
        return;
    }

    qr_code->qr_code_seconds_left--;

    if (qr_code->qr_code_seconds_left < 0)
    {
        ESP_LOGI(TAG, "Timer expired, setting DPP_AUTH_FAIL_BIT");
        qr_code->qr_code_timer = nullptr;
        xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_TIMEOUT_BIT);
        return;
    }

    ESP_LOGD(TAG, "Updating timer label to %d seconds remaining", qr_code->qr_code_seconds_left);

    std::string remaining_text = std::to_string(qr_code->qr_code_seconds_left) + " seconds remaining";
    lv_label_set_text(qr_code->qr_code_timer_label, remaining_text.c_str());
    ESP_LOGI(TAG, "qr_code_timer_cb finished");
}

// called when the user clicks on the wifi status in the status bar
void dpp_wifi_renew_button_cb(lv_event_t *e)
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    ESP_LOGI("dpp_wifi_renew_button_cb()", "QR Code Button Clicked");

    lv_obj_t *scr_cover = lv_obj_create(lv_disp_get_scr_act(NULL));
    lv_obj_set_size(scr_cover, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_pos(scr_cover, 0, 0);
    lv_obj_set_style_bg_color(scr_cover, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr_cover, LV_OPA_50, 0); // Adjust opacity as needed
    lv_obj_add_flag(scr_cover, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *msgbox = lv_msgbox_create(scr_cover);
    lv_obj_set_size(msgbox, 300, 200);
    lv_obj_center(msgbox);

    // Add title
    lv_msgbox_add_title(msgbox, "Do you want to renew your WiFi Configuration?");
    lv_obj_t *header = lv_msgbox_get_header(msgbox);
    lv_obj_set_height(header, 60); // Set height of the header container
    // lv_obj_t* close_button = lv_msgbox_add_close_button(msgbox);
    // Create and style the custom close button
    lv_obj_t *close_button = lv_btn_create(header);
    lv_obj_set_user_data(close_button, scr_cover);
    lv_obj_set_size(close_button, 40, 40);
    lv_obj_set_style_pad_all(close_button, 10, 0);
    lv_obj_align(close_button, LV_ALIGN_TOP_RIGHT, -10, 10);

    auto dpp_wifi_renew_close_button_event_handler = [](lv_event_t *e)
    {
        lv_obj_t *close_button = (lv_obj_t *)lv_event_get_target(e);
        lv_obj_t *scr_parent = (lv_obj_t *)lv_obj_get_user_data(close_button);
        ESP_LOGI("dpp_wifi_renew_close_button_event_handler()", "QR Code Close Button Clicked");
        // Delete the message box and overlay asynchronously to avoid blocking
        lv_obj_del_async(scr_parent);
    };

    lv_obj_add_event_cb(close_button, dpp_wifi_renew_close_button_event_handler, LV_EVENT_CLICKED, scr_cover);
    lv_obj_t *close_button_label = lv_label_create(close_button);
    lv_label_set_text(close_button_label, LV_SYMBOL_CLOSE);

    lv_obj_t *msgbox_content = lv_msgbox_get_content(msgbox);

    auto dpp_msgbox_no_cb = [](lv_event_t *e)
    {
        // Get user data, which is scr_cover in this case
        lv_obj_t *scr_cover = (lv_obj_t *)lv_event_get_user_data(e);
        // Log and delete the scr_cover object
        ESP_LOGI("dpp_msgbox_no_cb()", "NO Button Clicked");
        lv_obj_del_async(scr_cover);
    };

    auto dpp_msgbox_yes_cb = [](lv_event_t *e)
    {
        // Get user data, which is scr_cover in this case
        lv_obj_t *scr_cover = (lv_obj_t *)lv_event_get_user_data(e);
        // Log and delete the scr_cover object
        ESP_LOGI("dpp_msgbox_yes_cb()", "YES Button Clicked");
        xTaskCreate(dpp_task_start, "dpp_task_start", 2048 * 2, NULL, 21, NULL);
        lv_obj_del_async(scr_cover);
    };

    lv_obj_t *yes_button = lv_btn_create(msgbox_content);
    lv_obj_add_event_cb(yes_button, dpp_msgbox_yes_cb, LV_EVENT_CLICKED, scr_cover);
    lv_obj_t *yes_button_label = lv_label_create(yes_button);
    lv_label_set_text(yes_button_label, "Yes");

    lv_obj_t *no_button = lv_btn_create(msgbox_content);
    lv_obj_add_event_cb(no_button, dpp_msgbox_no_cb, LV_EVENT_CLICKED, scr_cover);
    lv_obj_t *no_button_label = lv_label_create(no_button);
    lv_label_set_text(no_button_label, "No");
}

void status_bar(void *params)
{
    static lv_obj_t *Status_Bar = nullptr;
    static lv_obj_t *spacer = nullptr;
    static lv_obj_t *Client_Count = nullptr;

    static lv_obj_t *WiFi_Status_Text_Wrapper = nullptr;
    static lv_obj_t *WiFi_Status_Text = nullptr;

    static lv_obj_t *Wifi_icon = nullptr;

    // Create status bar only once
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

    if (Status_Bar == nullptr)
    {
        Status_Bar = lv_obj_create(lv_scr_act());

        static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
        static int32_t row_dsc[] = {LV_GRID_FR(1)};

        lv_obj_set_size(Status_Bar, EXAMPLE_LCD_H_RES, status_bar_height);
        lv_obj_set_pos(Status_Bar, 0, 0);
        lv_obj_clear_flag(Status_Bar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_grid_dsc_array(Status_Bar, col_dsc, row_dsc);
        lv_obj_set_flex_align(Status_Bar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        spacer = lv_obj_create(Status_Bar);
        lv_obj_set_style_border_width(spacer, 0, 0);
        lv_obj_set_style_outline_width(spacer, 0, 0);
        lv_obj_set_style_opa(spacer, 0, 0);

        Client_Count = lv_label_create(Status_Bar);
        lv_label_set_text(Client_Count, "Clients: 0");
        lv_obj_set_style_border_width(Client_Count, 0, 0);
        lv_obj_set_style_outline_width(Client_Count, 0, 0);
        lv_obj_set_size(Client_Count, 100, 15);
        lv_obj_set_style_pad_right(Client_Count, 20, 0);

        WiFi_Status_Text_Wrapper = lv_obj_create(Status_Bar);
        lv_obj_add_event_cb(WiFi_Status_Text_Wrapper, dpp_wifi_renew_button_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_size(WiFi_Status_Text_Wrapper, 220, 15);
        lv_obj_set_style_pad_all(WiFi_Status_Text_Wrapper, 0, 0);
        lv_obj_set_style_border_width(WiFi_Status_Text_Wrapper, 0, 0);
        lv_obj_set_style_outline_width(WiFi_Status_Text_Wrapper, 0, 0);
        WiFi_Status_Text = lv_label_create(WiFi_Status_Text_Wrapper);
        lv_label_set_text(WiFi_Status_Text, "WiFi Status: DISCONNECTED");
        lv_obj_set_style_border_width(WiFi_Status_Text, 0, 0);
        lv_obj_set_style_outline_width(WiFi_Status_Text, 0, 0);
        lv_obj_set_size(WiFi_Status_Text, 220, 15);

        Wifi_icon = lv_obj_create(Status_Bar);
        lv_obj_set_size(Wifi_icon, 15, 15);
        lv_obj_set_style_border_width(Wifi_icon, 0, 0);
        lv_obj_set_style_radius(Wifi_icon, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_grid_cell(Wifi_icon, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_add_event_cb(Wifi_icon, dpp_wifi_renew_button_cb, LV_EVENT_CLICKED, NULL);
    }
    lock.~lock_guard();

    while (1)
    {
        // Wait for the update bit to be set
        xEventGroupWaitBits(ui_event_group, UI_STATUSBAR_UPDATE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

        std::lock_guard<std::recursive_mutex> lock2(lvgl_mutex);
        lv_obj_move_foreground(Status_Bar);

        // Get the bits from the event group
        EventBits_t wifi_bits = xEventGroupGetBits(WiFi_Functions::wifi_event_group);

        std::lock_guard<std::recursive_mutex> lock3(mqtt_server_mutex);
        std::string client_count = std::to_string(s_clients.size());
        lock3.~lock_guard();

        lv_label_set_text(Client_Count, ("Clients: " + client_count).c_str());

        // Update the WiFi icon based on the bits set
        if (wifi_bits & WiFi_Functions::WIFI_CONNECTED_BIT)
        {
            lv_obj_set_style_bg_color(Wifi_icon, lv_color_hex(COLOR_NEON_GREEN), 0);
            lv_label_set_text(WiFi_Status_Text, "WiFi Status: CONNECTED");
            ESP_LOGI("status_bar()", "WIFI_CONNECTED_BIT set, setting to GREEN");
        }
        else if (wifi_bits & WiFi_Functions::WiFi_DISCONNECTED_BIT)
        {
            lv_obj_set_style_bg_color(Wifi_icon, lv_color_hex(COLOR_ORANGE), 0);
            lv_label_set_text(WiFi_Status_Text, "WiFi Status: DISCONNECTED");
            ESP_LOGI("status_bar()", "WiFi_DISCONNECTED_BIT set, setting to ORANGE");
        }
        else if (wifi_bits & WiFi_Functions::WIFI_FAIL_BIT)
        {
            lv_obj_set_style_bg_color(Wifi_icon, lv_color_hex(COLOR_RED), 0);
            lv_label_set_text(WiFi_Status_Text, "WiFi Status: FAIL");
            ESP_LOGI("status_bar()", "WIFI_FAIL_BIT set, setting to RED");
        }
        else
        {
            ESP_LOGE("status_bar()", "No WiFi status bits set, setting to RED");
            lv_obj_set_style_bg_color(Wifi_icon, lv_color_hex(COLOR_RED), 0);
            lv_label_set_text(WiFi_Status_Text, "WiFi Status: FAIL");
        }
    }
}

void ui_init(lv_obj_t *scr)
{
    xTaskCreate(main_view, "main_view", 8000, NULL, 20, NULL);
    xTaskCreate(status_bar, "status_bar", 8000, NULL, 20, NULL);
}

void show_add_new_client_overlay()
{
    std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);

    lv_obj_t *msgbox = lv_msgbox_create(overlay);
    lv_obj_set_size(msgbox, 300, 200);
    lv_obj_center(msgbox);

    lv_msgbox_add_close_button(msgbox);

    lv_msgbox_add_title(msgbox, "Do you want to connect a new Client to the Broker?");

    lv_obj_t *header = lv_msgbox_get_header(msgbox);
    lv_obj_set_height(header, 60); // Set height of the header container
    lv_obj_t *btn;

    static lv_event_cb_t yes_btn_event_cb = [](lv_event_t *e)
    {
        lv_obj_t *overlay = (lv_obj_t *)(lv_event_get_user_data(e));
        lv_obj_del(overlay);
    };

    btn = lv_msgbox_add_footer_button(msgbox, "Yes");
    lv_obj_add_event_cb(btn, yes_btn_event_cb, LV_EVENT_CLICKED, NULL);

    static lv_event_cb_t no_btn_event_cb = [](lv_event_t *e)
    {
        lv_obj_t *overlay = (lv_obj_t *)(lv_event_get_user_data(e));
        lv_obj_del(overlay);
    };

    btn = lv_msgbox_add_footer_button(msgbox, "No");
    lv_obj_add_event_cb(btn, no_btn_event_cb, LV_EVENT_ALL, nullptr);
}