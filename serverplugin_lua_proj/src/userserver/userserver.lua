local serversdk = require("src.helpers.serversdk")
local sdktypes = require("src.helpers.sdktypes")
local routerserver = require("src.userserver.router")
local qmongo = require("src.helpers.qmongo")
local qhiredis = require("src.helpers.qhiredis")
local qzookeeper = require("src.helpers.qzookeeper")
local essentials = require("src.helpers.essentials")
local serverconfig = require("src.helpers.serverconfig")
local ffi = require("ffi")

ffi.cdef[[
    typedef struct qh3server qh3server;
    typedef void (*type_on_server_pre_start)(qh3server* server, void* user_arg);
    typedef void (*type_on_server_start)(qh3server* server, void* user_arg, const char* ip, uint16_t port);
    typedef void (*type_on_server_stop)(qh3server* server, void* user_arg);
    typedef void (*type_on_server_error)(qh3server* server, void* user_arg, int error_code);
    typedef void (*type_on_server_parse)(
        void* server, 
        void* user_arg,
        uint8_t* cid,
        uint16_t cid_len,
        const char* path,
        const char* buffer,
        unsigned long len,
        const char* headers_buffer,
        unsigned long headers_buffer_size
    );
]]

local server = {}
server.userserver = {}
server.userserver.__LOGTAG__ = "userserver"

-- Constructor
function server.userserver:new()
    local obj = {}
    obj.api_callbacks = {}
    obj.mongo = nil
    obj.hiredis = nil
    obj.zk = nil
    obj.zkconfig = nil
    obj.zk_value_change_callback_handle = nil
    obj.request_counter = 0
    obj.total_execution_time = 0
    obj.start_time = serversdk.sdklib.get_current_time_in_ms()
    setmetatable(obj, self)
    self.registry_id = serversdk.serverplugin.add_lua_object_to_registry(self)
    self.__index = self
    return obj
end

function server.userserver:get_mongo_driver()
    return self.mongo
end

function server.userserver:get_hiredis_driver()
    return self.hiredis
end

function server.userserver:get_qzookeeper_driver()
    return self.zk
end

function server.userserver:get_zkconfig()
    return self.zkconfig
end

function server.userserver:register_api(api_instance)
    if self.api_callbacks[api_instance:get_path()] then
        return
    end
    self.api_callbacks[api_instance:get_path()] = api_instance
    sdktypes.debug_print(sdktypes.LOG_LEVEL_0, "userserver", "api registered - " .. api_instance:get_path())
end

function server.userserver:unregister_api(path)
    if self.api_callbacks[path] then
        self.api_callbacks[path] = nil
        sdktypes.debug_print(sdktypes.LOG_LEVEL_0, "userserver", "api un-registered - " .. path)
    end
end

function server.userserver:unregister_api_instance(api_instance)
    if self.api_callbacks[api_instance:get_path()] then
        self.api_callbacks[api_instance:get_path()] = nil
        sdktypes.debug_print(sdktypes.LOG_LEVEL_0, "userserver", "api un-registered - " .. api_instance:get_path())
    end
end

local function on_zk_value_change(path, data, context)
    -- sdktypes.debug_print(sdktypes.LOG_LEVEL_0, "userserver", "api un-registered - " .. api_instance:get_path())
    print(string.format("Value changed! Path: %s, Data: %s", ffi.string(path), ffi.string(data)))
end

-- Assign FFI casted callback functions
server.userserver.on_server_pre_start = ffi.cast("type_on_server_pre_start", function(native_server, user_arg)
    print("on_server_pre_start triggered")
end)

server.userserver.on_server_start = ffi.cast("type_on_server_start", function(native_server, user_arg, ip, port)
    print("on_server_start triggered: IP = " .. ffi.string(ip) .. ", Port = " .. port)
    local hash_key = "servers:" .. serversdk.serverplugin.get_device_public_ip()
    local field_key = "server-" .. port
    local field_value = ffi.string(ip) .. ":" .. port
    local thiz = serversdk.serverplugin.get_from_registry(user_arg);
    thiz.hiredis:set_hash_value(hash_key, field_key, field_value)
end)

server.userserver.on_server_stop = ffi.cast("type_on_server_stop", function(native_server, user_arg)
    print("on_server_stop triggered")
end)

server.userserver.on_server_error = ffi.cast("type_on_server_error", function(native_server, user_arg, error_code)
    print("on_server_error triggered: Error Code = " .. error_code)
end)

