#pragma once
#include <wholeinclude.h>
#include "Crypto_Functions.h"
#include "nvs_flash.h"
#include "nvs.h"

/**
 * This Abstraction Layer for the ESP32 NVS offers easy to use function to implement in almost any relevant projects.
 * 
 * How to use:
 * 
 * - Before any function is used init() has to be called.
 * 
 * - Available are the following functions:
 *      - init (has to be called first atleast once, inits the NVS System)
 * 
 *      - resetNVS (erases everything on the NVS)
 * 
 *      - checkKeyExistence (returns true if any any data is stored under the given key, else false)
 * 
 *      - getSizeFromKey (first parameter )
 * 
 *      - getValueFromKey (Overloaded function for almost all datatypes currently supported by the ESP IDF)
 *        expects A: a key name to be searched for and B: a parameter the located value will be stored in
 *        C: the size of the allocated target storage container from the second parameter,
 *        can either be set to 0 if no allocation of the target container has been established
 *        or be set to the size of the prior allocated container given in parameter B
 *        -> returns any Errors that have occured or ESP_OK.
 * 
 *      - setValueToKey (Overloaded function for almost all datatypes currently supported by the ESP IDF)
 *        expects A: a key name to be inserted and B: a value that will be stored in the NVS
 *        C: For Blob data a third parameter "size" is expected that indicates the size of the object behind the void pointer (use sizeof() on the Object itself (!not the pointer to the Object))
 *        Any pointer to any valid Object may be supplied via the value parameter, this includes UDTs, casting to void* prior to supplying via parameter is not expected.
 *        -> returns any Errors that have occured or ESP_OK.

 * 
 * - currently only one namespace is available, it can be renamed by the user by changing the value of "nvs_namespace_name"
 * 
 * - the supplied key_name will be hashed using a simple DJB2 Algorythm, this will not be noticed by the user as long he does not use external functions or direct calls to the NVS System.
 *   The reason behind that is that the NVS key has a limited length of only 15 characters, too short for most SSIDs. To still support a SSID-Password as a key-value pair, I implemented this simple hash function.
 */

namespace nvs_wrapper
{
    inline int entry_count = 0;
    inline constexpr char *nvs_namespace_name = "NVS:";
    inline nvs_handle_t nvs_namespace_handle;

    void init();

    // This will delete all currently saved NVS Data
    esp_err_t resetNVS();

    bool checkKeyExistence(std::string key_name);

    esp_err_t getValueFromKey(std::string key_name, int32_t &retrieved_value, bool use_prehashed_key = false);

    esp_err_t getValueFromKey(std::string key_name, std::string &retrieved_value, bool use_prehashed_key = false);

    //for void* aka blob_types freeing of the allocated heap memory is to be handled by the user
    esp_err_t getValueFromKey(std::string key_name, void* retrieved_value, size_t &data_length, bool use_prehashed_key = false);
    esp_err_t getSizeFromKey(std::string key_name, size_t &data_length, bool save_to_nvs );

    //working
    esp_err_t setValueToKey(std::string key_name, int32_t value);

    //working
    esp_err_t setValueToKey(std::string key_name, std::string value);

    //this variant is currently not working to 100%
    esp_err_t setValueToKey(std::string key_name, void *value, size_t blob_size);
}