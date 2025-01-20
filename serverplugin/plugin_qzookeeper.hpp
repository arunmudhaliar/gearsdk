//
//  plugin_qzookeeper.hpp
//  qh3server
//
//  Created by Arun A on 19/01/25.
//

#ifndef plugin_qzookeeper_hpp
#define plugin_qzookeeper_hpp

#include "../common/sdktypes.hpp"
#include "../common/qstring.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declare the qzookeeper structure
typedef struct qzookeeper_t qzookeeper_t;

// Factory functions
EXPORT qzookeeper_t* qzookeeper_create(const char* name);
EXPORT void qzookeeper_destroy(qzookeeper_t* instance);

// Core methods
EXPORT int qzookeeper_connect(qzookeeper_t* instance, const char* url);
EXPORT void qzookeeper_shutdown(qzookeeper_t* instance);
EXPORT bool qzookeeper_is_running(qzookeeper_t* instance);
EXPORT bool qzookeeper_is_zk_active(qzookeeper_t* instance);
EXPORT int qzookeeper_get_connection_state(qzookeeper_t* instance);

// Data handling
EXPORT int qzookeeper_get_data(qzookeeper_t* instance, const char* zk_path, char* result, size_t result_size, const char* default_value);
EXPORT int qzookeeper_set_data(qzookeeper_t* instance, const char* zk_path, const char* data);
EXPORT int qzookeeper_delete_path(qzookeeper_t* instance, const char* zk_path);

// Value change callback
typedef void (*qzookeeper_value_changed_cb)(const char* path, const char* data, void* context);
EXPORT void qzookeeper_register_value_change_callback(qzookeeper_t* instance, qzookeeper_value_changed_cb callback, void* context);
EXPORT void qzookeeper_unregister_value_change_callback(qzookeeper_t* instance, qzookeeper_value_changed_cb callback, void* context);

// Utility methods
EXPORT const char* qzookeeper_state_to_string(int state);
EXPORT const char* qzookeeper_type_to_string(int state);

void qzookeeper_value_change_callback_internal(const qstring& path, const qstring& data, void* context);

#ifdef __cplusplus
}
#endif


#endif /* plugin_qzookeeper_hpp */
