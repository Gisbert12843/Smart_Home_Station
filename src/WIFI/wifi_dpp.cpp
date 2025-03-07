#include "wifi/wifi_dpp.h"

static std::shared_ptr<DPP_QR_Code> qr_code_ref = nullptr;

static void dpp_enrollee_event_cb(esp_supp_dpp_event_t event, void *data);
static esp_err_t dpp_enrollee_bootstrap(void);
static bool dpp_enrollee_init(void);

static void dpp_enrollee_event_cb(esp_supp_dpp_event_t event, void *data)
{
    constexpr const char *TAG = "dpp_enrollee_event_cb()";
    
    esp_err_t err;
    //ESP_LOGI(TAG, "dpp_enrollee_event_cb called with event: %d", event);

    switch (event)
    {
    case ESP_SUPP_DPP_URI_READY:
        //ESP_LOGI(TAG, "ESP_SUPP_DPP_URI_READY event");
        if (data != NULL)
        {

            qr_code_ref = DPP_QR_Code::create_QR_Code_from_url((char *)data);

            err = (esp_supp_dpp_start_listen());
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to start DPP listen: %s", esp_err_to_name(err));
                xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_AUTH_FAIL_BIT);
            }
            //ESP_LOGI(TAG, "Started esp_supp_dpp_start_listen");
        }
        break;
    case ESP_SUPP_DPP_CFG_RECVD:
    {
        //ESP_LOGI(TAG, "ESP_SUPP_DPP_CFG_RECVD event");
        s_retry_num = 0;

        memcpy(&s_dpp_wifi_config, data, sizeof(s_dpp_wifi_config));
        if (s_dpp_wifi_config.sta.ssid[0] == '\0' || s_dpp_wifi_config.sta.password[0] == '\0')
        {
            //ESP_LOGI(TAG, "SSID or password is null, setting DPP_AUTH_FAIL_BIT");
            xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_AUTH_FAIL_BIT);
            break;
        }
        size_t ssid_len = strlen((char *)s_dpp_wifi_config.sta.ssid);
        size_t password_len = strlen((char *)s_dpp_wifi_config.sta.password);
        //ESP_LOGI(TAG, "Received Wi-Fi configuration: SSID: %s, Password: %s", (char *)s_dpp_wifi_config.sta.ssid, (char *)s_dpp_wifi_config.sta.password);

        err = esp_wifi_set_config(WIFI_IF_STA, &s_dpp_wifi_config);

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set Wi-Fi configuration in ESP_SUPP_DPP_CFG_RECVD Event: %s", esp_err_to_name(err));
            xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_AUTH_FAIL_BIT);
            break;
        }

        if (ssid_len == 0 || password_len == 0 || ssid_len > sizeof(s_dpp_wifi_config.sta.ssid) - 1 || password_len > sizeof(s_dpp_wifi_config.sta.password) - 1)
        {
            //ESP_LOGI(TAG, "SSID or password is empty or too long, setting DPP_AUTH_FAIL_BIT");
            xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_AUTH_FAIL_BIT);
            break;
        }

        //ESP_LOGI(TAG, "Saving AP Configuration: SSID: %s, Password: %s", (char *)s_dpp_wifi_config.sta.ssid, (char *)s_dpp_wifi_config.sta.password);

        WiFi_Functions::AP_Credentials::saveAPConfiguration((char *)s_dpp_wifi_config.sta.ssid, (char *)s_dpp_wifi_config.sta.password);

        if (WiFi_Functions::connect_to_ap(WiFi_Functions::AP_Credentials::getInstance()))
        {
            //ESP_LOGI(TAG, "Successfully connected to AP");
            xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::WIFI_CONNECTED_BIT);
        }
        else
        {
            //ESP_LOGI(TAG, "Failed to connect to AP, setting DPP_AUTH_FAIL_BIT");
            xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_AUTH_FAIL_BIT);
        }
    }
    break;
    case ESP_SUPP_DPP_FAIL:
    {
        //ESP_LOGI(TAG, "ESP_SUPP_DPP_FAIL event");
        if (s_retry_num < 10)
        {
            //ESP_LOGI(TAG, "DPP Auth failed (Reason: %s), retrying...", esp_err_to_name((int)data));
            err = (esp_supp_dpp_start_listen());
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to start DPP listen after ESP_SUPP_DPP_FAIL event: %s", esp_err_to_name(err));
                xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_AUTH_FAIL_BIT);
            }
            s_retry_num++;
        }
        else
        {
            //ESP_LOGI(TAG, "Retry limit reached, setting DPP_AUTH_FAIL_BIT");
            xEventGroupSetBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_AUTH_FAIL_BIT);
        }
    }
    break;
    default:
    {
        //ESP_LOGI(TAG, "Unhandled DPP event: %d", event);
    }
    break;
    }
}