server.userserver.on_server_parse = ffi.cast("type_on_server_parse", function(
    native_server, native_user_arg, native_cid, cid_len, native_path, native_buffer, len, native_headers_buffer, native_headers_buffer_size)
    local thiz = serversdk.serverplugin.get_from_registry(native_user_arg);
    thiz.request_counter = thiz.request_counter + 1
    local parse_start_time = serversdk.sdklib.get_current_time_in_ms();

    -- Using pcall to safely call the process_request function
    local success, result = pcall(function()
        return thiz.process_request(native_server, native_user_arg, native_cid, cid_len, native_path, native_buffer, len, native_headers_buffer, native_headers_buffer_size)
    end)

    -- Check if the call was successful
    if success then
        if result then
            serversdk.serverplugin.qh3server_try_send_response(native_server, native_cid, cid_len, result, #result, nil, 0)
        else
            serversdk.serverplugin.qh3server_try_send_response(native_server, native_cid, cid_len, "{}", 2, nil, 0)
        end
    else
        local error_msg = result  -- `result` will contain the error message
        print("Error occurred during process_request: " .. error_msg)
        serversdk.serverplugin.qh3server_try_send_response(native_server, native_cid, cid_len, "{}", 2, nil, 0)
    end

    local execution_time = serversdk.sdklib.get_current_time_in_ms() - parse_start_time
    thiz.total_execution_time = thiz.total_execution_time + execution_time
    thiz:calculate_rps()
end)

server.userserver.process_request = ffi.cast("type_on_server_parse", function(
    native_server, native_user_arg, native_cid, cid_len, native_path, native_buffer, len, native_headers_buffer, native_headers_buffer_size)
    -- local parse_start_time = serversdk.sdklib.get_current_time_in_ms();
    local thiz = serversdk.serverplugin.get_from_registry(native_user_arg);
    local lua_path = ffi.string(native_path)
    -- print("on_server_parse triggered: Path = " .. lua_path)
    local result = "{}"
    if thiz.api_callbacks and thiz.api_callbacks[lua_path] then
        local api_instance = thiz.api_callbacks[lua_path]
        if api_instance and api_instance.get_post_cb then
            local post_cb = api_instance:get_post_cb()
            if post_cb then
                local c_result = post_cb(native_server, native_user_arg, native_cid, cid_len, native_path, native_buffer, len, native_headers_buffer, native_headers_buffer_size)
                result = ffi.string(c_result)
            end
        end
    end
    return result;
end)

-- Function to calculate and display requests per second on the same line
function server.userserver:calculate_rps()
    local current_time = serversdk.sdklib.get_current_time_in_ms()
    local elapsed_time_ms = current_time - self.start_time

    if elapsed_time_ms >= 1000 then
        local rps = (self.request_counter * 1000) / elapsed_time_ms
        local avg_execution_time = self.total_execution_time / math.max(self.request_counter, 1) -- Avoid division by zero

        io.write(string.format("\rRequests per second: %.2f | Avg execution time: %.2f ms", tonumber(rps), tonumber(avg_execution_time)))
        -- io.write(string.format("\rRequests per second: %.2f", tonumber(rps)))
        io.flush() -- Ensure the output is written to the terminal

        -- Reset the counter and time for the next interval
        self.request_counter = 0
        self.total_execution_time = 0
        self.start_time = current_time
    end
end

-- Run function to start the router
function server.userserver:run(native_router)
    -- Create a qmongo instance
    self.mongo = qmongo:new("", "gsdk_mongodb", routerserver.router.router_config.mongodb_uri, function(error)
        print("mongo error:", error)
    end)
    self.mongo:connect()
    local redis_ip, redis_port = essentials.extract_ip_and_port(routerserver.router.router_config.redis_address)
    if not redis_ip or not redis_port then
        print("Invalid redis IP and port format")
    end

    self.hiredis = qhiredis:new("hiredis", redis_ip, redis_port, "gsdkuser", "Fr0gmoon123")
    if self.hiredis ~= nil and self.hiredis:connect(0) ~= sdktypes.EXIT_SUCCESS then
        sdktypes.debug_print(sdktypes.LOG_LEVEL_0, server.userserver.__LOGTAG__, "hiredis connect failed !!!");
        return;
    end
    
    self.zk = qzookeeper:new("lua-zk")
    if self.zk ~= nil and self.zk:connect(routerserver.router.router_config.zk_uri) ~= sdktypes.EXIT_SUCCESS then
        sdktypes.debug_print(sdktypes.LOG_LEVEL_0, server.userserver.__LOGTAG__, "zk connect failed !!!");
        return;
    else
        self.zk_value_change_callback_handle = self.zk:register_value_change_callback(on_zk_value_change, self.registry_id)
    end

    -- load server config
    self.zkconfig = serverconfig.serverconfig:new(self.zk, nil)
    local release_type = (os.getenv("SERVER_ENV") == "production") and 'prod' or 'dev'
    local config_path = routerserver.router.router_config.root_dir .. '/configs/' .. release_type .. '/runtime-config.json'
    sdktypes.debug_print(sdktypes.LOG_LEVEL_4, server.userserver.__LOGTAG__, "reading " .. config_path)
    self.zkconfig:load(config_path, self.zk, "/qh3server")

    -- local temp = self.zk:get_data("/qh3server/server_config/user_token_expiry_time", "0");

    serversdk.serverplugin.spawn_qh3server(
        native_router,
        routerserver.router.router_config.router_address, 
        routerserver.router.router_config.mongodb_uri, 
        routerserver.router.router_config.redis_address, 
        routerserver.router.router_config.zk_uri, 
        routerserver.router.router_config.root_dir, 
        routerserver.router.router_config.command_port,
        routerserver.router.router_config.router_port_return,
        routerserver.router.router_config.app_id,
        self.on_server_pre_start,
        self.on_server_start,
        self.on_server_stop,
        self.on_server_error,
        self.on_server_parse,
        self
    )
end

-- Simulate destructor using the __gc metamethod
function server.userserver:__gc()
    print(self.name .. " object is being garbage collected.")
end

return server
