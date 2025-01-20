//
//  plugin_qzookeeper.cpp
//  qh3server
//
//  Created by Arun A on 19/01/25.
//

#include "plugin_qzookeeper.hpp"
#include "../qzookeeper/source/qzookeeper.hpp"

struct qzookeeper_t {
    qzookeeper* instance;
    std::map<qzookeeper_value_changed_cb, void*> value_change_callbacks;
};

EXPORT qzookeeper_t* qzookeeper_create(const char* name) {
    return new qzookeeper_t{new qzookeeper(name)};
}

EXPORT void qzookeeper_destroy(qzookeeper_t* instance) {
    delete instance->instance;
    delete instance;
}

EXPORT int qzookeeper_connect(qzookeeper_t* instance, const char* url) {
    int result = instance->instance->connect(url);
    if (result == EXIT_SUCCESS) {
        instance->instance->register_value_change_callback(qzookeeper_value_change_callback_internal, instance);
    }
    return result;
}

EXPORT void qzookeeper_shutdown(qzookeeper_t* instance) {
    instance->instance->shutdown();
}

EXPORT bool qzookeeper_is_running(qzookeeper_t* instance) {
    return instance->instance->is_running();
}

EXPORT bool qzookeeper_is_zk_active(qzookeeper_t* instance) {
    return instance->instance->is_zk_active();
}

EXPORT int qzookeeper_get_connection_state(qzookeeper_t* instance) {
    return instance->instance->get_connection_state();
}

EXPORT int qzookeeper_get_data(qzookeeper_t* instance, const char* zk_path, char* result, size_t result_size, const char* default_value) {
    qstring data;
    int rc = instance->instance->get_data(zk_path, data, default_value);
    if (rc == 0 && result) {
        strncpy(result, data.c_str(), result_size);
    }
    return rc;
}

EXPORT int qzookeeper_set_data(qzookeeper_t* instance, const char* zk_path, const char* data) {
    return instance->instance->set_data(zk_path, data);
}

EXPORT int qzookeeper_delete_path(qzookeeper_t* instance, const char* zk_path) {
    return instance->instance->delete_path(zk_path);
}

void qzookeeper_value_change_callback_internal(const qstring& path, const qstring& data, void* context) {
    qzookeeper_t* zkt = static_cast<qzookeeper_t*>(context);
    for (std::map<qzookeeper_value_changed_cb, void*>::iterator it = zkt->value_change_callbacks.begin(); it != zkt->value_change_callbacks.end(); it++) {
        it->first(path.c_str(), data.c_str(), it->second);
    }
}

EXPORT void qzookeeper_register_value_change_callback(qzookeeper_t* instance, qzookeeper_value_changed_cb callback, void* context) {
    std::map<qzookeeper_value_changed_cb, void*>::iterator it = instance->value_change_callbacks.find(callback);
    if (it != instance->value_change_callbacks.end()) {
        return;
    }
    instance->value_change_callbacks[callback] = context;
}

EXPORT void qzookeeper_unregister_value_change_callback(qzookeeper_t* instance, qzookeeper_value_changed_cb callback, void* context) {
    std::map<qzookeeper_value_changed_cb, void*>::iterator it = instance->value_change_callbacks.find(callback);
    if (it != instance->value_change_callbacks.end()) {
        instance->value_change_callbacks.erase(it);
    }
}

EXPORT const char* qzookeeper_state_to_string(int state) {
    return qzookeeper::state_to_string(state);
}

EXPORT const char* qzookeeper_type_to_string(int state) {
    return qzookeeper::type_to_string(state);
}
