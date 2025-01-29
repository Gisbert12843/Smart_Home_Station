#pragma once
#include <wholeinclude.h>
// #include "mbedtls/sha256.h"

// #include <esp32/rom/md5_hash.h>

namespace Crypto_Functions
{
    //Given any string this hashes it to a target length (default=14) using the DJB2 Algo.
    //Collision is possible. Non cryptographic.
    std::string hash_to_length(const std::string ssid, int target_length = 14);
}