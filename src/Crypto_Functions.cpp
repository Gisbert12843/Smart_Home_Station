#include "Crypto_Functions.h"

// DJB2 Hash Function
unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // DJB2 algorithm
    }
    return hash;
}

std::string Crypto_Functions::hash_to_length(const std::string ssid, int target_length)
{
    unsigned long hashedValue = hash((unsigned char *)(ssid.c_str()));

    // Convert the hash to a string and truncate it
    char buffer[target_length + 1];
    snprintf(buffer, sizeof(buffer), "%0*lx", target_length, hashedValue);
    std::string target_string(buffer);

    return target_string;
}