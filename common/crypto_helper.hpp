//
//  crypto_helper.hpp
//  common
//
//  Created by Arun A on 07/11/23.
//

#ifndef crypto_helper_hpp
#define crypto_helper_hpp

#include "./crypto/sha256.h"
#include <string.h>

class crypto_helper {
public:
    struct sha256_data {
        sha256_data(const char* in_, int sz) {
            in = in_;
            size = sz;
            memset(out, 0, sizeof(out));
        }
        const char* in = nullptr;
        int size = 0;
        uint8_t hash[SHA256_HASH_SIZE];
        char out[SHA256_HASH_SIZE * 2];
    };
    struct sha256_salt {
        sha256_salt(const char* salt_, int sz) {
            salt = salt_;
            size = sz;
        }
        const char* salt = nullptr;
        int size = 0;
    };

    static bool sha256(sha256_data& data);
    static bool sha256(sha256_data& data, sha256_salt& salt);
};
#endif /* crypto_helper_hpp */
