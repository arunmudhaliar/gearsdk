//
//  Copyright 2024 homenet25
//  crypto_helper.cpp
//  common
//
//  Created by Arun A on 07/11/23.
//

#include "crypto_helper.hpp"


bool crypto_helper::sha256(sha256_data& data) {
    SHA256_CTX foo;
    sha256_init(&foo);
    sha256_update(&foo, data.in, data.size);
    sha256_final(&foo, data.hash);
    for (int i = 0; i < SHA256_HASH_SIZE; i++) {
        snprintf(data.out + i * 2, 3, "%02x", data.hash[i]);
        //        printf ("%02x", data.hash[i]);
    }
    return true;
}

bool crypto_helper::sha256(sha256_data& data, sha256_salt& salt) {
    // method used : hash=SHA256(SHA256(data)+salt)
    SHA256_CTX foo;
    sha256_init(&foo);
    sha256_update(&foo, data.in, data.size);
    uint8_t data_hash[SHA256_HASH_SIZE];
    sha256_final(&foo, data_hash);
    char data_out[SHA256_HASH_SIZE * 2];
    for (int i = 0; i < SHA256_HASH_SIZE; i++) {
        snprintf(data_out + i * 2, 3, "%02x", data_hash[i]);
    }

    char* data_out_plus_salt = new char[sizeof(data_out) + salt.size];
    memcpy(data_out_plus_salt, data_out, sizeof(data_out));
    memcpy(data_out_plus_salt + sizeof(data_out), salt.salt, salt.size);
    sha256_init(&foo);
    sha256_update(&foo, data_out_plus_salt, sizeof(data_out) + salt.size);
    sha256_final(&foo, data.hash);
    for (int i = 0; i < SHA256_HASH_SIZE; i++) {
        snprintf(data.out + i * 2, 3, "%02x", data.hash[i]);
    }
    delete[] data_out_plus_salt;
    data_out_plus_salt = nullptr;
    return true;
}
