local ffi = require("ffi")

-- Define the C functions and types
ffi.cdef[[
    typedef struct qzookeeper_t qzookeeper_t;
    typedef void (*qzookeeper_value_changed_cb)(const char* path, const char* data, void* context);

    // Factory functions
    qzookeeper_t* qzookeeper_create(const char* name);
    void qzookeeper_destroy(qzookeeper_t* instance);

    // Core methods
    int qzookeeper_connect(qzookeeper_t* instance, const char* url);
    void qzookeeper_shutdown(qzookeeper_t* instance);
    bool qzookeeper_is_running(qzookeeper_t* instance);
    bool qzookeeper_is_zk_active(qzookeeper_t* instance);
    int qzookeeper_get_connection_state(qzookeeper_t* instance);

    // Data handling
    int qzookeeper_get_data(qzookeeper_t* instance, const char* zk_path, char* result, size_t result_size, const char* default_value);
    int qzookeeper_set_data(qzookeeper_t* instance, const char* zk_path, const char* data);
    int qzookeeper_delete_path(qzookeeper_t* instance, const char* zk_path);

    // Value change callback
    void qzookeeper_register_value_change_callback(qzookeeper_t* instance, qzookeeper_value_changed_cb callback, void* context);
    void qzookeeper_unregister_value_change_callback(qzookeeper_t* instance, qzookeeper_value_changed_cb callback, void* context);

    // Utility methods
    const char* qzookeeper_state_to_string(int state);
    const char* qzookeeper_type_to_string(int state);
]]

-- Detect platform and load library
local lib_path
local lib_serverplugin_debug = "libserverplugin-debug"
local lib_serverplugin_release = "libserverplugin"
local lib_serverplugin = (os.getenv("SERVER_ENV") == "production") and lib_serverplugin_release or lib_serverplugin_debug
if ffi.os == "OSX" then
    lib_path = "../serverplugin/" .. lib_serverplugin .. ".dylib"
elseif ffi.os == "Linux" then
    lib_path = "../serverplugin/" .. lib_serverplugin .. ".so"
else
    error("Unsupported platform")
end
local sdklib = ffi.load(lib_path)

-- Define the Lua class
local qzookeeper = {}
qzookeeper.__index = qzookeeper

-- Constructor
function qzookeeper:new(name)
    local obj = {}
    setmetatable(obj, self)
    obj.zk_instance = sdklib.qzookeeper_create(name)
    if obj.zk_instance == nil then
        error("Failed to create qzookeeper instance")
    end
    return obj
end

-- Destructor
function qzookeeper:destroy()
    if self.zk_instance ~= nil then
        sdklib.qzookeeper_destroy(self.zk_instance)
        self.zk_instance = nil
    end
end

-- Connect to Zookeeper
function qzookeeper:connect(url)
    return sdklib.qzookeeper_connect(self.zk_instance, url)
end

-- Shutdown Zookeeper
function qzookeeper:shutdown()
    sdklib.qzookeeper_shutdown(self.zk_instance)
end

-- Check if running
function qzookeeper:is_running()
    return sdklib.qzookeeper_is_running(self.zk_instance)
end

-- Check if Zookeeper is active
function qzookeeper:is_zk_active()
    return sdklib.qzookeeper_is_zk_active(self.zk_instance)
end

-- Get connection state
function qzookeeper:get_connection_state()
    return sdklib.qzookeeper_get_connection_state(self.zk_instance)
end

-- Get data
function qzookeeper:get_data(zk_path, default_value)
    local result_buffer = ffi.new("char[?]", 1024)
    local result = sdklib.qzookeeper_get_data(self.zk_instance, zk_path, result_buffer, 1024, default_value)
    if result == 0 then
        return ffi.string(result_buffer)
    else
        return nil, result
    end
end

-- Set data
function qzookeeper:set_data(zk_path, data)
    return sdklib.qzookeeper_set_data(self.zk_instance, zk_path, data)
end

-- Delete path
function qzookeeper:delete_path(zk_path)
    return sdklib.qzookeeper_delete_path(self.zk_instance, zk_path)
end

-- Register value change callback
function qzookeeper:register_value_change_callback(callback, context)
    local cb = ffi.cast("qzookeeper_value_changed_cb", callback)
    sdklib.qzookeeper_register_value_change_callback(self.zk_instance, cb, context)
    return cb -- Return the callback so it can be freed later
end

-- Unregister value change callback
function qzookeeper:unregister_value_change_callback(callback, context)
    sdklib.qzookeeper_unregister_value_change_callback(self.zk_instance, callback, context)
end

-- Convert state to string
function qzookeeper:state_to_string(state)
    return ffi.string(sdklib.qzookeeper_state_to_string(state))
end

-- Convert type to string
function qzookeeper:type_to_string(state)
    return ffi.string(sdklib.qzookeeper_type_to_string(state))
end

-- Destructor for garbage collection
function qzookeeper:__gc()
    self:destroy()
end

return qzookeeper
