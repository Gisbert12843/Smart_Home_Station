#include "wifi/WiFi_Functions.h"

#include "iostream"
#include "vector"

#include "lwip/ip_addr.h"
#include "esp_wifi_types.h"
#include "lwip/dns.h"

#include "ui/ui.h"
#include "wifi/wifi_dpp.h"

// Individually defined SSID and Password for each Module
#define AP_SSID "ESP32_MQTT_SERVER"
#define AP_PASSWORD "MQTT_SERVER_PASSWORD"

void WiFi_Functions::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    std::string TAG;
    static int8_t s_retry_num = 0;

    constexpr int8_t EXAMPLE_ESP_MAXIMUM_RETRY = 30;

    switch (event_id)
    {
    case (WIFI_EVENT_STA_START):
    {
        TAG = "WIFI_EVENT_STA_START";
        //ESP_LOGD(TAG.data(), "WIFI_EVENT_STA_START");
        xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WiFi_DISCONNECTED_BIT);
        xEventGroupSetBits(ui_event_group, UI_STATUSBAR_UPDATE_BIT);
    }
    break;

    case (WIFI_EVENT_STA_STOP):
    {
        TAG = "WIFI_EVENT_STA_STOP";
        //ESP_LOGD(TAG.data(), "WIFI_EVENT_STA_STOP");
        xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WiFi_DISCONNECTED_BIT);
        xEventGroupClearBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WIFI_CONNECTED_BIT);
        xEventGroupSetBits(ui_event_group, UI_STATUSBAR_UPDATE_BIT);
    }
    break;

    case WIFI_EVENT_STA_DISCONNECTED:
    {
        xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WiFi_DISCONNECTED_BIT);
        xEventGroupClearBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WIFI_CONNECTED_BIT);
        xEventGroupSetBits(ui_event_group, UI_STATUSBAR_UPDATE_BIT);

        TAG = "WIFI_EVENT_STA_DISCONNECTED";
        //ESP_LOGD(TAG.data(), "Connection to AP failed");

        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            //ESP_LOGD(TAG.data(), "Retrying to connect to the AP");
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
            s_retry_num++;
        }
        else
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
            s_retry_num = 0;
            xEventGroupClearBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WiFi_DISCONNECTED_BIT);
            xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WIFI_FAIL_BIT);
            xEventGroupSetBits(ui_event_group, UI_STATUSBAR_UPDATE_BIT);
        }
    }
    break;

    case IP_EVENT_STA_GOT_IP:
    {
        TAG = "IP_EVENT_STA_GOT_IP";
        //ESP_LOGD(TAG.data(), "IP_EVENT_STA_GOT_IP");

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        char local_ip[32];
        sprintf(local_ip, IPSTR, IP2STR(&event->ip_info.ip));
        char gateway_ip[32];
        sprintf(gateway_ip, IPSTR, IP2STR(&event->ip_info.gw));

        //ESP_LOGD(TAG.data(), "Connected as: \"%s\" to \"%s\"", local_ip, gateway_ip);
        s_retry_num = 0;

        xEventGroupClearBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WiFi_DISCONNECTED_BIT);
        xEventGroupClearBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WIFI_FAIL_BIT);
        xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WIFI_CONNECTED_BIT);
        xEventGroupSetBits(ui_event_group, UI_STATUSBAR_UPDATE_BIT);
    }
    break;

    default:
    {
        ESP_LOGW(TAG.data(), "Unhandled WiFi event: %d", event_id);
    }
    break;
    }
}

void WiFi_Functions::reset_wifi()
{
    if (WiFi_Functions::Fail_Bit_Listener_Handle != NULL)
        vTaskDelete(WiFi_Functions::Fail_Bit_Listener_Handle);
    WiFi_Functions::Fail_Bit_Listener_Handle = NULL;

    //ESP_LOGD("WiFi_Functions::reset_wifi()", "Fixing WiFi");
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_instance));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler_instance));

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_deinit());

    if (sta_netif)
    {
        esp_netif_destroy_default_wifi(sta_netif);
        sta_netif = nullptr;
    }
    if (ap_netif)
    {
        esp_netif_destroy_default_wifi(ap_netif);
        ap_netif = nullptr;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_delete_default());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_deinit());

    init_state = false;

    //ESP_LOGI("WiFi_Functions::de_init", "WiFi deinitialized successfully");

    init();
}

