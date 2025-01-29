
#include "mqtt/mqtt.h"
#include "utils/helper_functions.h"
#include "ui/ui.h"
#include "mqtt/mqtt_server.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "mdns.h"
#include "lwip/dns.h"

#include "mongoose/mongoose.h"
// #include "pwm.h"

#include "mutex"

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
#define esp_vfs_fat_spiflash_mount esp_vfs_fat_spiflash_mount_rw_wl
#define esp_vfs_fat_spiflash_unmount esp_vfs_fat_spiflash_unmount_rw_wl
#endif

static const char *TAG = "MQTT_BROKER_SERVER";

char *MOUNT_POINT = "/root";

extern std::recursive_mutex mqtt_server_mutex;

wl_handle_t mountFATFS(char *partition_label, char *mount_point)
{
    ESP_LOGI(TAG, "Initializing FAT file system");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formated before
    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 1,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};
    wl_handle_t s_wl_handle;
    esp_err_t err = esp_vfs_fat_spiflash_mount(mount_point, partition_label, &mount_config, &s_wl_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return -1;
    }
    ESP_LOGI(TAG, "Mount FAT filesystem on %s", mount_point);
    ESP_LOGI(TAG, "s_wl_handle=%" PRIi32, s_wl_handle);
    return s_wl_handle;
}

void initialise_mdns()
{

    constexpr char *TAG = "initialise_mdns";
    // initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    // set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK(mdns_hostname_set(CONFIG_MDNS_HOSTNAME));
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", CONFIG_MDNS_HOSTNAME);

    // initialize service
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_mqtt", "_tcp", 1883, NULL, 0));
}

void init_mongoose()
{
    constexpr char *TAG = "init_mongoose";

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    esp_netif_ip_info_t ip_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info));

    ESP_LOGI(TAG, "IP Address : " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "Subnet Mask: " IPSTR, IP2STR(&ip_info.netmask));
    ESP_LOGI(TAG, "Gateway    : " IPSTR, IP2STR(&ip_info.gw));

    // Initializing FAT file system
    char *partition_label = "storage";
    wl_handle_t s_wl_handle = mountFATFS(partition_label, MOUNT_POINT);
    if (s_wl_handle < 0)
    {
        ESP_LOGE(TAG, "mountFATFS fail");
        while (1)
        {
            vTaskDelay(1);
        }
    }

    /* Start MQTT Server using tcp transport */
    ESP_LOGI(TAG, "MQTT broker started on " IPSTR " using Mongoose v%s", IP2STR(&ip_info.ip), MG_VERSION);
    xTaskCreate(mqtt_server, "BROKER", 1024 * 4, NULL, 22, NULL);
    vTaskDelay(20); // You need to wait until the task launch is complete.
}


void send_mqtt_message(std::string topic, std::string message)
{

    std::lock_guard<std::recursive_mutex> lock(mqtt_server_mutex); // Ensure thread safety

    for (auto sub = s_subs.begin(); sub != s_subs.end(); sub++)
    {
        if (topic == std::string((*sub)->topic.buf, (*sub)->topic.len))
        {

            struct mg_mqtt_opts pub_opts;
            memset(&pub_opts, 0, sizeof(pub_opts)); // Zero the struct to avoid garbage values
            pub_opts.topic.buf = (char *)(topic.c_str());
            pub_opts.topic.len = topic.length();
            pub_opts.message.buf = (char *)(message.c_str());
            pub_opts.message.len = message.length();
            pub_opts.qos = 1;        // Set QoS to 1
            pub_opts.retain = false; // Set retain flag to false

            mg_mqtt_pub((*sub)->c, &pub_opts); // Publish the message

            ESP_LOGI("send_mqtt_message()", "Published message: \"%s\" to topic: \"%s\"", message.c_str(), topic.c_str());
        }
    }
}