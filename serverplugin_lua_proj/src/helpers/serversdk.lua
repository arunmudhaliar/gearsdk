local ffi = require("ffi")

ffi.cdef[[
    typedef unsigned short uint16;
    typedef unsigned int uint;
    typedef unsigned long long uint64;
    typedef enum {
            LEVEL_0, LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4
        } log_lvls;
    typedef enum {
            INFO_LOG, DEBUG_LOG, WARN_LOG, ERROR_LOG, LOG_TYPE_MAX
        } elog_type;

    void setup_signal_handler();
    void pre_init_serverplugin_sdk();
    uint get_live_connection_count(void* native_server);
    const char* get_device_public_ip();
    uint64 get_crc32(const char* str, int len);
    uint64 mod_crc32(uint64 adler, const char* buf, size_t len);
    void qh3server_try_send_response(void* qh3server, uint8_t* cid, uint16_t cid_len, 
        const char* payload, size_t len, 
        const char* user_data, size_t user_data_len);
    void spawn_qh3router(
        const char* router_address,
        const char* mongodb_uri,
        const char* redis_address,
        const char* zk_uri,
        const char* root_dir,
        uint16 command_port,
        uint16 router_port_return,
        const char* app_id,
        void* pre_start_cb,
        void* start_cb,
        void* stop_cb,
        void* error_cb,
        void* user_arg
    );

    typedef struct {} qh3server;
    typedef void (*type_on_server_pre_start)(qh3server* server, void* user_arg);
    typedef void (*type_on_server_start)(qh3server* server, void* user_arg, const char* ip, uint16_t port);
    typedef void (*type_on_server_stop)(qh3server* server, void* user_arg);
    typedef void (*type_on_server_error)(qh3server* server, void* user_arg, int error_code);
    typedef void (*type_on_server_parse)(qh3server* server, void* user_arg, uint8_t* cid, uint16_t cid_len, const char* path, const char* buffer, unsigned long len, const char* headers_buffer, unsigned long headers_buffer_size);
    void spawn_qh3server(
        void* router,
        const char* server_address,
        const char* mongodb_uri,
        const char* redis_address,
        const char* zk_uri,
        const char* root_dir,
        uint16 command_port,
        uint16 router_port_return,
        const char* app_id,
        void* pre_start_cb,
        void* start_cb,
        void* stop_cb,
        void* error_cb,
        void* parse_cb,
        void* user_arg
    );
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

local serverplugin = {}
serverplugin.get_device_public_ip = function()
    return ffi.string(sdklib.get_device_public_ip());
end

local registry = {}
local id_counter = 0

local function add_to_registry(obj)
    id_counter = id_counter + 1
    registry[id_counter] = obj
    return id_counter
end

local function remove_from_registry(id)
    registry[id] = nil
end

serverplugin.add_lua_object_to_registry = function (lua_obj)
    local lua_obj_id = add_to_registry(lua_obj)
    return ffi.cast("void*", lua_obj_id)
end

serverplugin.get_from_registry = function(user_arg_c)
    local retrieved_id = tonumber(ffi.cast("int", user_arg_c))
    return registry[retrieved_id]
end

serverplugin.get_and_clear_from_registry = function(user_arg_c)
    local retrieved_id = tonumber(ffi.cast("int", user_arg_c))
    local return_obj = registry[retrieved_id];
    remove_from_registry(retrieved_id);
    return return_obj;
end

serverplugin.spawn_qh3router = function(router_address, mongodb_uri, redis_address, zk_uri, root_dir, command_port, router_port_return, app_id, pre_start_cb, start_cb, stop_cb, error_cb, user_arg)
    local pre_start_cb_c = ffi.cast("void(*)(void*)", pre_start_cb)
    local start_cb_c = ffi.cast("void(*)(void*)", start_cb)
    local stop_cb_c = ffi.cast("void(*)(void*)", stop_cb)
    local error_cb_c = ffi.cast("void(*)(void*)", error_cb)
    -- local user_arg_c = ffi.cast("void*", user_arg)

    -- local lua_obj = { data = "Some Lua Object" }
    -- local lua_obj_id = add_to_registry(user_arg)
    local user_arg_c = serverplugin.add_lua_object_to_registry(user_arg);
    sdklib.spawn_qh3router(router_address, mongodb_uri, redis_address, zk_uri, root_dir, command_port, router_port_return, app_id, 
        pre_start_cb_c, start_cb_c, stop_cb_c, error_cb_c, user_arg_c);
end

serverplugin.spawn_qh3server = function(native_router, server_address, mongodb_uri, redis_address, zk_uri, root_dir, command_port, router_port_return, app_id,
    pre_start_cb, start_cb, stop_cb, error_cb, parse_cb, user_arg)
    local pre_start_cb_c = ffi.cast("type_on_server_pre_start", pre_start_cb)
    local start_cb_c = ffi.cast("type_on_server_start", start_cb)
    local stop_cb_c = ffi.cast("type_on_server_stop", stop_cb)
    local error_cb_c = ffi.cast("type_on_server_error", error_cb)
    local parse_cb_c = ffi.cast("type_on_server_parse", parse_cb)
    local user_arg_c = serverplugin.add_lua_object_to_registry(user_arg);
    sdklib.spawn_qh3server(native_router, server_address, mongodb_uri, redis_address, zk_uri, root_dir, command_port, router_port_return, app_id,
        pre_start_cb_c, start_cb_c, stop_cb_c, error_cb_c, parse_cb_c, user_arg_c);
end

serverplugin.qh3server_try_send_response = function(native_server, cid, cid_len, 
                                 payload, len, 
                                 user_data, user_data_len)
    sdklib.qh3server_try_send_response(native_server, cid, cid_len, 
                                                payload, len, 
                                                user_data, user_data_len);
end


print(serverplugin.get_device_public_ip())
sdklib.setup_signal_handler()
sdklib.pre_init_serverplugin_sdk()

print("Server plugin loaded successfully!")
return {
    serverplugin = serverplugin;
    sdklib = sdklib;
}