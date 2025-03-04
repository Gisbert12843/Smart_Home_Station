#include "UTILS/i2c_wrapper.h"

#include "driver/i2c_master.h"
#include <driver/i2c_types.h>

#include "esp_log.h"

#include <unordered_map>

#define I2C_PORT -1 // auto
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_SCL_IO GPIO_NUM_22

static std::unordered_map<int, i2c_master_dev_handle_t> i2c_dev_handle_map; // mapping slave address to dev_handle
static bool master_is_init = false;

esp_err_t i2c_functions::init_master()
{
    if (master_is_init)
        return 1;

    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags{.enable_internal_pullup = true, .allow_pd = false},
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t err;
    err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (err != ESP_OK)
        return err;
    return ESP_OK;
}

esp_err_t i2c_functions::deinit_master()
{
    if (!master_is_init)
        return 1;
    i2c_master_bus_handle_t bus_handle;
    esp_err_t err;
    err = i2c_master_get_bus_handle(I2C_PORT, &bus_handle);
    if (err != ESP_OK)
        return err;
    err = i2c_del_master_bus(bus_handle);
    if (err != ESP_OK)
        return err;
    return ESP_OK;
}

i2c_functions::slave::slave(uint16_t address) : address(address)
{
    begin_transmission();
}

esp_err_t i2c_functions::slave::begin_transmission()
{
    if (!master_is_init)
        return 1;
    if (i2c_dev_handle_map.find(address) == i2c_dev_handle_map.end())
    {
        ESP_LOGE("begin_transmission", "could not find dev_handle");
        return 2;
    }
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_handle_t bus_handle;
    esp_err_t err;
    err = i2c_master_get_bus_handle(I2C_PORT, &bus_handle);
    if (err != ESP_OK)
        return err;
    err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &i2c_dev_handle_map[address]);
    if (err != ESP_OK)
        return err;
    return ESP_OK;
}

esp_err_t i2c_functions::slave::end_transmission()
{
    if (!master_is_init)
        return ESP_ERR_INVALID_STATE;
    if (i2c_dev_handle_map.find(address) != i2c_dev_handle_map.end())
    {
        ESP_LOGE("end_transmission", "could not find dev_handle");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err;
    err = i2c_master_bus_rm_device(i2c_dev_handle_map[address]);
    if (err != ESP_OK)
        return err;

    i2c_dev_handle_map.erase(address);
    return ESP_OK;
}

esp_err_t i2c_functions::slave::transmit(uint8_t *data, size_t data_length)
{
    if (!master_is_init)
        return 1;
    if (i2c_dev_handle_map.find(address) != i2c_dev_handle_map.end())
    {
        return 2;
    }
    esp_err_t err;
    err = i2c_master_transmit(i2c_dev_handle_map[address], data, data_length, -1);
    if (err != ESP_OK)
        return err;
    return ESP_OK;
}