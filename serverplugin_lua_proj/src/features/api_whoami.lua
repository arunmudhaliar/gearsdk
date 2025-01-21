local serversdk = require("src.helpers.serversdk")
local cjson = require("cjson")
local api_interface = require("src.features.api_interface")
local ffi = require("ffi")

-- Create the api_whoami class
local api_whoami = {}
api_whoami.__index = api_whoami

-- Inherit from the interface_api
setmetatable(api_whoami, {__index = api_interface.interface_api})

-- Constructor
function api_whoami:new()
    local self = setmetatable({}, api_whoami)
    -- -- Check if this class implements the interface
    -- api_interface.check_interface_implementation(self, api_interface.interface_api)
    return self
end

-- Implement the methods required by the interface
function api_whoami:get_path()
    return "/whoami"
end

function api_whoami:get_post_cb()
    return self.parse_whoami
end


api_whoami.parse_whoami = ffi.cast("type_on_api_parse", function(
    native_server, native_user_arg, native_cid, cid_len, native_path, native_buffer, len, native_headers_buffer, native_headers_buffer_size)
    local response_json = {
        name = "qh3pluginserver",
        active_connections = serversdk.sdklib.get_live_connection_count(native_server)
    }
    -- Convert table to JSON string (assuming a json library is available)
    local json_string = cjson.encode(response_json);
    return json_string
end)

-- Return the class
return api_whoami
