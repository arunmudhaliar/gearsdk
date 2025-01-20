//
//  plugin_qhiredis.hpp
//  qh3server
//
//  Created by Arun A on 19/01/25.
//

#ifndef plugin_qhiredis_hpp
#define plugin_qhiredis_hpp

#include "../common/sdktypes.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// FFI export function types for hash iteration
typedef void (*redis_hash_iterator_cb)(const char* field, const char* value, void* arg);
typedef void (*redis_scan_iterator_cb)(const char* key, const char* field, const char* value, void* arg);

// Opaque struct for the qhiredis class
typedef struct qhiredis_t qhiredis_t;

// Function declarations for FFI
EXPORT qhiredis_t* qhiredis_create(const char* name, const char* redis_ip, uint16_t redis_port, const char* username, const char* password);
EXPORT void qhiredis_destroy(qhiredis_t* redis);
EXPORT int qhiredis_connect(qhiredis_t* redis, int unix_socket);
EXPORT void qhiredis_disconnect(qhiredis_t* redis);
EXPORT int qhiredis_set_value(qhiredis_t* redis, const char* key, const char* value);
EXPORT int qhiredis_set_value_exp(qhiredis_t* redis, const char* key, const char* value, int expiry_in_sec);
EXPORT int qhiredis_get_value(qhiredis_t* redis, const char* key, char* value, size_t value_len);
EXPORT int qhiredis_set_hash_value(qhiredis_t* redis, const char* hashkey, const char* field, const char* value);
EXPORT int qhiredis_get_hash_value(qhiredis_t* redis, const char* hashkey, const char* field, char* value, size_t value_len);
EXPORT int qhiredis_delete_hash_field(qhiredis_t* redis, const char* hashkey, const char* field);
EXPORT int qhiredis_incr(qhiredis_t* redis, const char* key, long long* value);
EXPORT int qhiredis_decr(qhiredis_t* redis, const char* key, long long* value);
EXPORT int qhiredis_incr_by(qhiredis_t* redis, const char* key, int delta, long long* value);
EXPORT int qhiredis_decr_by(qhiredis_t* redis, const char* key, int delta, long long* value);
EXPORT int qhiredis_expire_key(qhiredis_t* redis, const char* key, int expiry_in_sec);
EXPORT int qhiredis_delete_key(qhiredis_t* redis, const char* key);
EXPORT void qhiredis_iterate_hash(qhiredis_t* redis, const char* hashkey, void* arg, redis_hash_iterator_cb callback);
EXPORT void qhiredis_scan(qhiredis_t* redis, const char* prefix_key, void* arg, redis_scan_iterator_cb callback);

#ifdef __cplusplus
}
#endif

#endif /* plugin_qhiredis_hpp */