static esp_err_t dpp_enrollee_bootstrap(void)
{
    constexpr const char *TAG = "dpp_enrollee_bootstrap()";
    ESP_LOGD(TAG, "dpp_enrollee_bootstrap called");

    esp_err_t ret;
    size_t pkey_len = strlen(EXAMPLE_DPP_BOOTSTRAPPING_KEY);
    char *key = NULL;

    if (pkey_len)
    {
        //ESP_LOGI(TAG, "Generating bootstrapping key");
        char prefix[] = "30310201010420";
        char postfix[] = "a00a06082a8648ce3d030107";

        if (pkey_len != CURVE_SEC256R1_PKEY_HEX_DIGITS)
        {
            ESP_LOGW(TAG, "Invalid key length! Private key needs to be 32 bytes (or 64 hex digits) long");
            return ESP_ERR_INVALID_SIZE;
        }

        key = (char *)malloc(sizeof(prefix) + pkey_len + sizeof(postfix));
        if (!key)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for bootstrapping key");
            return ESP_ERR_NO_MEM;
        }
        sprintf(key, "%s%s%s", prefix, EXAMPLE_DPP_BOOTSTRAPPING_KEY, postfix);
    }

    ret = esp_supp_dpp_bootstrap_gen(EXAMPLE_DPP_LISTEN_CHANNEL_LIST, DPP_BOOTSTRAP_QR_CODE, key, EXAMPLE_DPP_DEVICE_INFO);

    if (key)
        free(key);

    return ret;
}

static bool dpp_enrollee_init(void)
{
    constexpr const char *TAG = "dpp_enrollee_init()";
    ESP_LOGD(TAG, "dpp_enrollee_init called");

    esp_err_t err;
    WiFi_Functions::de_init();

    WiFi_Functions::init();

    err = esp_wifi_stop();
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to stop Wi-Fi: %s", esp_err_to_name(err));
    }

    vTaskDelay(pdMS_TO_TICKS(600));

    //ESP_LOGI(TAG, "Initializing DPP");
    err = esp_supp_dpp_init(dpp_enrollee_event_cb);

    std::shared_ptr<DPP_QR_Code> dpp_qr_code;
    qr_code_ref = dpp_qr_code;

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "DPP init failed: %s", esp_err_to_name(err));
        qr_code_ref = nullptr;
        return false;
    }

    //ESP_LOGI(TAG, "Bootstrapping DPP");
    err = (dpp_enrollee_bootstrap());
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "DPP bootstrap failed: %s", esp_err_to_name(err));

        esp_supp_dpp_deinit();
        qr_code_ref = nullptr;
        return false;
    }

    //ESP_LOGI(TAG, "Starting Wi-Fi");
    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wi-Fi start failed");
        qr_code_ref = nullptr;
        esp_supp_dpp_deinit();
        return false;
    }

    //ESP_LOGI(TAG, "Waiting for DPP_BITS");
    EventBits_t bits = xEventGroupWaitBits(WiFi_Functions::wifi_event_group,
                                           WiFi_Functions::WIFI_CONNECTED_BIT | WiFi_Functions::DPP_AUTH_FAIL_BIT | WiFi_Functions::DPP_TIMEOUT_BIT | WiFi_Functions::DPP_ERROR_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(200000)); // Adding a timeout to prevent indefinite waiting
    std::lock_guard<std::recursive_mutex> lock(dpp_qr_code.get()->get_mutex());

    //ESP_LOGI(TAG, "Deinitializing DPP");
    esp_supp_dpp_deinit();

    if (bits & WiFi_Functions::WIFI_CONNECTED_BIT)
    {
        //ESP_LOGI(TAG, "Connected to AP");
        //ESP_LOGI(TAG, "dpp_enrollee_init finished");
        qr_code_ref = nullptr;

        return true;
    }
    else if (bits & WiFi_Functions::DPP_AUTH_FAIL_BIT)
    {
        ESP_LOGW(TAG, "DPP Auth failed");
        xEventGroupClearBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_AUTH_FAIL_BIT);
        qr_code_ref = nullptr;

        return false;
    }
    else if (bits & WiFi_Functions::DPP_TIMEOUT_BIT)
    {
        ESP_LOGW(TAG, "DPP Timed out");
        xEventGroupClearBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_TIMEOUT_BIT);
        qr_code_ref = nullptr;

        return false;
    }
    else if (bits & WiFi_Functions::DPP_ERROR_BIT)
    {
        ESP_LOGE(TAG, "DPP Error");
        xEventGroupClearBits(WiFi_Functions::wifi_event_group, WiFi_Functions::DPP_ERROR_BIT);
        qr_code_ref = nullptr;

        return false;
    }
    else
    {
        ESP_LOGE(TAG, "Unexpected Timeout/Error");
        qr_code_ref = nullptr;

        return false;
    }
    qr_code_ref = nullptr;
    return false;
}

// Task to start DPP Enrollee
void dpp_task_start(void *params)
{
    constexpr const char *TAG = "dpp_task_start()";
    ESP_LOGI(TAG, "dpp_task_start START");
    dpp_enrollee_init();
    ESP_LOGI(TAG, "dpp_task_start END");
    vTaskDelete(NULL);
}