void WiFi_Functions::de_init()
{
    if (init_state)
    {
        if (WiFi_Functions::Fail_Bit_Listener_Handle != NULL)
            vTaskDelete(WiFi_Functions::Fail_Bit_Listener_Handle);
        WiFi_Functions::Fail_Bit_Listener_Handle = NULL;

        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_instance));
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler_instance));

        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_deinit());

        if (sta_netif)
        {
            esp_netif_destroy_default_wifi(sta_netif);
            sta_netif = nullptr;
        }
        if (ap_netif)
        {
            esp_netif_destroy_default_wifi(ap_netif);
            ap_netif = nullptr;
        }

        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_delete_default());
        // Removed esp_netif_deinit() since it is not supported.

        init_state = false;
        //ESP_LOGI("WiFi_Functions::de_init", "WiFi deinitialized successfully");
    }
    else
    {
        //ESP_LOGI("WiFi_Functions::de_init", "WiFi not initialized, ignoring deinit call");
    }
}

void WiFi_Functions::init()
{
    constexpr const char *TAG = "WiFi_Functions::init";
    if (!init_state)
    {
        if (WiFi_Functions::Fail_Bit_Listener_Handle != NULL)
        {
            //ESP_LOGI(TAG, "Deleting Fail_Bit_Listener_Handle");
            vTaskDelete(WiFi_Functions::Fail_Bit_Listener_Handle);
        }
        WiFi_Functions::Fail_Bit_Listener_Handle = NULL;

        init_state = true;

        wifi_event_group = xEventGroupCreate();

        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_init());
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());

        sta_netif = esp_netif_create_default_wifi_sta();
        ap_netif = esp_netif_create_default_wifi_ap();
        assert(sta_netif != nullptr);
        assert(ap_netif != nullptr);

        ip_addr_t d;
        d.type = IPADDR_TYPE_V4;
        d.u_addr.ip4.addr = ipaddr_addr("8.8.8.8");
        dns_setserver(0, &d);
        d.u_addr.ip4.addr = ipaddr_addr("8.8.4.4");
        dns_setserver(1, &d);

        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifi_event_handler_instance));
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &ip_event_handler_instance));

        cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_STA));


        //ESP_LOGI("WiFi_Functions::init", "WiFi STA&AP initialized successfully");
    }
    else
    {
        //ESP_LOGI("WiFi_Functions::init", "WiFi already initialized");
    }
}

