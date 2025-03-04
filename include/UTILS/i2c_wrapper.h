#pragma once
#include <inttypes.h>
#include <esp_err.h>

namespace i2c_functions
{
    esp_err_t init_master();
    esp_err_t deinit_master();

    struct slave
    {
        const uint16_t address;
        slave(uint16_t address);
        ~slave()
        {
            end_transmission();
        }
        esp_err_t begin_transmission();
        esp_err_t end_transmission();
        esp_err_t transmit(uint8_t *data, size_t data_length);
    };

}