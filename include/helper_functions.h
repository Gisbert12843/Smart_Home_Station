#pragma once

#include "wholeinclude.h"
#include "esp_log.h"

namespace helper_functions
{
    inline void delay(int delay_in_ms)
    {
        ESP_LOGI("Delay()", "Delaying for %f seconds.\n", ((float)delay_in_ms) / 1000);
        vTaskDelay(delay_in_ms / portTICK_PERIOD_MS);
    }

    inline void printMemory()
    {
        ESP_LOGD(pcTaskGetName(NULL), "Stack High Water Mark: %u", static_cast<int>(uxTaskGetStackHighWaterMark(NULL)));
        ESP_LOGD(pcTaskGetName(NULL), "Free Heap Size: %u bytes\n\n", static_cast<int>(esp_get_free_heap_size()));
    }
}