WiFi_Functions::AP_Credentials *WiFi_Functions::loadCredentialsFromNVS(std::string p_nvs_namespace_name)
{
    constexpr const char *TAG = "AP_Credentials::loadCredentialsFromNVS";
    esp_err_t err = nvs_open(p_nvs_namespace_name.c_str(), NVS_READWRITE, &nvs_wrapper::nvs_namespace_handle);
    if (err != ESP_OK)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        ESP_LOGE(TAG, "Error opening NVS Namespace: \"%s\"", p_nvs_namespace_name.c_str());
        return nullptr;
    }

    // nvs_stats_t nvs_stats;
    // nvs_get_stats(nvs_wrapper::nvs_namespace_name, &nvs_stats);
    // entry_count = nvs_stats.used_entries;

    nvs_iterator_t it = NULL;
    err = nvs_entry_find(NVS_DEFAULT_PART_NAME, nvs_wrapper::nvs_namespace_name, NVS_TYPE_STR, &it);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "\nNo NVS Entries found in namespace: \"%s\"", p_nvs_namespace_name.c_str());
        return nullptr;
    }

    do
    {
        nvs_entry_info_t info;
        err = nvs_entry_info(it, &info);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return nullptr;

        std::string received_string = "";
        err = nvs_wrapper::getValueFromKey(std::string(info.key), received_string, true);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);

        if (err == ESP_OK)
        {
            //ESP_LOGI(TAG, "\nRead NVS Value: \"%s\"", received_string.c_str());

            std::string ssid = "";
            std::string password = "";

            if (received_string.size() > 0)
            {
                bool mode = 0;
                for (auto it : received_string)
                {
                    if (mode == 0)
                    {
                        if (it == '~')
                        {
                            mode = 1;
                            continue;;
                        }
                        else
                        {
                            ssid += it;
                        }
                    }
                    else if (mode == 1)
                    {
                        password += it;
                    }
                }

                if (WiFi_Functions::AP_Credentials::getWifi_ssid() != "" && WiFi_Functions::AP_Credentials::getWifi_ssid() == ssid)
                {
                    if (0 == strcmp(WiFi_Functions::AP_Credentials::getWifi_password().c_str(), password.c_str()))
                    {
                        //ESP_LOGI(TAG, "Old password: \"%s\" is equal to new password: \"%s\"", WiFi_Functions::AP_Credentials::getWifi_password().c_str(), password.c_str());
                        //ESP_LOGI(TAG, "Credentials are equal to the already stored ones, no opertation on saved credentials was performed.");
                        nvs_release_iterator(it);

                        return WiFi_Functions::AP_Credentials::getInstance();
                    }

                    //ESP_LOGI(TAG, "Old password: \"%s\" is NOT equal to new password: \"%s\"", WiFi_Functions::AP_Credentials::getWifi_password().c_str(), password.c_str());

                    WiFi_Functions::AP_Credentials::saveAPConfiguration(ssid, password, true);
                    //ESP_LOGI(TAG, "A Configuration was found that machtches this SSID. Credentials were updated.");
                    nvs_release_iterator(it);

                    return WiFi_Functions::AP_Credentials::getInstance(); // Found a match
                }

                WiFi_Functions::AP_Credentials::saveAPConfiguration(ssid, password, false);

                //ESP_LOGI(TAG, "The following Configuration has been loaded from NVS:\n\tSSID:     \"%s\"\n\tPassword: \"%s\"", ssid.c_str(), password.c_str());
                nvs_release_iterator(it);

                return WiFi_Functions::AP_Credentials::getInstance();
            }
            else
            {
                ESP_LOGE(TAG, "Error when reading from NVS, though getValueFromKey() suceeded: Reason: \"String.size <= 0\" Error: \"%s\"", esp_err_to_name(err));
                // err = nvs_entry_next(&it);
                // ESP_ERROR_CHECK_WITHOUT_ABORT(err);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Error when reading from NVS, though nvs_entry_next() suceeded: Reason: \"getValueFromKey != ESP_OK\" Error: \"%s\"", esp_err_to_name(err));
            // err = nvs_entry_next(&it);
            // ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        }
    } while (nvs_entry_next(&it) == ESP_OK);

    nvs_release_iterator(it);
    return nullptr;
}

esp_err_t WiFi_Functions::AP_Credentials::setCredentialToNVS(WiFi_Functions::AP_Credentials *cred)
{
    std::string wifi_password = cred->getWifi_password();

    std::string serializedData;
    serializedData.insert(serializedData.end(), cred->getWifi_ssid().begin(), cred->getWifi_ssid().end());
    serializedData.push_back('~'); // Separator
    serializedData.insert(serializedData.end(), cred->getWifi_password().begin(), cred->getWifi_password().end());

    // Add a null terminator to the end
    serializedData.push_back('\0');

    //ESP_LOGI("AP_Credentials::setCredentialToNVS", "Saving following string data to NVS: \"%s\"", serializedData.c_str());

    return nvs_wrapper::setValueToKey(cred->getWifi_ssid(), serializedData);
}

