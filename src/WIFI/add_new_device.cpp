#include "wifi/add_new_device.h"

#include "UI/ui_elements_popup.h"
#include "UTILS/i2c_wrapper.h"
#include "esp_log.h"

#define I2C_SLAVE_ADDRESS 0x69

static void cancel_event_cb(lv_event_t *e)
{
    bool *wait = (bool *)lv_event_get_user_data(e);
    *wait = false;
}

//TODO
void add_new_device_task(void *params)
{
    {//do not remove theses brackets they are used to limit the scope of the shared pointer
        std::shared_ptr<UI_Popup> pop_up = UI_Popup::create_popup("Add_New_Device");

        lv_obj_t *mbox = lv_msgbox_create(pop_up.get()->get_popup_obj());
        lv_obj_center(mbox);

        lv_msgbox_add_title(mbox, "Adding new Controller");

        lv_msgbox_add_text(mbox, "Please plug the Connector of the Controller into the Connector of the Broker, located on the bottom of the Broker.\nAfter that wait for the LED on the Controller to turn green.");

        lv_obj_t *btn;
        btn = lv_msgbox_add_footer_button(mbox, "Cancel");

        bool wait = true;
        lv_obj_add_event_cb(btn, cancel_event_cb, LV_EVENT_CLICKED, &wait);

        pop_up->show_popup();

        esp_err_t err = i2c_functions::init_master();
        if (err != ESP_OK)
        {
            ESP_LOGE("add_new_device_task()", "Error %d initializing I2C master", err);
            wait = false;
        }

        i2c_functions::slave newDevice = i2c_functions::slave(I2C_SLAVE_ADDRESS);

        while (wait)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    vTaskDelete(NULL);
}