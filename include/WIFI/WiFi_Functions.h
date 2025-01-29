#pragma once

#include "wholeinclude.h"
#include "utils/nvs_wrapper.h"

#include "utils/global_flags.h"

namespace WiFi_Functions
{
    inline EventGroupHandle_t wifi_event_group;

    inline EventBits_t WiFi_DISCONNECTED_BIT = BIT0;
    inline EventBits_t WIFI_CONNECTED_BIT = BIT1;
    inline EventBits_t WIFI_FAIL_BIT = BIT2;
    inline EventBits_t DPP_AUTH_FAIL_BIT = BIT4;
    inline EventBits_t DPP_TIMEOUT_BIT = BIT5;
    inline EventBits_t DPP_ERROR_BIT = BIT6;

    // State of WiFi Init, changed when calling init() and de_init()
    inline bool init_state = false;

    // WiFiEventHandlers
    inline esp_event_handler_instance_t wifi_event_handler_instance = NULL;
    inline esp_event_handler_instance_t ip_event_handler_instance = NULL;

    inline esp_netif_t *sta_netif;
    inline esp_netif_t *ap_netif;

    inline wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    inline char *dpp_uri = nullptr;

    inline TaskHandle_t Fail_Bit_Listener_Handle = NULL;
    // Bits for following the status of the WiFI Connection to the current AP
    // Used by e.g. wifi_fail_bit_listener

    class AP_Credentials
    {
    private:
        std::string wifi_ssid = "";
        std::string wifi_password = "";

        // Singleton instance
        inline static AP_Credentials *instance = nullptr;

        // Private constructor to prevent instantiation
        AP_Credentials(std::string p_wifi_ssid, std::string pwifi_password) : wifi_ssid(p_wifi_ssid), wifi_password(pwifi_password) {}
        AP_Credentials() {}

        // Private copy constructor to prevent copying
        AP_Credentials(const AP_Credentials &) = delete;

        // Private assignment operator to prevent assignment
        AP_Credentials &operator=(const AP_Credentials &) = delete;

        static esp_err_t setCredentialToNVS(AP_Credentials *cred);

        static esp_err_t setCredentialToNVS(std::string wifi_ssid, std::string wifi_password);

        void setWifi_ssid(const std::string &new_ssid)
        {
            wifi_ssid = new_ssid;
        }

        void setWifi_password(const std::string &new_password)
        {
            wifi_password = new_password;
        }

    public:
        static AP_Credentials *getInstance()
        {
            if (instance == nullptr)
            {
                instance = new AP_Credentials();
            }
            return instance;
        }
        static AP_Credentials *setInstance(std::string p_wifi_ssid, std::string pwifi_password)
        {
            if (instance == nullptr)
            {
                instance = new AP_Credentials(p_wifi_ssid, pwifi_password);
            }
            else
            {
                instance->setWifi_ssid(p_wifi_ssid);
                instance->setWifi_password(pwifi_password);
            }
            return instance;
        }

        ~AP_Credentials()
        {
            // Optional: clean-up code here
        }

        static std::string getWifi_ssid()
        {
            return getInstance()->wifi_ssid;
        }

        static std::string getWifi_password()
        {
            return getInstance()->wifi_password;
        }

        static AP_Credentials *saveAPConfiguration(const std::string &ssid, const std::string &password, bool update_NVS = true);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AP_Credentials *loadCredentialsFromNVS(std::string p_nvs_namespace_name = nvs_wrapper::nvs_namespace_name);

    void init();
    void reset_wifi();
    void de_init();

    bool connect_to_ap(AP_Credentials *connection);

    bool connect_to_nearest_ap();

    std::string find_available_networks();

    // returns 0 when not found, 1 if found and 2 on error
    int check_network_availability(std::string ssid);

    // Waits for the WiFi_FailBit to be set.
    // Then uses an infinity loop to scan if the previous network is available again.
    // If Errors persist during the scanning -> device restart
    // If previous network was not found in 5 Minutes try another stored network if it is available.
    // If that also fails return to the infinity loop else close the function
    void wifi_fail_bit_listener();
    void wifi_event_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data);
}

namespace WiFi_Task
{
    inline void Task_Start_Fail_Bit_Listener(void *params)
    {
        WiFi_Functions::wifi_fail_bit_listener();
        std::cout << "\n\n deleting Task_Start_Fail_Bit_Listener\n\n";
        WiFi_Functions::Fail_Bit_Listener_Handle = NULL;
        vTaskDelete(NULL);
    }
}

void stop_wifi_and_advertise_as_AP();