#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_dpp.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wholeinclude.h"

#include "ui/ui.h"
#include "wifi/WiFi_Functions.h"

#ifdef CONFIG_ESP_DPP_LISTEN_CHANNEL_LIST
#define EXAMPLE_DPP_LISTEN_CHANNEL_LIST CONFIG_ESP_DPP_LISTEN_CHANNEL_LIST
#else
#define EXAMPLE_DPP_LISTEN_CHANNEL_LIST "6"
#endif

#define EXAMPLE_DPP_BOOTSTRAPPING_KEY "9a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b"

#ifdef CONFIG_ESP_DPP_DEVICE_INFO
#define EXAMPLE_DPP_DEVICE_INFO CONFIG_ESP_DPP_DEVICE_INFO
#else
#define EXAMPLE_DPP_DEVICE_INFO 0
#endif

#define CURVE_SEC256R1_PKEY_HEX_DIGITS 64

// constexpr const char *_TAG = "wifi dpp-enrollee";
inline wifi_config_t s_dpp_wifi_config;

inline int s_retry_num = 0;

#define WIFI_MAX_RETRY_NUM 3

void dpp_enrollee_event_cb(esp_supp_dpp_event_t event, void *data);

esp_err_t dpp_enrollee_bootstrap(void);

bool dpp_enrollee_init(void);
// Task to start DPP Enrollee
void dpp_task_start(void *params);