esp_err_t WiFi_Functions::AP_Credentials::setCredentialToNVS(std::string wifi_ssid, std::string wifi_password)
{
    std::string serializedData;
    serializedData.insert(serializedData.end(), wifi_ssid.begin(), wifi_ssid.end());
    serializedData.push_back('~'); // Separator
    serializedData.insert(serializedData.end(), wifi_password.begin(), wifi_password.end());

    // Add a null terminator to the end
    serializedData.push_back('\0');

    //ESP_LOGI("AP_Credentials::setCredentialToNVS", "Saving following string data to NVS: \"%s\"", serializedData.c_str());

    return nvs_wrapper::setValueToKey(wifi_ssid, serializedData);
}

bool WiFi_Functions::connect_to_ap(AP_Credentials *connection)
{
    constexpr const char *TAG = "WiFi_Functions::connect_to_ap";
    if (connection == nullptr)
    {
        ESP_LOGE(TAG, "No connection credentials provided");
        return false;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, connection->getWifi_ssid().c_str(), sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, connection->getWifi_password().c_str(), sizeof(wifi_config.sta.password) - 1);

    //ESP_LOGI(TAG, "Attempting to connect to SSID: %s", wifi_config.sta.ssid);

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        //ESP_LOGI(TAG, "Connected to AP using the credentials:\n\tSSID: \"%s\" \n\tPassword: \"%s\"", connection->getWifi_ssid().c_str(), connection->getWifi_password().c_str());
        WiFi_Functions::AP_Credentials::saveAPConfiguration(connection->getWifi_ssid(), connection->getWifi_password());
        xTaskCreate(WiFi_Task::Task_Start_Fail_Bit_Listener, "Task_Start_Fail_Bit_Listener", 8 * 1024, NULL, 22, &Fail_Bit_Listener_Handle);
        xEventGroupClearBits(wifi_event_group, WIFI_FAIL_BIT);
        return true;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "Failed to connect to AP using the credentials: %s", connection->getWifi_ssid().c_str());
        xEventGroupClearBits(wifi_event_group, WIFI_FAIL_BIT);
    }
    else
    {
        ESP_LOGE(TAG, "Unexpected error while establishing initial AP connection");
    }

    return false;
}

std::string WiFi_Functions::find_available_networks()
{
    constexpr const char *TAG = "WiFi_Functions::find_available_networks";
    esp_err_t err;

    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true,
        .scan_time = 1000};

    err = esp_wifi_scan_start(&scan_config, true);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_stop());

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error starting Wi-Fi scan: %s", esp_err_to_name(err));
        std::vector<wifi_ap_record_t> wifi_records(30);
        uint16_t max_records = 30;
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_records(&max_records, wifi_records.data()));
        /*         ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop()); */
        return "";
    }

    // wifi_ap_record_t wifi_records[20];
    std::vector<wifi_ap_record_t> wifi_records(30);

    uint16_t max_records = 20;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_records(&max_records, wifi_records.data()));

    for (int i = 0; i < max_records; i++)
    {

        if (WiFi_Functions::AP_Credentials::getWifi_ssid() == (char *)wifi_records[i].ssid)
        {
            //ESP_LOGD(TAG, "Found present SSID: %s", (char *)(wifi_records[i].ssid));
            return (char *)(wifi_records[i].ssid);
        }
    }
    // ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
    //ESP_LOGD(TAG, "\n\n\nExited find available!\n\n\n");
    return "";
}

int WiFi_Functions::check_network_availability(std::string ssid)
{
    constexpr const char *TAG = "WiFi_Functions::check_network_availability";

    esp_err_t err = ESP_OK;

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());

    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true,
        .scan_time = 1000};

    err = esp_wifi_scan_start(&scan_config, true);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error starting Wi-Fi scan: %s", esp_err_to_name(err));
        // esp_wifi_stop();
        return 2;
    }

    uint16_t ap_num = 0;
    err = (esp_wifi_scan_get_ap_num(&ap_num));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error getting AP number: %s", esp_err_to_name(err));
        return 2;
    }

    std::vector<wifi_ap_record_t> wifi_records(ap_num);

    err = (esp_wifi_scan_get_ap_records(&ap_num, wifi_records.data()));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error getting AP records: %s", esp_err_to_name(err));
        return 2;
    }

    for (int i = 0; i < ap_num; i++)
    {
        std::cout << (char *)wifi_records[i].ssid << " was found.\n\n";

        if (ssid == (char *)wifi_records[i].ssid)
        {

            //ESP_LOGD(TAG, "\n\nSSID: %s was found.", ssid.c_str());
            return 1;
        }
    }

    //ESP_LOGD(TAG, "\n\nSSID: %s was not found.", ssid.c_str());

    return 0;
}

