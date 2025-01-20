//
//  plugin_qhiredis.cpp
//  qh3server
//
//  Created by Arun A on 19/01/25.
//

#include "plugin_qhiredis.hpp"
#include "../qhiredis/source/qhiredis.hpp"

extern "C" {

// Define the opaque struct for the qhiredis class instance
struct qhiredis_t {
    qhiredis* instance;
};

EXPORT qhiredis_t* qhiredis_create(const char* name, const char* redis_ip, uint16_t redis_port, const char* username, const char* password) {
    qhiredis_t* redis = DEBUG_NEW qhiredis_t;
    if (!redis) return nullptr;
    memset(redis, 0, sizeof(qhiredis_t));
    
    redis->instance = new qhiredis(name, redis_ip, redis_port, username, password);
    if (!redis->instance) {
        GX_DELETE(redis);
        return nullptr;
    }
    return redis;
}

EXPORT void qhiredis_destroy(qhiredis_t* redis) {
    if (!redis) return;
    GX_DELETE(redis->instance);
    GX_DELETE(redis);
}

EXPORT int qhiredis_connect(qhiredis_t* redis, int unix_socket) {
    if (!redis || !redis->instance) return -1;
    return redis->instance->connect_redis(unix_socket);
}

EXPORT void qhiredis_disconnect(qhiredis_t* redis) {
    if (!redis || !redis->instance) return;
    redis->instance->disconnect_redis();
}

EXPORT int qhiredis_set_value(qhiredis_t* redis, const char* key, const char* value) {
    if (!redis || !redis->instance) return -1;
    return redis->instance->set_value(key, value);
}

EXPORT int qhiredis_set_value_exp(qhiredis_t* redis, const char* key, const char* value, int expiry_in_sec) {
    if (!redis || !redis->instance) return -1;
    return redis->instance->set_value(key, value, expiry_in_sec);
}

EXPORT int qhiredis_get_value(qhiredis_t* redis, const char* key, char* value, size_t value_len) {
    if (!redis || !redis->instance) return -1;
    qstring result;
    int status = redis->instance->get_value(key, result);
    if (status == 0) {
        strncpy(value, result.c_str(), value_len - 1);
        value[value_len - 1] = '\0';
    }
    return status;
}

EXPORT int qhiredis_set_hash_value(qhiredis_t* redis, const char* hashkey, const char* field, const char* value) {
    if (!redis || !redis->instance) return -1;
    return redis->instance->set_hash_value(hashkey, field, value);
}

EXPORT int qhiredis_get_hash_value(qhiredis_t* redis, const char* hashkey, const char* field, char* value, size_t value_len) {
    if (!redis || !redis->instance) return -1;
    qstring result;
    int status = redis->instance->get_hash_value(hashkey, field, result);
    if (status == 0) {
        strncpy(value, result.c_str(), value_len - 1);
        value[value_len - 1] = '\0';
    }
    return status;
}

EXPORT int qhiredis_delete_hash_field(qhiredis_t* redis, const char* hashkey, const char* field) {
    if (!redis || !redis->instance) return -1;
    return redis->instance->delete_hash_field(hashkey, field);
}

// Implement other wrapper functions similarly, calling the appropriate methods on the `qhiredis` instance.

EXPORT void qhiredis_iterate_hash(qhiredis_t* redis, const char* hashkey, void* arg, redis_hash_iterator_cb callback) {
    if (!redis || !redis->instance || !callback) return;
    redis->instance->iterate_hash(hashkey, arg, callback);
}

EXPORT void qhiredis_scan(qhiredis_t* redis, const char* prefix_key, void* arg, redis_scan_iterator_cb callback) {
    if (!redis || !redis->instance || !callback) return;
    redis->instance->scan(prefix_key, arg, callback);
}

} // extern "C"
