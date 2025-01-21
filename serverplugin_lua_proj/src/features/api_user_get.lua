local serversdk = require("src.helpers.serversdk")
local cjson = require("cjson")
local api_interface = require("src.features.api_interface")
local ffi = require("ffi")
local messages = require("src.userserver.messages")
local essentials = require("src.helpers.essentials")
local sdktypes = require("src.helpers.sdktypes")
-- local bit32 = require("bit32")

-- Create the api_user_get class
local api_user_get = {}
api_user_get.__index = api_user_get
api_user_get.DEFAULT_USER_TOKEN_EXPIRY_TIME = 300

-- Inherit from the interface_api
setmetatable(api_user_get, {__index = api_interface.interface_api})

-- Constructor
function api_user_get:new()
    local self = setmetatable({}, api_user_get)
    -- -- Check if this class implements the interface
    -- api_interface.check_interface_implementation(self, api_interface.interface_api)
    return self
end

-- Implement the methods required by the interface
function api_user_get:get_path()
    return "/user_get"
end

function api_user_get:get_post_cb()
    return self.parse_user_get
end

-- local openssl = require("openssl")
api_user_get.parse_user_get = ffi.cast("type_on_api_parse", function(
    native_server, native_user_arg, native_cid, cid_len, native_path, native_buffer, len, native_headers_buffer, native_headers_buffer_size)

    local user_server_instance = serversdk.serverplugin.get_from_registry(native_user_arg)
    local user_get_msg_rq = cjson.decode(ffi.string(native_buffer, len));

    local native_crc = serversdk.sdklib.mod_crc32(0, nil, 0);
    native_crc = serversdk.sdklib.mod_crc32(native_crc, user_get_msg_rq.device.sys_name, #user_get_msg_rq.device.sys_name);
    native_crc = serversdk.sdklib.mod_crc32(native_crc, user_get_msg_rq.device.node_name, #user_get_msg_rq.device.node_name);
    native_crc = serversdk.sdklib.mod_crc32(native_crc, user_get_msg_rq.device.release, #user_get_msg_rq.device.release);
    native_crc = serversdk.sdklib.mod_crc32(native_crc, user_get_msg_rq.device.arch, #user_get_msg_rq.device.arch);

    local crc = tonumber(ffi.cast("int", native_crc))
    -- local crc_32bit = crc & 0xFFFFFFFF
    -- local crc_32bit = bit32.band(crc, 0xFFFFFFFF)

    local user_get_msg_response = messages.res_msg_user_get:new()
    user_get_msg_response.pid = string.format("%x", crc)  -- Convert crc to a hexadecimal string
    user_get_msg_response.user_name = string.format("guest-%x", crc)

    local time_result = essentials.get_time_utc_readable()
    local last_login_utc_time_value = time_result.utc_date_number
    user_get_msg_response.last_login = time_result.utc_date_string

    local hiredis_driver = user_server_instance:get_hiredis_driver()
    local redis_format_pid = "tokens:" .. user_get_msg_response.pid
    local token_in_redis = hiredis_driver:get_value(redis_format_pid);
    if token_in_redis ~= nil and #token_in_redis > 0 then
        user_get_msg_response.token = token_in_redis or ''
        sdktypes.debug_print(sdktypes.LOG_LEVEL_4, api_user_get.__LOGTAG__, string.format("token '%s' retrieved from redis for user id : %s", user_get_msg_response.token, crc))
    else
        user_get_msg_response.token = essentials.sha256(cjson.encode(user_get_msg_response))
        sdktypes.debug_print(sdktypes.LOG_LEVEL_4, api_user_get.__LOGTAG__, string.format("new token '%s' for user id : %s", user_get_msg_response.token, crc))
    end

    local user_token_expiry_time = user_server_instance:get_zkconfig():get_int32("server_config/user_token_expiry_time", api_user_get.DEFAULT_USER_TOKEN_EXPIRY_TIME)
    -- Set the token in Redis and check the result
    local result = hiredis_driver:set_value(redis_format_pid, user_get_msg_response.token, user_token_expiry_time)
    if result ~= 0 then
        sdktypes.debug_error(sdktypes.api_user_get.__LOGTAG__, "Failed to set token on redis.")
    end

    local gservers_map = {}
    api_user_get:get_gservers(user_server_instance, gservers_map)
    user_get_msg_response.gservers = api_user_get:convert_gservers_map(gservers_map)

    local query_result = user_server_instance:get_mongo_driver():find_and_upsert(
        'users',
        -- Lambda function for find query
        function(find_query)
            find_query['user.pid'] = user_get_msg_response.pid
        end,
        -- Lambda function for update query
        function(update_query)
            update_query['user.last_login'] = user_get_msg_response.last_login
            update_query['user.last_login_timestamp'] = last_login_utc_time_value
        end,
        -- Lambda function for insert query
        function(insert_query)
            insert_query['user.pid'] = user_get_msg_response.pid
            insert_query['user.name'] = user_get_msg_response.user_name
            insert_query['user.device.sys_name'] = user_get_msg_rq.device.sys_name
            insert_query['user.device.node_name'] = user_get_msg_rq.device.node_name
            insert_query['user.device.arch'] = user_get_msg_rq.device.arch
        end
    )

    if query_result == sdktypes.EXIT_SUCCESS then
        local room_config = user_server_instance:get_zkconfig():get_string("gserver/roomconfig", "")
        user_get_msg_response.room_list = cjson.decode(room_config) 
    else
        sdktypes.debug_error(api_user_get.__LOGTAG__, "user_get failed");
        return '{}';
    end

    local response_json = cjson.encode(user_get_msg_response)
    return response_json
end)


-- Function to convert the gservers_map to the desired format
function api_user_get:convert_gservers_map(gservers_map)
    local gservers = {}

    -- Iterate through the gservers_map table
    for addr, ports in pairs(gservers_map) do
        -- Push the formatted entry into the result table
        table.insert(gservers, {
            addr = addr,  -- The key (address)
            ports = ports  -- The array of ports
        })
    end

    return gservers
end

-- The main function that mimics the behavior of `getgservers`
function api_user_get:get_gservers(user_server_instance, gservers_map)
    -- Get the Redis driver (assuming it’s available via the user_server_interface)
    local hiredis_driver = user_server_instance:get_hiredis_driver()

    if not hiredis_driver then
        sdktypes.debug_error(api_user_get.__LOGTAG__, "Redis driver not available")
        return
    end

    -- Scan the Redis keys with the prefix "gservers"
    hiredis_driver:scan("gservers", nil, function(key, field, value)
        sdktypes.debug_print(sdktypes.LOG_LEVEL_4, api_user_get.__LOGTAG__, key .. " - " .. field .. ":" .. value)
        -- Add to the gservers_map table (acting like a Map in JavaScript)
        if gservers_map[key] then
            table.insert(gservers_map[key], value)
        else
            gservers_map[key] = {value}
        end
    end)
end

-- Return the class
return api_user_get
