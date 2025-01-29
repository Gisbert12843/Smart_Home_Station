extern "C"
{
    void app_main(void);
}
#include "wholeinclude.h"
#include "nvs_wrapper.h"
#include "WiFi_Functions.h"
#include "helper_functions.h"
#include "mqtt.h"
#include "tft.h"
#include "color_palette.h"
#include "ui.h"

#include "wifi_dpp.h"


void app_main()
{
    constexpr char *TAG = "app_main()";
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    helper_functions::delay(3000);
    nvs_wrapper::init();

    WiFi_Functions::init();
    initialise_mdns();

    tft_init();

    xTaskCreate(lvgl_manager_bg_task, "lvgl_manager_bg_task", 2048 * 4, NULL, 22, NULL);

    std::cout << "\n\n";
    helper_functions::delay(2000);

    WiFi_Functions::AP_Credentials *cred;
    cred = WiFi_Functions::loadCredentialsFromNVS();
    if (cred == nullptr)
    {
        // ESP_ERROR_CHECK(nvs_wrapper::resetNVS());
        ESP_LOGI(TAG, "\nNo Credentials found on NVS, creating new ones");
        xTaskCreate(dpp_task_start, "dpp_task_start", 2048 * 4, NULL, 22, NULL);
    }
    else
    {
        ESP_LOGI(TAG, "\nCredentials found on NVS");
        ESP_LOGI(TAG, "SSID: %s", cred->getWifi_ssid().c_str());
    }

    cred = WiFi_Functions::AP_Credentials::getInstance();
    while (!WiFi_Functions::connect_to_ap(cred))
    {
        ESP_LOGI(TAG, "\nFailed to connect to AP, retrying...");
        helper_functions::delay(2000);
    }

    ESP_LOGI(TAG, "\n\n\ninit_mongoose() is waiting for WiFi to connect\n\n\n");
    xEventGroupWaitBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "\n\n\ninit_mongoose() STARTING\n\n\n");

    vTaskDelay(pdMS_TO_TICKS(2000));
    init_mongoose();
}