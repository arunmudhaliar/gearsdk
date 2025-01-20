-- local serversdk = require("src.helpers.serversdk")
local cjson = require("cjson")
local api_interface = require("src.features.api_interface")
local ffi = require("ffi")

-- Create the api_ping class
local api_ping = {}
api_ping.__index = api_ping
api_ping.__LOGTAG__ = "api_ping"

-- Inherit from the interface_api
setmetatable(api_ping, { __index = api_interface.interface_api })

-- Constructor
function api_ping:new()
    local self = setmetatable({}, api_ping)  -- Create an instance of api_ping
    -- Check if this class implements the interface
    -- api_interface.check_interface_implementation(self, api_interface.interface_api)
    return self
end

-- Implementing the get_path method required by the interface
function api_ping:get_path()
    return "/ping"
end

-- Implementing the get_post_cb method required by the interface
function api_ping:get_post_cb()
    return self.parse_ping
end

-- -- Implementing the parse_ping method
-- function api_ping:parse_ping(native_server, cid, cid_len, user_server_interface, api_instance, path, buffer, len, headers, header_buffer_size)
--     -- Assuming 'buffer' is the body of the incoming request as a JSON string
--     local ping_rq = cjson.decode(buffer)  -- Parse the JSON string into a Lua table

--     -- Assuming the `ping_rq` contains a `msg` field
--     local response_json = {
--         pong = "qh3pluginserver",  -- Static pong response
--         msg = ping_rq.msg          -- Echo the message from the incoming request
--     }

--     -- Convert the Lua table to a JSON string and return it
--     return cjson.encode(response_json)
-- end

api_ping.parse_ping = ffi.cast("type_on_api_parse", function(
    native_server, userserver_registry_id, native_cid, cid_len, native_path, native_buffer, len, native_headers_buffer, native_headers_buffer_size)
    local ping_rq = cjson.decode(ffi.string(native_buffer, len))  -- Parse the JSON string into a Lua table

    -- Assuming the `ping_rq` contains a `msg` field
    local response_json = {
        pong = "qh3pluginserver",  -- Static pong response
        msg = ping_rq.msg          -- Echo the message from the incoming request
    }

    -- Convert the Lua table to a JSON string and return it
    return cjson.encode(response_json)
end)

-- Return the class
return api_ping