void WiFi_Functions::wifi_fail_bit_listener()
{
    constexpr const char *TAG = "wifi_fail_bit_listener";

    //ESP_LOGD(TAG, "Wifi_fail_bit_listener started.");

    // Wait for the WIFI_FAIL_BIT to be set
    xEventGroupWaitBits(WiFi_Functions::wifi_event_group, WIFI_FAIL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

    while (true)
    {
        short tries = 0;
        short error_count = 0;

        ESP_LOGW(TAG, "WIFI_FAIL_BIT was set. Reestablishing WIFI Connection.");

        // Attempt to reconnect to the same AP
        while (tries < 10)
        {
            tries++;
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            // Check network availability
            int availability = WiFi_Functions::check_network_availability(WiFi_Functions::AP_Credentials::getWifi_ssid());

            if (availability == 2)
            {
                // Unexpected error
                if (error_count >= 20)
                {
                    ESP_LOGE(TAG, "Unexpected Error keeps appearing. Restarting Device...");
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                    esp_restart();
                }

                ESP_LOGE(TAG, "Unexpected Error while testing for availability of %s. Next try in 5 secs.", WiFi_Functions::AP_Credentials::getWifi_ssid().c_str());
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                error_count++;
                continue;
            }
            else if (availability == 0)
            {
                //ESP_LOGD(TAG, "%s is currently not available. Next try in 30 secs.", WiFi_Functions::AP_Credentials::getWifi_ssid().c_str());
                vTaskDelay(30000 / portTICK_PERIOD_MS);
                continue;
            }
            else
            {
                // Connect to the AP
                if (WiFi_Functions::connect_to_ap(WiFi_Functions::AP_Credentials::getInstance()))
                {
                    //ESP_LOGI(TAG, "Connection to %s reestablished.", WiFi_Functions::AP_Credentials::getWifi_ssid().c_str());
                    return;
                }
                else
                {
                    //ESP_LOGD(TAG, "%s did not accept connection request. Next try in 30 secs.\n", WiFi_Functions::AP_Credentials::getWifi_ssid().c_str());
                    vTaskDelay(30000 / portTICK_PERIOD_MS);
                }
            }
        }

        //ESP_LOGD(TAG, "Previous AP not available after 10 attempts.");
        // Start DPP here
        dpp_enrollee_init();
    }
}

/**
 * @brief Saves the Access Point (AP) configuration with the given SSID and password.
 *
 * This function updates the AP configuration with the provided SSID and password.
 * If the provided SSID and password are the same as the current configuration,
 * it returns the current instance without making any changes.
 * Optionally, it can update the Non-Volatile Storage (NVS) with the new credentials (Default:true).
 *
 * @param ssid The SSID of the WiFi network.
 * @param password The password of the WiFi network.
 * @param update_NVS A boolean flag indicating whether to update the NVS with the new credentials.
 * @return A pointer to the AP_Credentials instance with the updated configuration.
 */
WiFi_Functions::AP_Credentials *WiFi_Functions::AP_Credentials::saveAPConfiguration(const std::string &ssid, const std::string &password, bool update_NVS)
{
    if (ssid == getWifi_ssid() && password == WiFi_Functions::AP_Credentials::getWifi_password())
    {
        return getInstance();
    }

    getInstance()->setWifi_ssid(ssid);
    getInstance()->setWifi_password(password);

    if (update_NVS)
    {
        nvs_wrapper::resetNVS();
        setCredentialToNVS(ssid, password);
    }
    return getInstance();
}

void stop_wifi_and_advertise_as_AP()
{
}