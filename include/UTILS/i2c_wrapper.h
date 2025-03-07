#pragma once
#include <inttypes.h>
#include <esp_err.h>

namespace i2c_functions
{
    /**
     * @brief Initialize the I2C master bus with predefined configuration
     *
     * This function configures and initializes the I2C master bus with settings defined
     * in header constants (I2C_PORT, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO).
     * Internal pull-ups are enabled by default.
     *
     * @note The function will fail if the master is already initialized
     *
     * @return esp_err_t
     *         - ESP_OK: I2C master initialized successfully
     *         - ESP_FAIL: Master already initialized
     *         - Other error codes from i2c_new_master_bus()
     */
    esp_err_t init_master();

    /**
     * @brief Deinitializes the I2C master driver
     *
     * This function cleans up resources used by the I2C master driver.
     * It retrieves the bus handle and then deletes the master bus.
     * If the master is not initialized, the function will return ESP_FAIL.
     *
     * @return
     *     - ESP_OK: Success
     *     - ESP_FAIL: Master was not initialized
     *     - Other error codes from I2C driver on failure
     *
     * @note After calling this function, the I2C master functionality will no longer be available
     * until it is initialized again.
     */
    esp_err_t deinit_master();

    struct slave
    {
        const uint16_t address;
        slave();
        slave(uint16_t address);
        ~slave();

        /*
         * @brief Begin the transmission to the slave device
         *
         * This function begins the transmission to the slave device with the given address.
         * Has to be called before any transmission to the slave device.
         * Requires the master to be initialized.
         * 
         * @return
         *     - ESP_OK: Transmission started successfully
         *     - ESP_ERR_INVALID_STATE: Master not initialized
         *     - ESP_ERR_NOT_FOUND: Device handle not found
         *     - Other error codes from I2C driver on failure
         */
        esp_err_t begin_transmission();
        /*
         * @brief End the transmission to the slave device
         *
         * This function ends the transmission to the slave device with the given address.
         *
         * @return
         *     - ESP_OK: Transmission ended successfully
         *     - ESP_ERR_INVALID_STATE: Master not initialized
         *     - ESP_ERR_NOT_FOUND: Device handle not found
         *     - Other error codes from I2C driver on failure
         */
        esp_err_t end_transmission();
        /*
         * @brief Transmit data to the slave device
         *
         * This function transmits data to the slave device with the given address.
         * The transmission has to be started with begin_transmission() before calling this function.
         *
         * @param data Pointer to the data buffer to be transmitted
         * @param data_length Length of the data buffer
         *
         * @return
         *     - ESP_OK: Data transmitted successfully
         *     - ESP_ERR_INVALID_STATE: Master not initialized
         *     - ESP_ERR_NOT_FOUND: Device handle not found
         *     - Other error codes from I2C driver on failure
         */
        esp_err_t transmit(uint8_t *data, size_t data_length);

        /*
         * @brief Receive data from the slave device
         *
         * This function receives data from the slave device with the given address.
         *
         * @param data Pointer to the data buffer to store the received data
         * @param data_length Length of the data buffer
         *
         * @return
         *     - ESP_OK: Data received successfully
         *     - ESP_ERR_INVALID_STATE: Master not initialized
         *     - ESP_ERR_NOT_FOUND: Device handle not found
         *     - Other error codes from I2C driver on failure
         */
        esp_err_t receive(uint8_t *data, size_t data_length);

        /*
         * @brief Probe the slave device
         *
         * This function probes the slave device with the given address.
         * If the device is found, it will return ESP_OK.
         * Requires the master to be initialized.
         *
         * @return
         *     - ESP_OK: Slave device found
         *     - ESP_ERR_NOT_FOUND: Slave device not found
         *     - Other error codes from I2C driver on failure
         */
        esp_err_t prope();
        esp_err_t prope(i2c_master_bus_handle_t *bus_handle);
    };
}